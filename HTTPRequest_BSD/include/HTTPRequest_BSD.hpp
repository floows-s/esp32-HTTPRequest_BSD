#pragma once
#include <cstring>

#include "HTTPRequestConfig.hpp"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "lwip/netdb.h"

/// @brief Note: This class isnt fully developt. It can make HTTP get requests.
///        Usage: First call send_and_wait_for_response(). This will block the code until it recieved a response or error.
///               Then check if the the method returned ESP_OK, if so then you can get the response with get_response().
class HTTPRequest_BSD{
public:
    HTTPRequest_BSD(std::string host, std::string url, http_request_type type = http_request_type::GET);
    HTTPRequest_BSD(HTTPRequestConfig_BSD request_config); 
    ~HTTPRequest_BSD();

    /* METHODS*/
    /// @brief Send request and block until getting a response or error/timeout.
    esp_err_t send_and_wait_for_response();
    
    /* GETTERS AND SETTERS*/
    std::string get_response() { return m_response; }
    const std::string get_log_tag() { return m_log_tag; }
    HTTPRequestConfig_BSD get_config() { return m_config; }
    
    /* MEMBERS */
    static const uint32_t MAX_RX_BUFFER_SIZE = 64; 
    static const uint32_t SOCKET_TIMEOUT_SEC = 5; 
private:

    /* METHODS */
    esp_err_t dns_lookup(addrinfo*& result);
    esp_err_t create_socket(int& sock, addrinfo* dns_lookup_res);
    esp_err_t connect_to_server(int sock, addrinfo* dns_lookup_res);

    /// @brief Logs all the IPv4 and IPv6 adress found of the host.
    ///        Note: the log level is verbose.
    /// @param value 
    void log_addrinfo_ip_adresses(addrinfo* value);

    /// @brief Logs the config. This is the request send to the server.
    ///        Note: the log level is verbose.
    void log_config();

    /* MEMBERS */
    HTTPRequestConfig_BSD m_config;
    std::string m_response;

    const char* m_log_tag = "HTTPRequest"; // Note: i declare this as a char* so i dont have to .c_str() everytime i want to use it
    const timeval m_socket_timeout = { // Send and recieve timeout 
        .tv_sec = SOCKET_TIMEOUT_SEC,
        .tv_usec = 0
    };
    const addrinfo m_dns_lookup_hints = {
        .ai_family = AF_INET, // IPv4
        .ai_socktype = SOCK_STREAM // TCP stream socekt
    };
};
