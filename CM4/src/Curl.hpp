/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef CURL_HPP
#define CURL_HPP

#include <curl/curl.h>
#include <vector>
#include <string>

namespace picod
{
class Curl
{

public:
    Curl(std::string url);  
    ~Curl();
    static size_t WriteCallback(void *contents, size_t size, size_t numItems, void *userData);
    void setopt(CURLoption option, long parameter);
    void setopt(CURLoption option, std::string parameter);
    void setopt(CURLoption option, double parameter);
    void setHeader(std::string header);
    void onReadData(char* contents, size_t size, size_t numItems);    
    CURLcode postRequest(std::string data, std::string & response,
        std::string & errorMsg);
private:
   
    CURL *curl_;
    struct curl_slist * httpHeaders_;
    std::string readBuffer_;
};
} // @END namespace picod



#endif // CURL_HPP