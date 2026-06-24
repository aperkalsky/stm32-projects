# STM32-compatible CRC32 implementation

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
