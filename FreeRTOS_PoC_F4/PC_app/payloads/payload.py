import struct
from dataclasses import dataclass
from struct import calcsize

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
