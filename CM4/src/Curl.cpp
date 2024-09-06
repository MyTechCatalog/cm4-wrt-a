/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "Curl.hpp"
namespace picod {
Curl::Curl(std::string url)
: httpHeaders_(nullptr)
{
    curl_ = curl_easy_init();
    if (curl_)
    {
        CURLcode res = curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        // Check for errors
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt() failed: %s\n",
                curl_easy_strerror(res));
        }

        if((res = curl_easy_setopt(curl_, 
            CURLOPT_WRITEFUNCTION, &Curl::WriteCallback)) != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt() failed: %s\n",
                curl_easy_strerror(res));
        }

        if((res = curl_easy_setopt(curl_, 
            CURLOPT_WRITEDATA, this)) != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt() failed: %s\n",
                curl_easy_strerror(res));
        }
    }
}

Curl::~Curl()
{
    if (httpHeaders_){
        curl_slist_free_all(httpHeaders_);
    }
    
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

CURLcode Curl::postRequest(std::string data, std::string & response,
    std::string & errorMsg)
{
    CURLcode res = curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, httpHeaders_);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt() failed: %s\n",
            curl_easy_strerror(res));
    }


    /* size of the POST data */
    res = curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.length());
    
    res = curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.c_str());

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt() failed: %s\n",
            curl_easy_strerror(res));
    }

    res = curl_easy_perform(curl_);

    if (res != CURLE_OK) {
        errorMsg = curl_easy_strerror(res);
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
            errorMsg.c_str());
    } else {
        response = readBuffer_;
        readBuffer_.clear();
    }

    return res;  
}

size_t Curl::WriteCallback(void *contents, size_t size, size_t numItems, void *userData)
{
    if (userData){
        Curl* pCurl = static_cast<Curl*>(userData);
        Curl & curlHelper = *pCurl;
        curlHelper.onReadData((char*)contents, size, numItems);
    }
    return size * numItems;
}

void Curl::setHeader(std::string header)
{
    httpHeaders_ = curl_slist_append(httpHeaders_, header.c_str());    
}

void  Curl::onReadData(char* contents, size_t size, size_t numItems)
{
    readBuffer_.append((char*)contents, size * numItems);
}

void Curl::setopt(CURLoption option, long parameter)
{
    CURLcode res =  curl_easy_setopt(curl_,option, parameter);
    // Check for errors
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt() failed: %s\n",
            curl_easy_strerror(res));
    } 
}

void Curl::setopt(CURLoption option, std::string parameter)
{
    CURLcode res =  curl_easy_setopt(curl_,option, parameter.c_str());
    // Check for errors
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt() failed: %s\n",
            curl_easy_strerror(res));
    }
}

void Curl::setopt(CURLoption option, double parameter)
{
    CURLcode res =  curl_easy_setopt(curl_,option, parameter);
    // Check for errors
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt() failed: %s\n",
            curl_easy_strerror(res));
    }    
}

}//@END namespace picod

