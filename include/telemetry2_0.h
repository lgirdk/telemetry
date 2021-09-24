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

#ifdef __cplusplus
extern "C" {
#endif

#define COMPONENT_NAME        "telemetry2_0"

// Data elements provided by telemetry 2.0
#define T2_ROOT_PARAMETER "Telemetry.ReportProfiles."
#define T2_EVENT_LIST_PARAM_SUFFIX ".EventMarkerList"
#define T2_EVENT_PARAM "Telemetry.ReportProfiles.EventMarker"
#define T2_PROFILE_UPDATED_NOTIFY "Telemetry.ReportProfiles.ProfilesUpdated"
#define T2_REPORT_PROFILE_PARAM "Device.X_RDKCENTRAL-COM_T2.ReportProfiles"
#define T2_REPORT_PROFILE_PARAM_MSG_PCK "Device.X_RDKCENTRAL-COM_T2.ReportProfilesMsgPack"

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
#define T2_CONFIG_READY    "/tmp/.t2ConfigReady"

#ifdef __cplusplus
}
#endif

#endif /* _TELEMETRY2_0_H_ */
