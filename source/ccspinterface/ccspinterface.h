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

#ifndef _CCSPINTERFACE_H_
#define _CCSPINTERFACE_H_

#include <ccsp_base_api.h>
#include <stdio.h>
#include <vector.h>

#include "telemetry2_0.h"

typedef struct _profileValues
{
    parameterValStruct_t **paramValues;
    int paramValueCount;
}profileValues;

typedef void (*TelemetryEventCallback)(char* eventInfo, void *user_data);

T2ERROR getParameterValue(const char* paramName, char **paramValue);

T2ERROR getParameterValues(const char **paramNames, const int paramNamesCount, parameterValStruct_t ***valStructs, int *valSize);

T2ERROR getParameterNames(const char *objName, parameterInfoStruct_t ***paramNamesSt, int *paramNamesLength);

T2ERROR getObjectParameterCount(const char* objName, unsigned int *count);

void freeParamValueSt(parameterValStruct_t **valStructs, int valSize);

void freeParamInfoSt(parameterInfoStruct_t **paramNamesSt, int paramNamesLength);

T2ERROR registerForTelemetryEvents(TelemetryEventCallback eventCB);

Vector* getProfileParameterValues(Vector *paramList);

#endif
