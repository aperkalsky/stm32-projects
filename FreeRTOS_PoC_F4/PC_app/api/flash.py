# flash-related API commands

from payloads.payload import GetFlashIdOut
from protocol.commands import CMD_GET_FLASH_ID
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