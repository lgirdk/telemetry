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
#include <string.h>
#include <stdlib.h>

#include <ansc_platform.h>

#include "t2collection.h"
#include "t2common.h"
#include "busInterface.h"
#include "rbusInterface.h"
#include "telemetry2_0.h"
#include "t2log_wrapper.h"
#include "profile.h"

#define buffLen 1024
#define maxParamLen 128

#define NUM_PROFILE_ELEMENTS 2

#define RBUS_METHOD_TIMEOUT 10

static rbusHandle_t t2bus_handle;
static TelemetryEventCallback eventCallBack;
static T2EventMarkerListCallback getMarkerListCallBack;
static dataModelCallBack dmProcessingCallBack;
static dataModelMsgPckCallBack dmMsgPckProcessingCallBack = NULL;
static dataModelSavedJsonCallBack dmSavedJsonProcessingCallBack;
static dataModelSavedMsgPackCallBack dmSavedMsgPackProcessingCallBack;
static hash_map_t *compTr181ParamMap = NULL;

static char* reportProfileVal = NULL ;
static char* reportProfilemsgPckVal = NULL ;
T2ERROR T2RbusConsumer(TriggerCondition *triggerCondition);

bool isRbusInitialized( ) {

    return t2bus_handle != NULL ? true : false;
}

void logHandler(
    rbusLogLevel level,
    const char* file,
    int line,
    int threadId,
    char* message)
{
  switch (level)
    {
        case RBUS_LOG_FATAL:
            T2Error("%s:%d %s\n", file, line, message);
            break;
        case RBUS_LOG_ERROR:
            T2Error("%s:%d %s\n", file, line, message);
            break;
        case RBUS_LOG_WARN:
            T2Warning("%s:%d %s\n", file, line, message);
            break;
        case RBUS_LOG_INFO:
            T2Info("%s:%d %s\n", file, line, message);
            break;
        case RBUS_LOG_DEBUG:
            T2Debug("%s:%d %s\n", file, line, message);
            break;
    }
    return;
}

static T2ERROR rBusInterface_Init( ) {
    T2Debug("%s ++in\n", __FUNCTION__);

    int ret = RBUS_ERROR_SUCCESS;   
  
    //rbus_setLogLevel(RBUS_LOG_DEBUG);
    rbus_registerLogHandler(logHandler);
  
    ret = rbus_open(&t2bus_handle, COMPONENT_NAME);
    if(ret != RBUS_ERROR_SUCCESS) {
        T2Error("%s:%d, init failed with error code %d \n", __func__, __LINE__, ret);
        return T2ERROR_FAILURE;
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static void rBusInterface_Uninit( ) {
    rbus_close(t2bus_handle);
}

T2ERROR getRbusParameterVal(const char* paramName, char **paramValue) {
    T2Debug("%s ++in \n", __FUNCTION__);
    
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusValue_t paramValue_t;
    rbusValueType_t rbusValueType ;
    char *stringValue = NULL;
#if 0
    rbusSetOptions_t opts;
    opts.commit = true;
#endif

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        return T2ERROR_FAILURE;
    }

    ret = rbus_get(t2bus_handle, paramName, &paramValue_t);
    if(ret != RBUS_ERROR_SUCCESS) {
        T2Error("Unable to get %s\n", paramName);
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

    #if defined(ENABLE_RDKV_SUPPORT)
    // Workaround as video platforms doesn't have a TR param which gives Firmware name
    // Existing dashboards doesn't like version with file name exentsion
    // Workaround stays until initiative for unified new TR param for version name gets implemented across board
    if(0 == strncmp(paramName, "Device.DeviceInfo.X_COMCAST-COM_FirmwareFilename" , maxParamLen )){
        char* temp = NULL ;
        temp = strstr(stringValue, "-signed.bin");
        if(!temp){
            temp = strstr(stringValue, ".bin");
        }
        if(temp)
            *temp = '\0';
    }
    #endif

    T2Debug("%s = %s\n", paramName, stringValue);
    *paramValue = stringValue;
    rbusValue_Release(paramValue_t);
    T2Debug("%s --out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

Vector* getRbusProfileParamValues(Vector *paramList) {
    T2Debug("%s ++in\n", __FUNCTION__);
    unsigned int i = 0;
    Vector *profileValueList = NULL;
    Vector_Create(&profileValueList);

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
    	Vector_Destroy(profileValueList, free);
        return NULL ;
    }
    char** paramNames = (char **) malloc(paramList->count * sizeof(char*));

    T2Debug("TR-181 Param count : %lu\n", (unsigned long)paramList->count);
    for( ; i < paramList->count; i++ ) { // Loop through paramlist from profile

        tr181ValStruct_t **paramValues = NULL;
        rbusProperty_t rbusPropertyValues = NULL;
        int paramValCount = 0;
        int iterate = 0;
        profileValues* profVals = (profileValues *) malloc(sizeof(profileValues));
        char *param = (char*)((Param *) Vector_At(paramList, i))->alias ;
        paramNames[0] = strdup(param);
        T2Debug("Calling rbus_getExt for %s \n", paramNames[0]);
        if(RBUS_ERROR_SUCCESS != rbus_getExt(t2bus_handle, 1, (const char**)paramNames, &paramValCount, &rbusPropertyValues)) {
            T2Error("Failed to retrieve param : %s\n", paramNames[0]);
            paramValCount = 0 ;
        } else {
            if(rbusPropertyValues == NULL || paramValCount == 0) {
                T2Info("ParameterName : %s Retrieved value count : %d\n", paramNames[0], paramValCount);
            }
        }

        profVals->paramValueCount = paramValCount;

        T2Debug("Received %d parameters for %s fetch \n", paramValCount, paramNames[0]);

        // Populate bus independent parameter value array
        if(paramValCount == 0) {
            paramValues = (tr181ValStruct_t**) malloc(sizeof(tr181ValStruct_t*));
            if(paramValues != NULL) {
                paramValues[0] = (tr181ValStruct_t*) malloc(sizeof(tr181ValStruct_t));
                paramValues[0]->parameterName = strdup(param);
                paramValues[0]->parameterValue = strdup("NULL");
                // If parameter doesn't exist in device we do populate with entry as NULL.
                // So count of populated data list has 1 entry and is not 0
                profVals->paramValueCount = 1;
            }
	    if(rbusPropertyValues != NULL){
	          rbusProperty_Release(rbusPropertyValues);
	    }
        }else {
            paramValues = (tr181ValStruct_t**) malloc(paramValCount * sizeof(tr181ValStruct_t*));
            if(paramValues != NULL) {
                rbusProperty_t nextProperty = rbusPropertyValues;
                for( iterate = 0; iterate < paramValCount; ++iterate ) { // Loop through values obtained from query for individual param in list
                    if(nextProperty) {
                        char* stringValue = NULL;
                        rbusValue_t value = rbusProperty_GetValue(nextProperty);
                        paramValues[iterate] = (tr181ValStruct_t*) malloc(sizeof(tr181ValStruct_t));
                        if(paramValues[iterate]) {
                            stringValue = (char*)rbusProperty_GetName(nextProperty);
                            paramValues[iterate]->parameterName = strdup(stringValue);

                            rbusValueType_t rbusValueType = rbusValue_GetType(value);
                            if(rbusValueType == RBUS_BOOLEAN) {
                                if(rbusValue_GetBoolean(value)) {
                                    paramValues[iterate]->parameterValue = strdup("true");
                                }else {
                                    paramValues[iterate]->parameterValue = strdup("false");
                                }
                            }else {
                                paramValues[iterate]->parameterValue = rbusValue_ToString(value, NULL, 0);
                            }

                            #if defined(ENABLE_RDKV_SUPPORT)
                            // Workaround as video platforms doesn't have a TR param which gives Firmware name
                            // Existing dashboards doesn't like version with file name exentsion
                            // Workaround stays until initiative for unified new TR param for version name gets implemented across board
                            if(0 == strncmp(stringValue, "Device.DeviceInfo.X_COMCAST-COM_FirmwareFilename" , maxParamLen )) {
                                char* temp = NULL;
                                temp = strstr(paramValues[iterate]->parameterValue, "-signed.bin");
                                if(!temp) {
                                    temp = strstr(paramValues[iterate]->parameterValue, ".bin");
                                }
                                if(temp)
                                    *temp = '\0';
                            }
                            #endif
                        }
                    }
                    nextProperty = rbusProperty_GetNext(nextProperty);
                }
                rbusProperty_Release(rbusPropertyValues);
            }
        }
        free(paramNames[0]);

        profVals->paramValues = paramValues;
        // End of populating bus independent parameter value array
        Vector_PushBack(profileValueList, profVals);
    } // End of looping through tr181 parameter list from profile
    if(paramNames)
        free(paramNames);

    T2Debug("%s --Out\n", __FUNCTION__);
    return profileValueList;
}

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t* filter, int interval, bool* autoPublish) {
    (void) handle;
    (void) filter;
    (void) interval;
    (void) autoPublish;
    T2Debug("%s ++in\n", __FUNCTION__);
    T2Info("eventSubHandler called:\n \taction=%s \teventName=%s\n", action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe", eventName);
    T2Debug("%s --out\n", __FUNCTION__);
    return RBUS_ERROR_SUCCESS;
}

/**
 * Data set handler for event receiving datamodel
 * Data being set will be an rbusProperty object with -
 *     eventName as property name
 *     eventValue as property value
 * This eliminates avoids the need for sending data with fixed delimiters
 * comapred to CCSP way of eventing .
 */
rbusError_t t2PropertyDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts) {

    T2Debug("%s ++in\n", __FUNCTION__);
    (void) handle;
    (void) opts;

    char const* paramName = rbusProperty_GetName(prop);
    if((strncmp(paramName, T2_EVENT_PARAM, maxParamLen) != 0) && (strncmp(paramName, T2_REPORT_PROFILE_PARAM, maxParamLen) != 0)
            && (strncmp(paramName, T2_REPORT_PROFILE_PARAM_MSG_PCK, maxParamLen) != 0)) {
        T2Debug("Unexpected parameter = %s \n", paramName);
        T2Debug("%s --out\n", __FUNCTION__);
        return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
    }

    T2Debug("Parameter name is %s \n", paramName);
    rbusValueType_t type_t;
    rbusValue_t paramValue_t = rbusProperty_GetValue(prop);
    if(paramValue_t) {
        type_t = rbusValue_GetType(paramValue_t);
    } else {
        return RBUS_ERROR_INVALID_INPUT;
	T2Debug("%s --out\n", __FUNCTION__);
    }
    if(strncmp(paramName, T2_EVENT_PARAM, maxParamLen) == 0) {
        if(type_t == RBUS_PROPERTY) {
            T2Debug("Received property type as value \n");
            rbusProperty_t objProperty = rbusValue_GetProperty(paramValue_t);
            char *eventName = (char*)rbusProperty_GetName(objProperty);
            if(eventName) {
                T2Debug("Event name is %s \n", eventName);
                rbusValue_t value = rbusProperty_GetValue(objProperty);
                type_t = rbusValue_GetType(value);
                if(type_t == RBUS_STRING) {
                    char* eventValue = rbusValue_ToString(value, NULL, 0);
                    if(eventValue) {
                        T2Debug("Event value is %s \n", eventValue);
                        eventCallBack((char*) strdup(eventName),(char*) strdup(eventValue) );
                        free(eventValue);
                    }
                }else {
                    T2Debug("Unexpected value type for property %s \n", eventName);
                }
            }
        }
    }else if(strncmp(paramName, T2_REPORT_PROFILE_PARAM, maxParamLen) == 0) {
        T2Debug("Inside datamodel handler \n");
        if(type_t == RBUS_STRING) {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            if(data) {
                T2Debug("Call datamodel function  with data %s \n", data);
                if(T2ERROR_SUCCESS != dmProcessingCallBack(data))
                {
                    free(data);
                    return RBUS_ERROR_INVALID_INPUT;
                }

                if (reportProfileVal){
                    free(reportProfileVal);
                    reportProfileVal = NULL;
                }
                if(reportProfilemsgPckVal) {
                    free(reportProfilemsgPckVal);
                    reportProfilemsgPckVal = NULL;
                }
                reportProfileVal = strdup(data);
                free(data);
            }
        } else {
            T2Debug("Unexpected value type for property %s \n", paramName);
        }

    }else if(strncmp(paramName, T2_REPORT_PROFILE_PARAM_MSG_PCK, maxParamLen) == 0) {
        T2Debug("Inside datamodel handler for message pack \n");
        if(dmMsgPckProcessingCallBack == NULL) {
            T2Debug("%s --out\n", __FUNCTION__);
            return RBUS_ERROR_ELEMENT_DOES_NOT_EXIST;
        }

        if(type_t == RBUS_STRING) {
            char* data = rbusValue_ToString(paramValue_t, NULL, 0);
            char *webConfigString = NULL;
            int stringSize = 0;
            if(data) {
                T2Debug("Call datamodel function  with data %s \n", data);
                webConfigString = AnscBase64Decode(data, &stringSize);
                if(NULL == webConfigString ||  0 == stringSize ){
                    T2Error("Invalid base64 input string. Ignore processing input configuration.\n");
		    free(data); //CID 168770: Resource leak
		    return RBUS_ERROR_INVALID_INPUT;
                }

                if(T2ERROR_SUCCESS != dmMsgPckProcessingCallBack(webConfigString, stringSize))
                {
                    free(data);
                    return RBUS_ERROR_INVALID_INPUT;
                }

                if(reportProfilemsgPckVal) {
                    free(reportProfilemsgPckVal);
                    reportProfilemsgPckVal = NULL;
                }
                if(reportProfileVal) {
                    free(reportProfileVal);
                    reportProfileVal = NULL;
                }
                reportProfilemsgPckVal = strdup(data);
                free(data);
            }
        } else {
            T2Debug("Unexpected value type for property %s \n", paramName);
        }
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return RBUS_ERROR_SUCCESS;
}

/**
 * Common data get handler for all parameters owned by t2.0
 * Implements :
 * 1] Telemetry.ReportProfiles.<Component Name>.EventMarkerList
 *    Returns list of events associated a component that needs to be notified to
 *    t2.0 in the form of single row multi instance rbus object
 */
rbusError_t t2PropertyDataGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts) {

    T2Debug("%s ++in\n", __FUNCTION__);
    (void) handle;
    (void) opts;
    char const* propertyName;
    char* componentName = NULL;
    Vector* eventMarkerListForComponent = NULL;
    int length = 0;

    propertyName = strdup(rbusProperty_GetName(property));
    if(propertyName) {
        T2Debug("Property Name is %s \n", propertyName);
    } else {
        T2Info("Unable to handle get request for property \n");
        T2Debug("%s --out\n", __FUNCTION__);
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(strncmp(propertyName, T2_REPORT_PROFILE_PARAM, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(reportProfileVal)
            rbusValue_SetString(value, reportProfileVal);
        else if(!reportProfilemsgPckVal){
            T2Info("Check the persistant folder for Active Profiles\n");
            char* temp = NULL;
            (*dmSavedJsonProcessingCallBack)(&temp);
            if (temp != NULL){    
                T2Info("Profiles from persistant folder %s \n",temp);
                rbusValue_SetString(value, temp);
                free(temp);
            }
            else
                rbusValue_SetString(value, "");
        }
	else
	    rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);
    }else if(strncmp(propertyName, T2_REPORT_PROFILE_PARAM_MSG_PCK, maxParamLen) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(reportProfilemsgPckVal)
            rbusValue_SetString(value, reportProfilemsgPckVal);
        else if (!reportProfileVal) {
            char* temp = NULL;
            int size;
            char* text;
            size = (*dmSavedMsgPackProcessingCallBack)(&temp);
            if (temp != NULL && size > 0){
                text = (char *) AnscBase64Encode (temp, size);
                T2Info("Profiles from persistant folder profile.msgpack \n");
                rbusValue_SetString(value, text);
                free(text);
		free(temp);
            }
            else
                rbusValue_SetString(value, "");
        }
	else
	    rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);

    }else {
        // START : Extract component name requesting for event marker list
        if(compTr181ParamMap != NULL)
            componentName = (char*) hash_map_get(compTr181ParamMap, propertyName);

        if(componentName) {
            T2Debug("Component name = %s \n", componentName);
        }else {
            T2Error("Component name is empty \n");
            free((char*)propertyName);
            propertyName = NULL;  //CID 158138: Resource leak
            return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
        }

        // END : Extract component name requesting for event marker list

        rbusValue_t value;
        rbusObject_t object;
        rbusProperty_t propertyList = NULL;

        getMarkerListCallBack(componentName, (void**)&eventMarkerListForComponent);
        length = Vector_Size(eventMarkerListForComponent);

        // START : create LL of rbusProperty_t object

        rbusValue_t fixedVal;
        rbusValue_Init(&fixedVal);
        rbusValue_SetString(fixedVal, "eventName");

        rbusProperty_t prevProperty = NULL;
        int i = 0;
        if(length == 0) {  // rbus doesn't like NULL objects and seems to crash. Setting empty values instead
            rbusProperty_Init(&propertyList, "", fixedVal);
        }else {
            for( i = 0; i < length; ++i ) {
                char* markerName = (char *) Vector_At(eventMarkerListForComponent, i);
                if(markerName) {
                    if(i == 0) {
                        rbusProperty_Init(&propertyList, markerName, fixedVal);
                        prevProperty = propertyList;
                    }else {
                        rbusProperty_t currentProperty = NULL;
                        rbusProperty_Init(&currentProperty, markerName, fixedVal);
                        if(prevProperty)
                            rbusProperty_SetNext(prevProperty, currentProperty);
                        prevProperty = currentProperty;
                        rbusProperty_Release(currentProperty);
                    }
                    T2Debug("%d Updated rbusProperty with name = %s \n", i, markerName);
                }
            }
            Vector_Destroy(eventMarkerListForComponent, free);
        }
        // END : create LL of rbusProperty_t object

        rbusObject_Init(&object, "eventList");
        rbusObject_SetProperties(object, propertyList);
        rbusValue_Init(&value);
        rbusValue_SetObject(value, object);
        rbusProperty_SetValue(property, value);

        rbusValue_Release(fixedVal);
        rbusValue_Release(value);
        rbusProperty_Release(propertyList);
        rbusObject_Release(object);

    }

    if(propertyName) {
        free((char*)propertyName);
        propertyName = NULL;
    }

    T2Debug("%s --out\n", __FUNCTION__);
    return RBUS_ERROR_SUCCESS;
}


T2ERROR registerRbusT2EventListener(TelemetryEventCallback eventCB)
{
    T2Debug("%s ++in\n", __FUNCTION__);

	T2ERROR status = T2ERROR_SUCCESS;
	rbusError_t ret = RBUS_ERROR_SUCCESS;
    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        return T2ERROR_FAILURE;
    }

    /**
     * Register data elements with rbus for EVENTS and Profile Updates.
     */
    rbusDataElement_t dataElements[2] = {
        {T2_EVENT_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, t2PropertyDataSetHandler, NULL, NULL, NULL, NULL}},
        {T2_PROFILE_UPDATED_NOTIFY, RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, (rbusEventSubHandler_t)eventSubHandler, NULL}}
    };
    ret = rbus_regDataElements(t2bus_handle, 2, dataElements);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        T2Error("Failed to register T2 data elements with rbus. Error code : %d\n", ret);
        status = T2ERROR_FAILURE ;
    }
    eventCallBack = eventCB ;

    T2Debug("%s --out\n", __FUNCTION__);
    return status;
}

T2ERROR unregisterRbusT2EventListener()
{
    rbusEvent_Unsubscribe(t2bus_handle, T2_EVENT_PARAM);
    rBusInterface_Uninit();
    return T2ERROR_SUCCESS;
}

/**
 * Register data elements for COMPONENT marker list over bus for common lib event filtering.
 * Data element over bus will be of the format :Telemetry.ReportProfiles.<componentName>.EventMarkerList
 */
T2ERROR regDEforCompEventList(const char* componentName, T2EventMarkerListCallback callBackHandler) {

    T2Debug("%s ++in\n", __FUNCTION__);
    char deNameSpace[125] = { '\0' };
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    T2ERROR status = T2ERROR_SUCCESS;

    if(!componentName)
        return status;

    if(compTr181ParamMap == NULL)
        compTr181ParamMap = hash_map_create();

    snprintf(deNameSpace, 124, "%s%s%s", T2_ROOT_PARAMETER, componentName, T2_EVENT_LIST_PARAM_SUFFIX);
    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    rbusDataElement_t dataElements[1] = { { deNameSpace, RBUS_ELEMENT_TYPE_PROPERTY, { t2PropertyDataGetHandler, NULL, NULL, NULL,NULL, NULL } } };
    ret = rbus_regDataElements(t2bus_handle, 1, dataElements);
    if(ret == RBUS_ERROR_SUCCESS) {
        T2Debug("Registered data element %s with bus \n ", deNameSpace);
        hash_map_put(compTr181ParamMap, (void*) strdup(deNameSpace), (void*) strdup(componentName));
        T2Debug("Save dataelement mapping, %s with component name %s \n ", deNameSpace, componentName);
    }else {
        T2Error("Failed in registering data element %s \n", deNameSpace);
        status = T2ERROR_FAILURE;
    }

    if(!getMarkerListCallBack)
        getMarkerListCallBack = callBackHandler;

    T2Debug("%s --out\n", __FUNCTION__);
    return status;
}

/**
 * Unregister data elements for COMPONENT marker list over bus
 * Data element over bus will be of the format :Telemetry.ReportProfiles.<componentName>.EventMarkerList
 */
void unregisterDEforCompEventList(){

    int count = 0;
    int i = 0;
    T2Debug("%s ++in\n", __FUNCTION__);

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return ;
    }

    if(!compTr181ParamMap) {
        T2Info("No data elements present to unregister");
        T2Debug("%s --out\n", __FUNCTION__);
        return;
    }

    count = hash_map_count(compTr181ParamMap);
    T2Debug("compTr181ParamMap has %d components registered \n", count);
    if(count > 0) {
        rbusDataElement_t dataElements[count];
        rbusCallbackTable_t cbTable = { t2PropertyDataGetHandler, NULL, NULL, NULL, NULL, NULL };
        for( i = 0; i < count; ++i ) {
            char *dataElementName = hash_map_lookupKey(compTr181ParamMap, i);
            if(dataElementName) {
                T2Debug("Adding %s to unregister list \n", dataElementName);
                dataElements[i].name = dataElementName;
                dataElements[i].type = RBUS_ELEMENT_TYPE_PROPERTY;
                dataElements[i].cbTable = cbTable;
            }
        }
        if(RBUS_ERROR_SUCCESS != rbus_unregDataElements(t2bus_handle, count, dataElements)) {
            T2Error("Failed to unregister to dataelements");
        }
    }
    T2Debug("Freeing compTr181ParamMap \n");
    hash_map_destroy(compTr181ParamMap, free);
    compTr181ParamMap = NULL;
    T2Debug("%s --out\n", __FUNCTION__);
}

/**
 * Register data elements for dataModel implementation.
 * Data element over bus will be Device.X_RDKCENTRAL-COM_T2.ReportProfiles,
 *    Device.X_RDKCENTRAL-COM_T2.ReportProfilesMsgPack
 */
T2ERROR regDEforProfileDataModel(dataModelCallBack dmCallBackHandler,  dataModelMsgPckCallBack dmMsgPckCallBackHandler, dataModelSavedJsonCallBack dmSavedJsonCallBack, dataModelSavedMsgPackCallBack dmSavedMsgPackCallBack) {

    T2Debug("%s ++in\n", __FUNCTION__);
    char deNameSpace[125] = { '\0' };
    char deMsgPck[125] = { '\0' };
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    T2ERROR status = T2ERROR_SUCCESS;
    dmSavedJsonProcessingCallBack = dmSavedJsonCallBack;
    dmSavedMsgPackProcessingCallBack = dmSavedMsgPackCallBack;

    snprintf(deNameSpace, 124 , "%s", T2_REPORT_PROFILE_PARAM);
    snprintf(deMsgPck, 124 , "%s", T2_REPORT_PROFILE_PARAM_MSG_PCK);
    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    rbusDataElement_t dataElements[NUM_PROFILE_ELEMENTS] = {
        {deNameSpace, RBUS_ELEMENT_TYPE_PROPERTY, {t2PropertyDataGetHandler, t2PropertyDataSetHandler, NULL, NULL, NULL, NULL}},
        {deMsgPck, RBUS_ELEMENT_TYPE_PROPERTY, {t2PropertyDataGetHandler, t2PropertyDataSetHandler, NULL, NULL, NULL, NULL}}
    };
    ret = rbus_regDataElements(t2bus_handle, NUM_PROFILE_ELEMENTS, dataElements);
    if(ret == RBUS_ERROR_SUCCESS) {
        T2Debug("Registered data element %s with bus \n ", deNameSpace);
    }else {
        T2Error("Failed in registering data element %s \n", deNameSpace);
        status = T2ERROR_FAILURE;
    }

    if (!dmProcessingCallBack)
        dmProcessingCallBack = dmCallBackHandler ;

    if(!dmMsgPckProcessingCallBack)
        dmMsgPckProcessingCallBack = dmMsgPckCallBackHandler;
    T2Debug("%s --out\n", __FUNCTION__);
    return status ;
}


T2ERROR publishEventsProfileUpdates() {
    T2Debug("%s ++in\n", __FUNCTION__);
    rbusEvent_t event;
    rbusObject_t data;
    rbusValue_t value;
    T2ERROR status = T2ERROR_SUCCESS;
    rbusError_t ret = RBUS_ERROR_SUCCESS;

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        return T2ERROR_FAILURE;
    }

    T2Debug("Publishing event for t2profile update notification to components \n");
    rbusValue_Init(&value);
    rbusValue_SetString(value, "t2ProfileUpdated");
    rbusObject_Init(&data, NULL);
    rbusObject_SetValue(data, "t2ProfileUpdated", value);

    event.name = T2_PROFILE_UPDATED_NOTIFY;
    event.data = data;
    event.type = RBUS_EVENT_GENERAL;
    ret = rbusEvent_Publish(t2bus_handle, &event);
    if(ret != RBUS_ERROR_SUCCESS) {
        T2Debug("provider: rbusEvent_Publish Event1 failed: %d\n", ret);
        status = T2ERROR_FAILURE;
    }

    rbusValue_Release(value);
	T2Debug("%s --out\n", __FUNCTION__);
    return status;
    
}

void reportEventHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;
    T2Debug("in Function %s \n", __FUNCTION__);
    T2Debug("Called the callback for the prop");
    const char* eventName = event->name;
    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    const char* eventValue = rbusValue_ToString(newValue,NULL,0);
    eventCallBack((char*) strdup(eventName),(char*) strdup(eventValue) );
}

void triggerCondtionReceiveHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
    (void)handle;

    rbusValue_t newValue = rbusObject_GetValue(event->data, "value");
    rbusValue_t oldValue = rbusObject_GetValue(event->data, "oldValue");
    rbusValue_t filter = rbusObject_GetValue(event->data, "filter");
    const char* eventName = event->name;
    const char* eventValue = rbusValue_ToString(newValue,NULL,0);
    T2Debug("Consumer receiver event for param %s\n and the value %s\n", event->name, eventValue);

    if(newValue){
        T2Debug("  New Value: %s \n", rbusValue_ToString(newValue,NULL,0));
    }
    if(oldValue){
        T2Debug("  Old Value: %s \n",  rbusValue_ToString(oldValue,NULL,0));
    }

    if(filter) {
      T2Debug("Filter event\n");
      if(rbusValue_GetBoolean(filter) == 1) {
        triggerReportOnCondtion(eventName, eventValue);
      }
    }
    else {
      T2Debug("ValueChange event\n");
      triggerReportOnCondtion(eventName, eventValue);
    }  

}

T2ERROR rbusT2ConsumerReg(Vector *triggerConditionList)
{
    int j;
    int ret = T2ERROR_SUCCESS;
    int status = T2ERROR_SUCCESS;
    T2Debug("--in %s \n", __FUNCTION__);

        for( j = 0; j < triggerConditionList->count; j++ ) {
                TriggerCondition *triggerCondition = ((TriggerCondition *) Vector_At(triggerConditionList, j));
		if(triggerCondition){
                    T2Debug("Adding to register consumer list \n");
                    status = T2RbusConsumer(triggerCondition);
		    if(status == T2ERROR_FAILURE)
                         ret = T2ERROR_FAILURE;
		    T2Debug("T2RbusConsumer return = %d\n", ret);
		}    
        }
    return ret;
}

T2ERROR rbusT2ConsumerUnReg(Vector *triggerConditionList)
{
    int j,rc;
    //char user_data[32] = {0};
    T2Debug("%s ++in\n", __FUNCTION__);

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    for( j = 0; j < triggerConditionList->count; j++ ) {
        TriggerCondition *triggerCondition = ((TriggerCondition *) Vector_At(triggerConditionList, j));
        T2Debug("Adding %s to unregister list \n", triggerCondition->reference);
        rbusFilter_RelationOperator_t filterOperator = RBUS_FILTER_OPERATOR_EQUAL;
        rbusFilter_t filter;
        rbusValue_t filterValue;
        rbusEventSubscription_t subscription = {triggerCondition->reference, NULL, 0, 0, (void *)triggerCondtionReceiveHandler, NULL, NULL, NULL};

        if(strcmp(triggerCondition->oprator,"lt") == 0)
        {
            filterOperator = RBUS_FILTER_OPERATOR_LESS_THAN;
        }
        if(strcmp(triggerCondition->oprator,"gt") == 0)
        {
            filterOperator = RBUS_FILTER_OPERATOR_GREATER_THAN;
        }
        if(strcmp(triggerCondition->oprator,"eq") == 0)
        {
            filterOperator = RBUS_FILTER_OPERATOR_EQUAL;
        }
        if(strcmp(triggerCondition->oprator,"any") == 0)
        {
            rc = rbusEvent_Unsubscribe(
            t2bus_handle,
            triggerCondition->reference);
            if (rc != RBUS_ERROR_SUCCESS)
            {
                T2Debug("%s UnSubscribe failed\n",__FUNCTION__);
            }
        }
        else
        {
            rbusValue_Init(&filterValue);
            rbusValue_SetInt32(filterValue, triggerCondition->threshold);
            rbusFilter_InitRelation(&filter, filterOperator, filterValue);
            subscription.filter = filter;
            rc = rbusEvent_UnsubscribeEx(t2bus_handle, &subscription, 1);
            if (rc != RBUS_ERROR_SUCCESS)
            {
                T2Debug("%s UnSubscribeEx failed\n",__FUNCTION__);
            }
        }
    }
    return T2ERROR_SUCCESS;
}

T2ERROR T2RbusConsumer(TriggerCondition *triggerCondition)
{
    int rc = RBUS_ERROR_SUCCESS;
    int ret = T2ERROR_SUCCESS;
    char user_data[32] = {0};
    //char componentName[] = "t2consumer";
    T2Debug("--in %s\n", __FUNCTION__);
    if(triggerCondition->isSubscribed == true){
       T2Debug("%s already subscribed\n", triggerCondition->reference);
       return T2ERROR_SUCCESS;
    }    
       
    rbusFilter_RelationOperator_t filterOperator; 
    rbusFilter_t filter;
    rbusValue_t filterValue;
    rbusEventSubscription_t subscription = {triggerCondition->reference, NULL, 0, 0, triggerCondtionReceiveHandler, NULL, NULL, NULL};
      
    if(strcmp(triggerCondition->oprator,"lt") == 0)
    {
       filterOperator = RBUS_FILTER_OPERATOR_LESS_THAN;
    }
    if(strcmp(triggerCondition->oprator,"gt") == 0)
    {
       filterOperator = RBUS_FILTER_OPERATOR_GREATER_THAN;
    }
    if(strcmp(triggerCondition->oprator,"eq") == 0)
    {
        filterOperator = RBUS_FILTER_OPERATOR_EQUAL;
    }

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Debug("Consumer: rbus_open failed: %d\n", rc);
        return T2ERROR_FAILURE;
    }
    strcpy(user_data,"Not used");
    if(strcmp(triggerCondition->oprator,"any") == 0)
    {
        T2Debug("filterOperator %s , threshold %d \n",triggerCondition->oprator, triggerCondition->threshold);
        rc = rbusEvent_Subscribe(
        t2bus_handle,
        triggerCondition->reference,
        triggerCondtionReceiveHandler,
        user_data,
        0);
        if (rc != RBUS_ERROR_SUCCESS){
            T2Error(" %s Subscribe failed\n",__FUNCTION__);
	    ret = T2ERROR_FAILURE;
        }
	else{
	    triggerCondition->isSubscribed = true;
	}
    }
    else	
    {
        T2Debug("Ex filterOperator %s ( %d ) , threshold %d \n",triggerCondition->oprator, filterOperator, triggerCondition->threshold);
        rbusValue_Init(&filterValue);
        rbusValue_SetInt32(filterValue, triggerCondition->threshold);
        rbusFilter_InitRelation(&filter, filterOperator, filterValue);
        subscription.filter = filter;
        rc = rbusEvent_SubscribeEx(t2bus_handle, &subscription, 1, 0);
	if (rc != RBUS_ERROR_SUCCESS){
            T2Error(" %s SubscribeEx failed\n",__FUNCTION__);
	    ret = T2ERROR_FAILURE;
        }
	else{
            triggerCondition->isSubscribed = true;
	}
        rbusValue_Release(filterValue);
        rbusFilter_Release(filter);
    }
    return ret;
}

T2ERROR T2RbusReportEventConsumer(char* reference, bool subscription)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    int rc = RBUS_ERROR_SUCCESS;
    if (!subscription){
        rc = rbusEvent_Unsubscribe(
        t2bus_handle,
        reference);
        if (rc != RBUS_ERROR_SUCCESS)
            T2Debug("--in %s there is an issue in unsubscribe \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return rc;
    }
    else{
        int ret = T2ERROR_SUCCESS;
        char user_data[32] = {0};
        //char componentName[] = "t2consumer";
        T2Debug("--in %s\n", __FUNCTION__);
        if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()){
            T2Debug("Consumer: rbus_open failed\n");
            T2Debug("%s --out\n", __FUNCTION__);
            return T2ERROR_FAILURE;
        }
        strcpy(user_data,"Not used");
        rc = rbusEvent_Subscribe(
        t2bus_handle,
        reference,
        reportEventHandler,
        user_data,
        0);
        if (rc != RBUS_ERROR_SUCCESS){
            T2Error(" %s Subscribe failed\n",__FUNCTION__);
            ret = T2ERROR_FAILURE;
        }
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;
    }
}

rbusError_t t2TriggerConditionGetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    (void)handle;
    (void)opts;

    T2Debug("Provider: Called get handler for [%s] \n", name);
    static int32_t mydata = 0;
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetInt32(value, mydata);
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

T2ERROR rbusMethodCaller(char *methodName, rbusObject_t* inputParams, char* payload, rbusMethodCallBackPtr rbusMethodCallBack ) {
    T2Debug("%s ++in\n", __FUNCTION__);

    T2ERROR ret = T2ERROR_FAILURE;
    int rc = RBUS_ERROR_BUS_ERROR;
    T2Info("methodName = %s payload = %s \n", methodName, payload);

    if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }
    rc = rbusMethod_InvokeAsync(t2bus_handle, methodName, *inputParams, rbusMethodCallBack, RBUS_METHOD_TIMEOUT);
    if (rc == RBUS_ERROR_SUCCESS) {
        ret = T2ERROR_SUCCESS ;
    }else {
        T2Error("rbusMethod_InvokeAsync invocation from %s failed with error code %d \n", __FUNCTION__ , rc);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

bool rbusCheckMethodExists(const char* rbusMethodName) {
    T2Debug("%s ++in\n", __FUNCTION__);

     rbusError_t rc = RBUS_ERROR_BUS_ERROR;
     rbusObject_t inParams, outParams;
     rbusValue_t value;

     T2Info("methodName = %s \n", rbusMethodName);

     if(!t2bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
         T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
         T2Debug("%s --out\n", __FUNCTION__);
         return false;
     }
     rbusObject_Init(&inParams, NULL);
     rbusValue_Init(&value);
     rbusValue_SetString(value, "");
     rbusObject_SetValue(inParams, "check", value);
     rc = rbusMethod_Invoke(t2bus_handle, rbusMethodName, inParams, &outParams);
     rbusValue_Release(value);
     rbusObject_Release(inParams);

     T2Debug("%s --out\n", __FUNCTION__);
     if (rc != RBUS_ERROR_SUCCESS) {
         T2Debug("rbusMethod_Invoke called: %s with return code \n  error = %s \n", rbusMethodName, rbusError_ToString(rc));
         T2Info("Rbus method %s doesn't exists \n", rbusMethodName );
         return false ;
     }
     rbusObject_Release(outParams);
     return true ;
}
