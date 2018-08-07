
//#include "diag.h"
//#include <stdint.h>
//#include <string.h>
//#include <stdarg.h>
//#include <platform/platform_stdlib.h>
//#include "netif.h"
//#include "lwip_netconf.h"
//#include <lwip/sockets.h>

//	WAKAMA
//#include "liblwm2m.h"
#include "internal_objects.h"
#include "internal.h"
//#include "lwm2m/context.h"
#include "lwm2m/c_connect.h"
#include "lwm2m/debug.h"
#include "lwm2mObjects/3312.h"
//	USER
#include "main.h"
//	GCC
#include "stdarg.h"
#include "stdio.h"
#include "sys/time.h"
//#include "stdlib.h"
//	LWIP
//#include "netconf.h"
#include "lwip_netconf.h"
//	SDK
#include "device.h"
#include "serial_api.h"
#include "wifi_conf.h"
#include "dhcp/dhcps.h"
#include "gpio_api.h"
//	RTOS
//#include "FreeRTOSConfig.h"
//#include "task.h"

#define UART_TX    PA_23
#define UART_RX    PA_18
#define GPIO_LED_PIN        PA_5

struct timeval tv;
int led_ctrl;
gpio_t gpio_led;
serial_t sobj;

char server_addr[30] = "coap://192.168.12.229:5555";

extern struct netif xnetif[NET_IF_NUM];

using namespace KnownObjects;
//lwm2m_client_context_t client_context;
//id3312::object relay;
//id3312::instance relayinst;

void AP_thread(void);
void STA_thread(void);
void wakaama_thread(void);
void print(const char *format, ...);

void print(const char *format, ...)
{
	va_list list;
	va_start(list, format);
	char buff[100];
	unsigned int i=0;

	vsprintf(buff, format, list);

	for(i = 0; buff[i] != 0; i++)
	{
		serial_putc(&sobj, buff[i]);
	}
	va_end(list);
}

void AP_thread(void)
{
	LwIP_Init();
	wifi_off();
	vTaskDelay(20);

	wifi_on(RTW_MODE_AP);

	char *ssid = "RTL_AP";
	rtw_security_t security_type = RTW_SECURITY_WPA2_AES_PSK;
	char *password = "12345678";
	int channel = 6;
	wifi_start_ap(ssid, security_type, password, strlen(ssid), strlen(password), channel);
	dhcps_init(&xnetif[0]);

	vTaskDelete(NULL);
}

void STA_thread(void)
{
//	SUZIURET EILISKUMA LWIP_INIT IR PVZ WIFI_CONNECT
	LwIP_Init();
	wifi_off();

	wifi_on(RTW_MODE_STA);
	wifi_set_autoreconnect(1);

	char *ssid = "rosettenet";
	char *password = "12345677";
	wifi_connect(ssid, RTW_SECURITY_WPA2_AES_PSK, password, strlen(ssid), strlen(password), -1, NULL);
	LwIP_DHCP(0, DHCP_START);

//	rtw_wifi_setting_t setting;
//	wifi_get_setting(WLAN0_NAME,&setting);
//	wifi_show_setting(WLAN0_NAME,&setting);

	xTaskCreate((TaskFunction_t)wakaama_thread, ((const char*)"wakaama_thread"), 10 * 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
	vTaskDelete(NULL);
}

void wakaama_thread(void)
{
	lwm2m_client_context_t client_context;
	id3312::object relay{};
	id3312::instance relayinst{};

    lwm2m_client_init(&client_context, "ameba_test");
    lwm2m_add_server(CTX(client_context), 123, server_addr, 30, false);

    client_context.deviceInstance.manufacturer = "test manufacturer";
    client_context.deviceInstance.model_name = "test model";
    client_context.deviceInstance.device_type = "sensor";
    client_context.deviceInstance.firmware_ver = "1.0";
    client_context.deviceInstance.serial_number = "140234-645235-12353";
#ifdef LWM2M_DEVICE_INFO_WITH_TIME
    client_context.deviceInstance.time_offset = 3;
    client_context.deviceInstance.timezone = "+03:00";
#endif

    relay.verifyWrite = [](id3312::instance* i, uint16_t res_id)
    {
            if(i->id == 0 && res_id == id3312::RESID::OnOff)
            {
            	print("/3312/0/5850 state: %d\r\n", i->OnOff);
//				digitalWrite(12, i->OnOff);
            }
            return true;
    };
    relayinst.id = 0;
    relay.addInstance(CTX(client_context), &relayinst);
    relay.registerObject(CTX(client_context), false);

    tv.tv_sec = 60;
    tv.tv_usec = 0;

	while(1)
	{
		lwm2m_process(CTX(client_context), &tv);
		vTaskDelay(pdMS_TO_TICKS(4000));
	}
}

int main(void)
{
	gpio_init(&gpio_led, GPIO_LED_PIN);
	gpio_dir(&gpio_led, PIN_OUTPUT);
	gpio_mode(&gpio_led, PullNone);

	serial_init(&sobj,UART_TX,UART_RX);
	serial_baud(&sobj,115200);
	serial_format(&sobj, 8, ParityNone, 1);

	led_ctrl = 1;
	gpio_write(&gpio_led, led_ctrl);

//	xTaskCreate(AP_thread, ((const char*)"AP_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate((TaskFunction_t)STA_thread, ((const char*)"STA_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL);

	vTaskStartScheduler();

	while(1)
	{
	}
}
