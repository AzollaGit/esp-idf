#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <esp_log.h>
#include <string.h>
#include <errno.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "app_mqtt.h"


#define PORT_NUMBER 6000


#define LED0_IO    2
#define LED1_IO    4
#define LED_PIN_SEL  ((1ULL<<LED0_IO) | (1ULL<<LED1_IO))

static const char *TAG = "socket_server";

/**
 * Create a listening socket.  We then wait for a client to connect.
 * Once a client has connected, we then read until there is no more data
 * and log the data read.  We then close the client socket and start
 * waiting for a new connection.
 */
void socket_server_task(void *ignore)
{
	struct sockaddr_in clientSocketAddr;
	struct sockaddr_in serverSocketAddr;

	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(TAG, "socket: %d %s", sock, strerror(errno));
		goto END;
	} 
    //#define HOST_IP          ((u32_t)0xC0A82802UL)  /** 192.168.43.2 */
	// Bind our server socket to a port.
	serverSocketAddr.sin_family = AF_INET;
	serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 使用WIFI IPv4 address: 192.168.43.21 
	serverSocketAddr.sin_port = htons(PORT_NUMBER);
	int rc  = bind(sock, (struct sockaddr *)&serverSocketAddr, sizeof(serverSocketAddr));
	if (rc < 0) {
		ESP_LOGE(TAG, "bind: %d %s", rc, strerror(errno));
		goto END;
	}

	// Flag the socket as listening for new connections.
	rc = listen(sock, 5);
	if (rc < 0) {
		ESP_LOGE(TAG, "listen: %d %s", rc, strerror(errno));
		goto END;
	}

    ESP_LOGI(TAG, "socket server init ok!");

	while (1) {
		// Listen for a new client connection.
		socklen_t clientSocketAddrLength = sizeof(clientSocketAddr);
		int clientSock = accept(sock, (struct sockaddr *)&clientSocketAddr, &clientSocketAddrLength);
		if (clientSock < 0) {
			ESP_LOGE(TAG, "accept: %d %s", clientSock, strerror(errno));
			goto END;
		} 

        ESP_LOGI(TAG, "clientSock connect ip: %d  port: %d", clientSocketAddr.sin_addr.s_addr, clientSocketAddr.sin_port);

		// We now have a new client ...
		int  recMaxSize = 64;
		uint8_t *recData = malloc(recMaxSize);
        memset(recData, 0, recMaxSize);

		// Loop reading data.
		while(1) {
			ssize_t recSize = recv(clientSock, recData, recMaxSize, 0);
			if (recSize < 0) {
				ESP_LOGE(TAG, "recv: %d %s", recSize, strerror(errno));
				goto END;
			} else if (recSize == 0) {
				break;
			} else {
                // Finished reading revData.
		        ESP_LOGI(TAG, "recData (size: %d) was: %s", recSize, recData);
                if (memcmp(recData, "OFF0", recSize) == 0) {
                    gpio_set_level(LED0_IO, 0);
                } else if (memcmp(recData, "ON0", recSize) == 0) {
                    gpio_set_level(LED0_IO, 1);
                } else if (memcmp(recData, "OFF1", recSize) == 0) {
                    gpio_set_level(LED1_IO, 0);
                } else if (memcmp(recData, "ON1", recSize) == 0) {
                    gpio_set_level(LED1_IO, 1);
                } 
				send(clientSock, recData, recSize, 0);
                memset(recData, 0, recSize);
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
		}
        ESP_LOGI(TAG, "clientSock disconnect...");
		free(recData);
		close(clientSock);
	}
	END:
	vTaskDelete(NULL);
	esp_restart();		// 如果TCP异常了，直接复位。
}

void app_led_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = LED_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(LED0_IO, 1);
    gpio_set_level(LED1_IO, 0);

}

void app_socket_server_init(void)
{
    app_led_init();
    xTaskCreate(&socket_server_task, "socket_server_task", 1024*2, NULL, 6, NULL);
}

