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

#ifndef _TELEMETRY2_0_H_
#define _TELEMETRY2_0_H_

#define COMPONENT_NAME        "telemetry2_0"

#define INFINITE_TIMEOUT      (unsigned int)~0

typedef enum
{
    T2ERROR_SUCCESS,
    T2ERROR_FAILURE,
    T2ERROR_INVALID_PROFILE,
    T2ERROR_PROFILE_NOT_FOUND,
    T2ERROR_PROFILE_NOT_SET,
    T2ERROR_MAX_PROFILES_REACHED,
    T2ERROR_MEMALLOC_FAILED,
    T2ERROR_INVALID_ARGS,
    T2ERROR_INTERNAL_ERROR
}T2ERROR;

#define T2_CACHE_FILE    "/tmp/t2_caching_file"
#define T2_ATOM_CACHE_FILE    "/tmp/t2_atom_caching_file"
#define T2_CACHE_LOCK_FILE    "/tmp/t2_lock_file"

#endif /* _TELEMETRY2_0_H_ */
