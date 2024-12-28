#include <Arduino.h>
#include <can.h>
#include <dronecan.h>

static bool shouldAcceptTransfer(const CanardInstance *ins,
                                 uint64_t *out_data_type_signature,
                                 uint16_t data_type_id,
                                 CanardTransferType transfer_type,
                                 uint8_t source_node_id);
static void onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer);
DroneCAN dronecan;

void setup()
{
    Serial.begin(115200);
    Serial.println("Node Start");

    CANInit(CAN_1000KBPS, 2);
    dronecan.init(onTransferReceived, shouldAcceptTransfer);
}

void loop()
{
    dronecan.cycle();
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
    if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        // check if we want to handle a specific service request
        switch (transfer->data_type_id)
        {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID:
        {
            dronecan.handle_GetNodeInfo(transfer);
            break;
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

bool shouldAcceptTransfer(const CanardInstance *ins,
                          uint64_t *out_data_type_signature,
                          uint16_t data_type_id,
                          CanardTransferType transfer_type,
                          uint8_t source_node_id)
{   // see https://github.com/dronecan/libcanard/blob/6f74bc67656882a4ee51966c7c0022d04fa1a3fb/examples/ServoNode/servo_node.c#L664
    // for how to use properly
    return true;
}