# this class represents the target device as an instance that executes commands

from api.firmware import FirmwareApi
from api.flash import FlashApi
from api.common import CommonApi
from protocol.tlv import TlvResponse, TlvRequest, build_request, parse_response
from protocol.transport import CdcTransport

SERIAL_PORT = "COM5"

class Device:

    def __init__(self, port):

        self.transport = CdcTransport(port)

        self.firmware = FirmwareApi(self)
        self.flash = FlashApi(self)
        self.common = CommonApi(self)

        self._seq = 0

    def execute(self, cmd, payload) -> TlvResponse:

        request = TlvRequest(cmd, self._seq, payload)
        self._seq += 1

        print(request)
        self.transport.send(build_request(request))
        response_raw = self.transport.receive()
#        print(response_raw)
        return  parse_response(response_raw)
