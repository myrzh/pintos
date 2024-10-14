
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
  one = 0,
  two_equal = 1,
  two_mixed = 2
};

struct semaphore norm_left_sema, norm_right_sema, emer_left_sema, emer_right_sema;
struct semaphore general_sema;
int norm_left, norm_right, emer_left, emer_right;
int was_started;
int now_crossing;
int cars_overall;
int cars_unblocked;
enum car_priority current_car;
enum car_direction current_direction;

void signal_sema() {
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

void place_car(enum place_mode mode) {
  switch (mode) {
    case one:
      sema_down(&general_sema);
      signal_sema();
      sema_down(&general_sema);
      now_crossing += 1;
      current_car = car_normal;
      break;
    case two_equal:
      signal_sema();
      sema_down(&general_sema);
      signal_sema();
      sema_down(&general_sema);
      now_crossing += 2;
      current_car = car_normal;
      break;
    case two_mixed:
      if (current_direction == dir_left) {
        sema_up(&emer_left_sema);
        signal_sema();
      } else if (current_direction == dir_right) {
        sema_up(&emer_right_sema);
        signal_sema();
      }
      sema_down(&general_sema);
      sema_down(&general_sema);
      now_crossing += 2;
      break;
    default:
      break;
  }
}

void choose_and_place() {
  int emer_count, norm_count;
  
  if (current_direction == dir_left) {
    emer_count = emer_left;
    norm_count = norm_left;
  } else if (current_direction == dir_right) {
    emer_count = emer_right;
    norm_count = norm_right;
  }

  if (emer_count >= 2) {
    current_car = car_emergency;
    place_car(two_equal);
  } else if (emer_count == 1 && norm_count >= 1) {
    place_car(two_mixed);
  } else if (emer_count == 1 || norm_count >= 2) {
    if (emer_count == 1) {
      current_car = car_emergency;
    }
    place_car(emer_count == 1 ? one : two_equal);
  } else {
    sema_down(&general_sema);
    sema_down(&general_sema);
  }
}

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

/** Called before test. Can initialize some synchronization objects. */
void narrow_bridge_init(void) {
  sema_init(&general_sema, 1);
  sema_init(&norm_left_sema, 0);
  sema_init(&emer_left_sema, 0);
  sema_init(&norm_right_sema, 0);
  sema_init(&emer_right_sema, 0);
}

void arrive_bridge(enum car_priority prio, enum car_direction dir) {
  update_counts(prio, dir, increment);
  thread_yield();

  if (was_started == 0) {
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

  if (cars_unblocked > 1) {
    cars_unblocked--;
    car_sema_down(prio, dir);
  }
  if (cars_unblocked == 1) {
    cars_unblocked--;
    sema_up(&general_sema);
    sema_up(&general_sema);
    choose_and_place();
    car_sema_down(prio, dir);
  }
}

void exit_bridge(enum car_priority prio, enum car_direction dir) {
  update_counts(prio, dir, decrement);
  cars_overall = norm_left + emer_left + norm_right + emer_right;
  now_crossing--;

  if (cars_overall != 0 && now_crossing % 2 == 0) {
    // Check if emergency cars are present and prioritize them
    if ((current_direction == dir_left && emer_right == 0 && emer_left > 0) ||
        (current_direction == dir_right && emer_left == 0 && emer_right > 0)) {
      current_direction = (current_direction == dir_left) ? dir_right : dir_left;
    } else {
      // Check if the opposite direction has no cars
      if ((current_direction == dir_left && (norm_right + emer_right) == 0) ||
          (current_direction == dir_right && (norm_left + emer_left) == 0)) {
        current_direction = (current_direction == dir_left) ? dir_right : dir_left;
      }
    }

    current_direction = (current_direction == dir_left) ? dir_right : dir_left;
    sema_up(&general_sema);
    sema_up(&general_sema);
    choose_and_place();
  }
}
