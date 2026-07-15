# firmware-related API

from payloads.payload import GetFwVersionOut
from protocol.commands import CMD_GET_FW_VERSION
from protocol.status import TlvStatus

class FirmwareApi:

    def __init__(self, device):
        self.device = device

    def get_version(self):

        response = self.device.execute(CMD_GET_FW_VERSION, b'')

        if response.status == TlvStatus.OK:
            return GetFwVersionOut.from_bytes(
                response.payload
            )

        return None