#include <Arduino.h>
#include <dronecan.h>
#include <IWatchdog.h>

static bool shouldAcceptTransfer(const CanardInstance *ins,
                                 uint64_t *out_data_type_signature,
                                 uint16_t data_type_id,
                                 CanardTransferType transfer_type,
                                 uint8_t source_node_id);
static void onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer);
DroneCAN dronecan;

uint32_t looptime = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("Node Start");

    dronecan.init(onTransferReceived, shouldAcceptTransfer);

    IWatchdog.begin(2000000); // if the loop takes longer than 2 seconds, reset the system
}

void loop()
{
    const uint32_t now = millis();

    // send our battery message at 10Hz
    if (now - looptime > 100)
    {
        looptime = millis();

        // collect MCU core temperature data
        int32_t vref = __LL_ADC_CALC_VREFANALOG_VOLTAGE(analogRead(AVREF), LL_ADC_RESOLUTION_12B);
        int32_t cpu_temp = __LL_ADC_CALC_TEMPERATURE(vref, analogRead(ATEMP), LL_ADC_RESOLUTION_12B);

        // construct dronecan packet
        uavcan_equipment_power_BatteryInfo pkt{};
        pkt.voltage = now / 10000;
        pkt.temperature = cpu_temp;

        // boilerplate to send a message
        uint8_t buffer[UAVCAN_EQUIPMENT_POWER_BATTERYINFO_MAX_SIZE];
        uint32_t len = uavcan_equipment_power_BatteryInfo_encode(&pkt, buffer);
        static uint8_t transfer_id;
        canardBroadcast(&dronecan.canard,
                        UAVCAN_EQUIPMENT_POWER_BATTERYINFO_SIGNATURE,
                        UAVCAN_EQUIPMENT_POWER_BATTERYINFO_ID,
                        &transfer_id,
                        CANARD_TRANSFER_PRIORITY_LOW,
                        buffer,
                        len);
    }

    dronecan.cycle();
    IWatchdog.reload();
}

/*
This function is called when we receive a CAN message, and it's accepted by the shouldAcceptTransfer function.
We need to do boiler plate code in here to handle parameter updates and so on, but you can also write code to interact with sent messages here.
*/
void onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer)
{

    // switch on data type ID to pass to the right handler function
    // if (transfer->transfer_type == CanardTransferTypeRequest)
    // check if we want to handle a specific service request
    switch (transfer->data_type_id)
    {

    case UAVCAN_EQUIPMENT_AHRS_MAGNETICFIELDSTRENGTH_ID:
    {
        uavcan_equipment_ahrs_MagneticFieldStrength pkt{};
        uavcan_equipment_ahrs_MagneticFieldStrength_decode(transfer, &pkt);
        Serial.print(pkt.magnetic_field_ga[0], 4);
        Serial.print(" ");
        Serial.print(pkt.magnetic_field_ga[1], 4);
        Serial.print(" ");
        Serial.print(pkt.magnetic_field_ga[2], 4);
        Serial.print(" ");
        Serial.println();
        break;
    }
    }

    if (transfer->transfer_type == CanardTransferTypeBroadcast)
    {
        // check if we want to handle a specific broadcast message
        switch (transfer->data_type_id)
        {
        case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID:
        {
            dronecan.handle_DNA_Allocation(transfer);
            break;
        }
        }
    }
    // switch on data type ID to pass to the right handler function
    else if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        // check if we want to handle a specific service request
        switch (transfer->data_type_id)
        {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID:
        {
            dronecan.handle_GetNodeInfo(transfer);
            break;
        }
        case UAVCAN_PROTOCOL_RESTARTNODE_ID:
        {
            while (1)
            {
            }; // force the watchdog to reset
        }
        case UAVCAN_PROTOCOL_PARAM_GETSET_ID:
        {
            dronecan.handle_param_GetSet(transfer);
            break;
        }
        case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID:
        {
            dronecan.handle_param_ExecuteOpcode(transfer);
            break;
        }
        case UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID:
        {
            dronecan.handle_begin_firmware_update(transfer);
            break;
        }
        }
    }
}

/*
 This callback is invoked by the library when it detects beginning of a new transfer on the bus that can be received
 by the local node.
 If the callback returns true, the library will receive the transfer.
 If the callback returns false, the library will ignore the transfer.
 All transfers that are addressed to other nodes are always ignored.

 This function must fill in the out_data_type_signature to be the signature of the message.
 */
bool shouldAcceptTransfer(const CanardInstance *ins,
                          uint64_t *out_data_type_signature,
                          uint16_t data_type_id,
                          CanardTransferType transfer_type,
                          uint8_t source_node_id)

{
    if (transfer_type == CanardTransferTypeRequest)
    {
        // check if we want to handle a specific service request
        switch (data_type_id)
        {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID:
        {
            *out_data_type_signature = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_PARAM_GETSET_ID:
        {
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID:
        {
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID:
        {
            *out_data_type_signature = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_SIGNATURE;
            return true;
        }
        }
    }
    if (transfer_type == CanardTransferTypeResponse)
    {
        // check if we want to handle a specific service request
        switch (data_type_id)
        {
        case UAVCAN_PROTOCOL_FILE_READ_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_FILE_READ_SIGNATURE;
            return true;
        }
    }
    if (transfer_type == CanardTransferTypeBroadcast)
    {
        // see if we want to handle a specific broadcast packet
        switch (data_type_id)
        {
        case UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND_ID:
        {
            *out_data_type_signature = UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID:
        {
            *out_data_type_signature = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
            return true;
        }
        }
    }

    return false;
}