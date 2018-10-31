# lwm2m-client-rtl8710

C++ implementation of lwm2m protocol client for use with RTL8710 MCU based devices.

## Compiling/Flashing

Project built on PlatformIO core. Refer to PlatformIO documentation for detailed instructions on core installation and project configuration at: http://docs.platformio.org/en/latest/

Currently the project uses a custom PlatformIO platform, developed for Linux and Windows systems.

To compile project run:

    platformio run

This command will download all libraries, tools, frameworks and platforms necessary to build, flash and debug the project.

------------------------------------------------------------------------------------------------------------------------------

To upload project run:

    platformio run -t upload

Most RTL8710 based development boards include a CMSIS-DAP interface for device flashing and debugging, so proper driver installation might be needed.

Instructions on DAP firmware installation can be found [here](https://github.com/8devices/platformio-realtek-rtl8710b#dap-firmware).

## Lwm2m

The application uses the lwm2m protocol, and includes examples of how to implement GPIO, PWM, ADC, I2C and SPI features using lwm2m objects.

To choose between building SPI, I2C or ADC example uncomment the appropriate '#define' - 'SPI_MASTER_OBJ', 'I2C_MASTER_OBJ' or 'ADC_OBJ'. Only one of these objects can be used at the same time.

### GPIO

**Refer to lwm2m object id3312**
* Write 1/0 to resource 'OnOff' to set GPIO pin PA12 to either high or low.

### PWM

**Object id3306**
* Resource 'OnOff' turns PWM output on or off for pin PA_0.
* 'Dimmer' sets the duty cycle. Can be an integer value [0-100].
* 'OnTime' lets you specify a time in micro seconds that you want PWM to be on.

### ADC

**Object id3202**
* Read resource 'AnalogInputCurrentValue' to measure voltage on pin PA19.

### SPI

**Object id26241 (look for header file in 'include' directory)**
* 'Frequency' specifies the SPI clock frequency.
* 'Mode' specifies SPI mode.
* 'Buffer' - data to be sent to SPI slave devices. Read this resource to get last received data from slave devices.
* 'Length' - received data length.
* 'Transaction' - once you have specified a data buffer, execute this resource to start SPI write command. 'Buffer' resource gets overwritten by data received from slaves.

### I2C

**Object id26241**
* 'Frequency' specifies the I2C clock frequency.
* 'Slave_address' specifies I2C slave address.
* 'Buffer' - data to be sent to I2C slave devices. After read operation this resource is overwritten with data received from slaves. If you want *read only* operation, then buffer must be empty.
* 'Length' - if value is larger than 0 specifies number of bytes to be read, else specifies *write only* operation.
* 'Transaction' - execute this resource to start I2C write, read or write/read operation (depends on previously defined conditions).

## Configuration

Once powered on the device connects to an access point and server configured inside its flash memory. Configuration is done with HTTP requests while the device is in access point mode. To switch device into AP mode pull UART_LOG_TX pin to ground for at least three seconds, on most developments boards this can be done by holding the 'UART DOWNLOAD' button.

------------------------------------------------------------------------------------------------------------------------------

Once connected to device AP, you can view the current configuration by sending a GET request to "http://[GATEWAY_ADDR]:80/ap", ex.:

    curl http://192.168.4.1:80/ap

------------------------------------------------------------------------------------------------------------------------------

To keep current configuration send a GET request to "http://[GATEWAY_ADDR]:80/keep", ex.:

    curl http://192.168.4.1:80/keep

------------------------------------------------------------------------------------------------------------------------------

To change current configuration send POST request to "http://[GATEWAY_ADDR]:80/ap", with JSON payload, ex.:

    curl http://192.168.4.1:80/ap -d '{"ssid":"ap_ssid","pass":"ap_password","client_name":"example_client","server_address":"coap://192.168.0.1:5555"}' -H 'Content-type:application/json'

## Controling device

Lwm2m functionality was tested out using example lwm2m rest server located at: https://github.com/8devices/wakaama/tree/master-rest/examples/rest-server

Documentation on server usage and client control API is found at: https://github.com/8devices/wakaama/blob/master-rest/examples/rest-server/RESTAPI.md

For easier GUI based device control it is recommended to use Node-RED programming tool: https://nodered.org/

Lwm2m protocol compatible nodes for the Node-RED programming utility can be found at: https://github.com/8devices/node-red-contrib-lesley

## Debugging

*in development*

## License

* All code in this project is provided under The MIT License (MIT)

* Project uses sdk-ameba-v4.0b framework provided by Realtek

* Project uses WakaamaNode library which is under The MIT License (MIT), Copyright (c) 2016 Openhab-Nodes

* WakaamaNode library in turn depends on Wakaama which is under the Eclipse Public License - v2.0
