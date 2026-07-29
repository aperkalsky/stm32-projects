import datetime
import sys
import os
import time

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F4_config import SERIAL_PORT, BAUD_RATE, FLASH_SIZE, FLASH_PAGE_SIZE

dev = Device(SERIAL_PORT, BAUD_RATE)

def read_one_shot():
#    address = 0x00000100
    address = 0x200
    size = FLASH_PAGE_SIZE

    result = dev.flash.read(address, size)

    if result is not None:
        print("Read OK")
        print(result.data.hex())
    else:
        print("Read failed")

def read_entire_chip():
    out_file = open("flash_contents.bin", "wb")

    start_time = time.perf_counter()

    for addr in range(0, FLASH_SIZE, FLASH_PAGE_SIZE):
        result = dev.flash.read(addr, FLASH_PAGE_SIZE)

        if result is not None:
            print(f"Read from {addr} OK")
            out_file.write(result.data)
        else:
            print(f"Read from {addr} failed")

    stop_time = time.perf_counter()

    out_file.close()

    elapsed_seconds = stop_time - start_time
    time_string = str(datetime.timedelta(seconds=int(elapsed_seconds)))

    print("Time elapsed = {}".format(time_string))

def read_entire_chip_in_chunks_of_128():
    chunk_size = int(FLASH_PAGE_SIZE / 2)

    # Using 'with' automatically closes the file when the block finishes
    with open("flash_contents_128.bin", "wb") as out_file:
        for addr in range(0, FLASH_SIZE, chunk_size):
            result = dev.flash.read(addr, chunk_size)
            if result is not None:
                print(f"Read from {addr} OK")
                out_file.write(result.data)
            else:
                print(f"Read from {addr} failed")

# to test a problem with few bytes reading in interrupt mode
def read_few_bytes_in_loop():
    num_bytes_to_read = 13
    max_address = 0x1000

    for addr in range(0, max_address, FLASH_PAGE_SIZE):
        result = dev.flash.read(addr, num_bytes_to_read)

        if result is not None:
            print(f"Read from {addr} OK")
            print(result.data.hex())
        else:
            print(f"Read from {addr} failed")

def read_interactive():
    address = input("Enter start address (dec/hex) to read from (xxx or 0xxx):\r\n").strip()

    if address.lower().startswith("0x"):
        try:
            address = int(address, 16)
        except ValueError:
            print("Error: Invalid hex characters after '0x'.")
            return
    elif address.isdecimal():
        address = int(address, 10)
    else:
        print("Error: Input must be a decimal number or a hex value prefixed with '0x'.")
        return

    length = input(f"Enter the number of bytes to read (1..{FLASH_PAGE_SIZE}):\r\n").strip()
    if length.isdigit():
        length = int(length)
        if length > 0 and length <= FLASH_PAGE_SIZE:
            result = dev.flash.read(address, length)

            if result is not None:
                print("Read OK")
                print(result.data.hex())
            else:
                print("Read failed")
        else:
            print("Number of bytes to read is out of range")

if __name__ == "__main__":
    read_entire_chip()
#    read_one_shot()
#    read_entire_chip_in_chunks_of_128()
#    read_interactive()
#    read_few_bytes_in_loop()

