/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/


#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <string.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <curl/curl.h>

#include "curlinterface.h"
#include "t2log_wrapper.h"

typedef enum _ADDRESS_TYPE
{
    ADDR_UNKNOWN,
    ADDR_IPV4,
    ADDR_IPV6
}ADDRESS_TYPE;

static ADDRESS_TYPE getAddressType(const char *cif) {
    struct ifaddrs *ifap, *ifa;
    ADDRESS_TYPE addressType = ADDR_UNKNOWN;

    getifaddrs(&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == NULL || strcmp(ifa->ifa_name, cif))
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
            addressType = ADDR_IPV4;
        else
            addressType = ADDR_IPV6;

        break;
    }

    freeifaddrs(ifap);
    return addressType;
}

static size_t writeToFile(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *) stream);
    return written;
}

static T2ERROR setHeader(CURL *curl, const char* destURL, struct curl_slist **headerList)
{
    
    T2Debug("%s ++in\n", __FUNCTION__);

    T2Debug("%s DEST URL %s \n", __FUNCTION__, destURL);
    CURLcode code=CURLE_OK;
    code = curl_easy_setopt(curl, CURLOPT_URL, destURL);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    code = curl_easy_setopt(curl, CURLOPT_SSLVERSION, TLSVERSION);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    code = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, HTTP_METHOD);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }

    if(getAddressType(INTERFACE) == ADDR_UNKNOWN)
    {
        T2Error("Unknown Address Type - returning failure\n");
        return T2ERROR_FAILURE;
    }
    else if(getAddressType(INTERFACE) == ADDR_IPV4)
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    else
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);

    curl_easy_setopt(curl, CURLOPT_INTERFACE, INTERFACE);

    *headerList = curl_slist_append(NULL, "Accept: application/json");
    curl_slist_append(*headerList, "Content-type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headerList);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static T2ERROR setPayload(CURL *curl, const char* payload)
{
    CURLcode code = CURLE_OK ;
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    if(code != CURLE_OK){
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(payload));
    if(code != CURLE_OK){
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    return T2ERROR_SUCCESS;
}

T2ERROR sendReportOverHTTP(char *httpUrl, char* payload)
{
    CURL *curl = NULL;
    FILE *fp = NULL;
    CURLcode res;
    T2ERROR ret = T2ERROR_FAILURE;
    long http_code;
    struct curl_slist *headerList = NULL;

    T2Debug("%s ++in\n", __FUNCTION__);
    curl = curl_easy_init();
    if (curl) {
        if(setHeader(curl, httpUrl, &headerList) != T2ERROR_SUCCESS)
        {
            T2Error("Failed to Set HTTP Header\n");
            curl_easy_cleanup(curl);
            return ret;
        }
        setPayload(curl, payload);

        fp = fopen(CURL_OUTPUT_FILE, "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (res != CURLE_OK)
            {
                fprintf(stderr, "curl failed: %s\n", curl_easy_strerror(res));
                T2Error("Failed to send report over HTTP, HTTP Response Code : %ld\n", http_code);
            }
            else
            {
                T2Info("Report Sent Successfully over HTTP : %ld\n", http_code);
                ret = T2ERROR_SUCCESS;
            }

            fclose(fp);
        }
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    }
    else
    {
        T2Error("Unable to initialize Curl\n");
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

T2ERROR sendCachedReportsOverHTTP(char *httpUrl, Vector *reportList)
{
    while(Vector_Size(reportList) > 0)
    {
        char* payload = (char *)Vector_At(reportList, 0);
        if(T2ERROR_FAILURE == sendReportOverHTTP(httpUrl, payload))
        {
            T2Error("Failed to send cached report, left with %d reports in cache \n", Vector_Size(reportList));
            return T2ERROR_FAILURE;
        }
        Vector_RemoveItem(reportList, payload, NULL);
        free(payload);
    }
    return T2ERROR_SUCCESS;
}
