# return statuses available in protocol

from enum import IntEnum

class TlvStatus(IntEnum):

    OK = 0x0000
    TIMEOUT = 0x0001
    NOT_IMPLEMENTED = 0x0002