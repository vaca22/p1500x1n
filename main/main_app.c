/* Console example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/event_groups.h"
#include "esp_wifi.h"

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "cmd_router.h"
#include "config.h"

#include "router_globals.h"
#include <esp_http_server.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#include "esp_event.h"
#include "freertos/semphr.h"
#include <sys/socket.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_gatt.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "main_app.h"
#include "myble.h"
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

#define MY_DNS_IP_ADDR 0x08080808

static int server_socket = 0;                        // 服务器socket
static struct sockaddr_in server_addr;                // server地址
static struct sockaddr_in client_addr;                // client地址
static unsigned int socklen = sizeof(client_addr);    // 地址长度
static int connect_socket = 0;                        // 连接socket
int TCP_PORT         =       8888;               // 监听客户端端口
bool g_rxtx_need_restart = false;                    // 异常后，重新连接标记



uint16_t connect_count = 0;
bool ap_connect = false;

esp_netif_t *wifiAP;
esp_netif_t *wifiSTA;

static const char *TAG = "ESP32 uart";


EventGroupHandle_t udp_event_group;
int UDP_PORT	=			8889;

struct sockaddr_in udp_client_addr;
int udp_connect_socket=0;
static int receive_udp=0;


void udp_close_socket()
{
    close(udp_connect_socket);
}

void udp_recv_data(void *pvParameters)
{
    int len = 0;            //长度
    char databuff[1024];    //缓存
    while (1){
        memset(databuff, 0x00, sizeof(databuff));//清空缓存
        //读取接收数据
        len = recvfrom(udp_connect_socket, databuff, sizeof(databuff), 0, (struct sockaddr *) &udp_client_addr, &socklen);
        if (len > 0){
            receive_udp=1;
            uart_write_bytes(UART_PORT_NUM, databuff, len);

        }else{
            break;
        }
    }
    udp_close_socket();
    vTaskDelete(NULL);
}

esp_err_t create_udp_client()
{
    udp_connect_socket = socket(AF_INET, SOCK_DGRAM, 0);                         /*参数和TCP不同*/
    if (udp_connect_socket < 0){

        close(udp_connect_socket);
        return ESP_FAIL;
    }



    struct sockaddr_in Loacl_addr;
    Loacl_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    Loacl_addr.sin_family = AF_INET;
    Loacl_addr.sin_port = htons(UDP_PORT);
    uint8_t res = 0;
    res = bind(udp_connect_socket, (struct sockaddr *)&Loacl_addr, sizeof(Loacl_addr));
    if(res != 0){
        printf("bind error\n");

    }



    return ESP_OK;
}


static void udp_connect(void *pvParameters)
{

    ESP_LOGI(TAG, "start udp connected");
    vTaskDelay(3000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "create udp Client");
    int socket_ret = create_udp_client();
    if (socket_ret == ESP_FAIL){
        ESP_LOGI(TAG, "create udp socket error,stop...");
        vTaskDelete(NULL);
    }else{
        ESP_LOGI(TAG, "create udp socket succeed...");
        //建立UDP接收数据任务
        if (pdPASS != xTaskCreate(&udp_recv_data, "recv_data", 4096, NULL, 4, NULL)){
            ESP_LOGI(TAG, "Recv task create fail!");
            vTaskDelete(NULL);
        }else{
            ESP_LOGI(TAG, "Recv task create succeed!");
        }
    }
    vTaskDelete(NULL);
}







// 建立tcp server
esp_err_t create_tcp_server(bool isCreatServer) {
    //首次建立server
    if (isCreatServer) {
        ESP_LOGI(TAG, "server socket....,port=%d", TCP_PORT);
        server_socket = socket(AF_INET, SOCK_STREAM, 0);//新建socket
        if (server_socket < 0) {
            close(server_socket);//新建失败后，关闭新建的socket，等待下次新建
            return ESP_FAIL;
        }
        //配置新建server socket参数
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        //bind:地址的绑定
        if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            close(server_socket);//bind失败后，关闭新建的socket，等待下次新建
            return ESP_FAIL;
        }
    }
    //listen，下次时，直接监听
    if (listen(server_socket, 5) < 0) {
        close(server_socket);//listen失败后，关闭新建的socket，等待下次新建
        return ESP_FAIL;
    }
    //accept，搜寻全连接队列
    connect_socket = accept(server_socket, (struct sockaddr *) &client_addr, &socklen);
    if (connect_socket < 0) {
        close(server_socket);//accept失败后，关闭新建的socket，等待下次新建
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}



void tcp_close_socket()
{
    close(connect_socket);
    close(server_socket);
}

// 接收数据任务
void tcp_recv_data(void *pvParameters)
{
    int len = 0;
    char databuff[1024];
    while (1){
        memset(databuff, 0x00, sizeof(databuff));//清空缓存
        len = recv(connect_socket, databuff, sizeof(databuff), 0);//读取接收数据
        g_rxtx_need_restart = false;
        if (len > 0){
            ESP_LOGI(TAG, "recvData: %s", databuff);//打印接收到的数组
            uart_write_bytes(UART_PORT_NUM, databuff, len);
        }else{
            //  show_socket_error_reason("recv_data", connect_socket);//打印错误信息
            g_rxtx_need_restart = true;//服务器故障，标记重连
            vTaskDelete(NULL);
        }
    }
    tcp_close_socket();
    g_rxtx_need_restart = true;//标记重连
    vTaskDelete(NULL);
}
// 建立TCP连接并从TCP接收数据
static void tcp_connect(void *pvParameters) {
    while (1) {
        g_rxtx_need_restart = false;
        ESP_LOGI(TAG, "start tcp connected");
        TaskHandle_t tx_rx_task = NULL;
        vTaskDelay(3000 / portTICK_RATE_MS);// 延时3S准备建立server
        ESP_LOGI(TAG, "create tcp server");
        int socket_ret = create_tcp_server(true);// 建立server
        if (socket_ret == ESP_FAIL) {// 建立失败
            ESP_LOGI(TAG, "create tcp socket error,stop...");
            continue;
        } else {// 建立成功
            ESP_LOGI(TAG, "create tcp socket succeed...");
            // 建立tcp接收数据任务
            if (pdPASS != xTaskCreate(&tcp_recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
                ESP_LOGI(TAG, "Recv task create fail!");
            } else {
                ESP_LOGI(TAG, "Recv task create succeed!");
            }
        }
        while (1) {
            vTaskDelay(3000 / portTICK_RATE_MS);
            if (g_rxtx_need_restart) {// 重新建立server，流程和上面一样
                ESP_LOGI(TAG, "tcp server error,some client leave,restart...");
                // 建立server
                if (ESP_FAIL != create_tcp_server(false)) {
                    if (pdPASS != xTaskCreate(&tcp_recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) {
                        ESP_LOGE(TAG, "tcp server Recv task create fail!");
                    } else {
                        ESP_LOGI(TAG, "tcp server Recv task create succeed!");
                        g_rxtx_need_restart = false;//重新建立完成，清除标记
                    }
                }
            }
        }
    }
    vTaskDelete(NULL);
}

static const int RX_BUF_SIZE = 1024;

int bau_num=115200;
int bau_index=4;


void init_uart(void) {
    const uart_config_t uart_config = {
            .baud_rate = bau_num,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}








extern struct os_mbuf *om;
extern uint16_t hrs_hrm_handle;
extern uint16_t conn_handle;
extern int ble_connect_flag;
static void uart_rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            if(ble_connect_flag){
                om = ble_hs_mbuf_from_flat(data, rxBytes);
                ble_gattc_notify_custom(conn_handle, hrs_hrm_handle, om);
            }

            if(receive_udp){
                sendto(udp_connect_socket, data, rxBytes, 0, (struct sockaddr *) &udp_client_addr, sizeof(udp_client_addr));
            }
            send(connect_socket, data, rxBytes, 0);//接收数据回发
            // ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}










static void initialize_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static int got_ip=0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    esp_netif_dns_info_t dns;

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ap_connect = true;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
            if(got_ip==0){


                got_ip=1;
                xTaskCreate(&udp_connect, "udp_connect", 4096, NULL, 5, NULL);
                xTaskCreate(&tcp_connect, "tcp_connect", 4096, NULL, 5, NULL);


                char cc[20];
                sprintf(cc,IPSTR,IP2STR(&event->event_info.got_ip.ip_info.gw));
                set_str("gw",cc);
                sprintf(cc,IPSTR,IP2STR(&event->event_info.got_ip.ip_info.netmask));
                set_str("netmask",cc);
            }

            if (esp_netif_get_dns_info(wifiSTA, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK) {
                ip_addr_t dnsserver;
                dnsserver.u_addr.ip4.addr = htonl(MY_DNS_IP_ADDR);
                dnsserver.type = IPADDR_TYPE_V4;
                dhcps_dns_setserver(&dnsserver);
                ESP_LOGI(TAG, "set dns to:" IPSTR, IP2STR(&dns.ip.u_addr.ip4));
            }
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "disconnected - retry to connect to the AP");
            ap_connect = false;
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            connect_count++;
            ESP_LOGI(TAG, "%d. station connected", connect_count);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            connect_count--;
            ESP_LOGI(TAG, "station disconnected - %d remain", connect_count);
            break;
        default:
            break;
    }
    return ESP_OK;
}

const int CONNECTED_BIT = BIT0;
#define JOIN_TIMEOUT_MS (2000)

void wifi_init(const char *ssid, const char *passwd, const char *static_ip, const char *subnet_mask,
               const char *gateway_addr, const char *ap_ssid, const char *ap_passwd) {
    ip_addr_t dnsserver;


    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifiAP = esp_netif_create_default_wifi_ap();
    wifiSTA = esp_netif_create_default_wifi_sta();

    tcpip_adapter_ip_info_t ipInfo_sta;
    if ((strlen(ssid) > 0) && (strlen(static_ip) > 0) && (strlen(subnet_mask) > 0) && (strlen(gateway_addr) > 0)) {
        ESP_LOGE(TAG, "try static ip");
        ipInfo_sta.ip.addr = ipaddr_addr(static_ip);
        ipInfo_sta.gw.addr = ipaddr_addr(gateway_addr);
        ipInfo_sta.netmask.addr = ipaddr_addr(subnet_mask);
        tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client
        tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo_sta);
    }

    esp_netif_ip_info_t ipInfo_ap;
    IP4_ADDR(&ipInfo_ap.ip, 192, 168, 4, 1);
    IP4_ADDR(&ipInfo_ap.gw, 192, 168, 4, 1);
    IP4_ADDR(&ipInfo_ap.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(wifiAP); // stop before setting ip WifiAP
    esp_netif_set_ip_info(wifiAP, &ipInfo_ap);
    esp_netif_dhcps_start(wifiAP);

    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* ESP WIFI CONFIG */
    wifi_config_t wifi_config = {0};
    wifi_config_t ap_config = {
            .ap = {
                    .channel = 0,
                    .authmode = WIFI_AUTH_WPA2_PSK,
                    .ssid_hidden = 0,
                    .max_connection = 8,
                    .beacon_interval = 100,
            }
    };

    strlcpy((char *) ap_config.sta.ssid, ap_ssid, sizeof(ap_config.sta.ssid));
    if (strlen(ap_passwd) < 8) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strlcpy((char *) ap_config.sta.password, ap_passwd, sizeof(ap_config.sta.password));
    }

    if ((strlen(ssid) > 0)&&(icr==1)) {
        strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strlcpy((char *) wifi_config.sta.password, passwd, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    }

    // Enable DNS (offer) for dhcp server
    dhcps_offer_t dhcps_dns_value = OFFER_DNS;
    dhcps_set_option_info(6, &dhcps_dns_value, sizeof(dhcps_dns_value));

    // Set custom dns server address for dhcp server
    dnsserver.u_addr.ip4.addr = htonl(MY_DNS_IP_ADDR);
    dnsserver.type = IPADDR_TYPE_V4;
    dhcps_dns_setserver(&dnsserver);


    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        pdFALSE, pdTRUE, JOIN_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(esp_wifi_start());

    if (strlen(ssid) > 0) {
        ESP_LOGI(TAG, "wifi_init_apsta finished.");
        ESP_LOGI(TAG, "connect to ap SSID: %s ", ssid);
    } else {
        ESP_LOGI(TAG, "wifi_init_ap with default finished.");
    }
}

char *ssid = NULL;
char *passwd = NULL;
char *static_ip = NULL;
char *ble_name = NULL;
char *tcpport = NULL;
char *udpport = NULL;
char *ap_ssid = NULL;
char *ap_passwd = NULL;
char *gw=NULL;
char *netmask=NULL;
char *bauchar=NULL;
char *is_connect_router=NULL;
int icr=0;
int bau_list[]={9600,19200,38400,115200,2000000};

char *param_set_default(const char *def_val) {
    char *retval = malloc(strlen(def_val) + 1);
    strcpy(retval, def_val);
    return retval;
}





void ble_uart(uart_port_t uart_num, const void *src, size_t size){
    uart_write_bytes(uart_num, src, size);
}




void app_main(void) {
    initialize_nvs();


    get_config_param_str("router_name", &ssid);
    if (ssid == NULL) {
        ssid = param_set_default("");
    }
    get_config_param_str("router_password", &passwd);
    if (passwd == NULL) {
        passwd = param_set_default("");
    }
    get_config_param_str("inner_ip", &static_ip);
    if (static_ip == NULL) {
        static_ip = param_set_default("");
    }
    get_config_param_str("ble_name", &ble_name);
    if (ble_name == NULL) {
        ble_name = param_set_default("ble_uart");
    }
    get_config_param_str("tcp_port", &tcpport);
    if (tcpport == NULL) {
        tcpport = param_set_default("8888");
    }else{
        TCP_PORT=atoi(tcpport);
    }
    get_config_param_str("udp_port", &udpport);
    if (udpport == NULL) {
        udpport = param_set_default("8889");
    }else{
        UDP_PORT=atoi(udpport);
    }
    get_config_param_str("ap_ssid", &ap_ssid);
    if (ap_ssid == NULL) {
        ap_ssid = param_set_default("ESP32_UART");
    }
    get_config_param_str("ap_passwd", &ap_passwd);
    if (ap_passwd == NULL) {
        ap_passwd = param_set_default("");
    }
    get_config_param_str("gw", &gw);
    if (gw == NULL) {
        gw = param_set_default("");
    }
    get_config_param_str("netmask", &netmask);
    if (netmask == NULL) {
        netmask = param_set_default("");
    }
    get_config_param_str("bau", &bauchar);
    if (bauchar == NULL) {
        bauchar = param_set_default("4");
    }else{
        if(atoi(bauchar)>=1){
            bau_index= atoi(bauchar);
            bau_num=bau_list[atoi(bauchar)-1];
        }

    }

    get_config_param_str("is_connect_router", &is_connect_router);
    if (is_connect_router == NULL) {
        icr=1;
        ESP_LOGE(TAG, "fuck3");
        bauchar = param_set_default("0");
    }else{
        if(is_connect_router[0]=='o'){
            ESP_LOGE(TAG, "fuck");
            icr=1;
        }else{
            ESP_LOGE(TAG, "fuck2");
            icr=0;
        }
    }
    // Setup WIFI
    wifi_init(ssid, passwd, static_ip, netmask, gw, ap_ssid, ap_passwd);


    char *lock = NULL;
    get_config_param_str("lock", &lock);
    if (lock == NULL) {
        lock = param_set_default("0");
    }
    if (strcmp(lock, "0") == 0) {
        ESP_LOGI(TAG, "Starting config web server");
        start_webserver();
    }
    free(lock);


    init_uart();
    xTaskCreate(uart_rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);

    send_uart_callback *ble_uart_callback;
    ble_uart_callback=(send_uart_callback *) malloc(sizeof(send_uart_callback));
    ble_uart_callback->func_name=ble_uart;
    register_uart(ble_uart_callback);
    init_ble();







}
