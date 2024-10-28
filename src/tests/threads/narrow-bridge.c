
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"
#include "threads/synch.h"

struct semaphore norm_left_sema, emer_left_sema, norm_right_sema, emer_right_sema;
int norm_left = 0, emer_left = 0, norm_right = 0, emer_right = 0;
int now_crossing = 0;

void process_arrived_car(enum car_priority prio, enum car_direction dir) {
    if (prio == car_normal && dir == dir_left) {
        norm_left++;
        sema_down(&norm_left_sema);
    } else if (prio == car_emergency && dir == dir_left) {
        emer_left++;
        sema_down(&emer_left_sema);
    } else if (prio == car_normal && dir == dir_right) {
        norm_right++;
        sema_down(&norm_right_sema);
    } else if (prio == car_emergency && dir == dir_right) {
        emer_right++;
        sema_down(&emer_right_sema);
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
        process_arrived_car(car_emergency, dir);
    } else if ((prio == car_normal && now_crossing == 2) || ((dir == dir_left) ? emer_left : emer_right) > 0) {
        process_arrived_car(car_normal, dir);
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
            sema_up(&emer_right_sema);
            emer_right--;
            if (emer_right > 0) {
                sema_up(&emer_right_sema);
                emer_right--;
            } else if (norm_right > 0) {
                sema_up(&norm_right_sema);
                norm_right--;
            }
        } else if (emer_left > 0) {
            sema_up(&emer_left_sema);
            emer_left--;
            if (emer_left > 0) {
                sema_up(&emer_left_sema);
                emer_left--;
            } else if (norm_left > 0) {
                sema_up(&norm_left_sema);
                norm_left--;
            }
        } else if (norm_right > 0) {
            sema_up(&norm_right_sema);
            norm_right--;
            if (norm_right > 0) {
                sema_up(&norm_right_sema);
                norm_right--;
            } 
        } else if (norm_left > 0) {
            sema_up(&norm_left_sema);
            norm_left--;
            if (norm_left > 0) {
                sema_up(&norm_left_sema);
                norm_left--;
            }
        }
    }
}
