
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"

struct semaphore bridge_semaphore;
struct semaphore left_queue;
struct semaphore right_queue;
int left_waiting;
int right_waiting;
int bridge_count;
enum car_direction current_direction;



// Called before test. Can initialize some synchronization objects.
void narrow_bridge_init(void)
{
    sema_init(&bridge_semaphore, 2);
    sema_init(&left_queue, 0);
    sema_init(&right_queue, 0);
    left_waiting = 0;
    right_waiting = 0;

    bridge_count = 0;
    current_direction = dir_left;
}

void arrive_bridge(enum car_priority prio, enum car_direction dir)
{
    if (prio == car_emergency) {
        sema_down(&bridge_semaphore);
        bridge_count++;
        current_direction = dir;
    } else {
        if (dir == dir_left) {
            left_waiting++;
            sema_down(&left_queue);
            sema_down(&bridge_semaphore);
            bridge_count++;
            left_waiting--;
        } else {
            right_waiting++;
            sema_down(&right_queue);
            sema_down(&bridge_semaphore);
            bridge_count++;
            right_waiting--;
        }
    }
}

void exit_bridge(enum car_priority prio, enum car_direction dir)
{
    bridge_count--;
    sema_up(&bridge_semaphore);

    if (prio == car_emergency) {
         if (bridge_count == 0) {
            if (dir == dir_left) {
                for (int i = 0; i < 2 && right_waiting > 0; i++) {
                    sema_up(&right_queue);
                    right_waiting--; // No direct access to waiters
                 }
               current_direction = dir_right;
             } else {
                 for (int i = 0; i < 2 && left_waiting > 0; i++) {
                    sema_up(&left_queue);
                    left_waiting--;
                }
                 current_direction = dir_left;
             }
         }
    } else {
         if(bridge_count < 2){
             if(current_direction == dir_left){
                 if(left_waiting > 0){
                     sema_up(&left_queue);
                     left_waiting--;
                 } else if (right_waiting > 0){
                     sema_up(&right_queue);
                     right_waiting--;
                     current_direction = dir_right;
                 }
             } else {
                 if(right_waiting > 0){
                     sema_up(&right_queue);
                     right_waiting--;
                 } else if(left_waiting > 0){
                    sema_up(&left_queue);
                    left_waiting--;
                    current_direction = dir_left;
                 }
             }
         }
    }
}
