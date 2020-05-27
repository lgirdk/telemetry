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

#ifndef  _CCSP_T2LOG_WRPPER_H_
#define  _CCSP_T2LOG_WRPPER_H_

#include <ccsp_trace.h>

#define DEBUG_INI_NAME  "/etc/debug.ini"
#define ENABLE_DEBUG_FLAG "/nvram/enable_t2_debug"

#define T2Error(...)                   T2Log(RDK_LOG_ERROR, __VA_ARGS__)
#define T2Info(...)                    T2Log(RDK_LOG_INFO, __VA_ARGS__)
#define T2Warning(...)                 T2Log(RDK_LOG_WARN, __VA_ARGS__)
#define T2Debug(...)                   T2Log(RDK_LOG_DEBUG, __VA_ARGS__)

void LOGInit();

void T2Log(unsigned int level, const char *msg, ...)
    __attribute__((format (printf, 2, 3)));

#endif
