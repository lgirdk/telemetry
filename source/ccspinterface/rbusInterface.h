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

#ifndef _RBUSINTERFACE_H_
#define _RBUSINTERFACE_H_

#include <stdio.h>
#include <vector.h>

#include "busInterface.h"
#include "telemetry2_0.h"

T2ERROR getRbusParameterVal(const char* paramName, char **paramValue);

Vector* getRbusProfileParamValues(Vector *paramList);

T2ERROR registerRbusT2EventListener(TelemetryEventCallback eventCB);

T2ERROR unregisterRbusT2EventListener();

#endif
