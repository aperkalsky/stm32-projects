Note on PC communications protocol
==================================

The protocol design is common for both F1 and F4 boards: TLV structure, STM32 HW calculated CRC

The command set is common for both boards, each command handler can be inplemented on one board or both

This is the reason for PC_app folder to be outside the project-specific implementation

The serial port parameters for each board shall be specified in one place, each script will import the relevant one

How to add a new command handler to the PC app side
---------------------------------------------------

- Add a new command code to commands.py
- If there's a new return status, add it to status.py
- If there's a payload (Tx, Rx or both) - add a new data class to payload.py
- Extend existing API module or create a new one (under 'api')
- if new API module was created, add it to the Device class in device.py
- Add a new test script file (either generic or under the board folder)