/**
 * @file bridge.c
 * @author Ben Williams '25 - benjamin.r.williams.25@dartmouth.edu
 * @brief Simulating cars on a one-lane road/brdge from Hanover to Vermont using threads
 * @date 2024-01-19
 * Usage: ./bridge num_cars_from_hanover num_cars_from_vermont
 *  ! ! ! Non integers in parameter will be treated as 0 ! ! !
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_CARS 5

// Locks and cvars
pthread_mutex_t bridge_lock =  PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv_green_to_Hanover = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_green_to_Vermont = PTHREAD_COND_INITIALIZER;
pthread_cond_t bridge_full = PTHREAD_COND_INITIALIZER;

// Global variables
int total_to_Hanover;
int total_to_Vermont;
int cars_finished_to_Hanover = 0;
int cars_finished_to_Vermont = 0;
int on_bridge_to_Hanover = 0;
int on_bridge_to_Vermont = 0;
int queued_in_hanover = 0;
int queued_in_vermont = 0;
bool green_to_Hanover = false;
bool green_to_Vermont = false;


/**
 * @brief Returns True with the probability (1 / modulo_prob)
 * @param modulo_prob 
 * @return true 
 * @return false 
 */
bool rand_bool(int modulo_prob) {
    return ((int) rand() % modulo_prob == 0);
}

/**
 * @brief Signals the maximum number of cars waiting for the green light in one direction
 * 
 * @param direction 
 */
void signal_new_greenlight(char* direction) {
    printf("signal recieved: %s\n", direction);
    pthread_cond_t *target;
    // Find the correct target based on the direction we are given
    if (strcmp(direction, "to_Vermont") == 0) {
        target = &cv_green_to_Vermont;
    } else {
        target = &cv_green_to_Hanover;
    }

    // Signal MAX_CARS vehicles that the light is green
    for (int car = 0; car < MAX_CARS; car++) {
        pthread_cond_signal(target);
    }
}

/**
 * @brief The car calling takes the bridge lock, increments the appropriate direction's count, and 
 * 
 * @param direction 
 */
void arrive_bridge(char* direction) {
    pthread_mutex_lock(&bridge_lock);
    // If we are not going to vermont, we must be going to hanover
    // We have already validated the direction string at the one_vehicle function
    bool to_Vermont = (strcmp(direction, "to_Vermont") == 0);

    // Use our current direction to find the relevant global variables for this car
    bool *relevant_green_light = to_Vermont ? &green_to_Vermont : &green_to_Hanover;
    pthread_cond_t *cv_relevant_green_light = to_Vermont ? &cv_green_to_Vermont : &cv_green_to_Hanover;
    int *relevant_on_bridge = to_Vermont ? &on_bridge_to_Vermont : &on_bridge_to_Hanover;
    int *relevant_queue = to_Vermont ? &queued_in_vermont : &queued_in_hanover;

    // We need to wait for the green light to go
    while (! (*relevant_green_light)) {
        pthread_cond_wait(cv_relevant_green_light, &bridge_lock);
    }

    // The car will make this green light
    *relevant_queue += 1;

    // This lets us know if the traffic light can sense cars on the other side
    int remaining_on_other_side = to_Vermont ? 
        total_to_Hanover - cars_finished_to_Hanover : 
        total_to_Vermont - cars_finished_to_Vermont;
    
    char* curr_side = to_Vermont ? "Vermont" : "Hanover";

    // A chance for the green light to switch
    if (remaining_on_other_side > 0) {
        if (rand_bool(3 * MAX_CARS)) {
            printf("||| To %s light turning yellow |||\n", curr_side);
            *relevant_green_light = false;
        }
    }
    
    // The bridge is full, but we are going to make it on once space opens
    if (*relevant_on_bridge == MAX_CARS) {
        // We are more likely to switch the light traffic is backed up
        if (*relevant_green_light && remaining_on_other_side > 0 && rand_bool(2 * MAX_CARS)) {
            printf("||| To %s light turning yellow |||\n", curr_side);
            *relevant_green_light = false;
        }
        pthread_cond_wait(&bridge_full, &bridge_lock);
    }

    *relevant_on_bridge += 1;
    *relevant_queue -= 1;

    pthread_mutex_unlock(&bridge_lock);
}

/**
 * @brief Simulates the car being on the bridge, and prints out the bridges current state
 * 
 * @param direction 
 */
void on_bridge(char* direction) {
    pthread_mutex_lock(&bridge_lock);

    // Total amount of cars on the bridge
    int on_bridge;
    if (strcmp(direction, "to_Hanover") == 0) {
        on_bridge = on_bridge_to_Hanover;
    } else {
        on_bridge = on_bridge_to_Vermont;
    }

    // Total cars that are on a side the waiting side of the bridge
    int remaining_to_Hanover = total_to_Hanover - cars_finished_to_Hanover - on_bridge_to_Hanover;
    int remaining_to_Vermont = total_to_Vermont - cars_finished_to_Vermont - on_bridge_to_Vermont;

    printf("Waiting in Hanover %d ==== On bridge: %d | Direction: %s ==== Waiting in Vermont %d\n", remaining_to_Vermont, on_bridge
        , direction, remaining_to_Hanover);

    if ((on_bridge_to_Hanover > 0 && on_bridge_to_Vermont > 0) || on_bridge > MAX_CARS) {
        fprintf(stderr, "Illegal travel. To hanover: %d | To Vermont: %d \n\n", on_bridge_to_Hanover, on_bridge_to_Vermont);
    }
    // Car driving time part 1
    sleep(1);
    pthread_mutex_unlock(&bridge_lock);
    
    // Car driving time part 2
    sleep(2);
}

/**
 * @brief Decrements the vehicle count on the bridge in the appropriate direction, and signals relevant variables 
 * 
 * @param direction 
 */
void exit_bridge(char* direction) {
    pthread_mutex_lock(&bridge_lock);
    int remaining_to_Hanover = total_to_Hanover - cars_finished_to_Hanover;
    int remaining_to_Vermont = total_to_Vermont - cars_finished_to_Vermont;
    
    
    // Use a boolean to represent our direction so we can define the other variables
    bool to_Vermont = (strcmp(direction, "to_Vermont") == 0);

    // Use our current direction to find the relevant global variables for this car
    bool *origin_green_light = to_Vermont ? &green_to_Vermont : &green_to_Hanover;
    bool *destination_green_light = to_Vermont ? &green_to_Hanover : &green_to_Vermont;
    pthread_cond_t *cv_origin_green_light = to_Vermont ? &cv_green_to_Vermont : &cv_green_to_Hanover;
    
    // Allows us to keep track of the number of cars left on the bridge
    int *on_bridge_to_destination = to_Vermont ? &on_bridge_to_Vermont : &on_bridge_to_Hanover;
    // Any cars that have been signaled for the green light, but the bridge is full
    int *origin_queue = to_Vermont ? &queued_in_vermont : &queued_in_hanover;
    // Remaining cars in both directions for if we should change the light
    int remaining_to_desination = to_Vermont ? remaining_to_Vermont : remaining_to_Hanover;
    int remaining_to_origin = to_Vermont ? remaining_to_Hanover : remaining_to_Vermont;
    // For string formatting when changing the green/yellow/red light
    char* origin_side = to_Vermont ? "Hanover" : "Vermont";
    char* destination_side = to_Vermont ? "Vermont" : "Hanover";
    // To keep track of how many cars have already finished
    int * cars_finished_to_destination = to_Vermont ? &cars_finished_to_Vermont : &cars_finished_to_Hanover;
    // For signaling the other green light
    char* opposing_direction = to_Vermont ? "to_Hanover" : "to_Vermont";
    
    // The car is now off the bridge and at the desination, but we still need to communicate
    *on_bridge_to_destination -= 1;
    remaining_to_desination -= 1;
    *cars_finished_to_destination += 1;
    
    // Green light is still on
    if (*origin_green_light) {
        // Cars are queued for this light
        if (*origin_queue > 0) {
            pthread_cond_signal(&bridge_full);
    
        // There are no cars left - so we should switch the light
        } else {
            // There are no cars left - so we should switch the light
            if (remaining_to_desination == 0) {
                printf("||| To %s light turning yellow |||\n", destination_side);
                sleep(1); // For dramatic effect
                *origin_green_light = false;
                printf("XXX To %s light turning red XXX\n", destination_side);
                sleep(1); // For dramatic effect
                *destination_green_light = true;
                printf("+++ To %s light turning green +++\n", origin_side);
                signal_new_greenlight(opposing_direction);
            } else {
                pthread_cond_signal(cv_origin_green_light);
            }
        }

        // Now a small chance to swap the green light if we sense cars on the other side
        if (remaining_to_origin > 0) {
            if (*origin_green_light && rand_bool(2 * MAX_CARS)) {
                printf("||| To %s light turning yellow |||\n", destination_side);
                *origin_green_light = false;
            }
        }
    } else {
        // Light is yellow --> let the remaining cars in queue go
        if (*origin_queue > 0) {
            pthread_cond_signal(&bridge_full);
        // All cars are off the bridge --> Turn the light red, the other light green, and signal 
        } else if (*on_bridge_to_destination == 0){
            printf("XXX To %s light turning red XXX\n", destination_side);
            sleep(1); // For dramatic effect
            printf("+++ To %s light turned green +++\n", origin_side);
            *destination_green_light = true;
            signal_new_greenlight(opposing_direction);
        }
    }

    pthread_mutex_unlock(&bridge_lock);
}

/**
 * @brief One vehicle takes the bridge lock and moves on the bridge
 * 
 * @param direction 
 */
void* one_vehicle(void* arg) {
    char* direction = (char*) arg;
    if (strcmp(direction, "to_Hanover") != 0 && strcmp(direction, "to_Vermont") != 0) {
        fprintf(stderr, "Invalid direction recieved %s\n", direction);
        return NULL;
    }

    arrive_bridge(direction);

    on_bridge(direction);

    exit_bridge(direction);

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Please enter two integers as parameter. Non integers will be treated as 0\n");
        return -1;
    }

    // Randomness initialization
    // srand(time(NULL));
    srand(1);

    total_to_Vermont = atoi(argv[1]);
    total_to_Hanover = atoi(argv[2]);

    // Coin flip for which green light starts first
    if (rand_bool(2)) {
        green_to_Hanover = true;
    } else {
        green_to_Vermont = true;
    }

    pthread_t all_car_threads[total_to_Vermont + total_to_Hanover];

    // Create all car threads for cars from hanover (this means Hanover may get a little bit of a head start...)
    for (int to_Vermont_car = 0; to_Vermont_car < total_to_Vermont; to_Vermont_car++) {
        pthread_create(&all_car_threads[to_Vermont_car], NULL, one_vehicle, "to_Vermont");
    }

    // Create all car threads for cars from vermont
    for (int to_Hanover_car = 0; to_Hanover_car < total_to_Hanover; to_Hanover_car++) {
        pthread_create(&all_car_threads[total_to_Vermont + to_Hanover_car], NULL, one_vehicle, "to_Hanover");
    }

    // Wait for child threads to finish before exiting main thread
    for (int thread = 0; thread < total_to_Hanover + total_to_Vermont; thread++) {
        pthread_join(all_car_threads[thread], NULL);
    }
    

    return 0;
}