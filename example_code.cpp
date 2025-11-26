/*
    Example code to use the HTTPRequest_BSD class.
    
    Important:
    I made this code while not knowing there was a esp-idf sollution for doing http(s) request.
    So when i found out i already wasted some time making this code and understanding BSD.
    I came to the conclusion its more logical to use the esp-idf sollution as its proberly more common, stable, and maintainable.
    So i stopped developing this. But i am proud of the code i made and still want to show the knowledge i gained.
    
    ! I didnt further test the code, i know you can get a valid response from an server. !
    
    Credit to this video: https://youtu.be/ZQatV_Fi0Vc.
    It has been a huge help with creating this and i highly recommend watching this video.
    He not only explains the code, but also all the parameters that you use in the BSD functions.
    And it made me understand everything i was doing, which also helped with debugging some things.
*/

#include <stdio.h>
#include <iostream>

#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "HTTPRequest_BSD/include/HTTPRequest.hpp"
#include "HTTPRequest_BSD/include/HTTPRequestConfig.hpp"

const char* log_tag = "app_main";

// HTTPREQUEST
extern "C" void app_main(void)
{    
    // Init nvs
    {
        esp_err_t result = nvs_flash_init();
        if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            result = nvs_flash_init();
        }
        ESP_ERROR_CHECK(result);
    }

    // Create default event loop (Should be called once every app)
    esp_event_loop_create_default();

    // Init netif (Should be called once every app)
    esp_netif_init();

    /* 
        Init and start wifi code goes here...
    */


    HTTPRequest_BSD req({
        .type = http_request_type::GET,
        .path = "/",
        .headers = {
            .host = "example.com",
            .other_headers = {
                {"Example-header", "example_value"}
            }
        }
    });

    /*
        Could also be used like this:
        HTTPRequest_BSD req("example.com", "/", http_request_type::GET);
    */
    
    esp_err_t result = req.send_and_wait_for_response();
    if(result != ESP_OK){
        ESP_LOGW(log_tag, "HTTP req failed...");
    }

    std::cout << "RESPONSE: \n" << req.get_response();
}
