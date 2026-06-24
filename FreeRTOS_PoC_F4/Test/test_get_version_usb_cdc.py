import serial
import struct
import sys

CMD_GET_FW_VERSION  = 0x01
CMD_GET_FLASH_ID    = 0x02

POLY = 0x04C11DB7


def stm32_crc32(data):
    crc = 0xFFFFFFFF

    padded = bytearray(data)

    while len(padded) % 4:
        padded.append(0)

    for i in range(0, len(padded), 4):

        word = (
            padded[i]
            | (padded[i + 1] << 8)
            | (padded[i + 2] << 16)
            | (padded[i + 3] << 24)
        )

        crc ^= word

        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ POLY) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF

    return crc


#
# Optional command-line argument:
#   python test_get_version_usb_cdc.py
#   python test_get_version_usb_cdc.py 100
#
repetitions = 1

if len(sys.argv) > 1:
    try:
        repetitions = int(sys.argv[1])

        if repetitions < 1:
            raise ValueError()

    except ValueError:
        print("Usage: python test_get_version_usb_cdc.py [repetitions]")
        sys.exit(1)


ser = serial.Serial(
    port="COM3",
    baudrate=115200,
    timeout=20
)

seq = 1
tx_payload = b''

for iteration in range(repetitions):

    packet = struct.pack(
        "<BHH",
        CMD_GET_FW_VERSION,
        len(tx_payload),
        seq
    )

    crc = stm32_crc32(packet) & 0xFFFFFFFF
    packet += struct.pack("<I", crc)

    print(f"\n=== Transaction {iteration + 1}/{repetitions} ===")
    print(f"Sending request with Seq={seq}")

    ser.write(packet)

    #
    # Read header
    #
    hdr = ser.read(7)

    if len(hdr) != 7:
        print("Timeout waiting for response")
        break

    resp_type, length, resp_seq, status = struct.unpack(
        "<BHHH",
        hdr
    )

    rx_payload = ser.read(length)

    if len(rx_payload) != length:
        print("Timeout reading payload")
        break

    crc_bytes = ser.read(4)

    if len(crc_bytes) != 4:
        print("Timeout reading CRC")
        break

    rx_crc = struct.unpack(
        "<I",
        crc_bytes
    )[0]

    calc_crc = stm32_crc32(
        hdr + rx_payload
    ) & 0xFFFFFFFF

    print(f"Type=0x{resp_type:02X}")
    print(f"Length={length}")
    print(f"Seq={resp_seq}")
    print(f"Status={status}")

    if rx_crc != calc_crc:
        print("CRC ERROR")
        print(f"Received CRC : 0x{rx_crc:08X}")
        print(f"Expected CRC : 0x{calc_crc:08X}")
        break

    if resp_type == CMD_GET_FW_VERSION:
        version = rx_payload.decode(
            "ascii",
            errors="ignore"
        )

        print("Version:", version)

    seq += 1

ser.close()