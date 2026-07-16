import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from api.pwm_led import MODE_MANUAL
from config.F1_config import SERIAL_PORT, BAUD_RATE

def set_brightness():

    val = input("Enter a brightness value in range 0 .. 999:\r\n")

    if val.isdigit():
        dev = Device(SERIAL_PORT, BAUD_RATE)
        status = dev.pwm.pwm_led_control(MODE_MANUAL, int(val))
        print(f"Status = {status}")
    else:
        print("Invalid brightness value")

def main():
    set_brightness()

if __name__ == "__main__":
    main()
