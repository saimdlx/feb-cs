/*
    Justification for BatteryState's
    STANDBY: Controls precharge and charge circuits, timestamps to check for errors.
    PRECHARGE: Controls precharge progress and closes upon completion to move states.
    CHARGE: Safety checks the charging process for faults until a safe standby is met.
    DRIVE: State maintained until shutdown command is sent, can and should be altered for faults.
    SHUTDOWN: Proc's discharge protocol, opens drainage relays.
    SOMEFAULT: Confirgurable error code manager that forces all relays and circuits to drain

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
        approach, I referred to those two and reddit for any value suggestions.
    */
    const static float MAX_VOLT = 4.2;
    const static float MIN_VOLT = 2.8;
    const static float MIN_TEMP_CHARGE = 0;
    const static float MAX_TEMP_CHARGE = 45;
    const static float MIN_TEMP_DISCHARGE = -20;
    const static float MAX_TEMP_DISCHARGE = 60;
    const static float PRECHARGER_TIMEOUT = 2000;
    const static float SHUTDOWN_TIMEOUT = 5000;
    const static float CURRENT_DROP_THRESHOLD = 0.5;
    const static float SAFE_INVERTER_THRESHOLD = 60;
    const static float PACK_VOLT = 600;
    /*
        The precharge sequence needs to be 90% of the pack value, not one cell. hv battery is 600 volts
    */
    /*
        curr_volt is per cell battery, inverter_volt is used to manage precharge logic.
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
                The precharge state should only be able to return a different state IF the conditions above are procced, otherwise we should stay in precharge
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
                I think unless we verifiably clear some error command, we're still on somefault mode.
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

int main(){
    return 0;
}