import sys
import os
from sysconfig import __main__

# Adds the parent directory (PC_app) to the Python path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from api.device import Device, SERIAL_PORT

def read_version_single():
    dev = Device(SERIAL_PORT)
    result = dev.firmware.get_version()
    print(result)

def read_version_in_loop():
    dev = Device(SERIAL_PORT)

    for i in range(1000):
        result = dev.firmware.get_version()
        print(f"i = {i} maj = {result.major} min = {result.minor}")

    print("Done")

def main():
    #    read_version_single()
    read_version_in_loop()

if __name__ == "__main__":
    main()
