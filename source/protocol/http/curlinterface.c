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
#include <pthread.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>

#include "curlinterface.h"
#include "reportprofiles.h"
#include "t2MtlsUtils.h"
#include "t2log_wrapper.h"

static pthread_once_t curlFileMutexOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t curlFileMutex;

typedef enum _ADDRESS_TYPE
{
    ADDR_UNKNOWN,
    ADDR_IPV4,
    ADDR_IPV6
}ADDRESS_TYPE;

static void sendOverHTTPInit(){
	pthread_mutex_init(&curlFileMutex, NULL);
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

#if defined(ENABLE_RDKB_SUPPORT)
     /* CID 125287: Unchecked return value from library */
    code = curl_easy_setopt(curl, CURLOPT_INTERFACE, INTERFACE);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }

#endif
    *headerList = curl_slist_append(NULL, "Accept: application/json");
    curl_slist_append(*headerList, "Content-type: application/json");

    code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headerList);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }

    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static T2ERROR setMtlsHeaders(CURL *curl, const char* certFile, const char* pPasswd) {
    CURLcode code = CURLE_OK;
    code = curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
    if(code != CURLE_OK) {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    code = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "P12");
    if(code != CURLE_OK) {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    /* set the cert for client authentication */
    code = curl_easy_setopt(curl, CURLOPT_SSLCERT, certFile);
    if(code != CURLE_OK) {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    code = curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPasswd);
    if(code != CURLE_OK) {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
    /* disconnect if we cannot authenticate */
    code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if(code != CURLE_OK) {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }

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

T2ERROR sendReportOverHTTP(char *httpUrl, char* payload) {
    CURL *curl = NULL;
    FILE *fp = NULL;
    CURLcode res, code = CURLE_OK;
    T2ERROR ret = T2ERROR_FAILURE;
    long http_code;
    struct curl_slist *headerList = NULL;
    char *pCertFile = NULL;
    char *pKeyFile = NULL;

    pid_t childPid;
    int sharedPipeFds[2];

    T2Debug("%s ++in\n", __FUNCTION__);

    if(pipe(sharedPipeFds) != 0) {
        T2Error("Failed to create pipe !!! exiting...\n");
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    if((childPid = fork()) < 0) {
        T2Error("Failed to fork !!! exiting...\n");
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    /**
     * Openssl has growing RSS which gets cleaned up only with OPENSSL_cleanup .
     * This cleanup is not thread safe and classified as run once per application life cycle.
     * Forking the libcurl calls so that it executes and terminates to release memory per execution.
     */
    if(childPid == 0) {

        curl = curl_easy_init();
        if(curl) {
            if(setHeader(curl, httpUrl, &headerList) != T2ERROR_SUCCESS) {
                T2Error("Failed to Set HTTP Header\n");
                curl_easy_cleanup(curl);
                return ret;
            }

            if(isMtlsEnabled() == true) {
                if(T2ERROR_SUCCESS == getMtlsCerts(&pCertFile, &pKeyFile)) {
                    setMtlsHeaders(curl, pCertFile, pKeyFile);
                }else {
                    T2Error("mTLS_cert get failed\n");
                    curl_easy_cleanup(curl); // CID 189985: Resource leak
                    return T2ERROR_FAILURE;
                }
            }

            setPayload(curl, payload);

            pthread_once(&curlFileMutexOnce, sendOverHTTPInit);
            pthread_mutex_lock(&curlFileMutex);

            fp = fopen(CURL_OUTPUT_FILE, "wb");
            if(fp) {
                /* CID 143029 Unchecked return value from library */
                code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                if(code != CURLE_OK) {
                    T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
                }
                res = curl_easy_perform(curl);
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if(res != CURLE_OK) {
                    fprintf(stderr, "curl failed: %s\n", curl_easy_strerror(res));
                    T2Error("Failed to send report over HTTP, HTTP Response Code : %ld\n", http_code);
                }else {
                    T2Info("Report Sent Successfully over HTTP : %ld\n", http_code);
                    ret = T2ERROR_SUCCESS;
                }

                fclose(fp);
            }
            if(NULL != pCertFile)
                free(pCertFile);

            if(NULL != pKeyFile)
                free(pKeyFile);
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);

            pthread_mutex_unlock(&curlFileMutex);
        }else {
            T2Error("Unable to initialize Curl\n");
        }

        close(sharedPipeFds[0]);
        write(sharedPipeFds[1], &ret, sizeof(T2ERROR));
        close(sharedPipeFds[1]);
        exit(0);

    }else {

        T2ERROR ret = T2ERROR_FAILURE;
        wait(NULL);
        // Get the return status via IPC from child process
        close(sharedPipeFds[1]);
        read(sharedPipeFds[0], &ret, sizeof(T2ERROR));
        close(sharedPipeFds[0]);
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;
    }

}

T2ERROR sendCachedReportsOverHTTP(char *httpUrl, Vector *reportList)
{
    while(Vector_Size(reportList) > 0)
    {
        char* payload = (char *)Vector_At(reportList, 0);
        if(T2ERROR_FAILURE == sendReportOverHTTP(httpUrl, payload))
        {
            T2Error("Failed to send cached report, left with %lu reports in cache \n", (unsigned long)Vector_Size(reportList));
            return T2ERROR_FAILURE;
        }
        Vector_RemoveItem(reportList, payload, NULL);
        free(payload);
    }
    return T2ERROR_SUCCESS;
}
