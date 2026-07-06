import sys
import os

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from api.device import Device, SERIAL_PORT
from api.flash import FLASH_SIZE, FLASH_PAGE_SIZE

dev = Device(SERIAL_PORT)

def read_one_shot():
#    address = 0x00000100
    address = 256
#    size = FLASH_PAGE_SIZE
    size = 128

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

if __name__ == "__main__":
#    read_entire_chip()
#    read_one_shot()
    read_entire_chip_in_chunks_of_128()

