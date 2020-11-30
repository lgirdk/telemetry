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

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cJSON.h"
#include "t2log_wrapper.h"
#include "reportprofiles.h"
#include "profilexconf.h"
#include "xconfclient.h"

#include "t2MtlsUtils.h"
#include "t2parserxconf.h"
#include "vector.h"
#include "persistence.h"
#include "telemetry2_0.h"
#include "busInterface.h"
#include "curlinterface.h"
#include "syscfg/syscfg.h"

#define RFC_RETRY_TIMEOUT 60
#define XCONF_RETRY_TIMEOUT 180
#define MAX_XCONF_RETRY_COUNT 5
#define IFINTERFACE      "erouter0"
#define XCONF_CONFIG_FILE  "DCMresponse.txt"
#define PROCESS_CONFIG_COMPLETE_FLAG "/tmp/t2DcmComplete"
#define HTTP_RESPONSE_FILE "/tmp/httpOutput.txt"

static const int MAX_URL_LEN = 1024;
static const int MAX_URL_ARG_LEN = 128;
static int xConfRetryCount = 0;
static bool stopFetchRemoteConfiguration = false;
static bool isXconfInit = false ;

static pthread_t xcrThread;
static pthread_mutex_t xcMutex;
static pthread_cond_t xcCond;

T2ERROR ReportProfiles_deleteProfileXConf(ProfileXConf *profile);

T2ERROR ReportProfiles_setProfileXConf(ProfileXConf *profile);

typedef enum _IFADDRESS_TYPE
{
    ADDR_UNKNOWN,
    ADDR_IPV4,
    ADDR_IPV6
}IFADDRESS_TYPE;

#if 0
static IFADDRESS_TYPE getAddressType(const char *cif) {
    struct ifaddrs *ifap, *ifa;
    IFADDRESS_TYPE addressType = 0;

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
#endif

static T2ERROR getBuildType(char* buildType) {
    char fileContent[255] = { '\0' };
    FILE *deviceFilePtr;
    char *pBldTypeStr = NULL;
    int offsetValue = 0;

    if (NULL == buildType) {
       return T2ERROR_FAILURE;
    }
    deviceFilePtr = fopen( DEVICE_PROPERTIES, "r");
    if (deviceFilePtr) {
        while (fscanf(deviceFilePtr, "%255s", fileContent) != EOF) {
            if ((pBldTypeStr = strstr(fileContent, "BUILD_TYPE")) != NULL) {
                offsetValue = strlen("BUILD_TYPE=");
                pBldTypeStr = pBldTypeStr + offsetValue;
                break;
            }
        }
        fclose(deviceFilePtr);
    }
    if(pBldTypeStr != NULL){
         strncpy(buildType, pBldTypeStr, BUILD_TYPE_MAX_LENGTH - 1);
         return T2ERROR_SUCCESS;
    }
    return T2ERROR_FAILURE;
}

#if !defined(ENABLE_RDKB_SUPPORT)
static char *getTimezone () {
    T2Debug("Retrieving the timezone value\n");
    int count = 0, i = 0;
    FILE *file, *fp;
    char *zoneValue = NULL;
    char *jsonDoc = NULL;
    static const char* jsonpath = NULL;
    static char* CPU_ARCH = NULL;
    char fileContent[255] = { '\0' };
    fp = fopen( DEVICE_PROPERTIES, "r");
    if (fp) {
        while (fscanf(fp, "%255s", fileContent) != EOF) {
            char *property = NULL;
            if ((property = strstr(fileContent, "CPU_ARCH")) != NULL) {
                property = property + strlen("CPU_ARCH=");
                CPU_ARCH = strdup(property);
                T2Debug("CPU_ARCH=%s\n",CPU_ARCH);
                break;
            }
        }
        fclose(fp);
    }
    jsonpath = "/opt/output.json";
    if((NULL != CPU_ARCH) && (0 == strcmp("x86", CPU_ARCH))){
            jsonpath = "/tmp/output.json";
    }
    T2Debug("Reading Timezone value from %s file...\n", jsonpath);
    while ( zoneValue == NULL){
          T2Debug ("timezone retry:%d\n",count);
          if (access(jsonpath, F_OK) != -1){
                  file = fopen( jsonpath, "r");
                  if (file) {
                        fseek(file, 0, SEEK_END);
                        long numbytes = ftell(file);
                        jsonDoc = (char*)malloc(sizeof(char)*(numbytes + 1));
                        fseek(file, 0, SEEK_SET);
                        fread(jsonDoc, numbytes, 1, file);
                        fclose(file);
                        cJSON *root = cJSON_Parse(jsonDoc);
                        if (root != NULL){
                            cJSON *array = cJSON_GetObjectItem(root, "xmediagateways");
                            if(array){
                                 for (i = 0 ; i < cJSON_GetArraySize(array) ; i++)
                                 {
                                      cJSON * subarray = cJSON_GetArrayItem(array, i);
                                      cJSON * timezone = cJSON_GetObjectItem(subarray, "timezone");
                                      if(timezone){
                                          char *time = cJSON_GetStringValue(timezone);
                                          zoneValue = strdup(time);
                                      }
                                 }
                            }
                        }
                        free(jsonDoc);
                        jsonDoc = NULL;
                        cJSON_Delete(root);
                 }
          }
          count++;
         if (count == 10){
             T2Debug("Timezone retry count reached the limit . Timezone data source is missing\n");
             break;
         }
     }
     if ( zoneValue == NULL) {
              T2Debug("Timezone value from %s is empty, Reading from  /opt/persistent/timeZoneDST file...\n",jsonpath);
              if (access("/opt/persistent/timeZoneDST", F_OK) != -1){
                      file = fopen ("/opt/persistent/timeZoneDST", "r");
                      if (NULL != file){
                              fseek(file, 0, SEEK_END);
                              long numbytes = ftell(file);
                              char *zone = (char*)malloc(sizeof(char)*(numbytes + 1));
                              fseek(file, 0, SEEK_SET);
                              while (fscanf (file, "%s", zone) != EOF){
                                        zoneValue = strdup(zone);
                              }
                        fclose(file);
                        free(zone);
                      }
              }
     
     }
     if(CPU_ARCH){
	     free(CPU_ARCH);
     }
     return zoneValue;
}
#endif

static T2ERROR appendRequestParams(char *buf, const int maxArgLen) {

    T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);

    int avaBufSize = maxArgLen, write_size = 0;
    char *paramVal = NULL;
    char *tempBuf = (char*) malloc(MAX_URL_ARG_LEN);
    char build_type[BUILD_TYPE_MAX_LENGTH] = { 0 };

    if(tempBuf == NULL)
    {
        T2Error("Failed to allocate memory for RequestParams\n");
        return T2ERROR_FAILURE;
    }

    if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_WAN_MAC, &paramVal)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "estbMacAddress=%s&", paramVal);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_WAN_MAC);
          goto error;
    }

    if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_FW_VERSION, &paramVal)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "firmwareVersion=%s&", paramVal);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_FW_VERSION);
          goto error;
    }

    if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_MODEL, &paramVal)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "model=%s&", paramVal);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_MODEL);
          goto error;
    }

    if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_PARTNER_ID, &paramVal)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "partnerId=%s&", paramVal);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_PARTNER_ID);
          goto error;
    }

    if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_ACCOUNT_ID, &paramVal)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "accountId=%s&", paramVal);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_ACCOUNT_ID);
          goto error;
    }

    if(T2ERROR_SUCCESS == getParameterValue(TR181_DEVICE_CM_MAC, &paramVal)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "ecmMacAddress=%s&", paramVal);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
    } else {
          T2Error("Failed to get Value for %s\n", TR181_DEVICE_CM_MAC);
          goto error;
    }

    if(T2ERROR_SUCCESS == getBuildType(build_type)) {
        memset(tempBuf, 0, MAX_URL_ARG_LEN);
        write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "env=%s&", build_type);
        strncat(buf, tempBuf, avaBufSize);
        avaBufSize = avaBufSize - write_size;
        free(paramVal);
        paramVal = NULL;
        ret = T2ERROR_SUCCESS;
    } else {
          T2Error("Failed to get Value for %s\n", "BUILD_TYPE");
          goto error;
    }

    // TODO Check relevance of this existing hardcoded data - can be removed if not used in production
     strncat(buf,
            "controllerId=2504&channelMapId=2345&vodId=15660&",
            avaBufSize);
     int slen = strlen("controllerId=2504&channelMapId=2345&vodId=15660&");
    avaBufSize = avaBufSize - slen;
#if !defined(ENABLE_RDKB_SUPPORT)
    char *timezone = NULL;
    timezone = getTimezone();
    if(timezone != NULL){
            memset(tempBuf, 0, MAX_URL_ARG_LEN);
            write_size = snprintf(tempBuf, MAX_URL_ARG_LEN, "timezone=%s&",timezone);
            strncat(buf, tempBuf, avaBufSize);
            avaBufSize = avaBufSize - write_size;
            free(timezone);
     } else{
	     T2Error("Failed to get Value for %s\n", "TIMEZONE");
             ret = T2ERROR_FAILURE;
	     goto error;
     }
#endif
     strncat(buf,"version=2", avaBufSize);
    T2Debug("%s:%d Final http get URL if size %d is : \n %s \n", __func__,
            __LINE__, avaBufSize, buf);
error:
    if (NULL != tempBuf) {
        free(tempBuf);
    }

    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

static size_t httpGetCallBack(void *response, size_t len, size_t nmemb,
        void *stream) {

    size_t realsize = len * nmemb;
    curlResponseData* httpResponse = (curlResponseData*) stream;

    char *ptr = (char*) realloc(httpResponse->data,
            httpResponse->size + realsize + 1);
    if (!ptr) {
        T2Error("%s:%u , T2:memory realloc failed\n", __func__, __LINE__);
        return 0;
    }
    httpResponse->data = ptr;
    memcpy(&(httpResponse->data[httpResponse->size]), response, realsize);
    httpResponse->size += realsize;
    httpResponse->data[httpResponse->size] = 0;

    return realsize;
}

static int  getcurrenttime(char* current_time_string,int timestampparams)
{
    time_t current_time;
    struct tm* c_time_string;
    /* Obtain current time. */
    current_time = time(NULL);
    if (current_time == ((time_t)-1))
    {
        T2Error("Failed to obtain the current time\n");
        current_time_string=NULL;
        return 1;
    }
    /* Convert to local time format. */
    c_time_string = localtime(&current_time);
    if (c_time_string == NULL)
    {
        T2Error("Failure to obtain the current time\n");
        current_time_string=NULL;
        return 1;
    }
    strftime(current_time_string, timestampparams, "%Y-%m-%d %H:%M:%S", c_time_string);
    return 0;
}

static T2ERROR doHttpGet(char* httpsUrl, char **data) {

    T2Debug("%s ++in\n", __FUNCTION__);

    T2Info("%s with url %s \n", __FUNCTION__, httpsUrl);
    CURL *curl;
    CURLcode code = CURLE_OK;
    long http_code = 0;
    long curl_code = 0;
    char buf[256];
    char current_time[20];
    int Timestamp_Status;
    char errbuf[CURL_ERROR_SIZE];

    char *pCertFile = NULL;
    char *pPasswd = NULL;
    // char *pKeyType = "PEM" ;

    pid_t childPid;
    int sharedPipeFdStatus[2];
    int sharedPipeFdDataLen[2];

    if(NULL == httpsUrl) {
        T2Error("NULL httpsUrl given, doHttpGet failed\n");
        return T2ERROR_FAILURE;
    }

    if(pipe(sharedPipeFdStatus) != 0) {
        T2Error("Failed to create pipe for status !!! exiting...\n");
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    if(pipe(sharedPipeFdDataLen) != 0) {
        T2Error("Failed to create pipe for data length!!! exiting...\n");
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

        T2ERROR ret = T2ERROR_FAILURE;
        curlResponseData* httpResponse = (curlResponseData *) malloc(sizeof(curlResponseData));
        httpResponse->data = malloc(1);
        httpResponse->size = 0;

        curl = curl_easy_init();

        if(curl) {

            code = curl_easy_setopt(curl, CURLOPT_URL, httpsUrl);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
            code = curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
            code = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
            code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
            code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpGetCallBack);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
            code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) httpResponse);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
            code = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
            if(code != CURLE_OK){
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }

            if(isMtlsEnabled() == true) {
                if(T2ERROR_SUCCESS == getMtlsCerts(&pCertFile, &pPasswd)) {
                    code = curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
                    if(code != CURLE_OK) {
                        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
                    }

                    code = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "P12");
                    if(code != CURLE_OK) {
                        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
                    }
                    code = curl_easy_setopt(curl, CURLOPT_SSLCERT, pCertFile);
                    if(code != CURLE_OK) {
                        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
                    }
                    code = curl_easy_setopt(curl, CURLOPT_KEYPASSWD, pPasswd);
                    if(code != CURLE_OK) {
                        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
                    }

                    /* disconnect if authentication fails */
                    code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
                    if(code != CURLE_OK) {
                        T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
                    }
                }else {
                    free(httpResponse->data);
                    free(httpResponse);
                    curl_easy_cleanup(curl); //CID 189986:Resource leak
                    T2Error("mTLS_get failure\n");
                    ret = T2ERROR_FAILURE;
                    goto status_return;
                }
            }

            /* Load server ca, device certificate and private key to curl object */
            addCertificatesToHTTPHeader(curl);

#if defined(ENABLE_RDKB_SUPPORT)
            code = curl_easy_setopt(curl, CURLOPT_INTERFACE, IFINTERFACE);
            if(code != CURLE_OK) {
                T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
            }
#endif      

            curl_code = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            snprintf(buf,sizeof(buf),"%d",curl_code);
            if (syscfg_set(NULL, "dcm_httpStatus", buf) == 0) 
            {
               T2Info("dcm_httpStatus set successfully\n");
            }
            snprintf(buf,sizeof(buf),"%s",errbuf);
            if (syscfg_set(NULL, "dcm_httpStatusString", buf) == 0)
            {
               T2Info("dcm_httpStatusString set successfully\n");
            }

            Timestamp_Status = getcurrenttime(current_time, sizeof(current_time));
            if (Timestamp_Status != 0)
            {
               T2Error("Failed to fetch current dcm_lastAttemptTimestamp\n");
               syscfg_set(NULL, "dcm_lastAttemptTimestamp", "");
            }
            else
            {
               if (syscfg_set(NULL, "dcm_lastAttemptTimestamp", current_time) == 0)
               {
                  T2Info("dcm_lastAttemptTimestamp set successfully\n");
               }
               else
               {
                  T2Error("Failed to set dcm_lastAttemptTimestamp\n");
               }
            }

            if(http_code == 200 && curl_code == CURLE_OK) {
                T2Info("%s:%d, T2:Telemetry XCONF communication success\n", __func__, __LINE__);
                size_t len = strlen(httpResponse->data);

                Timestamp_Status = getcurrenttime(current_time, sizeof(current_time));
                if (Timestamp_Status != 0)
                {
                   T2Error("Failed to fetch current dcm_lastSuccessTimestamp\n");
                   syscfg_set(NULL, "dcm_lastSuccessTimestamp", "");
                }
                else
                {
                   if (syscfg_set(NULL, "dcm_lastSuccessTimestamp", current_time) == 0)
                   {
                      T2Info("dcm_lastSuccessTimestamp set successfully \n");
                   }
                   else
                   {
                      T2Error("Failed to set dcm_lastSuccessTimestamp \n");
                   }
                }

                // Share data with parent
                close(sharedPipeFdDataLen[0]);
                write(sharedPipeFdDataLen[1], &len, sizeof(size_t));
                close(sharedPipeFdDataLen[1]);

                FILE *httpOutput = fopen(HTTP_RESPONSE_FILE, "w+");
                if(httpOutput){
                    T2Debug("Update config data in response file %s \n", HTTP_RESPONSE_FILE);
                    fputs(httpResponse->data, httpOutput);
                    fclose(httpOutput);
                } else{
                    T2Error("Unable to open %s file \n", HTTP_RESPONSE_FILE);
                }

                free(httpResponse->data);
                free(httpResponse);
                if(NULL != pCertFile)
                    free(pCertFile);

                if(NULL != pPasswd)
                    free(pPasswd);
                curl_easy_cleanup(curl);
            }else {
                T2Error("%s:%d, T2:Telemetry XCONF communication Failed with http code : %ld Curl code : %ld \n", __func__, __LINE__, http_code,
                        curl_code);
                T2Error("%s : curl_easy_perform failed with error message %s from curl \n", __FUNCTION__, curl_easy_strerror(curl_code));
                free(httpResponse->data);
                free(httpResponse);
                if(NULL != pCertFile)
                    free(pCertFile);
                if(NULL != pPasswd)
                    free(pPasswd);
                curl_easy_cleanup(curl);
                if(http_code == 404)
                    ret = T2ERROR_PROFILE_NOT_SET;
                else
                    ret = T2ERROR_FAILURE;
                goto status_return ;
            }
        }else {
            free(httpResponse->data);
            free(httpResponse);
            ret = T2ERROR_FAILURE;
            goto status_return ;
        }

        ret = T2ERROR_SUCCESS ;
        status_return :

        close(sharedPipeFdStatus[0]);
        write(sharedPipeFdStatus[1], &ret, sizeof(T2ERROR));
        close(sharedPipeFdStatus[1]);
        exit(0);

    }else { // Parent
        T2ERROR ret = T2ERROR_FAILURE;
        wait(NULL);
        // Get the return status via IPC from child process
        close(sharedPipeFdStatus[1]);
        read(sharedPipeFdStatus[0], &ret, sizeof(T2ERROR));
        close(sharedPipeFdStatus[0]);

        // Get the datas via IPC from child process
        if(ret == T2ERROR_SUCCESS) {
            size_t len = 0;
            close(sharedPipeFdDataLen[1]);
            read(sharedPipeFdDataLen[0], &len, sizeof(size_t));
            close(sharedPipeFdDataLen[0]);

            *data = malloc(len + 1);
            if(*data == NULL) {
                T2Error("Unable to allocate memory for XCONF config data \n");
                ret = T2ERROR_FAILURE;
            }else {
                memset(*data, '\0', len + 1);
                FILE *httpOutput = fopen(HTTP_RESPONSE_FILE, "r+");
                if(httpOutput){
                    fgets(*data, len + 1, httpOutput);
                    T2Debug("Configuration obtained from http server : \n %s \n", *data);
                    fclose(httpOutput);
                }
            }
        }
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;

    }
    
}

static T2ERROR fetchRemoteConfiguration(char *configURL, char **configData) {
    // Handles the https communications with the xconf server
    T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);

    int write_size = 0, availableBufSize = MAX_URL_LEN;
    char* urlWithParams = (char*) malloc(MAX_URL_LEN * sizeof(char));
    if (NULL != urlWithParams)
    {
        memset(urlWithParams, '0', MAX_URL_LEN * sizeof(char));
        write_size = snprintf(urlWithParams, MAX_URL_LEN, "%s?", configURL);
        availableBufSize = availableBufSize - write_size;
        // Append device specific arguments to the base URL
        if(T2ERROR_SUCCESS == appendRequestParams(urlWithParams, availableBufSize))
        {
            ret = doHttpGet(urlWithParams, configData);
            if (ret != T2ERROR_SUCCESS)
            {
                T2Error("T2:Curl GET of XCONF data failed\n");
            }
        }
        free(urlWithParams);
    }
    else
    {
        T2Error("Malloc failed\n");
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

static T2ERROR getRemoteConfigURL(char **configURL) {

    T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);

    char *paramVal = NULL;
     /**
     * Attempts to read from PAM before its ready creates deadlock in PAM .
     * Long pending unresolved issue with PAM !!!
     * PAM not ready is a definite case for caching the event and avoid bus traffic
     * */
    #if defined(ENABLE_RDKB_SUPPORT)
    int count = 0 , MAX_RETRY = 20 ;
    while (access( "/tmp/pam_initialized", F_OK ) != 0) {
        sleep(6);
        if(count >= MAX_RETRY)
            break ;
        count ++ ;
    }
    #endif

    if (T2ERROR_SUCCESS == getParameterValue(TR181_CONFIG_URL, &paramVal)) {
        if (NULL != paramVal) {
            if ((strlen(paramVal) > 8) && ((memcmp(paramVal, "http://", 7) == 0) || (memcmp(paramVal, "https://", 8) == 0))) {  // Allow both http and https for new endpoints
                T2Info("Setting config URL base location to : %s\n", paramVal);
                *configURL = paramVal;
                ret = T2ERROR_SUCCESS ;
            } else {
                T2Error("URL doesn't start with http or https !!! URL value received : %s .\n", paramVal);
                free(paramVal);
            }
        } else {
            ret = T2ERROR_FAILURE;
        }
    } else {
        T2Error("Failed to fetch value for parameter %s \n", TR181_CONFIG_URL);
        ret = T2ERROR_FAILURE ;
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return ret; 
}

static void* getUpdatedConfigurationThread(void *data)
{
    T2ERROR configFetch = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);
    struct timespec _ts;
    struct timespec _now;
    int n;
    char *configURL = NULL;
    char *configData = NULL;
    stopFetchRemoteConfiguration = false ;
    char buf[12];
    int maxAttempts = -1, attemptInterval = -1;
    int isValidUrl = 1;

    if (syscfg_get (NULL, "dcm_retry_maxAttempts", buf, sizeof(buf)) == 0)
    {
        maxAttempts = atoi(buf);
    }
    if (syscfg_get (NULL, "dcm_retry_attemptInterval", buf, sizeof(buf)) == 0)
    {
        attemptInterval = atoi(buf);
    }

    /* dcm_retry_maxAttempts and dcm_retry_attemptInterval can't be zero in syscfg db.
    If syscfg_get fails then assign default values. */
    if(maxAttempts <= 0)
    {
        maxAttempts = 3;
        T2Info("syscfg_get failed for dcm_retry_maxAttempts. Setting maxAttempts to default value(%d)\n", maxAttempts);
    }
    if(attemptInterval <= 0)
    {
        attemptInterval = 60;
        T2Info("syscfg_get failed for dcm_retry_attemptInterval. Setting attemptInterval to default value(%d)\n", attemptInterval);
    }

    if(T2ERROR_SUCCESS != getRemoteConfigURL(&configURL))
    {
        isValidUrl = 0;
        T2Error("Config Url is empty or Invalid!!!\n");
    }

    while(!stopFetchRemoteConfiguration && isValidUrl)
    {
        T2ERROR ret = fetchRemoteConfiguration(configURL, &configData);
        if(ret == T2ERROR_SUCCESS)
        {
            ProfileXConf *profile = 0;
            T2Debug("Config received successfully from URL : %s\n", configURL);
            T2Debug("Config received = %s\n", configData);

            if(T2ERROR_SUCCESS == processConfigurationXConf(configData, &profile))
            {

                if(!ProfileXConf_isNameEqual(profile->name))
                {
                    clearPersistenceFolder(XCONFPROFILE_PERSISTENCE_PATH);
                    if(T2ERROR_SUCCESS != saveConfigToFile(XCONFPROFILE_PERSISTENCE_PATH, XCONF_CONFIG_FILE, configData))
                    {
                        T2Error("Unable to save profile : %s to disk\n", profile->name);
                    }
                    if(ProfileXConf_isSet() && T2ERROR_SUCCESS != ReportProfiles_deleteProfileXConf(NULL))
                    {
                        T2Error("Unable to delete existing xconf profile \n");
                    }

                }
                else
                {
                    T2Info("Profile exists already, updating the config in file system\n");

                    if(T2ERROR_SUCCESS != saveConfigToFile(XCONFPROFILE_PERSISTENCE_PATH, XCONF_CONFIG_FILE, configData)) // Should be removed once XCONF sends new UUID for each update.
                    {
                        T2Error("Unable to update an existing config file : %s\n", profile->name);
                    }
                    T2Debug("Disable and Delete old profile %s\n", profile->name);
                    if(T2ERROR_SUCCESS != ReportProfiles_deleteProfileXConf(profile))
                    {
                        T2Error("Unable to delete old profile of : %s\n", profile->name);
                    }
                }

                T2Debug("Set new profile : %s\n", profile->name);
                if(T2ERROR_SUCCESS != ReportProfiles_setProfileXConf(profile))
                {
                    T2Error("Failed to set profile : %s\n", profile->name);
                }
                else
                {
                    T2Info("Successfully set new profile : %s\n", profile->name);
                    configFetch = T2ERROR_SUCCESS;
                }

                /* Set a cronjob for auto downloading DCMresponse.txt file */
                if (T2ERROR_SUCCESS == ProfileXConf_setCronForAutoDownload())
                {
                    T2Info("cronjob for auto downloading DCMresponse.txt file is set as %s\n", profile->autoDownloadInterval);
                }
                else
                {
                    T2Error("Failed to set cronjob for auto downloading DCMresponse.txt file\n");
                }

                // Touch a file to indicate script based supplementary services to proceed with configuration
                FILE *fp = NULL ;
                fp = fopen(PROCESS_CONFIG_COMPLETE_FLAG, "w+");
                if(fp)
                    fclose(fp);

            }
            if(configData != NULL) {
                free(configData);
                configData = NULL ;
            }
            break;
        }
        else if(ret == T2ERROR_PROFILE_NOT_SET)
        {
            T2Warning("XConf Telemetry profile not set for this device, uninitProfileList.\n");
            if(configData != NULL) {
                free(configData);
                configData = NULL ;
            }
            break;
        }
        else
        {
            if(configData != NULL) {
                free(configData);
                configData = NULL ;
            }
            xConfRetryCount++;
            if(xConfRetryCount >= maxAttempts)
            {
                xConfRetryCount = 0; // xConfRetryCount is global. So reset it.
                T2Error("Reached max xconf retry counts : %d, Using saved profile if exists until next reboot\n", maxAttempts);
                break;
            }
            T2Info("Waiting for %d sec before trying fetchRemoteConfiguration, No.of tries : %d\n", attemptInterval, xConfRetryCount);

            pthread_mutex_lock(&xcMutex);

            memset(&_ts, 0, sizeof(struct timespec));
            memset(&_now, 0, sizeof(struct timespec));
            clock_gettime(CLOCK_REALTIME, &_now);
            _ts.tv_sec = _now.tv_sec + attemptInterval;

            n = pthread_cond_timedwait(&xcCond, &xcMutex, &_ts);
            if(n == ETIMEDOUT)
            {
                T2Info("TIMEDOUT -- trying fetchConfigurations again\n");
            }
            else if (n == 0)
            {
                T2Error("XConfClient Interrupted\n");
            }
            else
            {
                T2Error("ERROR inside startXConfClientThread for timedwait, error code : %d\n", n);
            }
            pthread_mutex_unlock(&xcMutex);
        }
    }  // End of config fetch while
    if(configFetch == T2ERROR_FAILURE && !ProfileXConf_isSet())
    {
        T2Error("Failed to fetch updated configuration and no saved configurations on disk for XCONF, uninitializing  the process\n");
    }

    if(configURL)
        free(configURL);
    stopFetchRemoteConfiguration = true;
    // pthread_detach(pthread_self()); commenting this line as thread will detached by stopXConfClient
    T2Debug("%s --out\n", __FUNCTION__);
    return NULL;
}

void uninitXConfClient()
{
    T2Debug("%s ++in\n", __FUNCTION__);

    if(!stopFetchRemoteConfiguration)
    {
        stopFetchRemoteConfiguration = true;
        T2Info("fetchRemoteConfigurationThread signalled to stop\n");
        pthread_mutex_lock(&xcMutex);
        pthread_cond_signal(&xcCond);
        pthread_mutex_unlock(&xcMutex);

        pthread_join(xcrThread, NULL);

    }
    else
    {
        T2Debug("XConfClientThread is stopped already\n");
    }

    pthread_mutex_destroy(&xcMutex);
    pthread_cond_destroy(&xcCond);
    T2Debug("%s --out\n", __FUNCTION__);
    T2Info("Uninit XConf Client Successful\n");
}

T2ERROR initXConfClient()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    pthread_mutex_init(&xcMutex, NULL);
    pthread_cond_init(&xcCond, NULL);
    isXconfInit = true ;
    startXConfClient();
    T2Debug("%s --out\n", __FUNCTION__);
    T2Info("Init Xconf Client Success\n");
    return T2ERROR_SUCCESS;
}

T2ERROR stopXConfClient()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    pthread_detach(xcrThread);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR startXConfClient()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if (isXconfInit) {
        pthread_create(&xcrThread, NULL, getUpdatedConfigurationThread, NULL);
    } else {
    	T2Info("getUpdatedConfigurationThread is still active ... Ignore xconf reload \n");
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}
