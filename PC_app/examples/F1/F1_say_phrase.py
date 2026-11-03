import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F1_config import SERIAL_PORT, BAUD_RATE
#from protocol import status

#PHRASE = "Hello"
#PHRASE = "Hi! I am Sam"
#PHRASE = "Hi Felix! How are you? Is everything good?"
PHRASE = "Hello! My name is SAM"
PHRASE1 = "I work as a speech synthesizer"
PHRASE2 = "on S T M thirty two"

def say_hello():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    status = dev.speech.speech_say(PHRASE)
    print(f"Status = {status}")

def say_greeting():
    dev = Device(SERIAL_PORT, BAUD_RATE)
    dev.speech.speech_say("Hello! Nice to meet you")
    time.sleep(2.5)
    dev.speech.speech_say("My name is SAM")
    time.sleep(2)
    dev.speech.speech_say("I work as a speech synthesizer")
    time.sleep(3)
    dev.speech.speech_say("on S T M thirty two")

def main():
    say_hello()
#    say_greeting()

if __name__ == "__main__":
    main()
