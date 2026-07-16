# PWM LED control API

from payloads.payload import PwmLedCtlIn
from protocol.commands import CMD_PWM_LED_CTL
#from protocol.status import TlvStatus

MODE_MANUAL = 0
MODE_AUTO = 1

class PwmLedApi:

    def __init__(self, device):
        self.device = device

    def pwm_led_control(self, mode, param):

        payload = PwmLedCtlIn.pack(mode, param)
        response = self.device.execute(CMD_PWM_LED_CTL, payload)
        return response.status