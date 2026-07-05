import sys
import os

# Adds the parent directory (PC_app) to the Python path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from api.device import Device, SERIAL_PORT

dev =  Device(SERIAL_PORT)
result = dev.firmware.get_version()
print(result)