import sys
import os

from protocol import status

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F1_config import SERIAL_PORT, BAUD_RATE

def say_hello():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    status = dev.speech.say("Hello")
    print(f"Status = {status}")

def main():
    say_hello()

if __name__ == "__main__":
    main()
