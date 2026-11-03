# PWM LED control API

from payloads.payload import SayIn
from protocol.commands import CMD_SAY

class SpeechApi:

    def __init__(self, device):
        self.device = device

    def speech_say(self, text):
        payload = SayIn.pack(text)
        response = self.device.execute(CMD_SAY, payload)
        return response.status