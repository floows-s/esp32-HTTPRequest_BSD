#include "include/HTTPRequest_BSD.hpp"

HTTPRequest_BSD::HTTPRequest_BSD(std::string host, std::string path, http_request_type type) {
    this->m_config = {
        .type = type,
        .path = path,
        .headers = {
            .host = host
        } 
    };
}
HTTPRequest_BSD::HTTPRequest_BSD(HTTPRequestConfig_BSD request_config) : m_config(request_config) {

}

HTTPRequest_BSD::~HTTPRequest_BSD() {

}

esp_err_t HTTPRequest_BSD::send_and_wait_for_response(){
    m_response.clear();

    /* DNS LOOPUP*/
    struct addrinfo *dns_lookup_res = NULL;
    {
        esp_err_t result = dns_lookup(dns_lookup_res);
        if(result != ESP_OK) return result; 

        ESP_LOGV(m_log_tag, "DNS lookup succeeded!");
        log_addrinfo_ip_adresses(dns_lookup_res);
    }

    /* CREATE SOCKET */
    int sock = 0;
    {
        esp_err_t result = create_socket(sock, dns_lookup_res);
        if(result != ESP_OK) return result;

        ESP_LOGV(m_log_tag, "Socket created!");
    }

    /* CONNECT TO SERVER*/
    {
        esp_err_t result = connect_to_server(sock, dns_lookup_res);
        if(result != ESP_OK) return result;

        ESP_LOGV(m_log_tag, "Connected to server!");
    }

    // Dont need address info anymore
    freeaddrinfo(dns_lookup_res);

    /* SEND REQUEST */
    ESP_LOGI(m_log_tag, "Sending HTTP request to %s...", m_config.headers.host.c_str());
    {
        int result = send(
            sock, 
            m_config.to_string().c_str(), 
            strlen(m_config.to_string().c_str()),
            0 // Flags
        );

        if(result < 0){
            ESP_LOGE(m_log_tag, "Error (%d): Failed to send HTTP request: %s", errno, strerror(errno));
            log_config();
            close(sock);

            return ESP_FAIL;
        }
        
        ESP_LOGV(m_log_tag, "HTTP request send!");
    }

    /* RECIEVE RESPONSE */
    ESP_LOGV(m_log_tag, "Waiting for response...");
    
    char recv_buffer[MAX_RX_BUFFER_SIZE];
    uint32_t recv_total = 0;
    ssize_t recv_len = 0;

    // Recieve data until you have all the data or there has been a error
    while(true){ 
        recv_len = recv(sock, recv_buffer, MAX_RX_BUFFER_SIZE, 0);

        // Check for errors
        if(recv_len < 0){
            ESP_LOGE(m_log_tag, "Error (%d): Failed to recieve data: %s", errno, strerror(errno));
            log_config();
            
            m_response.clear();
            close(sock);
            return ESP_FAIL;
        }

        // Check if connection closed gracefully
        if(recv_len == 0){
            ESP_LOGV(m_log_tag, "Recieved all data!");
            break;
        }

        // Add to response
        m_response.append(recv_buffer);
        recv_total += recv_len;
    }

    ESP_LOGV(m_log_tag, "Total bytes recieved: %lu", recv_total);
    ESP_LOGI(m_log_tag, "Response recieved from %s!", m_config.headers.host.c_str());
    close(sock);

    return ESP_OK;
}


esp_err_t HTTPRequest_BSD::dns_lookup(addrinfo*& dns_lookup_res){
    int ret = getaddrinfo(
        m_config.headers.host.c_str(), // Host
        std::to_string(m_config.headers.port).c_str(), // Service name (port) 
        &m_dns_lookup_hints, // Lookup hints
        &dns_lookup_res // Where the result will be stored
    );

    if(ret !=0 || dns_lookup_res == NULL){
        ESP_LOGE(m_log_tag, "Error (%d): DNS lookup failed", ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void HTTPRequest_BSD::log_addrinfo_ip_adresses(addrinfo* value){
    /*  Note:
        The addresinfo is a linked list which include all found IP addresses.
        I have noticed that with the ESP it only has one adress.
        But in theory possible that it could have more adresses.
    */

    ESP_LOGV(m_log_tag, "IP adresses found: ");
    for(addrinfo* addr = value; addr != NULL; addr = addr->ai_next) {
        // IPv4 adress
        if(addr->ai_family == AF_INET){
            // Cast to right sock addr struct
            sockaddr_in* sa = (sockaddr_in* )addr->ai_addr;

            // Grab IP
            in_addr* ip = &sa->sin_addr;

            // Convert IP to string and log
            char ip_str[INET_ADDRSTRLEN] = "";
            inet_ntop(AF_INET, ip, ip_str, INET_ADDRSTRLEN);
            ESP_LOGV(m_log_tag, "IPv4: %s", ip_str);
        }

        // IPv6 adress
        if(addr->ai_family == AF_INET){
            // Cast to right sock addr struct
            sockaddr_in6* sa = (sockaddr_in6* )addr->ai_addr;

            // Grab IP
            in6_addr* ip = &sa->sin6_addr;

            // Convert IP to string and log
            char ip_str[INET6_ADDRSTRLEN] = "";
            inet_ntop(AF_INET6, ip, ip_str, INET6_ADDRSTRLEN);
            ESP_LOGV(m_log_tag, "IPv6: %s", ip_str);
        }
    }
}

esp_err_t HTTPRequest_BSD::create_socket(int& sock, addrinfo* dns_lookup_res){
    // Create socket
    sock = socket(dns_lookup_res->ai_family, dns_lookup_res->ai_socktype, dns_lookup_res->ai_protocol);
    if(sock < 0){
        ESP_LOGE(m_log_tag, "Error (%d): Failed to create socket: %s", errno, strerror(errno)); 
        return ESP_FAIL;
    }

    // Set send timeout
    {
        int ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &m_socket_timeout, sizeof(m_socket_timeout));
        if(ret < 0){
            ESP_LOGE(m_log_tag, "Error (%d): Failed to set socket send timeout: %s", errno, strerror(errno));
            close(sock);
            return ESP_FAIL;
        }
    }

    // Set recieve timeout
    {
        int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &m_socket_timeout, sizeof(m_socket_timeout));
        if(ret < 0){
            ESP_LOGE(m_log_tag, "Error (%d): Failed to set socket recieve timeout: %s", errno, strerror(errno));
            close(sock);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t HTTPRequest_BSD::connect_to_server(int sock, addrinfo* dns_lookup_res){
    // Connect with first IP adress found in the dns lookup
    int ret = connect(sock, dns_lookup_res->ai_addr, dns_lookup_res->ai_addrlen);
    if(ret < 0){
        ESP_LOGE(m_log_tag, "Error (%d): Failed to connect to server: %s", errno, strerror(errno));
        close(sock);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void HTTPRequest_BSD::log_config(){
    ESP_LOGV(m_log_tag, "Request-config dump: \n%s", m_config.to_string().c_str());
}