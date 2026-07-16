# return statuses available in protocol

from enum import IntEnum

class TlvStatus(IntEnum):

    OK = 0x0000
    TIMEOUT = 0x0001
    NOT_IMPLEMENTED = 0x0002
    TLV_STAT_INVALID_ARGUMENT = 0x0003
    TLV_STAT_FAILURE = 0x0004