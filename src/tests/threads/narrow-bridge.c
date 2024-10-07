
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"

struct semaphore bridge_semaphore;
struct semaphore left_semaphore;
struct semaphore right_semaphore;
int bridge_count;
enum car_direction current_direction;

// Called before test. Can initialize some synchronization objects.
void narrow_bridge_init(void)
{
    sema_init(&bridge_semaphore, 3); // Max 3 cars on the bridge
    sema_init(&left_semaphore, 0);   // Semaphore for left-going cars
    sema_init(&right_semaphore, 0);  // Semaphore for right-going cars
    bridge_count = 0;
    current_direction = dir_left; // Initialize direction
}

void arrive_bridge(enum car_priority prio, enum car_direction dir)
{
    if (prio == car_emergency) {
        sema_down(&bridge_semaphore);
        bridge_count++;
        return; // Emergency vehicles bypass the direction logic
    }

    if (dir == current_direction) {
        sema_down(&bridge_semaphore);
        bridge_count++;
        if (dir == dir_left) 
           sema_up(&left_semaphore);
        else
           sema_up(&right_semaphore);
    } else {
         if (dir == dir_left)
            sema_down(&left_semaphore);
         else
            sema_down(&right_semaphore);
         sema_down(&bridge_semaphore);
         bridge_count++;
    }
}

void exit_bridge(enum car_priority prio, enum car_direction dir)
{
    bridge_count--;
    sema_up(&bridge_semaphore);
    
    if (prio == car_emergency) return; // Emergency cars don't influence direction

    if (bridge_count == 0) {
        // Switch direction if there are waiting cars in the opposite direction
        if (dir == dir_left && !list_empty(&right_semaphore.waiters)) {
           current_direction = dir_right;
           for (int i = 0; i < 3 && !list_empty(&right_semaphore.waiters); i++) {
             sema_up(&right_semaphore);
           }
        } else if (dir == dir_right && !list_empty(&left_semaphore.waiters)) {
           current_direction = dir_left;
           for (int i = 0; i < 3 && !list_empty(&left_semaphore.waiters); i++) {
             sema_up(&left_semaphore);
           }
        }
    }
}
