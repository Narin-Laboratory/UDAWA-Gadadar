# UDAWA Gadadar - Universal Digital Agriculture Workflow Assistant

[![PlatformIO Build Test](https://github.com/Narin-Laboratory/UDAWA-Gadadar/actions/workflows/build-test.yml/badge.svg)](https://github.com/Narin-Laboratory/UDAWA-Gadadar/actions/workflows/build-test.yml)

Welcome to the official firmware repository for UDAWA Gadadar, a key component of the UDAWA Smart System! This project is dedicated to empowering small-scale farmers with precision agriculture technology, making farming more efficient, sustainable, and fun.

## What is UDAWA?

UDAWA, which stands for **Universal Digital Agriculture Workflow Assistant**, is an open-source platform designed to bring the power of the Internet of Things (IoT) to agriculture. Our mission is to create a "peasant-centric" ecosystem of hardware and software tools that are affordable, easy to use, and adaptable to the needs of small farmers.

We believe that technology can help bridge the gap between traditional farming practices and modern, data-driven agriculture. By providing tools for monitoring, control, and automation, UDAWA helps farmers save time, reduce waste, and gain valuable insights into their operations.

## UDAWA Gadadar: The Smart Power Controller

UDAWA Gadadar is the muscle of the UDAWA ecosystem. It's a smart power controller that allows you to manage and automate various electrical appliances in your greenhouse or farm, such as:

- Pumps for irrigation and fertigation
- Fans and blowers for climate control
- Grow lights for supplemental lighting
- Misters and foggers for humidity control

### Key Features:

- **Remote Control:** Turn your farm equipment on and off from anywhere using your smartphone.
- **Automation:** Schedule tasks based on time, duration, or sensor data (e.g., turn on the fan when the temperature rises).
- **Energy Monitoring:** Keep track of your electricity consumption to optimize usage and reduce costs.
- **Modular Design:** UDAWA Gadadar comes with four independent channels, each with its own energy sensor.
- **Open Source:** The firmware is fully open-source, allowing you to customize and extend its functionality.

## Hardware

The UDAWA Gadadar hardware is designed to be robust, reliable, and easy to assemble. Here's a sneak peek at the device:

**Image Placeholder: UDAWA Gadadar Device**
`![UDAWA Gadadar Device](path/to/your/image.png)`

## Software

The UDAWA Gadadar firmware is built on the PlatformIO ecosystem and runs on the popular ESP32 microcontroller. It's packed with features to ensure seamless integration with the UDAWA cloud platform and mobile app.

**Image Placeholder: UDAWA Mobile App Screenshot**
`![UDAWA Mobile App](path/to/your/screenshot.png)`

## Getting Started

Ready to build your own UDAWA Gadadar? Here's how to get started:

1. **Set up your environment:**
   ```bash
   ./setup_env.sh
   ```
2. **Build the firmware:**
   ```bash
   pio run -e Gadadar4Ch
   ```
3. **Upload the firmware to your ESP32.**
4. **Provision your device using the UDAWA mobile app.**

For more detailed instructions, please refer to our documentation (coming soon!).

## Contributing

UDAWA is a community-driven project, and we welcome contributions from everyone! Whether you're a farmer, an engineer, a developer, or a hobbyist, there are many ways you can get involved:

- **Report bugs and suggest features** by opening an issue.
- **Submit pull requests** with your own code, documentation, or design improvements.
- **Share your UDAWA projects** with the community.

## Our Dearest Contributors

This project would not be possible without the amazing contributions of the following individuals:

- I Wayan Aditya Suranata
- I Nyoman Kusuma Wardana
- Ngakan Kutha Krisnawijaya
- Gede Humaswara Prathama
- Dhimas Prayoga
- I Made Surya Adi Putra
- I Gusti Ngurah Darma Paramartha
- Adie Wahyudi Oktavia Gama
- I Putu Widia Prasetya
- I Nyoman Hary Kurniawan
- I Made Pande Darma Yuda
- I Kadek Arta Wiguna
- I Gusti Gede Dhipa Andaresta
- Hendri Gunawan
- Andhika Bintar Lorenzo Kristian Reinnamah
- I Gusti Nathan Agung Tanaka
- Muhammad Zafran Zulkifli
- You

## License

UDAWA Gadadar is licensed under the **GNU Affero General Public License v3.0**. This means you are free to use, modify, and distribute the software, but you must also make your modifications available under the same license. For more details, see the [LICENSE](LICENSE) file.