import struct

import serial

from protocol.crc import stm32_crc32
from protocol.tlv import TLV_TX_HEADER_SIZE

class CdcTransport:

    def __init__(self, port):
        self.ser = serial.Serial(port, 115200)

    def __del__(self):
        if hasattr(self, 'ser') and self.ser and self.ser.is_open:
            self.ser.close()

    def _read_exact(self, size: int) -> bytes:

        data = bytearray()

        while len(data) < size:

            chunk = self.ser.read(size - len(data))

            if not chunk:
                raise TimeoutError(
                    f"Timeout while reading {size} bytes"
                )

            data.extend(chunk)

        return bytes(data)

    def send(self, data: bytes):
#        for byte in data:
#            print("-%02X-" % byte)
        self.ser.write(data)

    def receive(self) -> bytes:

        # Read response header
        header = self._read_exact(TLV_TX_HEADER_SIZE)

        # Extract payload length
        _, length, _, _ = struct.unpack(
            "<BHHH",
            header
        )

        # Read payload + CRC32
        payload_and_crc = self._read_exact(
            length + 4
        )

        # check CRC
        packet = header + payload_and_crc

        received_crc = struct.unpack(
            "<I",
            packet[-4:]
        )[0]

        calculated_crc = stm32_crc32(
            packet[:-4]
        )

        if received_crc != calculated_crc:
            print(f"CRC mismatch. Received{received_crc} expected{calculated_crc}")

        return packet