"""RFID USB keyboard reader -> Windows laptop -> Arduino Uno USB serial.

Install PySerial: py -m pip install pyserial
Close the Arduino IDE Serial Monitor before running this script.
Make sure this terminal window has keyboard focus when scanning a card.
"""
import time
import serial

ARDUINO_PORT = "COM3"
BAUD_RATE = 9600


AUTHORIZED_CARD_IDS = {"0013017736"}


def main():
    with serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=1) as arduino:

        time.sleep(3)
        arduino.reset_input_buffer()
        print("RFID access system ready.")
        print("Keep this terminal focused. Scan a card, then press Enter if necessary.")
        print("Press Ctrl+C to quit.\n")

        while True:
            card_id = input("Scan RFID card: ").strip()
            if not card_id:
                continue
            print(f"Detected card ID: {card_id!r}")
            if card_id in AUTHORIZED_CARD_IDS:
                arduino.write(b"A")
                arduino.flush()
                print("Authorized card. Rotate the MPU6050 around its Z-axis once within 12 seconds.")
            else:
                arduino.write(b"D")
                arduino.flush()
                print("Unauthorized card. Red LED should light for 2 seconds.")
            print()


if __name__ == "__main__":
    try:
        main()
    except serial.SerialException as exc:
        print(f"Could not open Arduino serial port: {exc}")
        print("Check ARDUINO_PORT and close Arduino IDE Serial Monitor.")
    except KeyboardInterrupt:
        print("\nStopped.")
