
#include "FreeRTOSConfig.h"
#include "device.h"
#include "task.h"
#include "diag.h"
#include "main.h"
#include "wifi_conf.h"
#include "serial_api.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <platform/platform_stdlib.h>
#include "netif.h"
#include "lwip_netconf.h"
#include "liblwm2m.h"
#include <sockets.h>
#include "test_object.h"

#define UART_TX    PA_23
#define UART_RX    PA_18
#define GPIO_LED_PIN        PA_5

int led_ctrl;
gpio_t gpio_led;
serial_t sobj;

char * server_addr = "coap://192.168.12.229:5555";
lwm2m_client_t *client_t; //reles programoj sitas globalus
lwm2m_object_t *test_object;

extern struct netif xnetif[NET_IF_NUM];

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
//	SUZIURET EILISUKUMA LWIP_INIT IR PVZ WIFI_CONNECT
	LwIP_Init();
	wifi_off();
	vTaskDelay(20);

	wifi_on(RTW_MODE_STA);
	wifi_set_autoreconnect(1);

	char *ssid = "rosettenet";
	char *password = "12345677";
	wifi_connect(ssid, RTW_SECURITY_WPA2_AES_PSK, password, strlen(ssid), strlen(password), -1, NULL);
	LwIP_DHCP(0, DHCP_START);

//	rtw_wifi_setting_t setting;
//	wifi_get_setting(WLAN0_NAME,&setting);
//	wifi_show_setting(WLAN0_NAME,&setting);

	vTaskDelete(NULL);
}

void print_thread(void)
{
	char buf[40];
	int tick = 0;

	while(1)
	{
		tick++;
		sprintf(buf, "print_thread, tick = %d\r\n", tick);
		uart_send_string(&sobj, buf);
		vTaskDelay(pdMS_TO_TICKS(400));
	}
}
#include "wakaama_client_internal.h"
void wakaama_thread(void)
{
#if 0
	lwm2m_object_t *objArray[4];
	objArray[0] = get_security_object(123, "coap://192.168.12.229:5555", false);
	securityObjP = objArray[0];
	objArray[1] = get_server_object(123, "U", 40, false);
	serverObject = objArray[1];
	objArray[2] = get_object_device();
	objArray[3] = get_object_firmware();
//	objArray[4] = get_object_led(); paciam apsirasyt

	client_t = lwm2m_init(prv_connect_server, prv_buffer_send, NULL);
	lwm2m_configure(client_t, "Ameba_client_1", NULL, NULL, 4, objArray);
	lwm2m_start(client_t);

	while(1)
	{
		time_t timeout;
		lwm2m_step(client_t, &timeout);
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
#endif


	client_t = lwm2m_client_init("ameba_client");

	test_object = lwm2m_object_create(3312, false, test_object_get_meta());
	lwm2m_object_instances_add(test_object, test_object_create_instance());
	lwm2m_add_object(client_t, test_object);

	lwm2m_network_init(client_t, NULL);

	lwm2m_add_server(123, server_addr, 30, false, NULL, NULL, 0);

	struct timeval tv;
	tv.tv_sec = 60;
	tv.tv_usec = 0;

	while(1)
	{
		lwm2m_step(client_t, &(tv.tv_sec));
		vTaskDelay(pdMS_TO_TICKS(4000));
	}
}

void main(void)
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
	xTaskCreate(STA_thread, ((const char*)"STA_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL);
//	xTaskCreate(print_thread, ((const char*)"print_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate(wakaama_thread, ((const char*)"wakaama_thread"), 2048, NULL, tskIDLE_PRIORITY + 2, NULL);

	vTaskStartScheduler();

	while(1)
	{
	}
}

