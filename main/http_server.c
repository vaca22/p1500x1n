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
//#include "nvs_flash.h"
#include "esp_netif.h"
//#include "esp_eth.h"
//#include "protocol_examples_common.h"

#include <esp_http_server.h>

#include "pages.h"
#include "main_app.h"
#include "router_globals.h"

static const char *TAG = "HTTPServer";



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
                set_str("is_connect_router",param1);
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
                set_str("udp_port",param1);
            }
            if (httpd_query_key_value(buf, "bau", param1, sizeof(param1)) == ESP_OK) {
                preprocess_string(param1);
                ESP_LOGI(TAG, "Found URL query parameter => bau=%s  %d", param1, strlen(param1));
                set_str("bau",param1);
            }


        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char *resp_str = (const char *) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

static httpd_uri_t indexp = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_get_handler,
};



httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    char ble_name[] = "dada";
    char checked[] = "checked";
    int tcp_port= 83;
    int udp_port = 82;
    char bau[] = "115200";

    const char *config_page_template = CONFIG_PAGE;
    char *config_page = malloc(strlen(config_page_template) + 512);
    sprintf(config_page, config_page_template, ap_ssid, ap_passwd, ble_name, checked, ssid, passwd,
            static_ip, tcp_port, udp_port, bau);
    indexp.user_ctx = config_page;


    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &indexp);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

