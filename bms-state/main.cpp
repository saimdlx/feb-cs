/*
Alright, before even thinking about the code we need to figure out what the hell a BMS system is, and what states and events it could possibly
take. 

We know that state machines are traditionally programmed in a STATE and EVENT sorta manner, with a switch statement acting as the
logic that changes and controls states.

Iter 1 - 9/5/26
    - Let's start by at least creating a programmed blueprint of what we want, that includes the states and the transitions for the battery
    - The next steps should be trying to get some sort of input, mock or not, and test some box logic with it using functions. 
      This could be seperate or just something we make in a class, but I prefer the functional approach.
*/

#include <iostream>

enum BatteryState{
    START,
    STANDBY,
    CHARGE,
    DISCHARGE,
    SHUTDOWN,
    BADFAULT
};

struct BatteryVitals{

    float voltage;
    float current;
    float temperature;
    bool isFaulty;

};

int main(){
    return 0;
}