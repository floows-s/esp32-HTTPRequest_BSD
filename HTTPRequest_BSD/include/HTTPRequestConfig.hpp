#pragma once
#include <stdint.h>
#include <utility>
#include <vector>
#include <string>

/* REQUEST TYPE*/
/// @brief The type of HTTP request.
enum http_request_type{
    GET
    // POST <- Not implemented yet
};

std::string http_request_type_to_string(http_request_type value){
    switch(value){
        case GET: return "GET";
        default: return "<INVALID>";
    }
}

/* HEADERS */
/// @brief A struct holding the headers of an HTTP request.
///        The most important/required headers are members of the struct.
///        The rest of the headers can be declared in the member: other_headers.
struct HTTPRequestHeaders {
    std::string host = ""; // Required
    uint16_t port = 80;
    std::string user_agent = "esp-idf/1.0 esp32"; 
    std::string accept = "*/*"; 
    std::string connection = "close";
    std::vector<std::pair<std::string, std::string>> other_headers = {}; // Example: { {"Accept-Encoding", "deflate, gzip"}, {"Accept-Language", "en-US,en;q=0.5"} }
};

/* REQUEST */
/// @brief A struct holding the information needed to make a HTTP request.
struct HTTPRequestConfig_BSD {
    http_request_type type = http_request_type::GET;
    std::string path = "/";
    std::string http_version = "HTTP/1.1";
    HTTPRequestHeaders headers;
    std::string body = "";

    std::string to_string() const {
        std::string req_type = http_request_type_to_string(this->type);

        // Add request
        std::string result = req_type + " " + path + " " + http_version + "\r\n";

        // Add headers which are members
        result += "Host: " + headers.host + ":" + std::to_string(headers.port) + "\r\n" +
                  "User-Agent: " + headers.user_agent + "\r\n" +
                  "Accept: " + headers.accept + "\r\n"
                  "Connection: " + headers.connection + "\r\n";

        // Add the other headers
        for(std::pair<std::string, std::string> header : headers.other_headers){
            result += header.first + ": " + header.second + "\r\n";
        }

        // Add body if not empty
        if(body.length() > 0){
            result += body;
        }

        // Add a empty line, this is required
        result += "\r\n";

        return result;
    } 
};





