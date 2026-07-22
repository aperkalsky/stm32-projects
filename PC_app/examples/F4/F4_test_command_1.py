import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F4_config import SERIAL_PORT, BAUD_RATE

dev =  Device(SERIAL_PORT, BAUD_RATE)
start_time = time.perf_counter()
result = dev.common.test1()
stop_time = time.perf_counter()
print("Result = {} time elapsed = {}".format(result, stop_time - start_time))
