import sys
import os

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
    out_file = open("flash_contents.bin", "w")

    for addr in range(0, FLASH_SIZE, FLASH_PAGE_SIZE):
        result = dev.flash.read(addr, FLASH_PAGE_SIZE)

        if result is not None:
            print(f"Read from {addr} OK")
            out_file.write(result.data.hex())
        else:
            print(f"Read from {addr} failed")

    out_file.close()

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

    length = input("Enter the number of bytes to read (1..256):\r\n").strip()
    if length.isdigit():
        length = int(length)
        if length > 0 and length <= 256:
            result = dev.flash.read(address, length)

            if result is not None:
                print("Read OK")
                print(result.data.hex())
            else:
                print("Read failed")
        else:
            print("Number of bytes to read is out of range")

if __name__ == "__main__":
#    read_entire_chip()
#    read_one_shot()
#    read_entire_chip_in_chunks_of_128()
    read_interactive()

