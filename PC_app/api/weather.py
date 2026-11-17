# PWM LED control API

from payloads.payload import GetWeatherOut
from protocol.commands import CMD_GET_WEATHER
from protocol.status import TlvStatus


class WeatherApi:

    def __init__(self, device):
        self.device = device

    def get_weather(self):
        response = self.device.execute(CMD_GET_WEATHER, b'')

        if response.status == TlvStatus.TLV_STAT_OK:
            return GetWeatherOut.from_bytes(
                response.payload
            )

        return None
