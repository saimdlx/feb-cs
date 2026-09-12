/*
    Justification for BatteryState's
    STANDBY: Controls precharge and charge circuits, timestamps to check for errors.
    PRECHARGE: Controls precharge progress, relay to charge battery to safe threshold until standby.
    CHARGE: Safety checks the charging process for faults until a safe standby is met.
    DRIVE: State maintained until shutdown command is sent, can and should be altered for faults.
    SHUTDOWN: Activates discharge protocol, opens discharge path using shutdown circuit
    SOMEFAULT: Confirgurable error code manager that forces all relays and circuits to drain
*/
#include <iostream>
#include <cassert> 
/*
    Using standard assert library to ensure our state transitions go as planned.
*/

enum BatteryState {
    START,
    STANDBY,
    PRECHARGE,    
    CHARGE,
    DRIVE,
    SOMEFAULT,
    SHUTDOWN
};

struct BatteryVitals {
    /*
        These values were taken from the provided cell-battery manual, or the FSAE handbook. To stick with a no-AI
        approach, I referred to the data sheet and FSAE EV regulations as best as possible.
    */
    constexpr static float MAX_VOLT = 4.2;
    constexpr static float MIN_VOLT = 2.8;
    constexpr static float MIN_TEMP_CHARGE = 0;
    constexpr static float MAX_TEMP_CHARGE = 45;
    constexpr static float MIN_TEMP_DISCHARGE = -20;
    constexpr static float MAX_TEMP_DISCHARGE = 60;
    constexpr static float PRECHARGER_TIMEOUT = 2000;
    constexpr static float SHUTDOWN_TIMEOUT = 5000;
    constexpr static float CURRENT_DROP_THRESHOLD = 0.5;
    constexpr static float SAFE_INVERTER_THRESHOLD = 60;
    constexpr static float PACK_VOLT = 600;
    /*
        The precharge sequence needs to be 90% of the pack value, not one cell. hv battery is 600 volts
    */
    /*
        curr_volt is per cell battery
        curr_current is pack current
        curre_temp is cell temperature
        inverter_volt is used to manage precharge logic.
    */
    float curr_volt;
    float curr_current;
    float curr_temp;
    float curr_inverter_volt;
    /*
        Inverter voltage should be subject to two things, timeout, and if the voltage exceeds or meets a target voltage
    */

    //timing variables
    unsigned int curr_time;
    unsigned int precharge_time;
    unsigned int shutdown_time;

    //inputs
    bool start_cmd;
    bool stop_cmd;
    bool charge_cmd;
    bool clear_cmd;

    /*
        Hard stop diagnostics, stored as an integer to account for error codes and such.
    */
    int fault_err;
};

/*
    transitionLogic() will act as our main logic diagram, it'll verify safety checks, and perform state transfers based on vital data.
    Ideally, this is paired with some sort of a heartbeat system (a fast one) so to not miss crucial updates on the batteries vitals.
    The inputs are the current state, "currState", and the vitals of our car by reference (evaluate the struct).
*/
BatteryState transitionLogic(BatteryState currState, BatteryVitals& currVitals){
    /*
        Place global safety checks here, ABSOLUTE checks.
        Safety precheck, if the voltage doesn't adhere to the max or min, return a fault in voltage.
        Temperature check, if bounds are invalidated then return SOMEFAULT.
    */
    if (currVitals.curr_volt > BatteryVitals::MAX_VOLT || currVitals.curr_volt < BatteryVitals::MIN_VOLT){
        return SOMEFAULT;
    }
    if (currVitals.curr_temp > BatteryVitals::MAX_TEMP_DISCHARGE || currVitals.curr_temp < BatteryVitals::MIN_TEMP_DISCHARGE){
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
                currVitals.precharge_time = currVitals.curr_time;
                return PRECHARGE;
            }
            if (currVitals.charge_cmd){
                return CHARGE;
            }
            break;
        case PRECHARGE:
            if (currVitals.stop_cmd){
                currVitals.shutdown_time = currVitals.curr_time;
                return SHUTDOWN;
            }
            /*
                If the precharge time and the current time exceed the max timeout, somethings wrong.
            */
            if (currVitals.curr_time - currVitals.precharge_time > BatteryVitals::PRECHARGER_TIMEOUT){
                return SOMEFAULT;
            }

            /*
                Per fsae handbook, precharger should go up to 90 percent of the battery voltage EV 5.6.1
            */
            if (currVitals.curr_inverter_volt >= BatteryVitals::PACK_VOLT * 0.9){
                return DRIVE;
            }
            /*
                The precharge state should only be able to return a different state IF the conditions above are met, otherwise we should stay in precharge
            */
            return PRECHARGE;
        case CHARGE:
            if (currVitals.stop_cmd){
                currVitals.shutdown_time = currVitals.curr_time;
                return SHUTDOWN;
            }
            
            if(currVitals.curr_temp > BatteryVitals::MAX_TEMP_CHARGE || currVitals.curr_temp < BatteryVitals::MIN_TEMP_CHARGE){
                return SOMEFAULT;
            }

            if (currVitals.curr_volt >= (BatteryVitals::MAX_VOLT) && currVitals.curr_current <= BatteryVitals::CURRENT_DROP_THRESHOLD){
                return STANDBY;
            }
            return CHARGE;
        case DRIVE:
            if (currVitals.stop_cmd){
                currVitals.shutdown_time = currVitals.curr_time;
                return SHUTDOWN;
            }
            return DRIVE;
        case SOMEFAULT:
            /*
                Unless a clear_cmd is passed the system still keeps a somefault state.
            */
            if (currVitals.clear_cmd){
                return STANDBY;
            }
            return SOMEFAULT;
        case SHUTDOWN:
            /*
                Per FSAE, shutdown discharge time should be 5 seconds, anything greater returns a fault.
            */
            if (currVitals.curr_time - currVitals.shutdown_time > BatteryVitals::SHUTDOWN_TIMEOUT){
                return SOMEFAULT;
            }
            /*
                DC Voltage has to drop to below 60 to return to standby
            */
            if (currVitals.curr_inverter_volt < BatteryVitals::SAFE_INVERTER_THRESHOLD){
                return STANDBY;
            } 
            return SHUTDOWN;
    }
    return currState;
}

BatteryVitals great_vitals(){
    BatteryVitals goodv;
    goodv.curr_volt = 3.8;
    goodv.curr_current = 0;
    goodv.curr_temp = 25;
    goodv.curr_inverter_volt = 0;
    goodv.curr_time = 1000;
    goodv.precharge_time = 0;
    goodv.shutdown_time = 0;
    goodv.start_cmd = false;
    goodv.charge_cmd = false;
    goodv.stop_cmd = false;
    goodv.clear_cmd = false;
    goodv.fault_err = 0;
    return goodv;
};

void test_great_vitals(){
    std::cout << "Testing GREAT Vitals" << std::endl;
    BatteryVitals test = great_vitals();
    BatteryState states = START;

    states = transitionLogic(states, test);
    assert(states == STANDBY);

    test.start_cmd = true;
    states = transitionLogic(states, test);
    assert(states == PRECHARGE);
    assert(test.precharge_time == 1000);

    test.start_cmd = false;
    test.curr_time = 1500; 
    test.curr_inverter_volt = 540;
    states = transitionLogic(states, test);
    assert(states == DRIVE);

    test.stop_cmd = true;
    test.curr_time = 2000;
    states = transitionLogic(states, test);
    assert(states == SHUTDOWN);
    assert(test.shutdown_time == 2000);

    test.stop_cmd = false;
    test.curr_time = 3000;
    test.curr_inverter_volt = 20;
    states = transitionLogic(states, test);
    assert(states == STANDBY);

    /*
    State specific block to check for explicit error code clearance.
    */
    test.clear_cmd = true;
    states = transitionLogic(BatteryState::SOMEFAULT, test);
    assert(states == STANDBY);

    std::cout << "GREAT Vitals test cases passed" << std::endl;
};

void test_bad_vitals(){
    std::cout << "Testing BAD Vitals" << std::endl;
    {
        BatteryVitals badv = great_vitals();
        badv.curr_volt = 4.25; //overvolted
        BatteryState state = transitionLogic(BatteryState::DRIVE, badv);
        assert(state == BatteryState::SOMEFAULT);
    }
    {
        BatteryVitals badv = great_vitals();
        badv.curr_volt = 2.25; //undervolted
        BatteryState state = transitionLogic(BatteryState::DRIVE, badv);
        assert(state == BatteryState::SOMEFAULT);
    }
    {
        BatteryVitals badv = great_vitals();
        badv.curr_current = 10.0; //preventing early cuttoff due to voltage
        badv.curr_temp = 46.0; //exceeding max temperature
        BatteryState state = transitionLogic(BatteryState::CHARGE, badv);
        assert(state == BatteryState::SOMEFAULT);
    }    
    {
        BatteryVitals badv = great_vitals();
        badv.curr_current = 10; //preventing early cuttoff due to voltage
        badv.curr_temp = -1.0; //exceeding min temperature
        BatteryState state = transitionLogic(BatteryState::CHARGE, badv);
        assert(state == BatteryState::SOMEFAULT);
    }
    {
        BatteryVitals badv = great_vitals();
        badv.start_cmd = true;
        badv.curr_time = 2500;
        badv.precharge_time = 0;
        badv.curr_inverter_volt = 300; //Precharges failed
        BatteryState state = transitionLogic(BatteryState::PRECHARGE, badv);
        assert(state == BatteryState::SOMEFAULT);
    }
    {
        BatteryVitals badv = great_vitals();
        badv.curr_time = 6000;
        badv.shutdown_time = 0;
        badv.curr_inverter_volt = 150; //Failed to discharge below safe inverter threshold
        BatteryState state = transitionLogic(BatteryState::SHUTDOWN, badv);
        assert(state == BatteryState::SOMEFAULT);
    }
    {
        BatteryVitals badv = great_vitals();
        badv.curr_volt = 4.5;
        badv.clear_cmd = true; //attempt to clear error state WITHOUT addressing overvoltage fault
        BatteryState state = transitionLogic(BatteryState::SOMEFAULT, badv);
        assert(state == BatteryState::SOMEFAULT);
    }
    std::cout << "BAD Vitals test cases passed" << std::endl;
};


int main(){
    test_great_vitals();
    test_bad_vitals();
    return 0;
}