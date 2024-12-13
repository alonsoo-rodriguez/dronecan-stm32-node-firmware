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
    Serial.println("HElloW");

    CANInit(CAN_1000KBPS, 2);
    dronecan.init(onTransferReceived, shouldAcceptTransfer);
}

uint32_t looptime = 0;

void loop()
{

    const uint32_t now = millis();
    dronecan.processRx();
    dronecan.processTx();
    dronecan.request_DNA();

    if (now - looptime > 1000)
    {
        looptime = millis();
        dronecan.process1HzTasks(dronecan.micros64());
    }
}

/*
 This callback is invoked by the library when a new message or request or response is received.
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
        }
    }
    // we don't want any other messages
    return true;
}