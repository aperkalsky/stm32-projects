# Writes random number of bytes into each page, then verifies it

import sys
import os
import random
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F4_config import SERIAL_PORT, BAUD_RATE
from config.F4_config import FLASH_SIZE, FLASH_PAGE_SIZE

dev = Device(SERIAL_PORT, BAUD_RATE)

# generates random programming pattern per page
# returns random offset and random number of random bytes
def get_prog_pattern():
    offset = random.randint(1, FLASH_PAGE_SIZE - 1)
    num_bytes = random.randint(1, FLASH_PAGE_SIZE - offset)

    # Generates all random bytes in a single, fast operation
    random_data = random.randbytes(num_bytes)

    return offset, bytearray(random_data)

def write_data(address, data):
    result = dev.flash.write(address, data)
    print(f"result = {result}")

def read_data(address, num_bytes):
    result = dev.flash.read(address, num_bytes)

    if result is not None:
        print("Read OK")
        return result.data.hex()
    else:
        print("Read failed")
        return None

def iterate_pages():
    for page_addr in range(0, FLASH_SIZE, FLASH_PAGE_SIZE):
        offset, bytes_to_write = get_prog_pattern()
        print(f"address = {hex(page_addr)} offset: {offset}, length: {len(bytes_to_write)} last_addr = {offset + len(bytes_to_write)} bytes: {bytes_to_write.hex()}")
        write_data(page_addr + offset, bytes_to_write)
#        time.sleep(0.01)
        bytes_read = read_data(page_addr + offset, len(bytes_to_write))
        print(f"bytes_read = {bytes_read}")
#        time.sleep(0.01)
        if bytes_read is not None:
            print(bytes_read)
            num_failures = 0
            bytes_written = bytes_to_write.hex()
            for i in range(len(bytes_to_write)):
                if bytes_written[i] != bytes_read[i]:
                    num_failures += 1

            if num_failures > 0:
                print(f"Comparison failed. Num failures: {num_failures}")
            else:
                print("Comparison passed")

if __name__ == "__main__":
    iterate_pages()
