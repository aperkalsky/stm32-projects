import os

FILENAME = "random.bin"
TARGET_SIZE = 503103488  # 503,103,488 bytes
CHUNK_SIZE = 65536       # 64 KB chunks

print(f"Creating {FILENAME}...")

with open(FILENAME, "wb") as f:
    bytes_written = 0
    while bytes_written < TARGET_SIZE:
        # Calculate how many bytes are remaining for the final chunk
        remaining = TARGET_SIZE - bytes_written
        current_chunk = min(CHUNK_SIZE, remaining)
        
        # os.urandom provides cryptographically secure random bytes
        f.write(os.urandom(current_chunk))
        bytes_written += current_chunk

print(f"Success! Created {FILENAME} ({bytes_written} bytes).")
