from api.device import Device

dev =  Device("COM3")
result = dev.firmware.get_version()
print(result)