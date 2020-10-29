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
#include <stdlib.h>
#include <rbus/rbus.h>

#include "t2log_wrapper.h"
#include "busInterface.h"
#include "ccspinterface.h"
#include "rbusInterface.h"

static bool isRbus = false ;
static bool isBusInit = false ;

bool isRbusEnabled( ) {
    T2Debug("%s ++in \n", __FUNCTION__);
    if(RBUS_ENABLED == rbus_checkStatus()) {
        isRbus = true;
    } else {
        isRbus = false;
    }
    T2Debug("RBUS mode active status = %s \n", isRbus ? "true":"false");
    T2Debug("%s --out \n", __FUNCTION__);
    return isRbus;
}

static bool busInit( ) {
    T2Debug("%s ++in \n", __FUNCTION__);
    if(!isBusInit) {
        isRbusEnabled();
        isBusInit = true;
    }
    T2Debug("%s --out \n", __FUNCTION__);
    return isBusInit;
}

T2ERROR getParameterValue(const char* paramName, char **paramValue)
{
    T2Debug("%s ++in \n", __FUNCTION__);
    T2ERROR ret = T2ERROR_FAILURE ;
    if(!isBusInit)
        busInit();

    if(isRbus)
        ret = getRbusParameterVal(paramName,paramValue);
    else
        ret = getCCSPParamVal(paramName, paramValue);

    T2Debug("%s --out \n", __FUNCTION__);
    return ret;
}

Vector* getProfileParameterValues(Vector *paramList)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    Vector *profileValueList = NULL;
    if(!isBusInit)
        busInit();

    if(isRbus)
    	profileValueList = getRbusProfileParamValues(paramList);
    else
        profileValueList = getCCSPProfileParamValues(paramList);

    T2Debug("%s --Out\n", __FUNCTION__);
    return profileValueList;
}

void freeParamValueSt(tr181ValStruct_t **valStructs, int valSize) {
    int i;
    if(valStructs == NULL)
        return ;
    if(valSize) {
        for( i = 0; i < valSize; i++ ) {
            if(valStructs[i]) {
                if(valStructs[i]->parameterName)
                    free(valStructs[i]->parameterName);
                if(valStructs[i]->parameterValue)
                    free(valStructs[i]->parameterValue);
                free(valStructs[i]);
                valStructs[i] = NULL ;
            }
        }
        free(valStructs);
        valStructs = NULL;
    }
}

/**
 * Register with right bus call back dpending on dbus/rbus mode
 */
T2ERROR registerForTelemetryEvents(TelemetryEventCallback eventCB)
{
	T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!isBusInit)
        busInit();

    if (isRbus) {
    	ret = registerRbusT2EventListener(eventCB);
    } else {
    	ret = registerCcspT2EventListener(eventCB);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

T2ERROR unregisterForTelemetryEvents()
{
    return T2ERROR_SUCCESS;
}


T2ERROR busUninit()
{
    if (isRbus)
    {
    	unregisterRbusT2EventListener();
    }
    return T2ERROR_SUCCESS;
}
