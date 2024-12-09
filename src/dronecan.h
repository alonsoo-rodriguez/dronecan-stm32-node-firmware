#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <Arduino.h>

// include the headers for the generated DroneCAN messages from the
// dronecan_dsdlc compiler
#include <dronecan_msgs.h>



static uint64_t micros64(void);
static void handle_GetNodeInfo(CanardInstance *ins, CanardRxTransfer *transfer);
static void onTransferReceived(CanardInstance *ins, CanardRxTransfer *transfer);
static bool shouldAcceptTransfer(const CanardInstance *ins,
                                 uint64_t *out_data_type_signature,
                                 uint16_t data_type_id,
                                 CanardTransferType transfer_type,
                                 uint8_t source_node_id);
static void send_NodeStatus(void);
static void process1HzTasks(uint64_t timestamp_usec);
