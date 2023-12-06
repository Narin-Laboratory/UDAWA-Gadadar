import argparse
import subprocess

def flash_esp32(ip_address, port, sketch_bin, ota_password):
    try:
        # Run espota.py command to flash the ESP32
        command = [
            "python", "espota.py",
            "--ip", ip_address,
            "--port", str(port),
            "--auth", ota_password,
            "--file", sketch_bin
        ]

        # Run the command
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = process.communicate()

        # Print the output and error
        print("Output:", output.decode())
        print("Error:", error.decode())

        # Check if the flashing was successful
        if "OK" in output.decode():
            print("ESP32 flashed successfully over-the-air.")
        else:
            print("Error flashing ESP32:", error.decode())

    except Exception as e:
        print("An error occurred:", str(e))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Flash ESP32 over-the-air using espota.py")
    parser.add_argument("--ip", required=True, help="ESP32 IP address")
    parser.add_argument("--port", type=int, default=8266, help="ESP32 port (default: 8266)")
    parser.add_argument("--file", required=True, help="Path to the compiled binary (firmware.bin)")
    parser.add_argument("--auth", required=True, help="OTA password")

    args = parser.parse_args()

    # Call the function with command-line arguments
    flash_esp32(args.ip, args.port, args.file, args.auth)
