import sys
import os

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from api.device import Device, SERIAL_PORT

dev = Device(SERIAL_PORT)

address = 0x00001000
size = 255

result = dev.flash.read(address, size)

if result is not None:
    print("Read OK")
    print(result.data.hex())
else:
    print("Read failed")