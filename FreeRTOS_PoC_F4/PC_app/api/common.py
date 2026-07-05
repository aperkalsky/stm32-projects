# common (general-purpose) API

from protocol.commands import CMD_TEST_1
from protocol.status import TlvStatus

class CommonApi:

    def __init__(self, device):
        self.device = device

    def test1(self):
        response = self.device.execute(CMD_TEST_1, b'')

        if response.status == TlvStatus.OK:
            return True
        else:
            return False
