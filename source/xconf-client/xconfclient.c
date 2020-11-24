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

#include "t2log_wrapper.h"
#include "reportprofiles.h"
#include "profilexconf.h"
#include "xconfclient.h"
#include "t2parserxconf.h"
#include "vector.h"
#include "persistence.h"
#include "telemetry2_0.h"
#include "curlinterface.h"
#include "syscfg/syscfg.h"

#define RFC_RETRY_TIMEOUT 60
#define XCONF_RETRY_TIMEOUT 180
#define MAX_XCONF_RETRY_COUNT 5
#define IFINTERFACE      "erouter0"
#define XCONF_CONFIG_FILE  "DCMresponse.txt"
#define DEFAULT_CONFIG_FILE  "/usr/ccsp/telemetry/DCMresponse.txt"

static const int MAX_URL_LEN = 1024;
static const int MAX_URL_ARG_LEN = 128;
static int xConfRetryCount = 0;
static bool stopFetchRemoteConfiguration = false;
static bool isXconfInit = false ;

static pthread_t xcrThread;
static pthread_mutex_t xcMutex;
static pthread_cond_t xcCond;

typedef enum _IFADDRESS_TYPE
{
    ADDR_UNKNOWN,
    ADDR_IPV4,
    ADDR_IPV6
}IFADDRESS_TYPE;

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
            "controllerId=2504&channelMapId=2345&vodId=15660&version=2",
            avaBufSize);

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
    CURLcode code=CURLE_OK;
    long http_code = 0;
    long curl_code = 0;

    char buf[256];
    char current_time[20];
    int Timestamp_Status;
    char errbuf[CURL_ERROR_SIZE];

    if (NULL == httpsUrl) {
        T2Error("NULL httpsUrl given, doHttpGet failed\n");
        return T2ERROR_FAILURE;
    }
    curlResponseData* httpResponse = (curlResponseData *)malloc(sizeof(curlResponseData));
    httpResponse->data = malloc(1);
    httpResponse->size = 0;

    curl = curl_easy_init();

    if (curl) {
        code = curl_easy_setopt(curl, CURLOPT_URL, httpsUrl);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        code = curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        code = curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpGetCallBack);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * ) httpResponse);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        code = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }

        /* Load server ca, device certificate and private key to curl object */
        addCertificatesToHTTPHeader(curl);

        // Set interface and addr type
 /* For now, Let curl start hopping between v4/v6 address like it is there for legacy dca till STBIT-1511 gets resolved.*/
 /*

    if(getAddressType(IFINTERFACE) == ADDR_UNKNOWN)
      {
          T2Error("doHttpGet:: Unknown Address Type - returning failure\n");
          return T2ERROR_FAILURE;
     }
     else if(getAddressType(IFINTERFACE) == ADDR_IPV4)
         curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
     else
         curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
 */

        code = curl_easy_setopt(curl, CURLOPT_INTERFACE, IFINTERFACE);
        if(code != CURLE_OK){
           T2Error("%s : Curl set opts failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
        }
        //TODO - Introduce retry
        //TODO - configparamgen C APIs

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

        if (http_code == 200 && curl_code == CURLE_OK) {
            T2Info("%s:%d, T2:Telemetry XCONF communication success\n", __func__,
                    __LINE__);
            *data = strdup(httpResponse->data);

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

            free(httpResponse->data);
            free(httpResponse);
            curl_easy_cleanup(curl);
        } else {
            T2Error("%s:%d, T2:Telemetry XCONF communication Failed with http code : %ld Curl code : %ld \n", __func__,
                    __LINE__, http_code, curl_code);
            T2Error("%s : curl_easy_perform failed with error message %s from curl \n", __FUNCTION__, curl_easy_strerror(curl_code));
            free(httpResponse->data);
            free(httpResponse);
            curl_easy_cleanup(curl);
            if(http_code == 404)
                return T2ERROR_PROFILE_NOT_SET;
            else
                return T2ERROR_FAILURE;
        }
    }
    else
    {
	free(httpResponse->data);
    	free(httpResponse);
	return T2ERROR_FAILURE;
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
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

static T2ERROR loadDefaultConfFile (void)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    T2ERROR res = T2ERROR_FAILURE;
    char *configData = NULL;
    long fsize = 0;
    size_t bytes_count = 0;
    ProfileXConf *profile = 0;

    FILE *fptr = fopen(DEFAULT_CONFIG_FILE, "r");
    if(fptr)
    {
        fseek(fptr, 0, SEEK_END);
        fsize = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);
        configData = malloc(fsize + 1);

        if (configData)
        {
            bytes_count = fread(configData, 1, fsize, fptr);
            if (bytes_count != 0)
            {
                configData[fsize] = '\0';
                if(T2ERROR_SUCCESS == processConfigurationXConf(configData, &profile))
                {
                    if(T2ERROR_SUCCESS != saveConfigToFile(XCONFPROFILE_PERSISTENCE_PATH, XCONF_CONFIG_FILE, configData))
                    {
                        T2Error("Unable to save the default profile : %s to disk\n", profile->name);
                    }
                    else
                    {
                        T2Info("Successfully saved the default profile to nvram: %s\n", profile->name);
                    }

                    T2Debug("Set default profile : %s\n", profile->name);
                    if(T2ERROR_SUCCESS != ReportProfiles_setProfileXConf(profile))
                    {
                        T2Error("Failed to set default profile : %s\n", profile->name);
                    }
                    else
                    {
                        /* Set a cronjob for auto downloading DCMresponse.txt file */
                        if (T2ERROR_SUCCESS == ProfileXConf_setCronForAutoDownload())
                        {
                            T2Info("cronjob for auto downloading DCMresponse.txt file is set as %s\n", profile->autoDownloadInterval);
                        }
                        else
                        {
                            T2Error("Failed to set cronjob for auto downloading DCMresponse.txt file\n");
                        }
                        res = T2ERROR_SUCCESS;
                        T2Info("Successfully set default profile : %s\n", profile->name);
                    }
                }
            }

            free(configData);
        }

        fclose(fptr);
    }

    T2Debug("%s --out\n", __FUNCTION__);
    return res;
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
        T2Warning("Failed to fetch updated configuration and no saved configurations on disk for XCONF, Using the default configuration file from disk.\n");
        if(loadDefaultConfFile() != T2ERROR_SUCCESS)
        {
            T2Error("ERROR: Unable to load the default config file!!!\n");
        }
    }

    if(configURL)
        free(configURL);
    stopFetchRemoteConfiguration = true;
    pthread_detach(pthread_self());
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
