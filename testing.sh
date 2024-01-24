#!/bin/bash
# Ben Williams '25
# Some simple test cases for the bridge

printf "\n\nTesting with no arguments\n"
./bridge

printf "\n\nTesting with too many arguments\n"
./bridge 1 2 3

printf "\n\nTesting with no cars at all\n"
./bridge 0 0

printf "\n\nTesting with 25 cars on each side\n"
./bridge 25 25

printf "\n\nTesting with 25 cars going to Vermont, and none to Hanover\n"
./bridge 25 0

printf "\n\nTesting with 25 cars going to Hanover, and none to Vermont\n"
./bridge 0 25
