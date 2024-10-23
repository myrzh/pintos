
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"

enum operation
{
   increment = 0,
   decrement = 1
};

enum place_mode
{
  one = 0,       // One car
  two_equal = 1, // Two cars with equal priorities
  two_mixed = 2  // Two cars with different priorities
};

struct semaphore norm_left_sema, norm_right_sema, emer_left_sema, emer_right_sema;
struct semaphore mutex_sema; // Control bridge access
int norm_left, norm_right, emer_left, emer_right;
int was_started;    // Was the first direction chosen
int now_crossing;   // Number of cars on the bridge
int cars_overall;   // Number of waiting cars
int cars_unblocked; // Number of ready-to-move cars
enum car_priority current_car;
enum car_direction current_direction;

/** Unlock selected semaphore */
void car_sema_up() {
  if (current_car == car_normal && current_direction == dir_left) {
    sema_up(&norm_left_sema);
  } else if (current_car == car_emergency && current_direction == dir_left) {
    sema_up(&emer_left_sema);
  } else if (current_car == car_normal && current_direction == dir_right) {
    sema_up(&norm_right_sema);
  } else if (current_car == car_emergency && current_direction == dir_right) {
    sema_up(&emer_right_sema);
  }
}

/** Lock selected semaphore */
void car_sema_down(enum car_priority prio, enum car_direction dir) {
  if (prio == car_normal && dir == dir_left) {
    sema_down(&norm_left_sema);
  } else if (prio == car_emergency && dir == dir_left) {
    sema_down(&emer_left_sema);
  } else if (prio == car_normal && dir == dir_right) {
    sema_down(&norm_right_sema);
  } else if (prio == car_emergency && dir == dir_right) {
    sema_down(&emer_right_sema);
  }
}

/** Place one car, two equal or two mixed cars */
void place_car(enum place_mode mode) {
  switch (mode) {
    case one:
      sema_down(&mutex_sema); // Reserve one place on the bridge
      car_sema_up();          // Unlock current car semaphore
      sema_down(&mutex_sema); // Reserve another place on the bridge (so that others wouldn't drive)
      now_crossing += 1;
      current_car = car_normal;
      break;
    case two_equal:
      car_sema_up();
      sema_down(&mutex_sema);
      car_sema_up();
      sema_down(&mutex_sema);
      now_crossing += 2;
      current_car = car_normal;
      break;
    case two_mixed:
      // Place emergency in the first place, then normal car
      if (current_direction == dir_left) {
        sema_up(&emer_left_sema);
        car_sema_up();
      } else if (current_direction == dir_right) {
        sema_up(&emer_right_sema);
        car_sema_up();
      }
      sema_down(&mutex_sema);
      sema_down(&mutex_sema);
      now_crossing += 2;
      break;
    default:
      break;
  }
}

/** Choose the most appropriate car(s) and place */
void choose_and_place() {
  int emer_count, norm_count;
  
  if (current_direction == dir_left) {
    emer_count = emer_left;
    norm_count = norm_left;
  } else if (current_direction == dir_right) {
    emer_count = emer_right;
    norm_count = norm_right;
  }

  if (emer_count >= 2) { // If we have two emers, place them together
    current_car = car_emergency;
    place_car(two_equal);
  } else if (emer_count == 1 && norm_count >= 1) { // If we have one emer and at least one norm, place them together
    place_car(two_mixed);
  } else if (emer_count == 1 || norm_count >= 2) { // If we have one emer or at least two norms
    if (emer_count == 1) {
      current_car = car_emergency;
      place_car(one);       // If one emer, place it
    } else {
      place_car(two_equal); // Otherwise, place two norms
    }
  } else {                  // If no cars in current direction ...
    sema_down(&mutex_sema); // ... block the bridge anyway
    sema_down(&mutex_sema);
  }
}

/** Update counts of each type of the cars */
void update_counts(enum car_priority prio, enum car_direction dir, enum operation oper) {
  int *count = NULL;
  if (prio == car_normal && dir == dir_left) {
    count = &norm_left;
  } else if (prio == car_emergency && dir == dir_left) {
    count = &emer_left;
  } else if (prio == car_normal && dir == dir_right) {
    count = &norm_right;
  } else if (prio == car_emergency && dir == dir_right) {
    count = &emer_right;
  }

  if (oper == increment) {
    (*count)++;
  } else if (oper == decrement) {
    (*count)--;
  }
}

/** Called before test. Can initialize some synchronization objects. */
void narrow_bridge_init(void) {
  sema_init(&mutex_sema, 1);
  sema_init(&norm_left_sema, 0);
  sema_init(&emer_left_sema, 0);
  sema_init(&norm_right_sema, 0);
  sema_init(&emer_right_sema, 0);
}

/** Called every time the new car arrives */
void arrive_bridge(enum car_priority prio, enum car_direction dir) {
  update_counts(prio, dir, increment);
  thread_yield(); // Give another cars the opportunity to reach this code section

  if (was_started == 0) { // Determine first direction and count cars (once)
    if (emer_left > emer_right) {
      current_direction = dir_left;
    } else if (emer_left < emer_right) {
      current_direction = dir_right;
    } else if (norm_left >= norm_right) {
      current_direction = dir_left;
    } else if (norm_left < norm_right) {
      current_direction = dir_right;
    }
    cars_unblocked = norm_left + emer_left + norm_right + emer_right;
    was_started = 1;
  }

  if (cars_unblocked > 1) {   // If more than one unblocked car ...
    cars_unblocked--;
    car_sema_down(prio, dir); // ... send it to queue
  }
  if (cars_unblocked == 1) {  // If one unblocked car is left ...
    cars_unblocked--;
    sema_up(&mutex_sema);     // ... clear bridge ...
    sema_up(&mutex_sema);
    choose_and_place();       // ... choose and place the most appropriate car(s) ...
    car_sema_down(prio, dir); // ... and block current car
  }
}

/** Called every time the car needs to exit the bridge (already crossed) */
void exit_bridge(enum car_priority prio, enum car_direction dir) {
  update_counts(prio, dir, decrement);
  cars_overall = norm_left + emer_left + norm_right + emer_right; // Count waiting cars
  now_crossing--;

  if (cars_overall != 0 && now_crossing % 2 == 0) { // If we still have waiting cars and the bridge is free
    // Change direction if there are no emers in the other direction
    if ((current_direction == dir_left && emer_right == 0 && emer_left > 0) ||
        (current_direction == dir_right && emer_left == 0 && emer_right > 0)) {
      current_direction = (current_direction == dir_left) ? dir_right : dir_left;
    } else {
    // Otherwise, change direction if there are no cars in the other direction
      if ((current_direction == dir_left && (norm_right + emer_right) == 0) ||
          (current_direction == dir_right && (norm_left + emer_left) == 0)) {
        current_direction = (current_direction == dir_left) ? dir_right : dir_left;
      }
    }

    current_direction = (current_direction == dir_left) ? dir_right : dir_left; // Finally, change direction anyway
    sema_up(&mutex_sema); // Clear the bridge
    sema_up(&mutex_sema);
    choose_and_place();   // Choose and place the next most appropriate car(s)
  }
}
