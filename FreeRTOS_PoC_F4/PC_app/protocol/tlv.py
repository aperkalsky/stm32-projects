# The logic that builds and parses TLV requests

from dataclasses import dataclass
from typing import Optional
from protocol.crc import stm32_crc32
from protocol.status import TlvStatus
import struct

TLV_TYPE_SIZE   = 1
TLV_LENGTH_SIZE = 2
TLV_SEQ_SIZE    = 2
TLV_STATUS_SIZE = 2
TLV_CRC_SIZE    = 4

TLV_TX_HEADER_SIZE = (
    TLV_TYPE_SIZE +
    TLV_LENGTH_SIZE +
    TLV_SEQ_SIZE +
    TLV_STATUS_SIZE
)

@dataclass
class TlvRequest:
    cmd: int
    seq: int
    payload: Optional[bytes] = None

@dataclass
class TlvResponse:
    cmd: int
    seq: int
    status: TlvStatus
    payload: Optional[bytes] = None

def build_request(req: TlvRequest) -> bytes:

    payload = req.payload or b''

    hdr = struct.pack(
        "<BHH",
        req.cmd,
        len(payload),
        req.seq
    )

    crc = stm32_crc32(hdr + payload)

    return hdr + payload + struct.pack("<I", crc)

def parse_response(raw: bytes) -> TlvResponse:

    cmd, length, seq, status = struct.unpack(
        "<BHHH",
        raw[:TLV_TX_HEADER_SIZE]
    )

    payload = None

    if length > 0:
        payload = raw[TLV_TX_HEADER_SIZE:TLV_TX_HEADER_SIZE + length]

    return TlvResponse(
        cmd=cmd,
        seq=seq,
        status=TlvStatus(status),
        payload=payload
    )