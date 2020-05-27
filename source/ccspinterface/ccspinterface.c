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

#include "ansc_platform.h"
#include "ccspinterface.h"
#include "t2log_wrapper.h"
#include "syslog.h"
#include "vector.h"
#include "t2common.h"

/** TODO  1. Write all parameters to a file on first call to API. Subsequent calls to read parameters from file instead of bus call
 2. Add condition to check if bus_handle is NULL or not in all API
 */

#define CCSP_DBUS_INTERFACE_CR     "com.cisco.spvtg.ccsp.CR"

#ifdef _COSA_INTEL_USG_ATOM_
#define CCSP_COMPONENT_ID          "eRT.com.cisco.spvtg.ccsp.t2atom"       /* Use different componentId for Atom and Arm */
#else
#define CCSP_COMPONENT_ID          "eRT.com.cisco.spvtg.ccsp.telemetry"
#endif

void *bus_handle = NULL;

static T2ERROR CCSPInterface_Init()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    char *pCfg = CCSP_MSG_BUS_CFG;
    char *componentId = NULL;

#ifndef _COSA_INTEL_USG_ATOM_    /* Avoid duplicate componentId in multiprocessor devices */
    componentId = getComponentId();
#endif

    if (componentId == NULL)
        componentId = strdup(CCSP_COMPONENT_ID);

    int ret = CCSP_Message_Bus_Init(componentId, pCfg, &bus_handle, Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
    free(componentId);

    if (ret == -1)
    {
        T2Error("%s:%d, init failed\n", __func__, __LINE__);
        return T2ERROR_FAILURE;
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static int findDestComponent(const char *paramName, char **destCompName, char **destPath)
{
    int ret = -1, size =0;
    char dst_pathname_cr[256] = {0};
    componentStruct_t **ppComponents = NULL;
    T2Debug("%s ++in for paramName : %s\n", __FUNCTION__, paramName);
    sprintf(dst_pathname_cr, "%s%s", "eRT.", CCSP_DBUS_INTERFACE_CR);

    ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle, dst_pathname_cr, paramName, "", &ppComponents, &size);
    if ( ret == CCSP_SUCCESS && size >= 1)
    {
        *destCompName = strdup(ppComponents[0]->componentName);
        *destPath = strdup(ppComponents[0]->dbusPath);
    }
    else
    {
        T2Error("Failed to get component for %s ret: %d\n",paramName,ret);
        return ret;
    }
    free_componentStruct_t(bus_handle, size, ppComponents);

    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

T2ERROR getParameterValues(const char **paramNames, const int paramNamesCount, parameterValStruct_t ***valStructs, int *valSize)
{
    char *destCompName = NULL, *destCompPath = NULL;
    T2ERROR retErrCode = T2ERROR_FAILURE;
    T2Debug("%s ++in\n", __FUNCTION__);

    if(!bus_handle && T2ERROR_SUCCESS != CCSPInterface_Init())
    {
        return T2ERROR_FAILURE;
    }
    else if(paramNames == NULL || paramNamesCount <=0 )
    {
        T2Error("paramNames is NULL or paramNamesCount <= 0 - returning\n");
        return T2ERROR_INVALID_ARGS;
    }
    if(CCSP_SUCCESS == findDestComponent(paramNames[0], &destCompName, &destCompPath))
    {
        T2Debug("Calling CcspBaseIf_getParameterValues for : %s, paramCount : %d Destination name : %s and path %s\n", paramNames[0], paramNamesCount, destCompName, destCompPath);
        int ret = CcspBaseIf_getParameterValues(bus_handle, destCompName, destCompPath, paramNames, paramNamesCount, valSize, valStructs);
        if (ret != CCSP_SUCCESS)
        {
            T2Error("CcspBaseIf_getParameterValues failed for : %s with ret = %d\n", paramNames[0], ret);
        }
        else
        {
            retErrCode = T2ERROR_SUCCESS;
        }
    }
    else
    {
        T2Error("Unable to find supporting component for parameter : %s\n", paramNames[0]);
        return T2ERROR_FAILURE;
    }
    if(destCompName)
    {
        free(destCompName);
        destCompName = NULL;
    }
    if(destCompPath)
    {
        free(destCompPath);
        destCompPath = NULL;
    }

    T2Debug("%s --out \n", __FUNCTION__);
    return retErrCode;
}

T2ERROR getParameterNames(const char *objName, parameterInfoStruct_t ***paramNamesSt, int *paramNamesLength)
{
    T2ERROR ret = T2ERROR_FAILURE;
    char *destCompName = NULL, *destCompPath = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);

    if(!bus_handle && T2ERROR_SUCCESS != CCSPInterface_Init())
    {
        return T2ERROR_FAILURE;
    }
    else if(objName == NULL || objName[strlen(objName)-1] != '.')
    {
        T2Error("Invalid objectName, doesn't end with a wildcard '.'\n");
        return T2ERROR_INVALID_ARGS;
    }
    if(CCSP_SUCCESS == findDestComponent(objName, &destCompName, &destCompPath))
    {
        if ( CCSP_SUCCESS != CcspBaseIf_getParameterNames(bus_handle, destCompName, destCompPath, objName, 1, paramNamesLength, paramNamesSt))
        {
            T2Error("CcspBaseIf_getParameterValues failed for : %s\n", objName);
        }
        else
            ret = T2ERROR_SUCCESS;
    }
    else
    {
        T2Error("Unable to find supporting component for parameter : %s\n", objName);
    }
    if(destCompName)
    {
        free(destCompName);
        destCompName = NULL;
    }
    if(destCompPath)
    {
        free(destCompPath);
        destCompPath = NULL;
    }
    T2Debug("%s --out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

void freeParamValueSt(parameterValStruct_t **valStructs, int valSize)
{
    free_parameterValStruct_t(bus_handle, valSize, valStructs);
}

void freeParamInfoSt(parameterInfoStruct_t **paramInfoSt, int paramNamesLength)
{
    free_parameterInfoStruct_t(bus_handle, paramNamesLength, paramInfoSt);
}

T2ERROR getParameterValue(const char* paramName, char **paramValue)
{
    T2Debug("%s ++in \n", __FUNCTION__);
    parameterValStruct_t **valStructs = NULL;
    int valSize = 0;
    char *paramNames[1] = {NULL};
    if(!bus_handle && T2ERROR_SUCCESS != CCSPInterface_Init())
    {
        return T2ERROR_FAILURE;
    }

    paramNames[0] = strdup(paramName);
    if(T2ERROR_SUCCESS != getParameterValues(paramNames, 1, &valStructs, &valSize))
    {
        T2Error("Unable to get %s\n", paramName);
        return T2ERROR_FAILURE;
    }
    T2Debug("%s = %s\n", paramName, valStructs[0]->parameterValue);
    *paramValue = strdup(valStructs[0]->parameterValue);
    free(paramNames[0]);
    freeParamValueSt(valStructs, valSize);
    T2Debug("%s --out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

Vector* getProfileParameterValues(Vector *paramList)
{
    unsigned int i = 0;
    Vector *profileValueList = NULL;
    Vector_Create(&profileValueList);

    T2Debug("%s ++in\n", __FUNCTION__);

    char** paramNames = (char **)malloc(paramList->count * sizeof(char*));

    T2Info("TR-181 Param count : %d\n", paramList->count);
    for(; i < paramList->count; i++)
    {
        parameterValStruct_t **paramValues = NULL;
        int paramValCount = 0;
        profileValues *profVals = (profileValues *)malloc(sizeof(profileValues));
        paramNames[0] = strdup(((Param *)Vector_At(paramList, i))->alias);
        if(T2ERROR_SUCCESS == getParameterValues(paramNames, 1, &paramValues, &paramValCount))
        {
            T2Debug("ParameterName : %s Retrieved value count : %d\n", paramNames[0], paramValCount);
        }
        else
        {
            T2Error("Failed to retrieve param : %s\n", paramNames[0]);
        }
        free(paramNames[0]);

        profVals->paramValues = paramValues;
        profVals->paramValueCount = paramValCount;
        Vector_PushBack(profileValueList, profVals);
    }
    free(paramNames);

    T2Debug("%s --Out\n", __FUNCTION__);
    return profileValueList;
}


T2ERROR getObjectParameterCount(const char* objName, unsigned int *count)
{
    parameterInfoStruct_t **paramSts = NULL;
    if(T2ERROR_SUCCESS != getParameterNames(objName, &paramSts, count))
    {
        T2Error("Unable to get count of Device.WiFi.SSID.\n");
        return T2ERROR_FAILURE;
    }
    T2Debug("No.of parameters in object : %s = %d\n", objName, *count);
    freeParamInfoSt(paramSts, *count);
    return T2ERROR_SUCCESS;
}

T2ERROR registerForTelemetryEvents(TelemetryEventCallback eventCB)
{
    int ret;
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!bus_handle && T2ERROR_SUCCESS != CCSPInterface_Init())
    {
        return T2ERROR_FAILURE;
    }
    CcspBaseIf_SetCallback2(bus_handle, "telemetryDataSignal",
            eventCB, NULL);

    ret = CcspBaseIf_Register_Event(bus_handle, NULL, "telemetryDataSignal");
    if (ret != CCSP_Message_Bus_OK) {
        T2Error("CcspBaseIf_Register_Event failed\n");
        return T2ERROR_FAILURE;
    } else {
        T2Info("Registration with CCSP Bus successful, waiting for Telemetry Events from components...\n");
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR unregisterForTelemetryEvents()
{
    return T2ERROR_SUCCESS;
}
