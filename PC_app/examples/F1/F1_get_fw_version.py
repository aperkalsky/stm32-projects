import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device

from config.F1_config import SERIAL_PORT, BAUD_RATE

def read_version_single():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    result = dev.firmware.get_version()
    print(result)

def read_version_in_loop():
    dev = Device(SERIAL_PORT, BAUD_RATE)

    for i in range(10000):
        result = dev.firmware.get_version()
        print(f"i = {i} maj = {result.major} min = {result.minor}")
        time.sleep(0.05)

    print("Done")

def main():
    read_version_single()
#    read_version_in_loop()

if __name__ == "__main__":
    main()
