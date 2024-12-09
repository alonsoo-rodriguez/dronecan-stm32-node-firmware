#include <Arduino.h>
#include <can.h>
#include <dronecan.h>

CAN_msg_t CAN_rx_msg;

/*
  libcanard library instance and a memory pool for it to use
 */
static CanardInstance canard;
static uint8_t memory_pool[2048];


#define MY_NODE_ID 97


/*
  hold our node status as a static variable. It will be updated on any errors
 */
static struct uavcan_protocol_NodeStatus node_status;


/*
 This callback is invoked by the library when a new message or request or response is received.
*/
static void onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer)
{

    // switch on data type ID to pass to the right handler function
    // if (transfer->transfer_type == CanardTransferTypeRequest)
    // check if we want to handle a specific service request
    switch (transfer->data_type_id)
    {
    case UAVCAN_PROTOCOL_GETNODEINFO_ID:
    {
        Serial.println("NodeINFO");
        break;
    }
    case UAVCAN_EQUIPMENT_AHRS_MAGNETICFIELDSTRENGTH_ID:
    {
        uavcan_equipment_ahrs_MagneticFieldStrength pkt {};
        uavcan_equipment_ahrs_MagneticFieldStrength_decode(transfer, &pkt);
        Serial.print(pkt.magnetic_field_ga[0], 4); Serial.print(" ");
        Serial.print(pkt.magnetic_field_ga[1], 4); Serial.print(" ");
        Serial.print(pkt.magnetic_field_ga[2], 4); Serial.print(" ");
        Serial.println();
        break;
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
static bool shouldAcceptTransfer(const CanardInstance *ins,
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


/*
  send the 1Hz NodeStatus message. This is what allows a node to show
  up in the DroneCAN GUI tool and in the flight controller logs
 */
static void send_NodeStatus(void)
{
    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];

    node_status.uptime_sec = (uint64_t)micros() / 1000000ULL;
    node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    node_status.sub_mode = 0;
    // put whatever you like in here for display in GUI
    node_status.vendor_specific_status_code = 1234;

    uint32_t len = uavcan_protocol_NodeStatus_encode(&node_status, buffer);

    // we need a static variable for the transfer ID. This is
    // incremeneted on each transfer, allowing for detection of packet
    // loss
    static uint8_t transfer_id;

    int ret = canardBroadcast(&canard,
                    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                    UAVCAN_PROTOCOL_NODESTATUS_ID,
                    &transfer_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    buffer,
                    len);
    Serial.print(ret);
}

void processTx()
{
    for (const CanardCANFrame *txf = NULL; (txf = canardPeekTxQueue(&canard)) != NULL;)
    {
        CAN_msg_t CAN_TX_msg;
        CAN_TX_msg.id = txf->id;
        CAN_TX_msg.len = txf->data_len;
        CAN_TX_msg.format = EXTENDED_FORMAT;
        for(int i = 0; i < 8; i++)
        {
            CAN_TX_msg.data[i] = txf->data[i];
        }

        CANSend(&CAN_TX_msg);
        canardPopTxQueue(&canard); // fuck it we ball
    }
}


/*
  This function is called at 1 Hz rate from the main loop.
*/
static void process1HzTasks(uint64_t timestamp_usec)
{
    /*
      Purge transfers that are no longer transmitted. This can free up some memory
    */
    canardCleanupStaleTransfers(&canard, timestamp_usec);

    /*
     Transmit the node status message
     */
    send_NodeStatus();

    processTx();
}


void setup()
{
    Serial.begin(115200);
    Serial.println("HElloW");

    CANInit(CAN_1000KBPS, 2);
    // CANSetFilter(10);

    canardInit(&canard,
            memory_pool,
            sizeof(memory_pool),
            onTransferReceived,
            shouldAcceptTransfer,
            NULL);

    canardSetLocalNodeID(&canard, MY_NODE_ID);
}

uint32_t looptime = 0;

void loop()
{

    const uint32_t now = millis();

    if (CANMsgAvail())
    {
        CANReceive(&CAN_rx_msg);

        CanardCANFrame rx_frame;

        const uint64_t timestamp = micros();
        rx_frame.data_len = CAN_rx_msg.len;
        rx_frame.id = CAN_rx_msg.id;
        int i = 0;

        for (int i = 0; i < 8; i++)
        {
            rx_frame.data[i] = CAN_rx_msg.data[i];
        }

        int ret = canardHandleRxFrame(&canard, &rx_frame, timestamp);
    }

    if (now - looptime > 1000)
    {
        looptime = millis();
        process1HzTasks((uint64_t)micros());
    }

    
    
}
