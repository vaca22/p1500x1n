/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <sys/param.h>

#include "esp_netif.h"

#include <esp_http_server.h>
#include <esp_ota_ops.h>

#include "main_app.h"
#include "router_globals.h"

static const char *TAG = "HTTPServer";

extern const unsigned char upload_script_start[] asm("_binary_index_html_start");
extern const unsigned char upload_script_end[]   asm("_binary_index_html_end");

esp_timer_handle_t restart_timer;

static void restart_timer_callback(void* arg)
{
    ESP_LOGI(TAG, "Restarting now...");
    esp_restart();
}

esp_timer_create_args_t restart_timer_args = {
        .callback = &restart_timer_callback,
        /* argument specified here will be passed to timer callback function */
        .arg = (void*) 0,
        .name = "restart_timer"
};
/* An HTTP GET handler */
static esp_err_t index_get_handler(httpd_req_t *req) {
    char *buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param1[64];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ap_ssid", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => ap_ssid=%s  %d", param1, strlen(param1));
                set_str("ap_ssid",param1);
            }
            if (httpd_query_key_value(buf, "ap_password", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => ap_password=%s  %d", param1, strlen(param1));
                set_str("ap_password",param1);
            }
            if (httpd_query_key_value(buf, "ble_name", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => ble_name=%s  %d", param1, strlen(param1));
                set_str("ble_name",param1);
            }
            if (httpd_query_key_value(buf, "is_connect_router", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => is_connect_router=%s  %d", param1, strlen(param1));
                set_str("icr","1");
            }else{
                set_str("icr","0");
            }
            if (httpd_query_key_value(buf, "router_name", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => router_name=%s  %d", param1, strlen(param1));
                set_str("router_name",param1);
            }
            if (httpd_query_key_value(buf, "router_password", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => router_password=%s  %d", param1, strlen(param1));
                set_str("router_password",param1);
            }
            if (httpd_query_key_value(buf, "inner_ip", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => inner_ip=%s  %d", param1, strlen(param1));
                set_str("inner_ip",param1);
            }
            if (httpd_query_key_value(buf, "tcp_port", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => tcp_port=%s  %d", param1, strlen(param1));
                set_str("tcp_port",param1);
            }
            if (httpd_query_key_value(buf, "udp_port", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => udp_port=%s  %d", param1, strlen(param1));
                set_str("udp_port", param1);
            }
            if (httpd_query_key_value(buf, "bau", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                if(atoi(param1)!=0){
                    ESP_LOGI(TAG, "Found URL query parameter => bau=%s  %d", param1, strlen(param1));
                    set_str("bau",param1);
                }

            }

            esp_timer_start_once(restart_timer, 500000);

        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char *resp_str = (const char *) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}








static esp_err_t status_handler(httpd_req_t *req){
    static char json_response[1024];
    char * p = json_response;
    *p++ = '{';
    p+=sprintf(p, "\"wifi_name\":\"%s\",", ap_ssid);
    p+=sprintf(p, "\"wifi_password\":\"%s\",", ap_passwd);
    p+=sprintf(p, "\"ble_name\":\"%s\",", ble_name);
    p+=sprintf(p, "\"router_name\":\"%s\",", ssid);
    p+=sprintf(p, "\"router_password\":\"%s\",",passwd );
    p+=sprintf(p, "\"is_connect_router\":\"%d\",", icr);
    p+=sprintf(p, "\"inner_ip\":\"%s\",", static_ip);
    p+=sprintf(p, "\"tcp_port\":%d,", TCP_PORT);
    p+=sprintf(p, "\"udp_port\":%d,", UDP_PORT);
    p+=sprintf(p, "\"bau\":%d", bau_index);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}


static esp_err_t upload_post_handler(httpd_req_t *req)
{

    int update_mtu=1500;
    char buf[update_mtu] ;
    int received;

    int remaining = req->content_len;

    int tt=0;
    while (remaining > 0) {
        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        if ((received = httpd_req_recv(req, buf, MIN(remaining, update_mtu))) <= 0) {
            continue;
        }
        tt+=received;
//        esp_ota_write( update_handle, (const void *)buf, received);
        remaining -= received;
    }
    ESP_LOGE(TAG, "Total size : %d",tt);

    ESP_LOGI(TAG, "File reception complete");
//    esp_ota_end(update_handle);
//    esp_ota_set_boot_partition(update_partition);
    httpd_resp_sendstr(req, "File uploaded successfully");
//    esp_timer_start_once(restart_timer, 500000);
    return ESP_OK;
}


static httpd_uri_t indexp = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_get_handler,
};
httpd_uri_t status_uri = {
        .uri       = "/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
};
httpd_uri_t file_upload = {
        .uri       = "/update",
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
};

httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    char checked[] = "checked";
    int tcp_port= TCP_PORT;
    int udp_port = UDP_PORT;
    char bau[10];

    sprintf(bau,"%d",bau_num);

    const char *config_page_template = (char*)upload_script_start;
    int total=upload_script_end-upload_script_start;
    char *config_page = malloc(total+ 1);
    memcpy(config_page,upload_script_start,total);
    config_page[total]=0;
    indexp.user_ctx = config_page;
    esp_timer_create(&restart_timer_args, &restart_timer);

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &indexp);
        httpd_register_uri_handler(server,&status_uri);
        httpd_register_uri_handler(server,&file_upload);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

