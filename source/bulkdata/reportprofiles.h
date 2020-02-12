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

#ifndef _BULKDATA_H_
#define _BULKDATA_H_

#include <stdbool.h>
#include <cjson/cJSON.h>
#include "telemetry2_0.h"

#define MIN_REPORT_INTERVAL     10
#define MAX_BULKDATA_PROFILES    10
#define MAX_PARAM_REFERENCES    100
#define DEFAULT_MAX_REPORT_SIZE 51200
#define MAX_CACHED_REPORTS 5


typedef enum
{
    XML,
    XDR,
    CSV,
    JSON,
    MESSAGE_PACK
}ENCODING_TYPE;


typedef enum
{
    JSONRF_OBJHIERARCHY,
    JSONRF_KEYVALUEPAIR
}JSONFormat;

typedef enum
{
    TIMESTAMP_UNIXEPOCH,
    TIMESTAMP_ISO_8601,
    TIMESTAMP_NONE
}TimeStampFormat;

typedef struct _BulkData
{
    bool enable;
    unsigned int minReportInterval;
    char *protocols;
    char *encodingTypes;
    bool parameterWildcardSupported;
    int maxNoOfProfiles;
    int maxNoOfParamReferences;
    unsigned int maxReportSize;
}BulkData;


typedef struct _ReportProfile
{
    char *hash;
    char *config;
}ReportProfile;

T2ERROR initReportProfiles();

T2ERROR ReportProfiles_uninit();

void ReportProfiles_ProcessReportProfilesBlob(cJSON *profiles_root);

#endif /* _BULKDATA_H_ */
