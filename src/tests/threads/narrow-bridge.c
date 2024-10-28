
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"
#include "threads/synch.h"

enum action
{
   arrive = 0,
   exit = 1
};

struct semaphore norm_left_sema, emer_left_sema, norm_right_sema, emer_right_sema;
int norm_left = 0, emer_left = 0, norm_right = 0, emer_right = 0;
int now_crossing = 0;

void process_car(enum car_priority prio, enum car_direction dir, enum action act) {
    int *count = NULL;
    struct semaphore *sema = NULL;

    if (prio == car_normal && dir == dir_left) {
        count = &norm_left;
        sema = &norm_left_sema;
    } else if (prio == car_emergency && dir == dir_left) {
        count = &emer_left;
        sema = &emer_left_sema;
    } else if (prio == car_normal && dir == dir_right) {
        count = &norm_right;
        sema = &norm_right_sema;
    } else if (prio == car_emergency && dir == dir_right) {
        count = &emer_right;
        sema = &emer_right_sema;
    }

    if (act == arrive) {
        (*count)++;
        sema_down(sema);
    } else if (act == exit) {
        (*count)--;
        sema_up(sema);
    }
}

// Called before test. Can initialize some synchronization objects.
void narrow_bridge_init(void)
{
    sema_init(&norm_left_sema, 0);
    sema_init(&emer_left_sema, 0);
    sema_init(&norm_right_sema, 0);
    sema_init(&emer_right_sema, 0);
}

void arrive_bridge(enum car_priority prio, enum car_direction dir)
{    
    if (prio == car_emergency && now_crossing == 2) {
        process_car(car_emergency, dir, arrive);
        // process_arriving_car(car_emergency, dir);
    } else if ((prio == car_normal && now_crossing == 2) || ((dir == dir_left) ? emer_left : emer_right) > 0) {
        process_car(car_normal, dir, arrive);
        // process_arriving_car(car_normal, dir);
    }
    now_crossing++;
}

void exit_bridge(enum car_priority prio UNUSED, enum car_direction dir UNUSED)
{
    if (now_crossing > 0) {
        now_crossing--;
    }
    if (now_crossing == 0) {
        if (emer_right > 0) {
            process_car(car_emergency, dir_right, exit);
            if (emer_right > 0) {
                process_car(car_emergency, dir_right, exit);
            } else if (norm_right > 0) {
                process_car(car_normal, dir_right, exit);
            }
        } else if (emer_left > 0) {
            process_car(car_emergency, dir_left, exit);
            if (emer_left > 0) {
                process_car(car_emergency, dir_left, exit);
            } else if (norm_left > 0) {
                process_car(car_normal, dir_left, exit);
            }
        } else if (norm_right > 0) {
            process_car(car_normal, dir_right, exit);
            if (norm_right > 0) {
                process_car(car_normal, dir_right, exit);
            }
        } else if (norm_left > 0) {
            process_car(car_normal, dir_left, exit);
            if (norm_left > 0) {
                process_car(car_normal, dir_left, exit);
            }
        }
    }
}
