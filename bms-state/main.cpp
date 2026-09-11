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
    - For max/min voltage and temps they can be definite. 
      The cells we use have a safe voltage range of 2.8V - 4.2V and a comfortable temp range of up to 45degC.

Iter 3 - 9/9/2026
    - It's starting to go into crunch time, but luckily I think I have a solid implementation plan I can both explain and code using our states
      and vitals.

Iter 4 - 9/10/2026
    - Enlightened by discharge and precharge logistics (thank you gene)

*/

enum BatteryState {
    START,
    STANDBY,
    PRECHARGE, //figure it out
    DRIVE,
    CHARGE,
    SOMEFAULT,
    SHUTDOWN
};

struct BatteryVitals {
    //constants from manual, static because we don't want them reevaluated or changed per program run, and because syntax lol
    const static float MAX_VOLT = 4.2;
    const static float MIN_VOLT = 2.8;
    const static float MIN_TEMP_CHARGE = 0;
    const static float MAX_TEMP_CHARGE = 45;
    const static float MIN_TEMP_DISCHARGE = -20;
    const static float MAX_TEMP_DISCHARGE = 60;
    const static float PRECHARGER_TIMEOUT = 2000;
    const static float CURRENT_DROP_THRESHOLD = 0.5;

    //vital variables
    float curr_volt;
    float curr_current;
    float curr_temp;
    float curr_inverter_volt; //inverter voltage should be subject to two things, timeout, and if the voltage exceeds or meets a target voltage


    //timing variables for discharge
    float curr_time;
    float precharge_time;

    //inputs
    bool start_cmd;
    bool stop_cmd;
    bool charge_cmd;
    bool clear_cmd;

    //hard stop diagnostics, stored as an integer to account for error codes and such, ideally paired with actual bits?
    int fault_err;
};

/*
    transitionLogic() will act as our main logic diagram, it'll verify safety checks, and perform state transfers based on vital data.
    Ideally, this is paired with some sort of a heartbeat system (a fast one) so to not miss crucial updates on the batteries vitals.
    The inputs are the current state, "currState", and the vitals of our car by reference (evaluate the struct).
*/
BatteryState transitionLogic(BatteryState currState, const BatteryVitals& currVitals){
    /*
        Place global safety checks here, ABSOLUTE checks.
    */
    /*
        Safety precheck, if the voltage doesn't adhere to the max or min, return a fault in voltage.
    */
    if (currVitals.curr_volt > BatteryVitals::MAX_VOLT || currVitals.curr_volt < BatteryVitals::MIN_VOLT){
        return SOMEFAULT;
    }
    /*
        If this runs an error code appeared and wasn't cleared.
    */
    if (currVitals.fault_err > 0){ 
        return SOMEFAULT;
    }
    switch(currState){
        case START:
            return STANDBY;
        case STANDBY:
            if (currVitals.start_cmd){
                return PRECHARGE;
            }
            if (currVitals.charge_cmd){
                return CHARGE;
            }
            break;
        case PRECHARGE:
            if (currVitals.stop_cmd){
                return STANDBY;
            }
            /*
                If the precharge time and the current time exceed the max timeout, somethings wrong.
            */
            if (currVitals.curr_time - currVitals.precharge_time > BatteryVitals::PRECHARGER_TIMEOUT){
                return SOMEFAULT;
            }

            /*
                Per fsae handbook, precharger should go up to 90 percent of the battery voltage ev 5.6.1
            */
            if (currVitals.curr_inverter_volt >= currVitals.curr_volt * 0.9){
                return DRIVE;
            }
            /*
                The precharge state should only be able to return a different state IF the conditions above are procced, otherwise we should stay in precharge
            */
            return PRECHARGE;
        case CHARGE:
            //
            if (currVitals.stop_cmd){
                return STANDBY;
            }
            //
            if (currVitals.curr_volt >= (BatteryVitals::MAX_VOLT - 0.05) && currVitals.curr_current <= BatteryVitals::CURRENT_DROP_THRESHOLD){
                return STANDBY;
            }
            return CHARGE;
        case DRIVE:
            if (currVitals.stop_cmd){
                return STANDBY;
            }
            return DRIVE;
        case SOMEFAULT:
            /*
                I think unless we verifiably clear some error command, we're still on somefault mode.
            */
            if (currVitals.clear_cmd){
                return STANDBY;
            }
            return SOMEFAULT;
        case SHUTDOWN:
            return SHUTDOWN;
            
    }
    return currState;
}

int main(){
    return 0;
}