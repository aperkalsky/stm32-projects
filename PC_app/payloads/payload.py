# Payload definitions per TLV command (either Set or Get or both)

"""
                      format specifiers
+--------+--------------------+-------------+---------------+
| Format | C Type             | Python Type | Standard Size |
+--------+--------------------+-------------+---------------+
| x      | Pad byte           | No value    | 1 byte        |
| c      | char               | bytes (len1)| 1 byte        |
| b      | signed char        | int         | 1 byte        |
| B      | unsigned char      | int         | 1 byte        |
| ?      | _Bool              | bool        | 1 byte        |
| h      | short              | int         | 2 bytes       |
| H      | unsigned short     | int         | 2 bytes       |
| i      | int                | int         | 4 bytes       |
| I      | unsigned int       | int         | 4 bytes       |
| l      | long               | int         | 4 bytes       |
| L      | unsigned long      | int         | 4 bytes       |
| q      | long long          | int         | 8 bytes       |
| Q      | unsigned long long | int         | 8 bytes       |
| f      | float              | float       | 4 bytes       |
| d      | double             | float       | 8 bytes       |
| s      | char[]             | bytes       | Fixed size    |
+--------+--------------------+-------------+---------------+
"""

import struct
from dataclasses import dataclass
from struct import calcsize
from config.F4_config import FLASH_PAGE_SIZE

#--------------------------------------------
# direction IN - from the host to the board
# direction OUT - from the board to the host
#--------------------------------------------

UInt16 = int
UInt32 = int

class SerializablePayload:
    FORMAT = ""

    @classmethod
    def size(cls):
        return calcsize(cls.FORMAT)

@dataclass
class GetFwVersionOut(SerializablePayload):

    FORMAT = "<BB"

    major: int
    minor: int

    @classmethod
    def from_bytes(cls, payload):
        major, minor = struct.unpack(cls.FORMAT, payload)
        return cls(major, minor)

@dataclass
class GetFlashIdOut(SerializablePayload):

    FORMAT = "<I"

    id: int

    @classmethod
    def from_bytes(cls, payload):
        id = struct.unpack(cls.FORMAT, payload)
        return cls(id)

@dataclass
class ReadFlashIn(SerializablePayload):

    FORMAT = "<IH"   # address:uint32, size:uint16

    address: int
    size: int

    @classmethod
    def pack(cls, address: int, size: int) -> bytes:
        return struct.pack(cls.FORMAT, address, size)

@dataclass
class ReadFlashOut(SerializablePayload):

    data: bytes

    @classmethod
    def from_bytes(cls, payload: bytes):
        return cls(payload)

@dataclass
class WriteFlashIn(SerializablePayload):

    HEADER_FORMAT = "<IH"      # address:uint32, size (calculated internally):uint16

    address: int
    data: bytes

    @classmethod
    def pack(cls, address: int, data: bytes) -> bytes:
        size = len(data)

        if not (1 <= size <= FLASH_PAGE_SIZE):
            raise ValueError(
                f"data length must be between 1 and {FLASH_PAGE_SIZE} bytes"
            )

        return struct.pack(cls.HEADER_FORMAT, address, size) + data

@dataclass
class PwmLedCtlIn(SerializablePayload):

    FORMAT = "<BH"   # mode: uint8, param:uint16

    address: int
    size: int

    @classmethod
    def pack(cls, mode: int, param: int) -> bytes:
        return struct.pack(cls.FORMAT, mode, param)

@dataclass
class GetTemperatureOut(SerializablePayload):

    FORMAT = "<i"   # temperature: int32_t

    temperature: int

    @classmethod
    def from_bytes(cls, payload):
        temperature = struct.unpack(cls.FORMAT, payload)
        return cls(temperature)
