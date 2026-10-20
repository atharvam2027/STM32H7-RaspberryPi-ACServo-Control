
import serial
import time

SERIAL_PORT = "/dev/ttyAMA0"
BAUD_RATE = 115200
TIMEOUT = 1


def validate_command(command):
    parts = command.strip().split(",")

    try:
        if parts[0] in ("M1", "M2") and len(parts) == 3:
            angle = float(parts[1])
            rpm = int(parts[2])
            return 0 < angle <= 25200 and 1 <= rpm <= 100

        elif parts[0] == "BOTH" and len(parts) == 3:
            angle = float(parts[1])
            rpm = int(parts[2])
            return 0 < angle <= 25200 and 1 <= rpm <= 100

        elif parts[0] == "BOTH" and len(parts) == 5:
            angle1 = float(parts[1])
            rpm1 = int(parts[2])
            angle2 = float(parts[3])
            rpm2 = int(parts[4])

            return (
                0 < angle1 <= 25200 and 1 <= rpm1 <= 100
                and 0 < angle2 <= 25200 and 1 <= rpm2 <= 100
            )

    except ValueError:
        return False

    return False


def main():
    try:
        with serial.Serial(
            SERIAL_PORT,
            BAUD_RATE,
            timeout=TIMEOUT,
            write_timeout=2
        ) as ser:

            time.sleep(2)
            ser.reset_input_buffer()

            print("Connected to STM32.")
            print("Commands: M1,angle,rpm | M2,angle,rpm")
            print("          BOTH,angle,rpm")
            print("          BOTH,angle1,rpm1,angle2,rpm2")
            print("Type 'exit' to quit.\n")

            while True:
                command = input("Command: ").strip()

                if command.lower() == "exit":
                    break

                if not validate_command(command):
                    print("Invalid command.")
                    continue

                ser.write((command + "\n").encode("ascii"))
                ser.flush()
                print("Sent:", command)

                start_time = time.time()
                acknowledged = False

                while time.time() - start_time < 5:
                    response = ser.readline().decode(
                        "utf-8", errors="replace"
                    ).strip()

                    if not response:
                        continue

                    print("STM32:", response)

                    if response == "OK":
                        acknowledged = True
                        break

                    if response == "ERR":
                        break

                if not acknowledged:
                    print("No OK acknowledgement received.")
                    continue

                print("Waiting for movement to finish...")

                start_time = time.time()
                movement_timeout = 300
                completed = False

                while time.time() - start_time < movement_timeout:
                    response = ser.readline().decode(
                        "utf-8", errors="replace"
                    ).strip()

                    if not response:
                        continue

                    print("STM32:", response)

                    if response == "DONE":
                        completed = True
                        print("Movement completed.\n")
                        break

                    if response == "ERR":
                        print("STM32 reported an error.")
                        break

                if not completed:
                    print("Movement completion not confirmed.")

    except serial.SerialException as error:
        print("Serial connection error:", error)

    except KeyboardInterrupt:
        print("\nProgram interrupted by user.")


if __name__ == "__main__":
    main()
