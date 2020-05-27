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

#include "t2log_wrapper.h"
#include "reportprofiles.h"
#include "profilexconf.h"
#include "xconfclient.h"
#include "t2parserxconf.h"
#include "vector.h"
#include "persistence.h"
#include "telemetry2_0.h"

#define RFC_RETRY_TIMEOUT 60
#define XCONF_RETRY_TIMEOUT 180
#define MAX_XCONF_RETRY_COUNT 5

static const int MAX_URL_LEN = 1024;
static const int MAX_URL_ARG_LEN = 128;
static int xConfRetryCount = 0;
static bool stopFetchRemoteConfiguration = false;
static bool isXconfInit = false ;
static bool fetchRemoteConfigComplete = false;

static pthread_t xcrThread;
static pthread_mutex_t xcMutex;
static pthread_cond_t xcCond;

static T2ERROR getBuildType(char* buildType) {
    char fileContent[255] = { '\0' };
    FILE *deviceFilePtr;
    char *pBldTypeStr = NULL;
    int offsetValue = 0;
    deviceFilePtr = fopen( DEVICE_PROPERTIES, "r");

    if (NULL == buildType) {
       return T2ERROR_FAILURE;
    }

    if (deviceFilePtr) {
        while (fscanf(deviceFilePtr, "%s", fileContent) != EOF) {
            if ((pBldTypeStr = strstr(fileContent, "BUILD_TYPE")) != NULL) {
                offsetValue = strlen("BUILD_TYPE=");
                pBldTypeStr = pBldTypeStr + offsetValue;
                break;
            }
        }
        fclose(deviceFilePtr);
    }
    strncpy(buildType, pBldTypeStr, BUILD_TYPE_MAX_LENGTH - 1);
    return T2ERROR_SUCCESS;
}

static T2ERROR appendRequestParams(char *buf, const int maxArgLen) {

    T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);

    int avaBufSize = maxArgLen, write_size = 0;
    char *paramVal = NULL;
    char *tempBuf = (char*) malloc(MAX_URL_ARG_LEN);

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

    char build_type[BUILD_TYPE_MAX_LENGTH] = { 0 };
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
        T2Error("%s:%d, T2:memory realloc failed\n", __func__, __LINE__);
        return 0;
    }
    httpResponse->data = ptr;
    memcpy(&(httpResponse->data[httpResponse->size]), response, realsize);
    httpResponse->size += realsize;
    httpResponse->data[httpResponse->size] = 0;

    return realsize;
}

static T2ERROR doHttpGet(char* httpsUrl, char **data) {

    T2Debug("%s ++in\n", __FUNCTION__);

    T2Info("%s with url %s \n", __FUNCTION__, httpsUrl);
    CURL *curl;
    long http_code = 0;
    long curl_code = 0;

    if (NULL == httpsUrl) {
        T2Error("NULL httpsUrl given, doHttpGet failed\n");
        return -1;
    }
    curlResponseData* httpResponse = (curlResponseData *)malloc(sizeof(curlResponseData));
    httpResponse->data = malloc(1);
    httpResponse->size = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, httpsUrl);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpGetCallBack);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * ) httpResponse);

        //TODO - Set interface and addr type
        //TODO - Introduce retry
        //TODO - configparamgen C APIs

        curl_code = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (http_code == 200 && curl_code == CURLE_OK) {
            T2Info("%s:%d, T2:Telemetry XCONF communication success\n", __func__,
                    __LINE__);
            *data = httpResponse->data;
            free(httpResponse);
        } else {
            T2Error("%s:%d, T2:Telemetry XCONF communication Failed with http code : %ld Curl code : %ld \n", __func__,
                    __LINE__, http_code, curl_code);
            if(http_code == 404)
                return T2ERROR_PROFILE_NOT_SET;
            else
                return T2ERROR_FAILURE;
        }
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
        urlWithParams = NULL;
    }
    else
    {
        T2Error("Malloc failed\n");
    }
    return ret;
}

static T2ERROR getRemoteConfigURL(char **configURL) {

    T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);

    char *paramVal = NULL;
    if (T2ERROR_SUCCESS == getParameterValue(TR181_CONFIG_URL, &paramVal)) {
        if (NULL != paramVal) {
            if ((strlen(paramVal) > 8) && (0 == strncmp(paramVal,"https://", 8))) {  // Enforcing https for new endpoints
                T2Info("Setting config URL base location to : %s\n", paramVal);
                *configURL = paramVal;
                ret = T2ERROR_SUCCESS ;
            } else {
                T2Error("URL doesn't start with https or is invalid !!! URL value received : %s .\n", paramVal);
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

static void getUpdatedConfigurationThread(void *data)
{
    T2ERROR configFetch = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);
    struct timespec _ts;
    struct timespec _now;
    int n;
    char *configURL = NULL;
    char *configData = NULL;
    stopFetchRemoteConfiguration = false ;

    while(!stopFetchRemoteConfiguration && T2ERROR_SUCCESS != getRemoteConfigURL(&configURL))
    {
        pthread_mutex_lock(&xcMutex);

        memset(&_ts, 0, sizeof(struct timespec));
        memset(&_now, 0, sizeof(struct timespec));
        clock_gettime(CLOCK_REALTIME, &_now);
        _ts.tv_sec = _now.tv_sec + RFC_RETRY_TIMEOUT;

        T2Info("Waiting for %d sec before trying getRemoteConfigURL\n", RFC_RETRY_TIMEOUT);
        n = pthread_cond_timedwait(&xcCond, &xcMutex, &_ts);
        if(n == ETIMEDOUT)
        {
            T2Info("TIMEDOUT -- trying fetchConfigURLs again\n");
        }
        else if (n == 0)
        {
            T2Error("XConfClient Interrupted\n");
        }
        else
        {
            T2Error("ERROR inside startXConfClientThread for timedwait");
        }
        pthread_mutex_unlock(&xcMutex);
    }

    while(!stopFetchRemoteConfiguration)
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
                    if(T2ERROR_SUCCESS != saveConfigToFile(XCONFPROFILE_PERSISTENCE_PATH, profile->name, configData))
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

                    if(T2ERROR_SUCCESS != saveConfigToFile(XCONFPROFILE_PERSISTENCE_PATH, profile->name, configData)) // Should be removed once XCONF sends new UUID for each update.
                    {
                        T2Error("Unable to update an existing config file : %s\n", profile->name);
                    }
                    T2Debug("Disable and Delete old profile %s\n", profile->name);
                    if(T2ERROR_SUCCESS != ReportProfiles_deleteProfileXConf(profile->cachedReportList))
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

            }
            break;
        }
        else if(ret == T2ERROR_PROFILE_NOT_SET)
        {
            T2Warning("XConf Telemetry profile not set for this device, uninitProfileList.\n");
            break;
        }
        else
        {
            xConfRetryCount++;
            if(xConfRetryCount == MAX_XCONF_RETRY_COUNT)
            {
                T2Error("Reached max xconf retry counts : %d, Using saved profile if exists until next reboot\n", MAX_XCONF_RETRY_COUNT);
                break;
            }
            T2Info("Waiting for %d sec before trying fetchRemoteConfiguration, No.of tries : %d\n", XCONF_RETRY_TIMEOUT, xConfRetryCount);

            pthread_mutex_lock(&xcMutex);

            memset(&_ts, 0, sizeof(struct timespec));
            memset(&_now, 0, sizeof(struct timespec));
            clock_gettime(CLOCK_REALTIME, &_now);
            _ts.tv_sec = _now.tv_sec + XCONF_RETRY_TIMEOUT;

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
    stopFetchRemoteConfiguration = true;
    T2Debug("%s --out\n", __FUNCTION__);
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
