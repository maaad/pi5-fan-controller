# Pi5 Fan Controller

Standalone C++ application for temperature-based fan speed control on Raspberry Pi 5 with the official [Raspberry Pi Active Cooler](https://datasheets.raspberrypi.com/cooling/raspberry-pi-active-cooler-product-brief.pdf).

![Raspberry Pi Active Cooler](https://assets.raspberrypi.com/static/00d9d2ed445cf0573137a787bcbbd425/f2559/af3923c1-f1b1-40fd-8866-1baaf64086f9_Active%2BCooler.webp)

## Overview

This project provides a lightweight, systemd-integrated solution for controlling the Raspberry Pi Active Cooler fan speed based on CPU temperature. It reads temperature from multiple sensors and automatically adjusts fan speed through 5 levels (OFF, LOW, MEDIUM, HIGH, FULL) with configurable thresholds and hysteresis.

## Features

- Temperature-based fan speed control with 5 speed levels
- Reads temperature from multiple hwmon sensors (cpu_thermal and rp1_adc)
- Automatic fan speed adjustment based on configurable thresholds
- Hysteresis support to prevent rapid speed changes
- Comprehensive logging via systemd journald
- Systemd service integration
- No external dependencies - pure C++ standalone application
- Optimized for Raspberry Pi 5 with GCC optimizations

## Hardware Requirements

- Raspberry Pi 5
- [Raspberry Pi Active Cooler](https://datasheets.raspberrypi.com/cooling/raspberry-pi-active-cooler-product-brief.pdf) (official cooling solution)

## Software Requirements

- CMake 3.10 or higher
- C++17 compatible compiler (GCC/Clang)
- Linux system with systemd

## Building

```bash
git clone <repository-url>
cd pi5fancontroller
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

## Configuration

Configuration can be provided via:

1. **Configuration file**: `/etc/pi5-fan-controller/pi5-fan-controller.conf`
2. **Environment variables**: Set in `/etc/pi5-fan-controller/pi5-fan-controller.env` or systemd service
3. **Command line**: `--config <path>` option

### Configuration Parameters

- `FAN_PATH`: Path to fan control device (default: `/sys/class/thermal/cooling_device0/cur_state`)
- `HWMON0_NAME`: Name of first hwmon device (default: `cpu_thermal`)
- `HWMON1_NAME`: Name of second hwmon device (default: `rp1_adc`)
- `HYSTERESIS`: Temperature hysteresis in Celsius (default: 2.0)
- `OFF_THRESHOLD`: Temperature threshold for OFF speed (default: 53.0°C)
- `LOW_THRESHOLD`: Temperature threshold for LOW speed (default: 54.0°C)
- `MEDIUM_THRESHOLD`: Temperature threshold for MEDIUM speed (default: 59.0°C)
- `HIGH_THRESHOLD`: Temperature threshold for HIGH speed (default: 64.0°C)
- `FULL_THRESHOLD`: Temperature threshold for FULL speed (default: 70.0°C)
- `INTERVAL_SECONDS`: Control loop interval in seconds (default: 15)
- `DEBUG`: Enable debug logging (default: false)

## Installation

After building:

```bash
sudo make install
```

This will install:
- Executable: `/usr/local/bin/pi5_fan_controller`
- Systemd service: `/etc/systemd/system/pi5-fan-controller.service`
- Config file: `/etc/pi5-fan-controller/pi5-fan-controller.conf`

## Systemd Service

Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable pi5-fan-controller.service
sudo systemctl start pi5-fan-controller.service
```

Check status:

```bash
sudo systemctl status pi5-fan-controller.service
```

View logs:

```bash
sudo journalctl -u pi5-fan-controller.service -f
```

## Fan Speed Levels

The controller supports 5 fan speed levels:

- **OFF** (0): Fan is off
- **LOW** (1): Low speed
- **MEDIUM** (2): Medium speed
- **HIGH** (3): High speed
- **FULL** (4): Maximum speed

Speed is controlled via `/sys/class/thermal/cooling_device0/cur_state` by writing values 0-4.

## Temperature Control Logic

The controller reads temperature from two sensors and uses the average. Fan speed is determined by comparing the average temperature against configured thresholds:

- Temperature < `LOW_THRESHOLD`: OFF
- Temperature >= `LOW_THRESHOLD` and < `MEDIUM_THRESHOLD`: LOW
- Temperature >= `MEDIUM_THRESHOLD` and < `HIGH_THRESHOLD`: MEDIUM
- Temperature >= `HIGH_THRESHOLD` and < `FULL_THRESHOLD`: HIGH
- Temperature >= `FULL_THRESHOLD`: FULL

Hysteresis prevents rapid speed changes when temperature fluctuates near thresholds. When decreasing speed, the temperature must drop below the threshold minus the hysteresis value.

## Logging

Logs are written to systemd journal (journald) via stdout/stderr. View logs using:

```bash
sudo journalctl -u pi5-fan-controller.service -f
```

The application logs status changes (fan speed transitions) and temperature readings when debug mode is enabled.

## Troubleshooting

### Fan control file not found

Check if the fan control device exists:

```bash
ls -l /sys/class/thermal/cooling_device0/cur_state
```

### Temperature sensors not found

Check available hwmon devices:

```bash
ls -la /sys/class/hwmon/
cat /sys/class/hwmon/hwmon*/name
```

Update `HWMON0_NAME` and `HWMON1_NAME` in the configuration file if needed.

### Service fails to start

Check service logs:

```bash
sudo journalctl -u pi5-fan-controller.service -n 50
```

Check configuration:

```bash
sudo /usr/local/bin/pi5_fan_controller --help
```

### Permission denied

The service needs read/write access to thermal sensors and fan control. Ensure the service runs with appropriate permissions (typically as root or a user with access to `/sys/class/thermal/`).

## References

- [Raspberry Pi Active Cooler Product Brief](https://datasheets.raspberrypi.com/cooling/raspberry-pi-active-cooler-product-brief.pdf)
- [Raspberry Pi Active Cooler Image](https://assets.raspberrypi.com/static/00d9d2ed445cf0573137a787bcbbd425/f2559/af3923c1-f1b1-40fd-8866-1baaf64086f9_Active%2BCooler.webp)

## License

MIT License - see [LICENSE](LICENSE) file for details.

