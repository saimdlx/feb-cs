/*
Alright, before even thinking about the code we need to figure out what the hell a BMS system is, and what states and events it could possibly
take. 

We know that state machines are traditionally programmed in a STATE and EVENT sorta manner, with a switch statement acting as the
logic that changes and controls states.

Iter 1 - 9/5/26
    - Let's start by at least creating a programmed blueprint of what we want, that includes the states and the transitions for the battery
    - The next steps should be trying to get some sort of input, mock or not, and test some box logic with it using functions. 
      This could be seperate or just something we make in a class, but I prefer the functional approach.

Iter 2 - 9/7/2026
    - I want to polish what states I'll be working with before I move onto the logic for transitions, I've attempted to draw out a working
    state machine diagram. I'll sift through the FSAE ruleset, and proq google for rules and suggestions to data types.

    - I think we have a suitable state and vitals state machine, in both drawing and code.


*/

enum BatteryState {
    START,
    STANDBY,
    PRECHARGE,
    DRIVE,
    CHARGE,
    SOMEFAULT,
    SHUTDOWN
};

struct BatteryVitals {
    //sensor readings
    float voltage;
    float max_volt;
    float min_volt;
    float current;
    float max_temp;
    
    //inputs
    float volt_cap;
    bool start_cmd;
    bool charge_cmd;

    //hard stop diagnostics
    float fault_err;

};



int main(){
    return 0;
}