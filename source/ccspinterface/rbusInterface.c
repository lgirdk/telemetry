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

#include <rbus/rbus.h>
#include <rbus/rbus_object.h>
#include <rbus/rbus_property.h>
#include <rbus/rbus_value.h>
#include <stdlib.h>

#include "t2collection.h"
#include "t2common.h"
#include "busInterface.h"
#include "telemetry2_0.h"
#include "t2log_wrapper.h"


#define buffLen 1024
#define maxParamLen 50

static rbusHandle_t bus_handle;
static TelemetryEventCallback eventCallBack;
static T2EventMarkerListCallback getMarkerListCallBack;
static dataModelCallBack dmProcessingCallBack;

static hash_map_t *compTr181ParamMap = NULL;

static char* reportProfileVal = NULL ;

bool isRbusInitialized( ) {

    return bus_handle != NULL ? true : false;
}

static T2ERROR rBusInterface_Init( ) {
    T2Debug("%s ++in\n", __FUNCTION__);

    int ret = RBUS_ERROR_SUCCESS;
    ret = rbus_open(&bus_handle, COMPONENT_NAME);
    if(ret != RBUS_ERROR_SUCCESS) {
        T2Error("%s:%d, init failed with error code %d \n", __func__, __LINE__, ret);
        return T2ERROR_FAILURE;
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static void rBusInterface_Uninit( ) {
    rbus_close(bus_handle);
}

T2ERROR getRbusParameterVal(const char* paramName, char **paramValue) {
    T2Debug("%s ++in \n", __FUNCTION__);

    rbusError_t ret = RBUS_ERROR_SUCCESS;
    rbusValue_t paramValue_t;
    rbusValueType_t rbusValueType ;
    char *stringValue = NULL;
    rbusSetOptions_t opts;
    opts.commit = true;

    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        return T2ERROR_FAILURE;
    }

    ret = rbus_get(bus_handle, paramName, &paramValue_t);
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
    T2Debug("%s = %s\n", paramName, stringValue);
    *paramValue = stringValue;
    rbusValue_Release(paramValue_t);
    T2Debug("%s --out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

Vector* getRbusProfileParamValues(Vector *paramList) {
    T2Debug("%s ++in\n", __FUNCTION__);
    unsigned int i = 0;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    Vector *profileValueList = NULL;
    Vector_Create(&profileValueList);

    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
    	Vector_Destroy(profileValueList, free);
        return NULL ;
    }
    char** paramNames = (char **) malloc(paramList->count * sizeof(char*));

    T2Debug("TR-181 Param count : %d\n", paramList->count);
    for( ; i < paramList->count; i++ ) { // Loop through paramlist from profile

        tr181ValStruct_t **paramValues = NULL;
        rbusProperty_t rbusPropertyValues = NULL;
        int paramValCount = 0;
        int iterate = 0;
        profileValues* profVals = (profileValues *) malloc(sizeof(profileValues));
        char *param = ((Param *) Vector_At(paramList, i))->alias ;
        paramNames[0] = strdup(param);
        T2Debug("Calling rbus_getExt for %s \n", paramNames[0]);
        if(RBUS_ERROR_SUCCESS != rbus_getExt(bus_handle, 1, paramNames, &paramValCount, &rbusPropertyValues)) {
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
                            stringValue = rbusProperty_GetName(nextProperty);
                            paramValues[iterate]->parameterName = strdup(stringValue);
                            paramValues[iterate]->parameterValue = rbusValue_ToString(value, NULL, 0);
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
rbusError_t t2PropertyDataSetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetOptions_t* opts) {

    T2Debug("%s ++in\n", __FUNCTION__);
    (void) handle;
    (void) opts;

    char const* paramName = rbusProperty_GetName(prop);
    if((strcmp(paramName, T2_EVENT_PARAM) != 0) && (strcmp(paramName, T2_REPORT_PROFILE_PARAM) != 0)) {
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
    if(strcmp(paramName, T2_EVENT_PARAM) == 0) {
        if(type_t == RBUS_PROPERTY) {
            T2Debug("Received property type as value \n");
            rbusProperty_t objProperty = rbusValue_GetProperty(paramValue_t);
            char *eventName = rbusProperty_GetName(objProperty);
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
    }else if(strcmp(paramName, T2_REPORT_PROFILE_PARAM) == 0) {
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
                reportProfileVal = strdup(data);
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
rbusError_t t2PropertyDataGetHandler(rbusHandle_t handle, rbusProperty_t property) {

    T2Debug("%s ++in\n", __FUNCTION__);
    (void) handle;
    char const* propertyName;
    char* componentName;
    Vector* eventMarkerListForComponent = NULL;
    char **eventMarkerList = NULL;
    int length = 0;

    rbusError_t ret = RBUS_ERROR_SUCCESS;


    propertyName = strdup(rbusProperty_GetName(property));
    if(propertyName) {
        T2Debug("Property Name is %s \n", propertyName);
    } else {
        T2Info("Unable to handle get request for property \n");
        T2Debug("%s --out\n", __FUNCTION__);
        return RBUS_ERROR_INVALID_INPUT;
    }

    if(strcmp(propertyName, T2_REPORT_PROFILE_PARAM) == 0) {
        rbusValue_t value;
        rbusValue_Init(&value);
        if(reportProfileVal)
            rbusValue_SetString(value, reportProfileVal);
        else
            rbusValue_SetString(value, "");
        rbusProperty_SetValue(property, value);
        rbusValue_Release(value);

    } else {
        // START : Extract component name requesting for event marker list
        if(compTr181ParamMap != NULL)
            componentName = (char*) hash_map_get(compTr181ParamMap, propertyName);

        if(componentName) {
            T2Debug("Component name = %s \n", componentName);
        }else {
            T2Error("Component name is empty \n");
            return RBUS_ERROR_DESTINATION_RESPONSE_FAILURE;
        }

        // END : Extract component name requesting for event marker list

        rbusValue_t value;
        rbusObject_t object;
        rbusProperty_t propertyList = NULL;

        getMarkerListCallBack(componentName, &eventMarkerListForComponent);
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
        free(propertyName);
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
    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        return T2ERROR_FAILURE;
    }

    /**
     * Register data elements with rbus for EVENTS and Profile Updates.
     */
    rbusDataElement_t dataElements[2] = {
        {T2_EVENT_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {NULL, t2PropertyDataSetHandler, NULL, NULL}},
        {T2_PROFILE_UPDATED_NOTIFY, RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, eventSubHandler}}
    };
    ret = rbus_regDataElements(bus_handle, 2, dataElements);
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
    rbusEvent_Unsubscribe(bus_handle, T2_EVENT_PARAM);
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
    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    rbusDataElement_t dataElements[1] = { { deNameSpace, RBUS_ELEMENT_TYPE_PROPERTY, { t2PropertyDataGetHandler, NULL, NULL, NULL } } };
    ret = rbus_regDataElements(bus_handle, 1, dataElements);
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

    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
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
        rbusCallbackTable_t cbTable = { t2PropertyDataGetHandler, NULL, NULL, NULL };
        char* compName = (char*) hash_map_get_first(compTr181ParamMap);
        for( i = 0; i < count; ++i ) {
            char *dataElementName = hash_map_lookupKey(compTr181ParamMap, i);
            if(dataElementName) {
                T2Debug("Adding %s to unregister list \n", dataElementName);
                dataElements[i].name = dataElementName;
                dataElements[i].type = RBUS_ELEMENT_TYPE_PROPERTY;
                dataElements[i].cbTable = cbTable;
            }
        }
        if(RBUS_ERROR_SUCCESS != rbus_unregDataElements(bus_handle, count, dataElements)) {
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
 * Data element over bus will be Device.X_RDKCENTRAL-COM_T2.ReportProfiles
 */
T2ERROR regDEforProfileDataModel(dataModelCallBack dmCallBackHandler) {

    T2Debug("%s ++in\n", __FUNCTION__);
    char deNameSpace[125] = { '\0' };
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    T2ERROR status = T2ERROR_SUCCESS;


    snprintf(deNameSpace, 124 , "%s", T2_REPORT_PROFILE_PARAM);
    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
        T2Error("%s Failed in getting bus handles \n", __FUNCTION__);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    rbusDataElement_t dataElements[1] = {
        {deNameSpace, RBUS_ELEMENT_TYPE_PROPERTY, {t2PropertyDataGetHandler, t2PropertyDataSetHandler, NULL, NULL}}
    };
    ret = rbus_regDataElements(bus_handle, 1, dataElements);
    if(ret == RBUS_ERROR_SUCCESS) {
        T2Debug("Registered data element %s with bus \n ", deNameSpace);
    }else {
        T2Error("Failed in registering data element %s \n", deNameSpace);
        status = T2ERROR_FAILURE;
    }

    if (!dmProcessingCallBack)
        dmProcessingCallBack = dmCallBackHandler ;
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

    if(!bus_handle && T2ERROR_SUCCESS != rBusInterface_Init()) {
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
    ret = rbusEvent_Publish(bus_handle, &event);
    if(ret != RBUS_ERROR_SUCCESS) {
        T2Debug("provider: rbusEvent_Publish Event1 failed: %d\n", ret);
        status = T2ERROR_FAILURE;
    }

    rbusValue_Release(value);
	T2Debug("%s --out\n", __FUNCTION__);
    return status;
    
}
