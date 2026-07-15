Note on PC communications protocol
==================================

The protocol design is common for both F1 and F4 boards: TLV structure, STM32 HW calculated CRC

The command set is common for both boards, each command handler can be inplemented on one board or both

This is the reason for PC_app folder to be outside the project-specific implementation

The serial port parameters for each board shall be specified in one place, each script will import the relevant one
