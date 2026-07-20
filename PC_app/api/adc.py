# ADC-related API commands

from payloads.payload import GetTemperatureOut
from protocol.commands import CMD_GET_TEMPERATURE
from protocol.status import TlvStatus

class AdcApi:

    def __init__(self, device):
        self.device = device

    def get_temperature(self):

        response = self.device.execute(CMD_GET_TEMPERATURE, b'')

        if response.status == TlvStatus.OK:
            return GetTemperatureOut.from_bytes(
                response.payload
            )

        return None
