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

#ifndef _BUSINTERFACE_H_
#define _BUSINTERFACE_H_

#include <vector.h>
#include "telemetry2_0.h"

#if defined(CCSP_SUPPORT_ENABLED)
#define CCSP_DBUS_INTERFACE_CR     "com.cisco.spvtg.ccsp.CR"

#ifdef _COSA_INTEL_USG_ATOM_
#define CCSP_COMPONENT_ID          "eRT.com.cisco.spvtg.ccsp.t2atom"       /* Use different componentId for Atom and Arm */
#else
#define CCSP_COMPONENT_ID          "eRT.com.cisco.spvtg.ccsp.telemetry"
#endif
#endif // CCSP_SUPPORT_ENABLED 

typedef struct
{
    char *parameterName;
    char *parameterValue;

} tr181ValStruct_t;

typedef struct _profileValues
{
    tr181ValStruct_t **paramValues;  // CCSP Base Api Object
    int paramValueCount;
}profileValues;

typedef void (*TelemetryEventCallback)(char* eventInfo, char* user_data);

typedef void (*T2EventMarkerListCallback)(const char* componentName, void **eventMarkerList);

typedef T2ERROR (*dataModelCallBack)(char* dataBlob);

bool isRbusEnabled();

T2ERROR getParameterValue(const char* paramName, char **paramValue);

Vector* getProfileParameterValues(Vector *paramList);

void freeParamValueSt(tr181ValStruct_t **valStructs, int valSize);

T2ERROR registerForTelemetryEvents(TelemetryEventCallback eventCB);

// Needs to be called only in rBus mode
T2ERROR regDEforCompEventList(const char* componentName, T2EventMarkerListCallback callBackHandler) ;

void unregisterDEforCompEventList();

T2ERROR regDEforProfileDataModel(dataModelCallBack dmCallBack);

T2ERROR publishEventsProfileUpdates() ;

T2ERROR busUninit() ;

#endif /* SOURCE_CCSPINTERFACE_BUSINTERFACE_H_ */
