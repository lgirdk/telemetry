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
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <ccsp/ccsp_memory.h>
#include <ccsp/ccsp_custom.h>
#include <ccsp/ccsp_base_api.h>
#include <rbus/rbus.h>

#include "telemetry_busmessage_sender.h"

#include "t2collection.h"
#include "vector.h"
#include "telemetry2_0.h"
#include "telemetry_busmessage_internal.h"

#define MAX_CACHED_EVENTS_LIMIT 50
#define T2_COMPONENT_READY    "/tmp/.t2ReadyToReceiveEvents"
#define T2_SCRIPT_EVENT_COMPONENT "telemetry_client"

static const char* CCSP_FIXED_COMP_ID = "com.cisco.spvtg.ccsp.t2commonlib" ;
static char *componentName = NULL;
static void *bus_handle = NULL;
static const char* RFC_T2_ENABLE_PARAM = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable" ;
static bool isRFCT2Enable = false ;
static bool isT2Ready = false;
static bool getParamStatus = false;
static bool isRbusEnabled = false ;

static hash_map_t *eventMarkerMap = NULL;

pthread_mutex_t eventMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FileCacheMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t markerListMutex = PTHREAD_MUTEX_INITIALIZER;

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

static T2ERROR getCCSPParamVal(const char* paramName, char **paramValue)
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


static void rBusInterface_Uninit( ) {
    rbus_close(bus_handle);
}

static T2ERROR initMessageBus( ) {
    // EVENT_DEBUG("%s ++in\n", __FUNCTION__);
    T2ERROR status = T2ERROR_SUCCESS;
    char* component_id = CCSP_FIXED_COMP_ID;
    char *pCfg = CCSP_MSG_BUS_CFG;
    char *rbusSettings = NULL;

    if(RBUS_ENABLED == rbus_checkStatus()) {
        // EVENT_DEBUG("%s:%d, T2:rbus is enabled\n", __func__, __LINE__);
        char commonLibName[124] = { '\0' };
        // Bus handles should be unique across the system
        if(componentName) {
            snprintf(commonLibName, 124, "%s%s", "t2_lib_", componentName);
        }else {
            snprintf(commonLibName, 124, "%s", component_id);
        }
        status = rbus_open((rbusHandle_t*) &bus_handle, commonLibName);
        if(status != RBUS_ERROR_SUCCESS) {
            EVENT_ERROR("%s:%d, init using component name %s failed with error code %d \n", __func__, __LINE__, commonLibName, status);
            status = T2ERROR_FAILURE;
        }
        isRbusEnabled = true;
    } else {
        int ret = 0 ;
        ret = CCSP_Message_Bus_Init(component_id, pCfg, &bus_handle, Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
        if(ret == -1) {
            EVENT_ERROR("%s:%d, T2:initMessageBus failed\n", __func__, __LINE__);
            status = T2ERROR_FAILURE ;
        } else {
            status = T2ERROR_SUCCESS ;
        }
    }
    // EVENT_DEBUG("%s --out\n", __FUNCTION__);
    return status;
}

static T2ERROR getRbusParameterVal(const char* paramName, char **paramValue) {

    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusValue_t paramValue_t;
    rbusValueType_t rbusValueType ;
    char *stringValue = NULL;
    rbusSetOptions_t opts;
    opts.commit = true;

    if(!bus_handle && T2ERROR_SUCCESS != initMessageBus()) {
        return T2ERROR_FAILURE;
    }

    ret = rbus_get(bus_handle, paramName, &paramValue_t);
    if(ret != RBUS_ERROR_SUCCESS) {
        EVENT_ERROR("Unable to get %s\n", paramName);
        return T2ERROR_FAILURE;
    }
    rbusValueType = rbusValue_GetType(paramValue_t);
    if(rbusValueType == RBUS_BOOLEAN) {
        if (rbusValue_GetBoolean(paramValue_t)){
            stringValue = strdup("true");
        } else {
            stringValue = strdup("false");
        }
    } else {
        stringValue = rbusValue_ToString(paramValue_t, NULL, 0);
    }
    EVENT_DEBUG("%s = %s\n", paramName, stringValue);
    *paramValue = stringValue;
    rbusValue_Release(paramValue_t);

    return T2ERROR_SUCCESS;
}

T2ERROR getParamValue(const char* paramName, char **paramValue)
{
    T2ERROR ret = T2ERROR_FAILURE ;
    if(isRbusEnabled)
        ret = getRbusParameterVal(paramName,paramValue);
    else
        ret = getCCSPParamVal(paramName, paramValue);

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

static bool initRFC( ) {
    char* paramValue = NULL;
    bool status = true ;
    // Check for RFC and proceed - if true - else return now .
    if(!bus_handle) {
        if(initMessageBus() != 0) {
            EVENT_ERROR("initMessageBus failed\n");
            status = false ;
        }
    }
    if((getParamStatus == false) && bus_handle) {
        if(T2ERROR_SUCCESS == getParamValue(RFC_T2_ENABLE_PARAM, &paramValue)) {
            if(paramValue != NULL && (strncasecmp(paramValue, "true", 4) == 0)) {
                isRFCT2Enable = true;
            }
            getParamStatus = true;
            status = true;
            free(paramValue);
        }else {
            status = false;
        }
    }

    return status;
}

/**
 * In rbus mode, should be using rbus subscribed param
 * from telemetry 2.0 instead of direct api for event sending
 */
int filtered_event_send(const char* data, char *markerName) {
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    int status = 0 ;
    // EVENT_DEBUG("%s ++in\n", __FUNCTION__);
    if(!bus_handle) {
        EVENT_DEBUG("bus_handle is null .. exiting !!! \n");
        return ret;
    }

    if(isRbusEnabled) {
        // Filter data from marker list
        pthread_mutex_lock(&markerListMutex);
        if(componentName && (0 != strcmp(componentName, T2_SCRIPT_EVENT_COMPONENT))) { // Events from scripts needs to be sent without filtering
            bool isEventingEnabled = false;
            if(markerName && eventMarkerMap) {
                if(hash_map_get(eventMarkerMap, markerName)) {
                    isEventingEnabled = true;
                }
            }
            if(!isEventingEnabled) {
                EVENT_DEBUG("rbus mode : marker %s not present in list, ignore sending \n", markerName);
                pthread_mutex_unlock(&markerListMutex);
                return status;
            }
        }else {
            EVENT_DEBUG("rbus mode : Eventing for %s from %s. Send events without filtering.\n", markerName, componentName);
        }
        pthread_mutex_unlock(&markerListMutex);
        // End of event filtering

        rbusProperty_t objProperty = NULL ;
        rbusValue_t objVal, value;
        rbusSetOptions_t options;
        options.commit = true;

        rbusValue_Init(&objVal);
        rbusValue_SetString(objVal, data);
        rbusProperty_Init(&objProperty, markerName, objVal);

        rbusValue_Init(&value);
        rbusValue_SetProperty(value, objProperty);

        // EVENT_DEBUG("rbus_set with param [%s] with value [%s]\n", T2_EVENT_PARAM, data);
        ret = rbus_set(bus_handle, T2_EVENT_PARAM, value, &options);
        if(ret != RBUS_ERROR_SUCCESS) {
            EVENT_ERROR("rbus_set Failed for [%s] with error [%d]\n", T2_EVENT_PARAM, ret);
            status = -1 ;
        }else {
            status = 0 ;
        }
        // Release all rbus data structures
        rbusValue_Release(value);
        rbusProperty_Release(objProperty);
        rbusValue_Release(objVal);

    } else {
        int markerNameLen = strlen(markerName);
        int dataLen = strlen(data);
        int eventDataLen = strlen(markerName) + strlen(data) + DELIMITER_LEN + 1;
        char* buffer = (char*) malloc(eventDataLen * sizeof(char));
        if(buffer) {
            snprintf(buffer, eventDataLen, "%s%s%s", markerName, MESSAGE_DELIMITER, data);
            ret = CcspBaseIf_SendTelemetryDataSignal(bus_handle, buffer);
            if(ret != CCSP_SUCCESS)
                status = -1;
            free(buffer);
        }else {
            EVENT_ERROR("Unable to allocate meory for event [%s]\n", markerName);
            status = -1 ;
        }
    }
    //EVENT_DEBUG("%s --out with status %d \n", __FUNCTION__, status);
    return status;
}

/**
 * Receives an rbus object as value which conatins a list of rbusPropertyObject
 * rbusProperty name will the eventName and value will be null
 */
static T2ERROR doPopulateEventMarkerList( ) {
    // EVENT_DEBUG("%s ++in\n", __FUNCTION__);

    T2ERROR status = T2ERROR_SUCCESS;
    char deNameSpace[1][124] = { '\0' };
    char *param = NULL;
    if(!isRbusEnabled)
        return T2ERROR_SUCCESS;

    int numProperties = 0;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusValue_t paramValue_t;

    if(!bus_handle && T2ERROR_SUCCESS != initMessageBus()) {
        EVENT_ERROR("Unable to get message bus handles \n");
        //EVENT_DEBUG("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    snprintf(deNameSpace[0], 124, "%s%s%s", T2_ROOT_PARAMETER, componentName, T2_EVENT_LIST_PARAM_SUFFIX);
    EVENT_DEBUG("rbus mode : Query marker list with data element = %s \n", deNameSpace[0]);

    pthread_mutex_lock(&markerListMutex);
    // EVENT_DEBUG("Clean up eventMarkerMap \n");
    if(eventMarkerMap != NULL)
        hash_map_destroy(eventMarkerMap, free);


    ret = rbus_get(bus_handle, deNameSpace[0], &paramValue_t);
    if(ret != RBUS_ERROR_SUCCESS) {
        EVENT_ERROR("rbus mode : No event list configured in profiles %s\n", deNameSpace[0]);
        pthread_mutex_unlock(&markerListMutex);
        return T2ERROR_SUCCESS;
    }

    rbusValueType_t type_t = rbusValue_GetType(paramValue_t);
    if(type_t != RBUS_OBJECT) {
        EVENT_ERROR("rbus mode : Unexpected data object received for %s get query \n", deNameSpace[0]);
        rbusValue_Release(paramValue_t);
        pthread_mutex_unlock(&markerListMutex);
        return T2ERROR_FAILURE;
    }

    rbusObject_t objectValue = rbusValue_GetObject(paramValue_t);
    if(objectValue) {
        eventMarkerMap = hash_map_create();
        rbusProperty_t rbusPropertyList = rbusObject_GetProperties(objectValue);
        EVENT_DEBUG("\t rbus mode :  Update event map for component %s with below events : \n", componentName);
        while(NULL != rbusPropertyList) {
            char* eventname = rbusProperty_GetName(rbusPropertyList);
            if(eventname && strlen(eventname) > 0) {
                EVENT_DEBUG("\t %s \n", eventname);
                hash_map_put(eventMarkerMap, (void*) strdup(eventname), (void*) strdup(eventname));
            }
            rbusPropertyList = rbusProperty_GetNext(rbusPropertyList);
        }
    }else {
        EVENT_DEBUG("rbus mode : No configured event markers for %s \n", componentName);
    }
    pthread_mutex_unlock(&markerListMutex);
    rbusValue_Release(paramValue_t);
    EVENT_DEBUG("%s --out\n", __FUNCTION__);
    return status;

}

static void rbusEventReceiveHandler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription) {
    EVENT_DEBUG("%s ++in\n", __FUNCTION__);
    char* eventName = event->name;
    if(eventName) {
        if(0 == strcmp(eventName, T2_PROFILE_UPDATED_NOTIFY))
            doPopulateEventMarkerList();
    }else {
        EVENT_DEBUG("eventName is null \n");
    }
    EVENT_DEBUG("%s --out\n", __FUNCTION__);
}

static bool isCachingRequired( ) {

    if(!initRFC()) {
        EVENT_ERROR("initRFC failed - cache the events\n");
        return true;
    }

    if(isRFCT2Enable && !isT2Ready) {
        if(access( T2_COMPONENT_READY, F_OK) != -1) {
            EVENT_DEBUG("T2 component is ready, flushing the cache\n");
            isT2Ready = true;
            if(isRbusEnabled) {
                rbusError_t ret = RBUS_ERROR_SUCCESS;
                if(componentName && (0 != strcmp(componentName, "telemetry_client"))) {
                    doPopulateEventMarkerList();
                }
                ret = rbusEvent_Subscribe(bus_handle, T2_PROFILE_UPDATED_NOTIFY, rbusEventReceiveHandler, "T2Event");
                if(ret != RBUS_ERROR_SUCCESS) {
                    EVENT_DEBUG("Unable to subscribe to event %s with rbus error code : %d\n", T2_PROFILE_UPDATED_NOTIFY, ret);
                }
            }
            return false;
        }else {
            EVENT_DEBUG("isRFCT2Enable is true but T2 is not ready to receive events yet\n");
            return true;
        }
    }
    return false;
}

static int report_or_cache_data(char* telemetry_data, char* markerName) {
    int ret = 0;
    pthread_mutex_lock(&eventMutex);
    if(isCachingRequired()) {
        EVENT_DEBUG("Caching the event : %s \n", telemetry_data);
        int markerNameLen = strlen(markerName);
        int dataLen = strlen(telemetry_data);
        int eventDataLen = strlen(markerName) + strlen(telemetry_data) + DELIMITER_LEN + 1;
        char* buffer = (char*) malloc(eventDataLen * sizeof(char));
        if(buffer) {
            // Caching format needs to be same for operation between rbus/dbus modes across reboots
            snprintf(buffer, eventDataLen, "%s%s%s", markerName, MESSAGE_DELIMITER, telemetry_data);
            cacheEventToFile(buffer);
            free(buffer);
        }
        pthread_mutex_unlock(&eventMutex);
        return T2ERROR_SUCCESS ;
    }
    pthread_mutex_unlock(&eventMutex);

    if(isT2Ready) {
        // EVENT_DEBUG("T2: Sending event : %s\n", telemetry_data);
        ret = filtered_event_send(telemetry_data, markerName);
        if(0 != ret) {
            EVENT_ERROR("%s:%d, T2:telemetry data send failed, status = %d \n", __func__, __LINE__, ret);
        }
    }
    return ret;
}

/**
 * Initialize the component name with unique name
 */
void t2_init(char *component) {
    componentName = strdup(component);
}

void t2_uninit(void) {
    if(componentName) {
        free(componentName);
        componentName = NULL ;
    }

    if(isRbusEnabled)
        rBusInterface_Uninit();
}

T2ERROR t2_event_s(char* marker, char* value) {

    int ret;
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
    ret = report_or_cache_data(value, marker);
    if(ret != -1) {
        retStatus = T2ERROR_SUCCESS;
    }
    pthread_mutex_unlock(&sMutex);
    return retStatus;
}

T2ERROR t2_event_f(char* marker, double value) {

    int ret;
    T2ERROR retStatus = T2ERROR_FAILURE ;

     pthread_mutex_lock(&fMutex);
     if ( NULL == marker ) {
         EVENT_ERROR("%s:%d Error in input parameters \n", __func__, __LINE__);
         pthread_mutex_unlock(&fMutex);
         return T2ERROR_FAILURE;
     }

     char *buffer = (char*) malloc(MAX_DATA_LEN * sizeof(char));
     if (NULL != buffer) {
         snprintf(buffer, MAX_DATA_LEN, "%f", value);
         ret = report_or_cache_data(buffer, marker);
         if (ret != -1) {
             retStatus = T2ERROR_SUCCESS ;
         }
         free(buffer);
     } else {
         EVENT_ERROR("%s:%d Error unable to allocate memory \n", __func__, __LINE__);
     }
     pthread_mutex_unlock(&fMutex);
     return retStatus ;
}

T2ERROR t2_event_d(char* marker, int value) {

    int ret;
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
         snprintf(buffer, MAX_DATA_LEN, "%d", value);
         ret = report_or_cache_data(buffer, marker);
         if (ret != -1) {
             retStatus = T2ERROR_SUCCESS ;
         }
         free(buffer);
     } else {
         EVENT_ERROR("%s:%d Error unable to allocate memory \n", __func__, __LINE__);
     }
     pthread_mutex_unlock(&dMutex);
     return retStatus ;
}
