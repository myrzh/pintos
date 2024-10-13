
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

struct semaphore norm_left_sema, norm_right_sema, emer_left_sema, emer_right_sema;
struct semaphore general_sema;
int norm_left, norm_right, emer_left, emer_right;
int was_started = 0;
int now_crossing;
int cars_overall;
int cars_unblocked;
enum car_priority current_car = car_normal;
enum car_direction current_direction;

/** Set two cars of one type if bridge if full emty */
void _up_two_to_bridge() {
  if (current_car == car_normal && current_direction == dir_left) {
    sema_up(&norm_left_sema);
  } else if (current_car == car_emergency && current_direction == dir_left) {
    sema_up(&emer_left_sema);
  } else if (current_car == car_normal && current_direction == dir_right) {
    sema_up(&norm_right_sema);
  } else if (current_car == car_emergency && current_direction == dir_right) {
    sema_up(&emer_right_sema);
  }
  sema_down(&general_sema);
  now_crossing += 2;
  current_car = car_normal;
}  

void _up_solo_to_bridge() {
  sema_down(&general_sema);
  if (current_car == car_normal && current_direction == dir_left) {
    sema_up(&norm_left_sema);
  } else if (current_car == car_emergency && current_direction == dir_left) {
    sema_up(&emer_left_sema);
  } else if (current_car == car_normal && current_direction == dir_right) {
    sema_up(&norm_right_sema);
  } else if (current_car == car_emergency && current_direction == dir_right) {
    sema_up(&emer_right_sema);
  }
  sema_down(&general_sema);
  now_crossing += 1;
  current_car = car_normal;
}

/** Move normal car with emergency if emergency is last on current direction */
void _last_emer_w_normal() {
  if (current_direction == dir_left) {
    sema_up(&emer_left_sema);
    sema_up(&norm_left_sema);
  } else if (current_direction == dir_right) {
    sema_up(&emer_right_sema);
    sema_up(&norm_right_sema);
  }
  sema_down(&general_sema);
  sema_down(&general_sema);
  now_crossing += 2;
}

/** Choose correct cars to move */
void move_to_bridge() {
  switch (current_direction) {
    case dir_left:
      if (emer_left >= 2) {
        current_car = car_emergency;
        _up_two_to_bridge();
      } else if (emer_left == 1) {
        if (norm_left >= 1) {
          _last_emer_w_normal();
        } else {
          current_car = car_emergency;
          _up_solo_to_bridge();
        }
      } else if (norm_left >= 2) {
        _up_two_to_bridge();
      } else if (norm_left == 1) {
        _up_solo_to_bridge();
      } else {
        sema_down(&general_sema);
        sema_down(&general_sema);
      }
      break;
    case dir_right:
      if (emer_right >= 2) {
        current_car = car_emergency;
        _up_two_to_bridge();
      } else if (emer_right == 1) {
        if (norm_right >= 1) {
          _last_emer_w_normal();
        } else {
          current_car = car_emergency;
          _up_solo_to_bridge();
        }
      } else if (norm_right >= 2) {
        _up_two_to_bridge();
      } else if (norm_right == 1) {
        _up_solo_to_bridge();
      } else {
        sema_down(&general_sema);
        sema_down(&general_sema);
      }
      break;
    default:
      break;
  }
}

void update_counts(enum car_priority prio, enum car_direction dir, enum operation oper) {
  switch (oper) {
    case increment:
      if (prio == car_normal && dir == dir_left) {
        norm_left++;
      } else if (prio == car_emergency && dir == dir_left) {
        emer_left++;
      } else if (prio == car_normal && dir == dir_right) {
        norm_right++;
      } else if (prio == car_emergency && dir == dir_right) {
        emer_right++;
      }
      break;
    case decrement:
      if (prio == car_normal && dir == dir_left) {
        norm_left--;
      } else if (prio == car_emergency && dir == dir_left) {
        emer_left--;
      } else if (prio == car_normal && dir == dir_right) {
        norm_right--;
      } else if (prio == car_emergency && dir == dir_right) {
        emer_right--;
      }
      break;
    default:
      break;
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

enum car_direction opos_dir(enum car_direction dir) {
  if (dir == dir_left) {
    return dir_right;
  }
  return dir_left;
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
      current_direction == dir_left;
    } else if (emer_left < emer_right) {
      current_direction == dir_right;
    } else if (norm_left > norm_right) {
      current_direction == dir_left;
    } else if (norm_left < norm_right) {
      current_direction == dir_right;
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
    move_to_bridge();
    car_sema_down(prio, dir);
  }
}

void exit_bridge(enum car_priority prio, enum car_direction dir) {
  update_counts(prio, dir, decrement);
  cars_overall = norm_left + emer_left + norm_right + emer_right;
  now_crossing--;
  if (cars_overall != 0 && now_crossing % 2 == 0) {
    if (current_direction == dir_left) {
      if (emer_right == 0 && emer_left > 0) {
        current_direction = opos_dir(current_direction);
      }
      if (current_direction == dir_left) {
        if ((current_direction == dir_right || current_direction == dir_left) && (norm_right + emer_right) == 0) {
          current_direction = opos_dir(current_direction);
        }
      } else if (current_direction == dir_right) {
        if ((current_direction == dir_right || current_direction == dir_left) && (norm_left + emer_left) == 0) {
          current_direction = opos_dir(current_direction);
        }
      }
    } else if (current_direction == dir_right) {
      if (emer_left == 0 && emer_right > 0) {
        current_direction = opos_dir(current_direction);
      }
      if (current_direction == dir_left) {
        if ((current_direction == dir_right || current_direction == dir_left) && (norm_right + emer_right) == 0) {
          current_direction = opos_dir(current_direction);
        }
      } else if (current_direction == dir_right) {
        if ((current_direction == dir_right || current_direction == dir_left) && (norm_left + emer_left) == 0) {
          current_direction = opos_dir(current_direction);
        }
      }
    }
    current_direction = opos_dir(current_direction);
    sema_up(&general_sema);
    sema_up(&general_sema);
    move_to_bridge();
  }
}
