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
#include <signal.h>

#include "curlinterface.h"
#include "reportprofiles.h"
#include "t2MtlsUtils.h"
#include "t2log_wrapper.h"
#include "busInterface.h"

extern sigset_t blocking_signal;

#if defined(ENABLE_RDKB_SUPPORT)

#if defined(WAN_FAILOVER_SUPPORTED)
   static char waninterface[256];
#endif
#endif
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

#if defined(WAN_FAILOVER_SUPPORTED)

    code = curl_easy_setopt(curl, CURLOPT_INTERFACE, waninterface);
    if(code != CURLE_OK) {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }
#else
     /* CID 125287: Unchecked return value from library */
    code = curl_easy_setopt(curl, CURLOPT_INTERFACE, INTERFACE);
    if(code != CURLE_OK){
       T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
    }

#endif
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

    /* Load server ca, device certificate and private key to curl object */
    if(addCertificatesToHTTPHeader(curl) != T2ERROR_SUCCESS )
    {
        return T2ERROR_FAILURE;
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

static CURLcode load_private_key(void *sslctx)
{

    CURLcode return_status = CURLE_SSL_CERTPROBLEM;

    /* If the private key cannot be loaded directly, then we need to decrypt it. */
    if( (SSL_CTX_use_PrivateKey_file((SSL_CTX *)sslctx, PRIVATE_KEY, SSL_FILETYPE_PEM) != 1) )
    {
        T2Error("Unable to add Private key to the request \n");
    }
    else
    {
        T2Info("Private key successfully added to the request\n");
        return_status = CURLE_OK;
    }

    return return_status;
}

static CURLcode load_certificates(CURL *curl, void *sslctx, void *parm)
{

    X509 *dev_cert = NULL;
    X509 *man_cert = NULL;
    FILE *f_cert = fopen(DEVICE_CERT, "rb");
    FILE *f_man = fopen(MANUFACTURER_CERT, "rb");
    int ret = -1;
    CURLcode return_status = CURLE_OK;

    if (f_cert == NULL || f_man == NULL)
    {
        if (f_cert)
        {
            T2Error("Unable to open device certificate!!!\n");
            fclose(f_cert);
        }
        if (f_man)
        {
            T2Error("Unable to open manufacturer certificate!!!\n");
            fclose(f_man);
        }

        return CURLE_SSL_CERTPROBLEM;
    }
    /* Load device certificate to ssl-ctx structure*/
    if (f_cert)
    {
        dev_cert = d2i_X509_fp(f_cert, NULL);
        if (dev_cert)
        {
            ret = SSL_CTX_use_certificate((SSL_CTX *)sslctx, dev_cert);
        }

        if (ret != 1)
        {
            return_status = CURLE_SSL_CERTPROBLEM;
        }
        fclose(f_cert);
    }

    if (return_status == CURLE_OK)
    {
        /* Form a certificate chain by Concatenating manufacturer certificate to device certificate */
        if (f_man)
        {
            ret = -1;
            man_cert = d2i_X509_fp(f_man, NULL);
            if (man_cert)
            {
                /* The x509 certificate provided to SSL_CTX_add_extra_chain_cert() will be freed by the library when the SSL_CTX is destroyed.
                we should not free man_cert */
                ret = SSL_CTX_add_extra_chain_cert((SSL_CTX *)sslctx, man_cert);
            }

            if (ret != 1)
            {
                T2Error("Failed to concatenate manufacturer certificate to device certificate!!!\n");
                return_status = CURLE_SSL_CERTPROBLEM;
            }
        }
    }
    if (dev_cert)
    {
        /* SSL_CTX_use_certificate function copy the certificate to ssl using SSL_new() function. So it is safe to free dev_cert */
        X509_free(dev_cert);
        dev_cert = NULL;
    }
    if (return_status == CURLE_OK)
    {
        T2Info("Device certificates successfully added to the request\n");
        return_status = load_private_key(sslctx);
    }
    else
    {
        T2Error("Unable to add device certificates to the request, error code - %d\n", ret);
    }
    /* Close the manufacturer certificate */
    if(f_man)
        fclose(f_man);

    return return_status;
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
    bool mtls_enable = false;
    pid_t childPid;
    int sharedPipeFds[2];

    T2Debug("%s ++in\n", __FUNCTION__);

    if(pipe(sharedPipeFds) != 0) {
        T2Error("Failed to create pipe !!! exiting...\n");
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }
#if defined(ENABLE_RDKB_SUPPORT)

#if defined(WAN_FAILOVER_SUPPORTED)
    char *paramVal = NULL;
    memset(waninterface, 0, sizeof(waninterface));
    snprintf(waninterface, sizeof(waninterface), "%s", INTERFACE); 

 if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_CURRENT_WAN_IFNAME, &paramVal)) {
        if(strlen(paramVal) >0) {
            memset(waninterface, 0, sizeof(waninterface));
            snprintf(waninterface, sizeof(waninterface), "%s", paramVal);
        }
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_CURRENT_WAN_IFNAME);
    }
 #endif
 #endif
    mtls_enable = isMtlsEnabled();
    // Block the userdefined signal handlers before fork
    pthread_sigmask(SIG_BLOCK,&blocking_signal,NULL);
    if((childPid = fork()) < 0) {
        T2Error("Failed to fork !!! exiting...\n");
        // Unblock the userdefined signal handlers
        pthread_sigmask(SIG_UNBLOCK,&blocking_signal,NULL);
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

            if(mtls_enable == true) {
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
        // Use waitpid insted of wait we can have multiple child process
        waitpid(childPid,NULL,0);
        // Unblock the userdefined signal handlers before fork
        pthread_sigmask(SIG_UNBLOCK,&blocking_signal,NULL);
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

T2ERROR addCertificatesToHTTPHeader(CURL *curl)
{

    CURLcode code = CURLE_OK;
    T2ERROR ret = T2ERROR_SUCCESS;

    code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if(code != CURLE_OK)
    {
        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        ret = T2ERROR_FAILURE;
    }

    if(ret == T2ERROR_SUCCESS)
    {
        code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        if(code != CURLE_OK)
        {
            T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            ret = T2ERROR_FAILURE;
        }
    }

    if(ret == T2ERROR_SUCCESS)
    {
        code = curl_easy_setopt(curl, CURLOPT_CAINFO, SERVER_CA_CERT);
        if(code != CURLE_OK)
        {
            T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            ret = T2ERROR_FAILURE;
        }
    }

    if(ret == T2ERROR_SUCCESS)
    {
        /* Add device certificates and key to the handle */
        code = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, *load_certificates);
        if(code != CURLE_OK)
        {
            T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            ret = T2ERROR_FAILURE;
        }
    }

    return ret;

}
