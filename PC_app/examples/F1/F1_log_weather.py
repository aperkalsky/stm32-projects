import csv
import os
import sys
import time
from datetime import datetime

# Adds the parent directory (PC_app) to the Python path (two levels up)
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from api.device import Device
from config.F1_config import BAUD_RATE, SERIAL_PORT

POLLING_PERIOD_SECONDS = 5
SERIAL_READ_TIMEOUT_SECONDS = 1
WEATHER_VALUE_PRECISION = 2
TIMESTAMP_FORMAT = "%Y-%m-%d--%H-%M-%S"


def main():
    start_timestamp = datetime.now().strftime(TIMESTAMP_FORMAT)
    csv_filename = f"{start_timestamp}.csv"
    dev = None

    try:
        dev = Device(SERIAL_PORT, BAUD_RATE)
        dev.transport.ser.timeout = SERIAL_READ_TIMEOUT_SECONDS
        with open(csv_filename, "w", newline="", encoding="utf-8") as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(("timestamp", "temperature", "humidity", "pressure"))
            csv_file.flush()
            print(f"Logging weather measurements to {csv_filename}")

            measurements = 0
            while True:
                try:
                    result = dev.weather.get_weather()
                except OSError as exc:
                    print(f"\nConnection lost: {exc}")
                    break

                if result is None:
                    print("\nWeather request failed; stopping.")
                    break

                timestamp = datetime.now().strftime(TIMESTAMP_FORMAT)
                writer.writerow(
                    (
                        timestamp,
                        f"{result.temperature:.{WEATHER_VALUE_PRECISION}f}",
                        f"{result.humidity:.{WEATHER_VALUE_PRECISION}f}",
                        f"{result.pressure:.{WEATHER_VALUE_PRECISION}f}",
                    )
                )
                csv_file.flush()
                measurements += 1
                print(f"\rMeasurements: {measurements}", end="", flush=True)

                time.sleep(POLLING_PERIOD_SECONDS)
    except OSError as exc:
        print(f"Unable to connect to the device: {exc}")
    except KeyboardInterrupt:
        print("\nLogging interrupted.")
    finally:
        if dev is not None and dev.transport.ser.is_open:
            dev.transport.ser.close()


if __name__ == "__main__":
    main()
