import sys
import os

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F1_config import SERIAL_PORT, BAUD_RATE
#from protocol import status

def say_hello():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    status = dev.speech.speech_say("Hello")
    print(f"Status = {status}")

def main():
    say_hello()

if __name__ == "__main__":
    main()
