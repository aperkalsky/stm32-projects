import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device

from config.F1_config import SERIAL_PORT, BAUD_RATE

def read_temperature_single():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    result = dev.adc.get_temperature()
    if result != None:
        print("Temperature = %.2f" % ((float)(result.temperature[0]) / 100))
    else:
        print("Temperature reading failed")

def main():
    read_temperature_single()
#    read_version_in_loop()

if __name__ == "__main__":
    main()
