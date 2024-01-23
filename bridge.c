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
pthread_cond_t cv_green_to_hanover = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_green_to_vermont = PTHREAD_COND_INITIALIZER;
pthread_cond_t bridge_full = PTHREAD_COND_INITIALIZER;

// Global variables
int total_to_hanover;
int total_to_vermont;
int cars_finished_to_hanover = 0;
int cars_finished_to_vermont = 0;
int on_bridge_to_hanover = 0;
int on_bridge_to_vermont = 0;
int queued_in_hanover = 0;
int queued_in_vermont = 0;
bool green_to_hanover = false;
bool green_to_vermont = false;


// Declaring functions ahead of usage
// void* one_vehicle(void* arg);
// void arrive_bridge(char* direction);
// void on_bridge(char* direction);
// void exit_bridge(char* direction);

/**
 * @brief Returns True with the probability of 1 / modulo_prob
 *      Used here to randomly flip green lights sometimes
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
    pthread_cond_t *target;
    // Find the correct target based on the direction we are given
    if (strcmp(direction, "to_vermont") == 0) {
        target = &cv_green_to_vermont;
    } else {
        target = &cv_green_to_hanover;
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

    if (strcmp(direction, "to_vermont") == 0) {
        // Wait for the green light
        while (! green_to_vermont) {
            // printf("Red light to_vermont\n");
            pthread_cond_wait(&cv_green_to_vermont, &bridge_lock);
        }

        queued_in_hanover += 1;
        int remaining_to_hanover = total_to_hanover - cars_finished_to_hanover;
        // Somewhat rarely and randomly switch the greenlight
        if (remaining_to_hanover > 0) {
            if (rand_bool(2 * MAX_CARS)) {
                printf("||| To Vermont light turning yellow |||\n");
                green_to_vermont = false;
            }
        }

        // Bridge is full, need to wait
        if (on_bridge_to_vermont == MAX_CARS) {

            // More frequently switch the greenlight if we are backed up
            if (remaining_to_hanover > 0) {
                if (green_to_vermont && rand_bool(MAX_CARS)) {
                    printf("||| To Vermont light turning yellow |||\n");
                    green_to_vermont = false;
                }
            }
            pthread_cond_wait(&bridge_full, &bridge_lock);
            // printf("Now allowed on bridge - was too many cars (going to vermont)\n");
        }

        on_bridge_to_vermont += 1;
        queued_in_hanover -= 1;
        // printf("\tNow entering the bridge (to vermont). On Bridge outgoing: %d. On Bridge incoming: %d\n", on_bridge_to_vermont, on_bridge_to_hanover);

    } else {
        while (! green_to_hanover) {
            // printf("Red light to_hanover\n");
            pthread_cond_wait(&cv_green_to_hanover, &bridge_lock);
        }

        int remaining_to_vermont = total_to_vermont - cars_finished_to_vermont;
        // Somewhat rarely and randomly switch the greenlight
        if (remaining_to_vermont > 0) {
            if (rand_bool(2 * MAX_CARS)) {
                printf("||| To Hanover light turning yellow |||\n");
                green_to_hanover = false;
            }
        }
        // if (rand_bool(10)) {
        //     green_to_hanover = false;
        // }
        // Cars incoming, need to wait
        // if (on_bridge_to_vermont > 0) {
        //     pthread_cond_wait(&open_to_hanover, &bridge_lock);
        // }
        queued_in_vermont += 1;
        // Bridge is full, need to wait
        if (on_bridge_to_hanover == MAX_CARS) {
            // More frequently switch the greenlight if we are backed up
            if (remaining_to_vermont > 0) {
                if (green_to_hanover && rand_bool(MAX_CARS)) {
                    printf("||| To Hanover light turning yellow |||\n");
                    green_to_hanover = false;
                }
            }

            pthread_cond_wait(&bridge_full, &bridge_lock);
            // printf("Now allowed on bridge - was too many cars (going to hanover)\n");
        }
        on_bridge_to_hanover += 1;
        queued_in_vermont -= 1;
        // printf("\tNow entering the bridge (to hanover). On Bridge outgoing: %d. On Bridge incoming: %d\n", on_bridge_to_hanover, on_bridge_to_vermont);
    }

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
    if (strcmp(direction, "to_hanover") == 0) {
        on_bridge = on_bridge_to_hanover;
    } else {
        on_bridge = on_bridge_to_vermont;
    }

    // Total cars that are on a side the waiting side of the bridge
    int remaining_to_hanover = total_to_hanover - cars_finished_to_hanover - on_bridge_to_hanover;
    int remaining_to_vermont = total_to_vermont - cars_finished_to_vermont - on_bridge_to_vermont;

    printf("Waiting in Hanover %d ==== On bridge: %d | Direction: %s ==== Waiting in Vermont %d\n", remaining_to_vermont, on_bridge
        , direction, remaining_to_hanover);
    
    // if (on_bridge > MAX_CARS) {
    //     printf("!!!!!!!! More on bridge than allowed !!!!!\n\n");
    // }

    if ((on_bridge_to_hanover > 0 && on_bridge_to_vermont > 0) || on_bridge > MAX_CARS) {
        printf("Illegal travel. To hanover: %d | To Vermont: %d \n\n", on_bridge_to_hanover, on_bridge_to_vermont);
    }
    // Car driving time part 1
    sleep(0.5);
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
    int remaining_to_hanover = total_to_hanover - cars_finished_to_hanover;
    int remaining_to_vermont = total_to_vermont - cars_finished_to_vermont;

    // printf("my direction is %s\n", direction);
    
    if (strcmp(direction, "to_vermont") == 0) {
        on_bridge_to_vermont -= 1;
        remaining_to_vermont -= 1;

        // Green light is on, we can continue to let new cars on from our side
        if (green_to_vermont) {
            // Cars are queued for this green light
            if (queued_in_hanover > 0) {
                pthread_cond_signal(&bridge_full);
            } else {
                // Switch the light here no matter what if there are no cars left
                if (remaining_to_vermont == 0) {
                    printf("||| To Vermont light turning yellow |||\n");
                    sleep(1); // For dramatic effect
                    green_to_vermont = false;
                    printf("XXX To Vermont light turning red XXX\n");
                    sleep(1); // For dramatic effect
                    green_to_hanover = true;
                    printf("+++ To Hanover light turning green +++\n");
                    signal_new_greenlight("to_hanover");
                } else {
                    pthread_cond_signal(&cv_green_to_vermont);
                }
            }
            // Randomly switch the light with probability 1/ (2 * MAX_CARS)
            if (remaining_to_hanover > 0) {
                if (green_to_vermont && rand_bool(2 * MAX_CARS)) {
                    printf("||| To Vermont light turning yellow |||\n");
                    green_to_vermont = false;
                }
            }

        } else {
            // Allow the final waiting cars to go
            if (queued_in_vermont > 0) {
                pthread_cond_signal(&bridge_full);
            
            // We are safe - switch the lights and signal MAX_CARS cars on the other end
            } else if (on_bridge_to_vermont == 0) {
                printf("XXX To Vermont light turning red XXX\n");
                sleep(1); // For dramatic effect
                printf("+++ To Hanover light turning green +++\n");
                green_to_hanover = true;
                signal_new_greenlight("to_hanover");
            }
            
        }

        // if (on_bridge_to_vermont == 0) {
        //     pthread_cond_broadcast(&open_to_hanover);
        // } else {
        //     pthread_cond_signal(&bridge_full);
        // }
        cars_finished_to_vermont += 1;
    } else {
        on_bridge_to_hanover -= 1;
        remaining_to_hanover -= 1;
        // printf("now %d on bridge to hanover with green_to_hanover as %d\n", on_bridge_to_hanover, green_to_hanover);

        // Green light is on, we can continue to let new cars on from our side
        if (green_to_hanover) {
            // Cars are queued for this green light
            if (queued_in_vermont > 0) {
                // printf("I think the bridge is full with WIH %d\n", queued_in_hanover);
                pthread_cond_signal(&bridge_full);
            } else {
                // printf("enter here\n");
                // Switch the light here no matter what if there are no cars left
                if (remaining_to_hanover == 0) {
                    printf("||| To Hanover light turning yellow |||\n");
                    sleep(1); // For dramatic effect
                    green_to_hanover = false;
                    printf("XXX To Hanover light turning red XXX\n");
                    sleep(1); // For dramatic effect
                    green_to_vermont = true;
                    printf("+++ To Vermont light turning green +++\n");
                    signal_new_greenlight("to_vermont");
                } else {
                    pthread_cond_signal(&cv_green_to_hanover);
                }
            }
            // Randomly switch the light with low probability relative to bridge size
            if (remaining_to_vermont > 0) {
                if (green_to_hanover && rand_bool(2 * MAX_CARS)) {
                    printf("||| To Hanover light turning yellow |||\n");
                    green_to_hanover = false;
                }
            }

        } else {
            // Allow the final waiting cars to go
            if (queued_in_vermont > 0) {
                pthread_cond_signal(&bridge_full);
            
            // We are safe - switch the lights and signal MAX_CARS cars on the other end
            } else if (on_bridge_to_hanover == 0) {
                // printf("signaling to vermont cars here\n");
                printf("XXX To Hanover light turning red XXX\n");
                sleep(1); // For dramatic effect
                green_to_vermont = true;
                printf("+++ To Vermont light turning green +++\n");
                signal_new_greenlight("to_vermont");
            }
        }
        // if (on_bridge_to_hanover == 0) {
        //     pthread_cond_broadcast(&open_to_vermont);
        // } else {
        //     pthread_cond_signal(&bridge_full);
        // }
        // pthread_cond_signal(&bridge_full);q
        cars_finished_to_hanover += 1;
    }
    // printf("\tExiting bridge from %s. Current GL status: TH: %d, TV: %d, QIH %d, QIV %d BTH %d BTV %d\n", direction, green_to_hanover, green_to_vermont
    // , queued_in_hanover, queued_in_vermont, on_bridge_to_hanover, on_bridge_to_vermont);
    pthread_mutex_unlock(&bridge_lock);
}

/**
 * @brief One vehicle takes the bridge lock and moves on the bridge
 * 
 * @param direction 
 */
void* one_vehicle(void* arg) {
    char* direction = (char*) arg;
    if (strcmp(direction, "to_hanover") != 0 && strcmp(direction, "to_vermont") != 0) {
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
    srand(time(NULL));
    // srand(1);

    total_to_vermont = atoi(argv[1]);
    total_to_hanover = atoi(argv[2]);

    // Coin flip for which green light starts first
    if (rand_bool(2)) {
        green_to_hanover = true;
    } else {
        green_to_vermont = true;
    }

    pthread_t all_car_threads[total_to_vermont + total_to_hanover];

    // Create all car threads for cars from hanover (this means Hanover may get a little bit of a head start...)
    for (int to_vermont_car = 0; to_vermont_car < total_to_vermont; to_vermont_car++) {
        pthread_create(&all_car_threads[to_vermont_car], NULL, one_vehicle, "to_vermont");
    }

    // Create all car threads for cars from vermont
    for (int to_hanover_car = 0; to_hanover_car < total_to_hanover; to_hanover_car++) {
        pthread_create(&all_car_threads[total_to_vermont + to_hanover_car], NULL, one_vehicle, "to_hanover");
    }

    // if (rand_bool(2)) {
    //     printf("green to hanover starts\n");
    //     green_to_hanover = true;
    //     signal_new_greenlight("to_hanover");
    // } else {
    //     printf("green to vermont starts\n");
    //     green_to_vermont = true;
    //     signal_new_greenlight("to_vermont");
    // }

    for (int thread = 0; thread < total_to_hanover + total_to_vermont; thread++) {
        pthread_join(all_car_threads[thread], NULL);
    }
    

    return 0;
}