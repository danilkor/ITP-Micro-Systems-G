# C++ MQTT CLI Client for House Remote Control

This project is a simple C++ command-line client for controlling smart home devices via MQTT.

## Features

- Connects to an MQTT broker
- Publishes control messages (e.g., turn lights on/off)
- Subscribes to device status topics

## Requirements

- C++17 or later
- [Paho MQTT C++ library](https://github.com/eclipse/paho.mqtt.cpp)
- MQTT broker (e.g., Mosquitto)

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./mqtt_cli --broker tcp://localhost:1883 --topic house/lights --message "ON"
```

## Configuration

Edit the source or use command-line arguments to set broker address, topics, and messages.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.