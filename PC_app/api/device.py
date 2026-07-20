# this class represents the target device as an instance that executes commands

from api.firmware import FirmwareApi
from api.flash import FlashApi
from api.common import CommonApi
from api.pwm_led import PwmLedApi
from api.adc import AdcApi
from protocol.tlv import TlvResponse, TlvRequest, build_request, parse_response
from protocol.transport import CdcTransport

class Device:

    def __init__(self, port, baud_rate):

        self.transport = CdcTransport(port, baud_rate)

        self.firmware = FirmwareApi(self)
        self.flash = FlashApi(self)
        self.common = CommonApi(self)
        self.pwm = PwmLedApi(self)
        self.adc = AdcApi(self)

        self._seq = 0

    def execute(self, cmd, payload) -> TlvResponse:

        request = TlvRequest(cmd, self._seq, payload)
        self._seq += 1

        self.transport.send(build_request(request))
        response_raw = self.transport.receive()
#        for byte in response_raw: print(" %02X " % byte)
        return  parse_response(response_raw)
