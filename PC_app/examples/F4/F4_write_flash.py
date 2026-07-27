import sys
import os

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F4_config import SERIAL_PORT, BAUD_RATE
#from config.F4_config import FLASH_SIZE, FLASH_PAGE_SIZE

dev = Device(SERIAL_PORT, BAUD_RATE)

def simple_write():
    address = 0x00000202
    data = b"\x01\x02\x03\x04\x05"
#    size = FLASH_PAGE_SIZE

    result = dev.flash.write(address, data)

    print(result)

def main():
    simple_write()

if __name__ == "__main__":
    main()
