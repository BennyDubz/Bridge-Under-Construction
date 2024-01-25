# Bridge Under Construction
### Ben Williams '25 - benjamin.r.williams.25@dartmouth.edu

## Description

This program simulates traffic on the bridge under construction between Hanover, NH and Vermont. We represent vehicles as individual threads that coordinate using conditional variables. The cars need to prevent collisions as well as having too many vehicles on the bridge at once - all while allowing cars on both sides to travel (ie - no side gets starved).

## Usage

You can run the program by first running `make` in the command line, followed by `./bridge num_to_vermont num_to_hanover`, where both `num_to_vermont` and `num_to_hanover` are integers. If non-integers are entered for these two arguments, they will be treated as 0.

The program is designed to prevent starvation on any one side of the bridge by having a random chance to switch the green light at various points. These chances are proportional to the `MAX_CARS` which is defined at the top of the `bridge.c` file. To coincide with reality, we define `MAX_CARS` to be 5, but it will work with any integer `MAX_CARS > 0`. 

## Testing

Some simple tests can be done by either running `make test` or running `testing.sh`.


