# return statuses available in protocol

from enum import IntEnum

class TlvStatus(IntEnum):

    TLV_STAT_OK                 = 0x0000
    TLV_STAT_TIMEOUT            = 0x0001
    TLV_STAT_NOT_IMPLEMENTED    = 0x0002
    TLV_STAT_INVALID_ARGUMENT   = 0x0003
    TLV_STAT_FAILURE            = 0x0004
    TLV_STAT_NO_ROOM            = 0x0005