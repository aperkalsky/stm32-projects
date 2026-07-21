import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F4_config import SERIAL_PORT, BAUD_RATE

def read_id():
    dev =  Device(SERIAL_PORT, BAUD_RATE)
    result = dev.flash.get_id()
    print("Flash ID = %08X" % result.id)

def read_id_multiple():
    dev =  Device(SERIAL_PORT, BAUD_RATE)

    for i in range(100):
        result = dev.flash.get_id()
        print("Flash ID = %08X" % result.id)
        time.sleep(0.1)

if __name__ == "__main__":
#    read_id()
    read_id_multiple()