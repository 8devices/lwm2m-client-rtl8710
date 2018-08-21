
//#include "diag.h"
//#include <stdint.h>
//#include <string.h>
//#include <stdarg.h>
//#include <platform/platform_stdlib.h>
//#include "netif.h"
//#include "lwip_netconf.h"
//#include <lwip/sockets.h>

//	WAKAAMA
//#include "liblwm2m.h"
#include "internal_objects.h"
#include "internal.h"
//#include "lwm2m/context.h"
#include "lwm2m/c_connect.h"
#include "lwm2m/debug.h"
//	WAKAAMA OBJECTS
#include "lwm2mObjects/3312.h"
#include "lwm2mObjects/3306.h"
#include "lwm2mObjects/3202.h"
//	USER
#include "main.h"
//	GCC
#include "stdarg.h"
#include "stdio.h"
#include "sys/time.h"
#include "stdarg.h"
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
#include "pwmout_api.h"
#include "timer_api.h"
#include "analogin_api.h"
#include "sys_api.h"
#include "httpd/httpd.h"
#include "cJSON.h"
#include "flash_api.h"
//	RTOS
//#include "FreeRTOSConfig.h"
//#include "task.h"
//	TEST_OBJECTS
//#include "test_objects/pwm.h"

#define UART_LOG_TX PA_30
#define UART_TX    PA_23
#define UART_RX    PA_18
#define GPIO_LED_PIN        PA_5

struct timeval tv;
int led_ctrl;
gpio_t gpio_led;
gpio_t gpio_test;
pwmout_t gpio_pwm;
serial_t sobj;
gtimer_t timer;
analogin_t adcin;
lwm2m_client_context_t client_context;

extern int lwip_init_done;
extern struct netif xnetif[NET_IF_NUM];

struct network_info_t
{
	char sta_ssid[32];
	char sta_pass[64];
	char wak_server[64];
	char wak_client_name[32];
	uint32_t crc;
} network_info;
uint32_t poly = 0x82f63b78;
flash_t flash;

enum json_reply
{
	BUFFER_PARSE_FAIL = 1,
	JSON_MISSING,
	JSON_EMPTY,
};

const char get_reply[] = "Current config:\r\n	connect to AP: SSID - %s\r\n	lwm2m server address - %s\r\n	lwm2m client name - %s\r\n";

using namespace KnownObjects;
//lwm2m_client_context_t client_context;
//id3312::object relay;
//id3312::instance relayinst;

void AP_thread(void);
void STA_thread(void);
void wakaama_thread(void);
extern "C" {
void pwmout_start(pwmout_t* obj);
void pwmout_stop(pwmout_t* obj);
void gpio_deinit(gpio_t *obj);
}
int set_config_eeprom(void);
int get_config_eeprom(void);
uint32_t gen_crc(const network_info_t *buff);

void pwm_done_cb(void)
{
	pwmout_stop(&gpio_pwm);
}

void pwm_init(id3306::object* obj, id3306::instance* inst)
{
	pwmout_init(&gpio_pwm, PA_0);
	pwmout_write(&gpio_pwm, 0.5);
	pwmout_period_ms(&gpio_pwm, 300);

	gtimer_init(&timer, 1);

    obj->verifyWrite = [](id3306::instance* i, uint16_t res_id)
    {
//    	SWITCH CASE PADARYT
            if(res_id == id3306::RESID::OnOff)
            {
            	if(i->OnOff)
            	{
            		pwmout_start(&gpio_pwm);
            	}
            	else
            	{
            		pwmout_stop(&gpio_pwm);
            	}
            }
            else if(res_id == id3306::RESID::Dimmer)
            {
            	if(i->Dimmer < 0)
            		return false;
            	float percent = ((float)(i->Dimmer)) / 100;
            	pwmout_write(&gpio_pwm, percent);
            }
            else if(res_id == id3306::RESID::OnTime)
            {
            	gtimer_start_one_shout(&timer, i->OnTime, (void*)pwm_done_cb, 1);
            }
            return true;
    };
    inst->id = 0;
    obj->addInstance(CTX(client_context), inst);
    obj->registerObject(CTX(client_context), false);
}

void adc_init(id3202::object* obj, id3202::instance* inst)
{
	analogin_init(&adcin, PA_19);
    inst->id = 0;
    obj->addInstance(CTX(client_context), inst);
    obj->registerObject(CTX(client_context), false);
}

uint8_t handle_json(char *json)
{
	cJSON_Hooks memoryHook;
	memoryHook.malloc_fn = malloc;
	memoryHook.free_fn = free;
	cJSON_InitHooks(&memoryHook);

	cJSON *Object, *ssid, *pass, *client_name, *server_address;

	Object = cJSON_Parse(json);
	if(Object == NULL)
	{
		return BUFFER_PARSE_FAIL;
	}

	ssid = cJSON_GetObjectItem(Object, "ssid");
	pass = cJSON_GetObjectItem(Object, "pass");
	client_name = cJSON_GetObjectItem(Object, "client_name");
	server_address = cJSON_GetObjectItem(Object, "server_address");

	if(!(ssid && pass && client_name && server_address))
	{
		cJSON_Delete(Object);
		return JSON_MISSING;
	}
	if(!(ssid->valuestring[0]) || !(pass->valuestring[0]) || !(client_name->valuestring[0]) || !(server_address->valuestring[0]))
	{
		cJSON_Delete(Object);
		return JSON_EMPTY;
	}

	memset(&network_info, 0, sizeof(network_info_t));
	strncpy(network_info.sta_ssid, ssid->valuestring, sizeof(network_info.sta_ssid));
	strncpy(network_info.sta_pass, pass->valuestring, sizeof(network_info.sta_pass));
	strncpy(network_info.wak_client_name, client_name->valuestring, sizeof(network_info.wak_client_name));
	strncpy(network_info.wak_server, server_address->valuestring, sizeof(network_info.wak_server));
	network_info.crc = gen_crc(&network_info);
	set_config_eeprom();
	cJSON_Delete(Object);
	return 0;
}

void keep_cb(struct httpd_conn *conn)
{
	char *response = "Keeping current configuration\r\n";
	httpd_response_write_header_start(conn, "200 OK", "text/plain", strlen(response));
	httpd_response_write_header(conn, "Connection", "close");
	httpd_response_write_header_finish(conn);
	httpd_response_write_data(conn, (uint8_t*)response, strlen(response));
	httpd_conn_close(conn);
	httpd_stop();

	xTaskCreate((TaskFunction_t)STA_thread, ((const char*)"STA_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL);
}

void ap_cb(struct httpd_conn *conn)
{
	char *response;
	if(httpd_request_is_method(conn, "GET"))
	{
		get_config_eeprom();
		if(gen_crc(&network_info) != network_info.crc)
		{
			printf("CRC doesn't match\r\n");
			printf("CRC: 0x%08x\r\n", network_info.crc);
			response = "Device memory corrupt, reconfigure with POST request\r\n";
			httpd_response_write_header_start(conn, "500 INTERNAL SERVER ERROR", "text/plain", strlen(response));
		}
		else
		{
			sprintf(response, get_reply, network_info.sta_ssid, network_info.wak_server, network_info.wak_client_name);
			httpd_response_write_header_start(conn, "200 OK", "text/plain", strlen(response));
		}
		httpd_response_write_header(conn, "Connection", "close");
		httpd_response_write_header_finish(conn);
		httpd_response_write_data(conn, (uint8_t*)response, strlen(response));
		httpd_conn_close(conn);
		return;
	}
	else if(httpd_request_is_method(conn, "POST"))
	{
		char buffer[200];
		httpd_request_read_data(conn, (uint8_t*)buffer, 200);
		uint8_t status = handle_json(buffer);
		if(status)
		{
			switch(status)
			{
			case BUFFER_PARSE_FAIL:
				response = "Couldn't parse buffer\r\n";
				httpd_response_write_header_start(conn, "500 INTERNAL SERVER ERROR", "text/plain", strlen(response));
				break;
			case JSON_MISSING:
				response = "One or more JSON attributes missing\r\n";
				httpd_response_write_header_start(conn, "400 BAD REQUEST", "text/plain", strlen(response));
				break;
			case JSON_EMPTY:
				response = "One or more JSON attributes value empty\r\n";
				httpd_response_write_header_start(conn, "400 BAD REQUEST", "text/plain", strlen(response));
				break;
			}
			httpd_response_write_header(conn, "Connection", "close");
			httpd_response_write_header_finish(conn);
			httpd_response_write_data(conn, (uint8_t*)response, strlen(response));
			httpd_conn_close(conn);
			return;
		}
		response = "Configuration set, device switching to station mode\r\n";
		httpd_response_write_header_start(conn, "200 OK", "text/plain", strlen(response));
		httpd_response_write_header(conn, "Connection", "close");
		httpd_response_write_header_finish(conn);
		httpd_response_write_data(conn, (uint8_t*)response, strlen(response));
		httpd_conn_close(conn);
		httpd_stop();
		xTaskCreate((TaskFunction_t)STA_thread, ((const char*)"STA_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL);
	}
	else
	{
		httpd_response_method_not_allowed(conn, NULL);
		httpd_conn_close(conn);
	}
}

void AP_thread(void)
{
	wifi_off();
	if(!lwip_init_done) LwIP_Init();
	wifi_on(RTW_MODE_AP);

	char *ssid = "RTL_AP";
	rtw_security_t security_type = RTW_SECURITY_WPA2_AES_PSK;
	char *password = "12345678";
	int channel = 6;
	wifi_start_ap(ssid, security_type, password, strlen(ssid), strlen(password), channel);
	dhcps_init(&xnetif[0]);

	httpd_reg_page_callback("/keep", keep_cb);
	httpd_reg_page_callback("/ap", ap_cb);
	httpd_start(80, 5, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_NONE);

	vTaskDelete(NULL);
}

void STA_thread(void)
{
	get_config_eeprom();
	if(gen_crc(&network_info) != network_info.crc)
	{
		printf("CRC doesn't match\r\n");
		printf("CRC: 0x%08x\r\n", network_info.crc);
		vTaskDelete(NULL);
	}

	wifi_off();
	if(!lwip_init_done) LwIP_Init();
	wifi_on(RTW_MODE_STA);
	wifi_set_autoreconnect(1);

	wifi_connect(network_info.sta_ssid, RTW_SECURITY_WPA2_AES_PSK, network_info.sta_pass, strlen(network_info.sta_ssid), strlen(network_info.sta_pass), -1, NULL);
	LwIP_DHCP(0, DHCP_START);

//	rtw_wifi_setting_t setting;
//	wifi_get_setting(WLAN0_NAME,&setting);
//	wifi_show_setting(WLAN0_NAME,&setting);

	xTaskCreate((TaskFunction_t)wakaama_thread, ((const char*)"wakaama_thread"), 2 * 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
	vTaskDelete(NULL);
}

void wakaama_thread(void)
{
	id3312::object relay{};
	id3312::instance relayinst{};
	id3306::object pwm{};
	id3306::instance pwminst{};
	id3202::object adc{};
	id3202::instance adcinst{};

    lwm2m_client_init(&client_context, network_info.wak_client_name);
    lwm2m_add_server(CTX(client_context), 123, network_info.wak_server, 30, false);

    client_context.deviceInstance.manufacturer = "test manufacturer";
    client_context.deviceInstance.model_name = "test model";
    client_context.deviceInstance.device_type = "sensor";
    client_context.deviceInstance.firmware_ver = "1.0";
    client_context.deviceInstance.serial_number = "140234-645235-12353";
#ifdef LWM2M_DEVICE_INFO_WITH_TIME
    client_context.deviceInstance.time_offset = 3;
    client_context.deviceInstance.timezone = "+03:00";
#endif

//    relay.verifyWrite = [](id3312::instance* i, uint16_t res_id)
//    {
//            if(i->id == 0 && res_id == id3312::RESID::OnOff)
//            {
//            	printf("/3312/0/5850 state: %d\r\n", i->OnOff);
//            	gpio_write(&gpio_led, i->OnOff);
//            	float percentage = ((gpio_pwm.pulse)/(gpio_pwm.period));
//            	if(i->OnOff)
//            		pwmout_write(&gpio_pwm, percentage + 0.1);
//            	else
//            		pwmout_write(&gpio_pwm, percentage - 0.1);
//            }
//            return true;
//    };
//    relayinst.id = 0;
//    relay.addInstance(CTX(client_context), &relayinst);
//    relay.registerObject(CTX(client_context), false);

	pwm_init(&pwm, &pwminst);
	adc_init(&adc, &adcinst);

    tv.tv_sec = 60;
    tv.tv_usec = 0;

	while(1)
	{
		adcinst.AnalogInputCurrentValue = analogin_read(&adcin);
		printf("ADC value = %f\r\n", adcinst.AnalogInputCurrentValue);
		lwm2m_process(CTX(client_context), &tv);
		lwm2m_watch_and_reconnect(CTX(client_context), &tv, 20);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

int main(void)
{
//	PMAP_Init();
	gpio_init(&gpio_test, UART_LOG_TX);
	gpio_dir(&gpio_test, PIN_INPUT);
	gpio_mode(&gpio_test, PullUp);

//	if(!gpio_read(&gpio_test))
//	{
		xTaskCreate((TaskFunction_t)AP_thread, ((const char*)"AP_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL);
//	}
//	else
//	{
//		xTaskCreate((TaskFunction_t)STA_thread, ((const char*)"STA_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL);
//	}

	gpio_deinit(&gpio_test);
	sys_log_uart_on();

//	Pinmux_Config((uint8_t)(pmap_func[].PinName), pmap_func[].Function);
//	Pinmux_Config((uint8_t)(pmap_func[37].PinName), pmap_func[37].Function);
//	Pinmux_Config((uint8_t)(pmap_func[38].PinName), pmap_func[38].Function);

//	gpio_init(&gpio_led, GPIO_LED_PIN);
//	gpio_dir(&gpio_led, PIN_OUTPUT);
//	gpio_mode(&gpio_led, PullNone);
//
//	serial_init(&sobj,UART_TX,UART_RX);
//	serial_baud(&sobj,115200);
//	serial_format(&sobj, 8, ParityNone, 1);

	vTaskStartScheduler();

	while(1)
	{
	}
}

int set_config_eeprom(void)
{
	flash_erase_sector(&flash, FLASH_APP_BASE);
	return flash_stream_write(&flash, FLASH_APP_BASE, sizeof(network_info_t), (uint8_t*)&network_info);
}

int get_config_eeprom(void)
{
	return flash_stream_read(&flash, FLASH_APP_BASE, sizeof(network_info_t), (uint8_t*)&network_info);
}

uint32_t gen_crc(const network_info_t *buff)
{
	uint8_t len = (sizeof(network_info_t) - 4) / 4;
	uint32_t *ptr = (uint32_t *)buff;
	uint32_t crc = 0xffffffff;

	for(uint8_t i = 0; i < len; i++)
	{
		crc = crc ^ *(ptr + i);
		for(uint8_t j = 0; j < 8; j++)
		{
			if(crc & 1)
			{
				crc = (crc >> 1) ^ poly;
			}
			else
			{
				crc = crc >> 1;
			}
		}
	}
	return ~crc;
}

#ifdef LWM2M_WITH_LOGS
void lwm2m_printf(const char * format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	printf(buffer);
	va_end(args);
}
#endif
