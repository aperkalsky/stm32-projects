# flash-related API commands

from payloads.payload import GetFlashIdOut, ReadFlashOut, ReadFlashIn
from protocol.commands import CMD_GET_FLASH_ID, CMD_READ_FLASH
from protocol.status import TlvStatus

class FlashApi:

    def __init__(self, device):
        self.device = device

    def get_id(self):

        response = self.device.execute(CMD_GET_FLASH_ID, b'')

        if response.status == TlvStatus.OK:
            return GetFlashIdOut.from_bytes(
                response.payload
            )

        return None

    def read(self, address: int, size: int):

        payload = ReadFlashIn.pack(address, size)
        response = self.device.execute(CMD_READ_FLASH, payload)

        if response.status == TlvStatus.OK and response.payload is not None:
            # Firmware returns a full flash page buffer; slice to requested size
            return ReadFlashOut.from_bytes(response.payload[:size])
        return None