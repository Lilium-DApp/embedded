# Embedded device

## Components

- Main controller `ESP32-S3 WROOM Freenove`
- Temperature and humidity sensor `DHT11`
- CO sensor / Air quality `MQ135`

## Runtime
The main controller run the update of the sensors every one seconds and send then to `MQTT` server using the `ESP32-S3 WROOM Freenove` wifi connection, on topics:

- `Lilium/temperature`
- `Lilium/humidity`
- `Lilium/CO` 

## Dev Enviroment

The `ARDUINO IDE` platform was used in the development process. The complete source code is located in the "embedded" folder of this repository, while the libraries are stored in the "libraries" folder.
