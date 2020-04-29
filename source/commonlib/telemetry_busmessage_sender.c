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


#include <stdbool.h>
#include <unistd.h>
#include "telemetry_busmessage_sender.h"
#include "vector.h"
#include "telemetry2_0.h"
#include "telemetry_busmessage_internal.h"

#define MAX_CACHED_EVENTS_LIMIT 50
#define T2_COMPONENT_READY    "/tmp/.t2ReadyToReceiveEvents"

static void *bus_handle = NULL;
static const char* RFC_T2_ENABLE_PARAM = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable" ;
static bool isRFCT2Enable = false ;
static bool isT2Ready = false;
static bool GetParamStatus = false;

pthread_mutex_t eventMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FileCacheMutex = PTHREAD_MUTEX_INITIALIZER;

T2ERROR getParamValues(char **paramNames, const int paramNamesCount, parameterValStruct_t ***valStructs, int *valSize)
{
    if (paramNames == NULL || paramNamesCount <= 0) {
        EVENT_ERROR("paramNames is NULL or paramNamesCount <= 0 - returning\n");
        return T2ERROR_INVALID_ARGS;
    }

    int ret = CcspBaseIf_getParameterValues(bus_handle, destCompName, destCompPath, paramNames,
                                            paramNamesCount, valSize, valStructs);
    if (ret != CCSP_SUCCESS) {
        EVENT_ERROR("CcspBaseIf_getParameterValues failed for : %s with ret = %d\n", paramNames[0],
                ret);
        return T2ERROR_FAILURE;
    }
    return T2ERROR_SUCCESS;
}

static void freeParamValue(parameterValStruct_t **valStructs, int valSize)
{
    free_parameterValStruct_t(bus_handle, valSize, valStructs);
}

static T2ERROR getParamValue(const char* paramName, char **paramValue)
{
    parameterValStruct_t **valStructs = NULL;
    int valSize = 0;
    char *paramNames[1] = {NULL};
    paramNames[0] = strdup(paramName);
    if(T2ERROR_SUCCESS != getParamValues(paramNames, 1, &valStructs, &valSize))
    {
        EVENT_ERROR("Unable to get %s\n", paramName);
        return T2ERROR_FAILURE;
    }
    *paramValue = strdup(valStructs[0]->parameterValue);
    free(paramNames[0]);
    freeParamValue(valStructs, valSize);
    return T2ERROR_SUCCESS;
}

static int initMessageBus() {

    int ret = 0;
    char* component_id = "com.cisco.spvtg.ccsp.t2commonlib";
    char *pCfg = CCSP_MSG_BUS_CFG;
    ret = CCSP_Message_Bus_Init(component_id, pCfg, &bus_handle,
            Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
    if (ret == -1) {
        EVENT_ERROR("%s:%d, T2:initMessageBus failed\n", __func__, __LINE__);
    }
    return ret;
}

static int cacheEventToFile(char* telemetry_data)
{

        int fd;
        struct flock fl = { F_WRLCK, SEEK_SET, 0,       0,     0 };
        pthread_mutex_lock(&FileCacheMutex);

        if ((fd = open(T2_CACHE_LOCK_FILE, O_RDWR | O_CREAT, 0666)) == -1)
        {
                EVENT_ERROR("%s:%d, T2:open failed\n", __func__, __LINE__);
                pthread_mutex_unlock(&FileCacheMutex);
                return -1;
        }

        if(fcntl(fd, F_SETLKW, &fl) == -1)  /* set the lock */
        {
                EVENT_ERROR("%s:%d, T2:fcntl failed\n", __func__, __LINE__);
                pthread_mutex_unlock(&FileCacheMutex);
                close(fd);
                return -1;
        }

        FILE *fp = fopen(T2_CACHE_FILE, "a");
        if (fp == NULL) {
               EVENT_ERROR("%s: File open error %s\n", __FUNCTION__, T2_CACHE_FILE);
               goto unlock;
        }
        fprintf(fp, "%s\n", telemetry_data);
        fclose(fp);

unlock:

        fl.l_type = F_UNLCK;  /* set to unlock same region */
        if (fcntl(fd, F_SETLK, &fl) == -1) {
                EVENT_ERROR("fcntl failed \n");
        }
        close(fd);
        pthread_mutex_unlock(&FileCacheMutex);
        return 0;
}

static bool
initRFC () {
    char* paramValue = NULL;
    bool status = true ;
    // Check for RFC and proceed - if true - else return now .
    if(!bus_handle)
    {
        if(initMessageBus() != 0)
        {
            EVENT_ERROR("initMessageBus failed\n");
            status = false ;
        }
    }
    if((GetParamStatus == false) && bus_handle)
    {
        if(T2ERROR_SUCCESS == getParamValue(RFC_T2_ENABLE_PARAM, &paramValue))
        {
            if(paramValue != NULL && strncasecmp(paramValue, "true", 4) == 0)
            {
                isRFCT2Enable = true;
            }
            GetParamStatus = true;
            status = true ;
        }
        else {
            status = false;
        }
    }
    return status;
}


static bool isCachingRequired () {

    if(!initRFC())
    {
        EVENT_ERROR("initRFC failed - cache the events\n");
        return true;
    }
    if(isRFCT2Enable && !isT2Ready)
    {
        if( access( T2_COMPONENT_READY, F_OK ) != -1 )
        {
            EVENT_DEBUG("T2 component is ready, flushing the cache\n");
            isT2Ready = true;
            return false;
       }
        else
        {
            EVENT_DEBUG("isRFCT2Enable is true but T2 is not ready to receive events yet\n");
            return true;
        }
    }
    return false;
}

static int send_data_via_telemetrysignal(char* telemetry_data) {
    int ret = 0;
    pthread_mutex_lock(&eventMutex);
    if(isCachingRequired()) {
        EVENT_DEBUG("Caching the event : %s \n", telemetry_data);
        cacheEventToFile(telemetry_data);
        pthread_mutex_unlock(&eventMutex);
        return T2ERROR_SUCCESS ;
    }

    pthread_mutex_unlock(&eventMutex);
    if (isT2Ready) {
        EVENT_DEBUG("T2: Sending event : %s\n", telemetry_data);
        ret = CcspBaseIf_SendTelemetryDataSignal(bus_handle, telemetry_data);
        if (ret != CCSP_SUCCESS) {
            EVENT_ERROR("%s:%d, T2:telemetry data send failed\n", __func__, __LINE__);
        }
    }
    free(telemetry_data);
    telemetry_data = NULL;
    return ret;
}

T2ERROR t2_event_s(char* marker, char* value) {

    int ret = -1;
    T2ERROR retStatus = T2ERROR_FAILURE ;

    pthread_mutex_lock(&sMutex);
    if ( NULL == marker || NULL == value) {
        EVENT_ERROR("%s:%d Error in input parameters \n", __func__, __LINE__);
        pthread_mutex_unlock(&sMutex);
        return T2ERROR_FAILURE;
    }

    // If data is empty should not be sending the empty marker over bus
    if ( 0 == strlen(value) || strcmp(value, "0") == 0 ) {
        pthread_mutex_unlock(&sMutex);
        return T2ERROR_SUCCESS;
    }

    char *buffer = (char*) malloc(MAX_DATA_LEN * sizeof(char));
    if (NULL != buffer) {
        snprintf(buffer, MAX_DATA_LEN, "%s%s%s", marker, MESSAGE_DELIMITER, value);
        ret = send_data_via_telemetrysignal(buffer);
        if (ret != -1) {
            retStatus = T2ERROR_SUCCESS ;
        }
    } else {
        EVENT_ERROR("%s:%d Error unable to allocate memory \n", __func__, __LINE__);
    }
    pthread_mutex_unlock(&sMutex);
    return retStatus;
}

T2ERROR t2_event_f(char* marker, double value) {

    int ret = -1;
    T2ERROR retStatus = T2ERROR_FAILURE ;

     pthread_mutex_lock(&fMutex);
     if ( NULL == marker ) {
         EVENT_ERROR("%s:%d Error in input parameters \n", __func__, __LINE__);
         pthread_mutex_unlock(&fMutex);
         return T2ERROR_FAILURE;
     }

     char *buffer = (char*) malloc(MAX_DATA_LEN * sizeof(char));
     if (NULL != buffer) {
         snprintf(buffer, MAX_DATA_LEN, "%s%s%f", marker, MESSAGE_DELIMITER, value);
         ret = send_data_via_telemetrysignal(buffer);
         if (ret != -1) {
             retStatus = T2ERROR_SUCCESS ;
         }
     } else {
         EVENT_ERROR("%s:%d Error unable to allocate memory \n", __func__, __LINE__);
     }
     pthread_mutex_unlock(&fMutex);
     return retStatus ;
}

T2ERROR t2_event_d(char* marker, int value) {

    int ret = -1;
    T2ERROR retStatus = T2ERROR_FAILURE ;

     pthread_mutex_lock(&dMutex);
     if ( NULL == marker ) {
         EVENT_ERROR("%s:%d Error in input parameters \n", __func__, __LINE__);
         pthread_mutex_unlock(&dMutex);
         return T2ERROR_FAILURE;
     }

     if (value == 0) {  // Requirement from field triage to ignore reporting 0 values
         pthread_mutex_unlock(&dMutex);
         return T2ERROR_SUCCESS;
     }

     char *buffer = (char*) malloc(MAX_DATA_LEN * sizeof(char));
     if (NULL != buffer) {
         snprintf(buffer, MAX_DATA_LEN, "%s%s%d", marker, MESSAGE_DELIMITER, value);
         ret = send_data_via_telemetrysignal(buffer);
         if (ret != -1) {
             retStatus = T2ERROR_SUCCESS ;
         }
     } else {
         EVENT_ERROR("%s:%d Error unable to allocate memory \n", __func__, __LINE__);
     }
     pthread_mutex_unlock(&dMutex);
     return retStatus ;
}
