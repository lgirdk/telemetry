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
#include "t2common.h"
#include "busInterface.h"
#ifdef LIBRDKCERTSEL_BUILD
#include "rdkcertselector.h"
#define FILESCHEME "file://"
#endif
#ifdef LIBRDKCONFIG_BUILD
#include "rdkconfig.h"
#endif

extern sigset_t blocking_signal;

typedef struct
{
    bool curlStatus;
    CURLcode curlResponse;
    CURLcode curlSetopCode;
    long http_code;
    int lineNumber;

} childResponse ;

#ifdef LIBRDKCERTSEL_BUILD
static rdkcertselector_h curlCertSelector = NULL;
#endif
#if defined(ENABLE_RDKB_SUPPORT) && !defined(_WNXL11BWL_PRODUCT_REQ_)

#if defined(WAN_FAILOVER_SUPPORTED) || defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
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

static T2ERROR setHeader(CURL *curl, const char* destURL, struct curl_slist **headerList,childResponse *childCurlResponse)
{

    //T2Debug("%s ++in\n", __FUNCTION__);
    if(curl == NULL || destURL == NULL)
    {
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }

    //T2Debug("%s DEST URL %s \n", __FUNCTION__, destURL);
    CURLcode code=CURLE_OK;
    code = curl_easy_setopt(curl, CURLOPT_URL, destURL);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }
    code = curl_easy_setopt(curl, CURLOPT_SSLVERSION, TLSVERSION);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }
    code = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, HTTP_METHOD);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }
    code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }

#if defined(ENABLE_RDKB_SUPPORT) && !defined(_WNXL11BWL_PRODUCT_REQ_)

#if defined(WAN_FAILOVER_SUPPORTED) || defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
    code = curl_easy_setopt(curl, CURLOPT_INTERFACE, waninterface);
    if(code != CURLE_OK) {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }
#else
     /* CID 125287: Unchecked return value from library */
    code = curl_easy_setopt(curl, CURLOPT_INTERFACE, INTERFACE);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }

#endif
#endif
    *headerList = curl_slist_append(NULL, "Accept: application/json");
    curl_slist_append(*headerList, "Content-type: application/json");

    code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headerList);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	return T2ERROR_FAILURE;
    }

    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	return T2ERROR_FAILURE;
    }

    /* Load server ca, device certificate and private key to curl object */
    if(addCertificatesToHTTPHeader(curl) != T2ERROR_SUCCESS )
    {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }

    childCurlResponse->curlSetopCode = code;
    childCurlResponse->lineNumber = __LINE__;
    //T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static T2ERROR setMtlsHeaders(CURL *curl, const char* certFile, const char* pPasswd, childResponse *childCurlResponse) {
    if(curl == NULL || certFile == NULL || pPasswd == NULL){
        childCurlResponse->lineNumber = __LINE__;
	return T2ERROR_FAILURE;
    }
    CURLcode code = CURLE_OK;
#ifndef LIBRDKCERTSEL_BUILD
    code = curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
    if(code != CURLE_OK) {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
#endif
    code = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "P12");
    if(code != CURLE_OK) {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
    /* set the cert for client authentication */
    code = curl_easy_setopt(curl, CURLOPT_SSLCERT, certFile);
    if(code != CURLE_OK) {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
    code = curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPasswd);
    if(code != CURLE_OK) {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
    /* disconnect if we cannot authenticate */
    code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if(code != CURLE_OK) {
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
    childCurlResponse->curlSetopCode = code;
    childCurlResponse->lineNumber = __LINE__;
    return T2ERROR_SUCCESS;
}

static T2ERROR setPayload(CURL *curl, const char* payload, childResponse *childCurlResponse)
{
    if(curl == NULL || payload == NULL){
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
    CURLcode code = CURLE_OK ;
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
	    return T2ERROR_FAILURE;
    }
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(payload));
    if(code != CURLE_OK){
        childCurlResponse->curlSetopCode = code;
        childCurlResponse->lineNumber = __LINE__;
        return T2ERROR_FAILURE;
    }
    childCurlResponse->curlSetopCode = code;
    childCurlResponse->lineNumber = __LINE__;
    return T2ERROR_SUCCESS;
}
#ifdef LIBRDKCERTSEL_BUILD
void curlCertSelectorFree()
{
    rdkcertselector_free(&curlCertSelector);
    if(curlCertSelector == NULL){
        T2Info("%s, T2:Cert selector memory free\n", __func__);
    }else{
        T2Info("%s, T2:Cert selector memory free failed\n", __func__);
    }
}
static void curlCertSelectorInit()
{
    if(curlCertSelector == NULL)
    {
        curlCertSelector = rdkcertselector_new( NULL, NULL, "MTLS" );
        if(curlCertSelector == NULL){
            T2Error("%s, T2:Cert selector initialization failed\n", __func__);
        }else{
            T2Info("%s, T2:Cert selector initialization successfully\n", __func__);
        }
    }
}
#endif    

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

#if defined(_LG_OFW_)

T2ERROR sendReportOverHTTP(char *httpUrl, char *payload, pid_t* outForkedPid) {

    char command[256];
    T2ERROR ret = T2ERROR_FAILURE;

    if(httpUrl == NULL || payload == NULL)
    {
        return ret;
    }

    //copy the json payload to /tmp/telemetry_report.txt before sending the report using http_send
    FILE *fpReport = fopen(TELEMETRY_REPORT, "w");
    if(fpReport != NULL)
    {
        fprintf(fpReport,"cJSON Report = %s\n", payload);
        fclose(fpReport);
    }
    T2Info("cJSON Report written to file %s\n",TELEMETRY_REPORT);

    // take file instead of json content.
    snprintf(command, sizeof(command), "/usr/bin/http_send %s %s", httpUrl,TELEMETRY_REPORT);
    if(system(command) != 0) {
        T2Error("%s,%d: %s  command failed\n", __FUNCTION__, __LINE__, command);
        return ret;
    }

    return T2ERROR_SUCCESS;

}

T2ERROR sendReportOverHTTP_bin(char *httpUrl, char *payload, pid_t* outForkedPid) {
    CURL *curl = NULL;
    FILE *fp = NULL;
    CURLcode res, code = CURLE_OK;
    T2ERROR ret = T2ERROR_FAILURE;
    childResponse childCurlResponse = {0};
    struct curl_slist *headerList = NULL;
    char buf[256];
    char current_time[20];
    int Timestamp_Status;
    char errbuf[CURL_ERROR_SIZE];
    char *pCertFile = NULL;
    char *pKeyFile = NULL;
    long http_code;
    bool mtls_enable = false;

    T2Info("%s ++in\n", __FUNCTION__);
    if(httpUrl == NULL || payload == NULL)
    {
        return ret;
    }
    mtls_enable = isMtlsEnabled();
    if(mtls_enable == true && T2ERROR_SUCCESS != getMtlsCerts(&pCertFile, &pKeyFile)){
        T2Error("mTLS_cert get failed\n");
        if(NULL != pCertFile)
            free(pCertFile);
        if(NULL != pKeyFile)
            free(pKeyFile);
        return ret;
    }

    T2Info("httpurl %s payload : %s \n ", httpUrl,payload);
    curl = curl_easy_init();
    if(curl) {
            childCurlResponse.curlStatus = true;
            if(setHeader(curl, httpUrl, &headerList, &childCurlResponse) != T2ERROR_SUCCESS) {
                curl_easy_cleanup(curl);
                goto child_cleanReturn;
            }
             T2Info("setHeader  set successfully\n");
            if((mtls_enable == true) && (setMtlsHeaders(curl, pCertFile, pKeyFile, &childCurlResponse)!= T2ERROR_SUCCESS)) {
                curl_easy_cleanup(curl); // CID 189985: Resource leak
                goto child_cleanReturn;
            }
             T2Info("setMTLSHeader  set successfully\n");
            if (setPayload(curl, payload, &childCurlResponse)!= T2ERROR_SUCCESS){
                curl_easy_cleanup(curl); // CID 189985: Resource leak
                goto child_cleanReturn;
            }
            T2Info("setPayload  set successfully\n");

            fp = fopen(CURL_OUTPUT_FILE, "wb");
            if(fp) {
                /* CID 143029 Unchecked return value from library */
                code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fp);
                if(code != CURLE_OK) {
                    // This might not be working we need to review this
                    childCurlResponse.curlSetopCode = code;
                }
                curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
                res = curl_easy_perform(curl);
                 T2Info("curl_easy_perform  successfully\n");
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

                snprintf(buf,sizeof(buf),"%d",http_code);
                if (telemetry_syscfg_set("upload_httpStatus", buf) == 0)
                {
                    T2Info("upload_httpStatus set successfully\n");
                }
                if (http_code == 200)
                {
                    telemetry_syscfg_set("upload_httpStatusString", "OK");
                }
                else
                {
                    telemetry_syscfg_set("upload_httpStatusString", errbuf);
                }
                Timestamp_Status = getcurrenttime(current_time, sizeof(current_time));
                if (Timestamp_Status != 0)
                {
                    T2Error("Failed to fetch current upload_lastAttemptTimestamp\n");
                    telemetry_syscfg_set("upload_lastAttemptTimestamp", "");
                }
                else
                {
                    if (telemetry_syscfg_set("upload_lastAttemptTimestamp", current_time) == 0)
                    {
                        T2Info("upload_lastAttemptTimestamp set successfully\n");
                    }
                    else
                    {
                        T2Error("Failed to set upload_lastAttemptTimestamp\n");
                    }
                }

                if(res != CURLE_OK || http_code != 200) {
                    fprintf(stderr, "curl failed: %s\n", curl_easy_strerror(res));
                    childCurlResponse.lineNumber = __LINE__;
                }else {
                    childCurlResponse.lineNumber = __LINE__;

                    Timestamp_Status = getcurrenttime(current_time, sizeof(current_time));
                    if (Timestamp_Status != 0)
                    {
                        T2Error("Failed to fetch current upload_lastSuccessTimestamp\n");
                        telemetry_syscfg_set("upload_lastSuccessTimestamp", "");
                    }
                    else
                    {
                        if (telemetry_syscfg_set("upload_lastSuccessTimestamp", current_time) == 0)
                        {
                            T2Info("upload_lastSuccessTimestamp set successfully \n");
                        }
                        else
                        {
                            T2Error("Failed to set upload_lastSuccessTimestamp \n");
                        }
                    }
                    T2Info("Report Sent Successfully over HTTP : %ld\n", http_code);
                    ret = T2ERROR_SUCCESS;
                }
                childCurlResponse.curlResponse = res;
                childCurlResponse.http_code = http_code;

                fclose(fp);
            }
            curl_slist_free_all(headerList);
            curl_easy_cleanup(curl);


       }else {
            childCurlResponse.curlStatus = false;
      }

child_cleanReturn:

        ret = T2ERROR_FAILURE;
        if(NULL != pCertFile)
            free(pCertFile);
        if(NULL != pKeyFile)
            free(pKeyFile);
        T2Info("The return status is CurlStatus : %d\n", childCurlResponse.curlStatus);
        T2Info("The return status SetopCode: %s; ResponseCode : %s; HTTP_CODE : %ld; Line Number : %d \n", curl_easy_strerror(childCurlResponse.curlSetopCode), curl_easy_strerror(childCurlResponse.curlResponse), childCurlResponse.http_code, childCurlResponse.lineNumber);
        if (childCurlResponse.http_code == 200 && childCurlResponse.curlResponse == CURLE_OK){
            ret = T2ERROR_SUCCESS;
            T2Info("Report Sent Successfully over HTTP : %ld\n", childCurlResponse.http_code);
        }
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;


}

#else

T2ERROR sendReportOverHTTP(char *httpUrl, char *payload, pid_t* outForkedPid) {
    CURL *curl = NULL;
    FILE *fp = NULL;
    CURLcode code = CURLE_OK;
    T2ERROR ret = T2ERROR_FAILURE;
    childResponse childCurlResponse = {0};
    struct curl_slist *headerList = NULL;
    CURLcode curl_code = CURLE_OK;
#ifdef LIBRDKCERTSEL_BUILD
    rdkcertselectorStatus_t curlGetCertStatus;
    char *pCertURI = NULL;
    char *pEngine=NULL;
#endif
    char buf[256];
    char current_time[20];
    int Timestamp_Status;
    char errbuf[CURL_ERROR_SIZE];
    char *pCertFile = NULL;
    char *pKeyFile = NULL;
#ifdef LIBRDKCONFIG_BUILD
    size_t sKey = 0;
#endif
    long http_code;
    bool mtls_enable = false;
    pid_t childPid;
    int sharedPipeFds[2];

    T2Debug("%s ++in\n", __FUNCTION__);
    if(httpUrl == NULL || payload == NULL)
    {
        return ret;
    }
    if(pipe(sharedPipeFds) != 0) {
        T2Error("Failed to create pipe !!! exiting...\n");
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;
    }
#ifdef LIBRDKCERTSEL_BUILD
    curlCertSelectorInit();
#endif
#if defined(ENABLE_RDKB_SUPPORT) && !defined(_WNXL11BWL_PRODUCT_REQ_)

#if defined(WAN_FAILOVER_SUPPORTED) || defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE) && !defined(_LG_OFW_)
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
#ifndef LIBRDKCERTSEL_BUILD
    if(mtls_enable == true && T2ERROR_SUCCESS != getMtlsCerts(&pCertFile, &pKeyFile)){
        T2Error("mTLS_cert get failed\n");
        if(NULL != pCertFile)
            free(pCertFile);
        if(NULL != pKeyFile){
          #ifdef LIBRDKCONFIG_BUILD
            sKey = strlen(pKeyFile);
            if (rdkconfig_free((unsigned char**)&pKeyFile, sKey) == RDKCONFIG_FAIL) {
                return T2ERROR_FAILURE;
            }
          #else
            free(pKeyFile);
          #endif
        }
        return ret;
    }
#endif
    // Block the userdefined signal handlers before fork
    pthread_sigmask(SIG_BLOCK,&blocking_signal,NULL);
    if((childPid = fork()) < 0) {
        T2Error("Failed to fork !!! exiting...\n");
        // Unblock the userdefined signal handler
#ifndef LIBRDKCERTSEL_BUILD
        if(NULL != pCertFile)
            free(pCertFile);
        if(NULL != pKeyFile){
          #ifdef LIBRDKCONFIG_BUILD
            sKey = strlen(pKeyFile);
            if (rdkconfig_free((unsigned char**)&pKeyFile, sKey) == RDKCONFIG_FAIL) {
                return T2ERROR_FAILURE;
            }
          #else
            free(pKeyFile);
          #endif
        }
#endif
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
        if(curl){
            childCurlResponse.curlStatus = true;
            if(setHeader(curl, httpUrl, &headerList, &childCurlResponse) != T2ERROR_SUCCESS) {
                curl_easy_cleanup(curl);
                goto child_cleanReturn;
            }
            if (setPayload(curl, payload, &childCurlResponse)!= T2ERROR_SUCCESS){
                curl_easy_cleanup(curl); // CID 189985: Resource leak
                goto child_cleanReturn;
            }
#ifdef LIBRDKCERTSEL_BUILD
            pEngine= rdkcertselector_getEngine(curlCertSelector);
            if(pEngine != NULL){
                code = curl_easy_setopt(curl, CURLOPT_SSLENGINE, pEngine);
            }else{
                code = curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
            }
            if(code != CURLE_OK) {
                curl_easy_cleanup(curl);
                goto child_cleanReturn;
            }
            do{
                pCertFile = NULL;
                pKeyFile = NULL;
                pCertURI = NULL;
                curlGetCertStatus= rdkcertselector_getCert(curlCertSelector, &pCertURI, &pKeyFile);              
                if(curlGetCertStatus != certselectorOk)
                {
                    T2Error("%s, T2:Failed to retrieve the certificate.\n", __func__);
                    curlCertSelectorFree();
                    curl_easy_cleanup(curl);
                    goto child_cleanReturn;
                }else{
                    // skip past file scheme in URI
                    pCertFile = pCertURI;
                    if ( strncmp( pCertFile, FILESCHEME, sizeof(FILESCHEME)-1 ) == 0 ) {
                        pCertFile += (sizeof(FILESCHEME)-1);
                    }
#endif
                    if((mtls_enable == true) && (setMtlsHeaders(curl, pCertFile, pKeyFile, &childCurlResponse)!= T2ERROR_SUCCESS)) {
                        curl_easy_cleanup(curl); // CID 189985: Resource leak
                        goto child_cleanReturn;
                    }
                    pthread_once(&curlFileMutexOnce, sendOverHTTPInit);
                    pthread_mutex_lock(&curlFileMutex);

                    fp = fopen(CURL_OUTPUT_FILE, "wb");
                    if(fp) {
                        /* CID 143029 Unchecked return value from library */
                        code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fp);
                        if(code != CURLE_OK) {
                            // This might not be working we need to review this
                            childCurlResponse.curlSetopCode = code;
                        }
                        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
                        curl_code = curl_easy_perform(curl);
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

                        snprintf(buf,sizeof(buf),"%d",http_code);
                        if (telemetry_syscfg_set("upload_httpStatus", buf) == 0)
                        {
                            T2Info("upload_httpStatus set successfully\n");
                        }
                        if (http_code == 200)
                        {
                            telemetry_syscfg_set("upload_httpStatusString", "OK");
                        }
                        else
                        {
                            telemetry_syscfg_set("upload_httpStatusString", errbuf);
                        }
                        Timestamp_Status = getcurrenttime(current_time, sizeof(current_time));
                        if (Timestamp_Status != 0)
                        {
                            T2Error("Failed to fetch current upload_lastAttemptTimestamp\n");
                            telemetry_syscfg_set("upload_lastAttemptTimestamp", "");
                        }
                        else
                        {
                            if (telemetry_syscfg_set("upload_lastAttemptTimestamp", current_time) == 0)
                            {
                                T2Info("upload_lastAttemptTimestamp set successfully\n");
                            }
                            else
                            {
                                T2Error("Failed to set upload_lastAttemptTimestamp\n");
                            }
                        }

                        if(curl_code != CURLE_OK || http_code != 200) {
                            fprintf(stderr, "curl failed: %s\n", curl_easy_strerror(curl_code));
                            childCurlResponse.lineNumber = __LINE__;
                        }else {
                            childCurlResponse.lineNumber = __LINE__;

                            Timestamp_Status = getcurrenttime(current_time, sizeof(current_time));
                            if (Timestamp_Status != 0)
                            {
                                T2Error("Failed to fetch current upload_lastSuccessTimestamp\n");
                                telemetry_syscfg_set("upload_lastSuccessTimestamp", "");
                            }
                            else
                            {
                                if (telemetry_syscfg_set("upload_lastSuccessTimestamp", current_time) == 0)
                                {
                                    T2Info("upload_lastSuccessTimestamp set successfully \n");
                                }
                                else
                                {
                                    T2Error("Failed to set upload_lastSuccessTimestamp \n");
                                }
                            }
                            T2Info("Report Sent Successfully over HTTP : %ld\n", http_code);
                            ret = T2ERROR_SUCCESS;
                        }
                        childCurlResponse.curlResponse = curl_code;
                        childCurlResponse.http_code = http_code;

                        fclose(fp);
                    }
                    curl_slist_free_all(headerList);
                    pthread_mutex_unlock(&curlFileMutex);
#ifdef LIBRDKCERTSEL_BUILD
                }
            }while(rdkcertselector_setCurlStatus(curlCertSelector, curl_code, (const char*)httpUrl) == TRY_ANOTHER);
#endif
            curl_easy_cleanup(curl);
        }else {
            childCurlResponse.curlStatus = false;
        }

        child_cleanReturn :
#ifndef LIBRDKCERTSEL_BUILD
        if(NULL != pCertFile)
            free(pCertFile);

        if(NULL != pKeyFile){
          #ifdef LIBRDKCONFIG_BUILD
            sKey = strlen(pKeyFile);
            if (rdkconfig_free((unsigned char**)&pKeyFile, sKey) == RDKCONFIG_FAIL) {
                return T2ERROR_FAILURE;
            }
          #else
            free(pKeyFile);
          #endif
        }
#endif
        close(sharedPipeFds[0]);
        if( -1 == write(sharedPipeFds[1], &childCurlResponse, sizeof(childResponse))){
            fprintf(stderr, "unable to write to shared pipe from pid : %d \n", getpid());
            T2Error("unable to write \n");
        }
        close(sharedPipeFds[1]);
        exit(0);

    }else {
        T2ERROR ret = T2ERROR_FAILURE;
        if(outForkedPid){
            *outForkedPid = childPid ;
        }
        // Use waitpid insted of wait we can have multiple child process
        waitpid(childPid,NULL,0);
        // Unblock the userdefined signal handlers before fork
        pthread_sigmask(SIG_UNBLOCK,&blocking_signal,NULL);
        // Get the return status via IPC from child process
        if ( -1 == close(sharedPipeFds[1])){
            T2Error("Failed in close \n");
        }
        if( -1 == read(sharedPipeFds[0], &childCurlResponse, sizeof(childResponse))){
            T2Error("unable to read from the pipe \n");
        }
        if ( -1 == close(sharedPipeFds[0])){
            T2Error("Failed in close the pipe\n");
        }
#ifndef LIBRDKCERTSEL_BUILD
        if(NULL != pCertFile)
            free(pCertFile);
        if(NULL != pKeyFile){
          #ifdef LIBRDKCONFIG_BUILD
            sKey = strlen(pKeyFile);
            if (rdkconfig_free((unsigned char**)&pKeyFile, sKey) == RDKCONFIG_FAIL) {
                return T2ERROR_FAILURE;
            }
          #else
            free(pKeyFile);
          #endif
        }
#endif
        T2Info("The return status from the child with pid %d is CurlStatus : %d\n",childPid, childCurlResponse.curlStatus);
        //if(childCurlResponse.curlStatus == CURLE_OK) commenting this as we are observing childCurlResponse.curlStatus as 1, from line with CID 143029 Unchecked return value from library
        T2Info("The return status from the child with pid %d SetopCode: %s; ResponseCode : %s; HTTP_CODE : %ld; Line Number : %d \n", childPid, curl_easy_strerror(childCurlResponse.curlSetopCode), curl_easy_strerror(childCurlResponse.curlResponse), childCurlResponse.http_code, childCurlResponse.lineNumber);
        if (childCurlResponse.http_code == 200 && childCurlResponse.curlResponse == CURLE_OK){
            ret = T2ERROR_SUCCESS;
            T2Info("Report Sent Successfully over HTTP : %ld\n", childCurlResponse.http_code);
        }
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;
    }

}
#endif

T2ERROR sendCachedReportsOverHTTP(char *httpUrl, Vector *reportList)
{
    if(httpUrl == NULL || reportList == NULL)
    {
        return T2ERROR_FAILURE;
    }
    while(Vector_Size(reportList) > 0)
    {
        char* payload = (char *)Vector_At(reportList, 0);
        if(T2ERROR_FAILURE == sendReportOverHTTP(httpUrl, payload, NULL))
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
