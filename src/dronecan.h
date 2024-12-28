#ifndef ARDU_DRONECAN
#define ARDU_DRONECAN

#include <dronecan_msgs.h>
#include <Arduino.h>
#include <can.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define C_TO_KELVIN(temp) (temp + 273.15f)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

class DroneCAN
{
protected:
    uint8_t memory_pool[1024];
    struct uavcan_protocol_NodeStatus node_status;
    CanardCANFrame CAN_TX_msg;
    CanardCANFrame CAN_rx_msg;
    CanardCANFrame rx_frame;
    uint32_t looptime;
    bool led_state = false;

    struct firmware_update
    {
        char path[256];
        uint8_t node_id;
        uint8_t transfer_id;
        uint32_t last_read_ms;
        int fd;
        uint32_t offset;
    } fwupdate;

    struct parameter
    {
        char *name;
        enum uavcan_protocol_param_Value_type_t type;
        float value;
        float min_value;
        float max_value;
    };

public:
    void init(CanardOnTransferReception onTransferReceived, CanardShouldAcceptTransfer shouldAcceptTransfer);
    int node_id = 0;
    int preferred_node_id = 69;
    CanardInstance canard;
    parameter parameters[4] = {
        {"CAN_NODE", UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE, 0, 0, 127},
        {"MyPID_P", UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE, 1.2, 0.1, 5.0},
        {"MyPID_I", UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE, 1.35, 0.1, 5.0},
        {"MyPID_D", UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE, 0.025, 0.001, 1.0},
    };
    static uint64_t micros64();
    static void getUniqueID(uint8_t id[16]);
    void handle_GetNodeInfo(CanardRxTransfer* transfer);
    void handle_param_GetSet(CanardRxTransfer* transfer);
    void handle_param_ExecuteOpcode(CanardRxTransfer* transfer);
    int handle_DNA_Allocation(CanardRxTransfer *transfer);
    void request_DNA();
    void handle_begin_firmware_update(CanardRxTransfer* transfer);
    void send_firmware_read();
    void handle_file_read_response(CanardRxTransfer* transfer);
    void send_NodeStatus(void);
    void process1HzTasks(uint64_t timestamp_usec);
    void processTx();
    void processRx();
    void cycle();
    
    struct dynamic_node_allocation{
        uint32_t send_next_node_id_allocation_request_at_ms;
        uint32_t node_id_allocation_unique_id_offset;
    } DNA;

};

#endif // ARDU_DRONECAN