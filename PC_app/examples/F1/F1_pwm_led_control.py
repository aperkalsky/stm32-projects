import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from api.pwm_led import MODE_MANUAL
from config.F1_config import SERIAL_PORT, BAUD_RATE

def set_brightness():
    val = input("Enter a brightness value in percents (0 .. 100):\r\n")

    if val.isdigit():
        dev = Device(SERIAL_PORT, BAUD_RATE)
        status = dev.pwm.pwm_led_control(MODE_MANUAL, int(val))
        print(f"Status = {status}")
    else:
        print("Invalid brightness value")

def imitate_breathe():
    delay_between_steps = 0.01
    num_loops = 5

    dev = Device(SERIAL_PORT, BAUD_RATE)

    for loop_index in range(num_loops):
        print(f"Loop {loop_index}")

        for i in range(100):
            dev.pwm.pwm_led_control(MODE_MANUAL, i)
            time.sleep(delay_between_steps)
        for i in range(99, 1, -1):
            dev.pwm.pwm_led_control(MODE_MANUAL, i)
            time.sleep(delay_between_steps)

def main():
#    set_brightness()
    imitate_breathe()

if __name__ == "__main__":
    main()
