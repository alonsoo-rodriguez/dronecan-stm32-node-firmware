### Arduino-DroneCAN

**Arduino-DroneCAN** is a PlatformIO/Arduino-based reference implementation of a DroneCAN/UAVCAN v0 node targeting STM32-based boards (for example `MicroNode`, `CoreNode`, and `MicroNodePlus`).
It wraps `libcanard` and the DroneCAN DSDL set in a small C++ helper (`DroneCAN` class) and demonstrates:

- **Dynamic node ID allocation (DNA)**
- **Node status and parameter management (`GetSet` / `ExecuteOpcode`)**
- **Firmware update over DroneCAN (`BeginFirmwareUpdate` + file read)**
- **Periodic telemetry publishing (for example `uavcan.equipment.power.BatteryInfo`)**

The project is intended both as a **ready-to-use node firmware** and as a **template** for building custom DroneCAN peripherals.

---

### Features

- **DroneCAN stack integration**
  - `libcanard`-based transport and message marshalling.
  - Full dynamic node ID allocation flow with unique ID derived from the STM32 CPU ID.
  - Node status broadcasting (`uavcan.protocol.NodeStatus`) at 1 Hz.

- **Parameter system**
  - Strongly-typed parameters (`INT`, `REAL`, `BOOLEAN`) exposed over DroneCAN.
  - Backed by EEPROM with min/max validation.
  - Implements `uavcan.protocol.param.GetSet` and `ExecuteOpcode` (save/erase).

- **Firmware update pipeline**
  - Handles `uavcan.protocol.file.BeginFirmwareUpdate` requests.
  - Fetches firmware chunks using `uavcan.protocol.file.Read`.
  - Passes state to the bootloader via a shared RAM structure, then resets into the bootloader.

- **Reference application**
  - Example battery telemetry publisher demonstrating periodic `BatteryInfo` messages and integration with the DroneCAN stack.
  - Illustrates a safe loop structure around `DroneCAN::cycle()` and a watchdog (`IWatchdog`).

- **Multi-board support via PlatformIO**
  - `Micro-Node` (with and without bootloader).
  - `Core-Node` (H743-based, CAN FD capable).
  - `Micro-Node-Plus` (H723-based, CAN FD capable).
  - Custom `variants/` definitions and linker scripts.

---

### Repository layout

- **`src/`**
  - `main.cpp`: reference node application.
    - Creates a `DroneCAN` instance and defines `custom_parameters` (for example `NODEID`, `PARM_1`, `PARM_2`).
    - Initializes the application and watchdog, then enters a `while (true)` loop instead of using `loop()`.
    - Publishes `uavcan.equipment.power.BatteryInfo` at 10 Hz and services the DroneCAN stack via `dronecan.cycle()`.

- **`lib/libArduinoDroneCAN/`**
  - Arduino library that encapsulates the DroneCAN stack.
  - `src/dronecan.cpp`: core `DroneCAN` class implementation.
    - CAN driver initialization (`CANInit`, `CANSend`, `CANReceive`).
    - `DroneCAN::init()` overloads (full callbacks versus convenience init).
    - Dynamic node ID allocation request/response logic.
    - Parameter storage, validation, and EEPROM persistence.
    - Node info and node status responders.
    - Firmware update request handling and file read helpers.
    - Receive/transmit queues and adapters for libcanard callbacks.
    - Minimal default handlers (`DroneCANonTransferReceived`, `DroneCANshouldAcceptTransfer`).

- **`lib/dronecan/`**
  - Auto-generated C sources and headers from DroneCAN DSDLs (for example `uavcan.equipment.*`, `uavcan.protocol.*`).
  - Built as part of the library; normally you do not edit these by hand.

- **`variants/`**
  - Board variant definitions for `MicroNode`, `CoreNode`, and `MicroNodePlus`.
  - Linker scripts (for example `ldscript.ld`, `ldscript-no-bootloader.ld`).
  - Pin mappings and peripheral configuration for the PlatformIO `ststm32` platform.

- **`examples/`**
  - Example applications (for example `thermocouple-MCP9600`, `INA_current_sensor`, `HC_SR04`) showing how to build custom DroneCAN-based sensors on top of this stack.

- **`platformio.ini`**
  - PlatformIO configuration defining environments:
    - `Micro-Node-Bootloader` (default).
    - `Micro-Node-No-Bootloader`.
    - `Core-Node-No-Bootloader`.
    - `Micro-Node-Plus-No-Bootloader`.
  - Configures the platform (`ststm32`), board, upload/debug tools (`stlink`), `variants_dir`, linker scripts, and compile-time flags (`-DCANL431`, `-DCANH743`, `-DACANFD`, and others).

- **`.gitmodules`**
  - References DroneCAN-related submodules (`dronecan_dsdlc`, `pydronecan`, `DSDL`, `libcanard`).

---

### Hardware and dependencies

- **Target MCUs**
  - STM32-based custom boards (`MicroNode`, `CoreNode`, `MicroNodePlus`).
  - Typical configuration:
    - On-chip ADC channels for battery voltage, current, and temperature.
    - On-chip CAN or CAN FD controller with an external transceiver.

- **Development environment**
  - **PlatformIO** (VSCode extension or CLI).
  - **Framework**: Arduino for STM32 (`framework = arduino`).
  - **Platform**: `ststm32`.
  - **Upload/Debug**: `stlink`.

- **DroneCAN stack dependencies**
  - `libcanard` (submodule).
  - DroneCAN DSDL repository and generated C types.

You will need a working CAN (or CAN FD, depending on the board) bus with proper termination and a DroneCAN-enabled flight controller or GUI tool (for example DroneCAN GUI or Mission Planner) to interact with the node.

---

### Getting started

#### 1. Clone and initialize submodules

```bash
git clone https://github.com/<your-org>/Arduino-DroneCAN.git
cd Arduino-DroneCAN
git submodule update --init --recursive
```

#### 2. Install PlatformIO

- **VSCode**: install the PlatformIO IDE extension.
- **CLI**:

```bash
pip install platformio
```

#### 3. Select the target environment

In `platformio.ini`, choose the environment that matches your hardware:

- `env:Micro-Node-Bootloader` (default, application paired with an existing bootloader).
- `env:Micro-Node-No-Bootloader`.
- `env:Core-Node-No-Bootloader`.
- `env:Micro-Node-Plus-No-Bootloader`.

From the PlatformIO CLI, specify an environment explicitly, for example:

```bash
pio run -e Micro-Node-Bootloader
```

#### 4. Build and flash

Connect your ST-LINK to the board, then:

```bash
# Build
pio run -e Micro-Node-Bootloader

# Flash
pio run -e Micro-Node-Bootloader -t upload

# Optional: open the serial monitor
pio device monitor -b 115200
```

On boot you should see diagnostic messages on the serial console. The node will request a dynamic node ID on the CAN bus and begin advertising `NodeStatus`.

---

### Configuring node parameters

Parameters are defined in `src/main.cpp` as `custom_parameters` and consumed by the `DroneCAN` class. The default set includes:

- **`NODEID`** (`INT`): preferred node ID (0–127). Used during dynamic node ID allocation to request a specific ID where possible.
- **`PARM_1`, `PARM_2`** (`REAL`): generic application parameters (0.0–100.0) that you can repurpose in your own application.

At runtime, these parameters can be inspected and modified using any DroneCAN parameter tool (for example the DroneCAN GUI or Mission Planner’s DroneCAN parameter interface):

- **Read/write by name or index**.
- **Persist to EEPROM** using `ExecuteOpcode(SAVE)`.
- **Reset to defaults** using `ExecuteOpcode(ERASE)`.

Within the firmware, you can manipulate parameters directly using:

- `dronecan.getParameter("PARM_1");`
- `dronecan.setParameter("PARM_1", 50.0f);`

Long-term changes should be persisted via the parameter protocol or via explicit EEPROM writes if you extend the storage model.

---

### Firmware update over DroneCAN

This firmware supports **over-the-bus firmware updates**:

1. A DroneCAN management tool sends `uavcan.protocol.file.BeginFirmwareUpdate` to the node.
2. `DroneCAN::handle_begin_firmware_update()`:
   - Stores update metadata (server node ID, image path, node ID) into a shared RAM structure at a fixed address.
   - Responds with a success code.
   - Flushes outstanding transfers and triggers an MCU reset (`NVIC_SystemReset()`).
3. On the next boot, the bootloader (not included in this repository) is expected to:
   - Read the shared RAM structure.
   - Use `uavcan.protocol.file.Read` to fetch firmware chunks.
   - Program the new image and then hand control back to the updated application.

If you implement your own bootloader, you must mirror the RAM layout and conventions used here (for example the `app_bootloader_comms` structure and base address) to complete the update pipeline.

---

### Extending the node

To build your own DroneCAN peripheral on top of this project:

1. **Define parameters** in `src/main.cpp` (or a dedicated application module).
2. **Add application logic** inside the `while (true)` loop in `setup()`:
   - Poll sensors.
   - Build DroneCAN messages using the DSDL-generated types under `lib/dronecan/include`.
   - Send messages via helper functions (for example `sendUavcanMsg(dronecan.canard, pkt);`).
3. **Add new message handlers** by:
   - Extending `DroneCANonTransferReceived()` with additional `data_type_id` cases.
   - Implementing corresponding methods on `DroneCAN` for clarity and reuse.
4. **Respect timing and watchdog constraints**:
   - Keep the loop non-blocking (avoid long `delay()` calls).
   - Call `dronecan.cycle()` as frequently as possible to maintain CAN throughput.
   - Ensure `IWatchdog.reload()` is called often enough wherever you add long-running logic.

---

### Development and testing

- **On-bus verification**
  - Use a DroneCAN GUI or a flight controller to verify:
    - Node presence and periodic `NodeStatus`.
    - Parameter enumeration and updates.
    - Firmware update workflow.
    - Custom data types (for example `BatteryInfo` and any sensor-specific messages you add).

- **Logging and debugging**
  - Use `DroneCAN::debug()` to send log messages over DroneCAN (`uavcan.protocol.debug.LogMessage`).
  - Use the serial port for early bring-up and non-real-time diagnostics.
  - ST-LINK GDB debugging is enabled via `debug_tool = stlink` and `debug_build_flags = -O0 -g`.

- **Tests**
  - PlatformIO supports unit tests via the `test/` directory; current validation is primarily hardware-in-the-loop.
  - For new logic, prefer small, testable helpers (pure C/C++ functions) which can be covered by PlatformIO’s testing framework.

---

### Contribution guidelines

- **Coding style**
  - Follow the existing C++ style in `lib/libArduinoDroneCAN`.
  - Prefer small, well-scoped functions with explicit ownership and lifetimes.
  - Avoid comments that simply restate obvious code; document protocol details, invariants, and hardware constraints instead.

- **Pull requests**
  - Keep changes logically scoped (for example protocol feature, board support, or bug fix).
  - Update or extend examples when adding new public-facing behaviour.
  - Test on hardware when changing CAN handling, timing, or firmware update paths and describe your test matrix in the pull request.

- **Submodules**
  - Do not commit generated code from modified DSDLs without aligning with the upstream DroneCAN repositories.
  - When updating submodules, bump them intentionally and note the upstream commit hashes in the pull request description.

---

### License

Add the correct license text or a pointer to the `LICENSE` file here.
If a `LICENSE` file exists in this repository, that file is the authoritative source.

