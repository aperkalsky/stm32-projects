import sys
import os

# Adds the parent directory (PC_app) to the Python path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from api.device import Device

dev =  Device("COM3")
result = dev.flash.get_id()
print("Flash ID = %08X" % result.id)