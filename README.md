# lwm2m-client-rtl8710

C++ implementation of lwm2m protocol client for use with RTL8710 MCU based devices.

# Compiling/Flashing

Project built on PlatformIO core. Refer to PlatformIO documentation for detailed instructions on core installation and project configuration at: http://docs.platformio.org/en/latest/

Currently the project uses a custom PlatformIO platform, developed for Linux and Windows systems.

To compile project run:

    platformio run

This command will download all libraries, tools, frameworks and platforms necessary to build, flash and debug the project.

------------------------------------------------------------------------------------------------------------------------------

To upload project run:

    platformio run -t upload

Most RTL8710 based development boards include a CMSIS-DAP interface for device flashing and debugging, so proper driver installation might be needed.

------------------------------------------------------------------------------------------------------------------------------

# Lwm2m

The application uses the lwm2m protocol, and includes examples of how to implement GPIO, PWM, ADC, I2C and SPI features using lwm2m objects.

To choose between building SPI, I2C or ADC example uncomment the appropriate '#define' - 'SPI_MASTER_OBJ', 'I2C_MASTER_OBJ' or 'ADC_OBJ'. Only one of these objects must be used at the same time.

------------------------------------------------------------------------------------------------------------------------------

# Configuration

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

# Controling device

Lwm2m functionality was tested out using example lwm2m rest server located at: https://github.com/8devices/wakaama/tree/master-rest/examples/rest-server

Documentation on server usage and client control API is found at: https://github.com/8devices/wakaama/blob/master-rest/examples/rest-server/RESTAPI.md

------------------------------------------------------------------------------------------------------------------------------

For easier GUI based device control it is recommended to use Node-RED programming tool: https://nodered.org/

Lwm2m protocol compatible nodes for the Node-RED programming utility can be found at: https://github.com/8devices/node-red-contrib-lesley

# Debugging

*in development*

# License

* All code is provided under The MIT License (MIT)

* Application developed using Realtek provided framework, Copyright (c) Realtek

* Lwm2m protocol implementation is provided under The MIT License (MIT), Copyright (c) 2016 Openhab-Nodes
