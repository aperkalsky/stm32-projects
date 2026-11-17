import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device

from config.F1_config import SERIAL_PORT, BAUD_RATE

def get_weather_single():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    result = dev.weather.get_weather()
    if result != None:
        print("Temperature = %.2f Humidity = %.2f Pressure = %.2f" % (result.temperature, result.humidity, result.pressure))
    else:
        print("Temperature reading failed")

def main():
    get_weather_single()

if __name__ == "__main__":
    main()
