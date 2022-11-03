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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t2parserxconf.h"
#include "xconfclient.h"
#include "busInterface.h"
#include "reportprofiles.h"
#include "t2log_wrapper.h"
#include "t2common.h"
#include "t2commonutils.h"

#define MT_EVENT_PATTERN   "<event>"
#define MT_EVENT_PATTERN_LENGTH 7
#define MT_TR181PARAM_PATTERN   "<message_bus>"
#define MT_TR181PATAM_PATTERN_LENGTH 13
#define SPLITMARKER_SUFFIX  "_split"
#define SCHEDULE_CRON_SIZE  15
#define DEFAULT_MAXRANDOMDELAY "300"

#define XCONF_END_POINT_PARAMETER "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TelemetryEndpoint.URL"
#define MAX_END_POINT_LEN 128

static int getScheduleInSeconds(const char* cronPattern) {
    unsigned int scheduleIntervalInMin = 15; // Default value from field settings
    unsigned int scheduleIntervalInSec = 0 ;
    char* input = NULL;
    if (NULL != cronPattern) {
        input = strdup(cronPattern);
        char* inputMatchPattern = strstr(input, "/");
        if (NULL != inputMatchPattern) {
            inputMatchPattern = inputMatchPattern + 1; // 1 offset value
            int inputMatchSize = strlen(inputMatchPattern);
            char *discardPattern = strstr(inputMatchPattern, " *");
            if (NULL != discardPattern) {
                int discardPatternLen = 0;
                discardPatternLen = strlen(discardPattern);
                int timeInMinDataLen = inputMatchSize - discardPatternLen;
                char scheduleString[10] = { '\0' };
                strncpy(scheduleString, inputMatchPattern, timeInMinDataLen);
                scheduleIntervalInMin = atoi(scheduleString);
            }

        }
	/* CID 159640 -Dereference before null check */
        free(input);
    }
    // Convert minutes to seconds
    scheduleIntervalInSec = scheduleIntervalInMin * 60 ;
    return scheduleIntervalInSec ;
}

/**
 * Comparator eventually called by qsort
 */
static int compareLogFileNames(const void *g1, const void *g2)
{
   GrepMarker** p1 = (GrepMarker**) g1 ;
   GrepMarker** p2 = (GrepMarker**) g2 ;

   if ( NULL != p1 && NULL != p2) {
       return strcmp((*p1)->logFile, (*p2)->logFile);
   } else {
       T2Error("compareLogFileNames : either p1 or p2 is NULL \n");
       return -1 ;
   }
}

static T2ERROR addParameter(ProfileXConf *profile, const char* name, const char* ref, const char* fileName, int skipFreq)
{
    if(skipFreq == -1)
    {
        // T2Debug("Adding TR-181 Parameter : %s\n", ref);
        Param *param = (Param *)malloc(sizeof(Param));
        memset(param, 0, sizeof(Param));
        param->name = strdup(name);
        param->alias = strdup(ref);

        Vector_PushBack(profile->paramList, param);
    }
    else if(fileName == NULL) //Event Marker
    {
        char *splitSuffix = NULL;
        // T2Debug("Adding Event Marker :: Param/Marker Name : %s ref/pattern/Comp : %s skipFreq : %d\n", name, ref, skipFreq);
        EventMarker *eMarker = (EventMarker *)malloc(sizeof(EventMarker));
        memset(eMarker, 0, sizeof(EventMarker));
        eMarker->markerName = strdup(name);
        eMarker->compName = strdup(ref);
        splitSuffix = strstr(name, SPLITMARKER_SUFFIX);
        if(splitSuffix != NULL && strcmp(splitSuffix, SPLITMARKER_SUFFIX) == 0)
        {
            eMarker->mType = MTYPE_XCONF_ABSOLUTE;
            eMarker->u.markerValue = NULL;
        }
        else
        {
            eMarker->mType = MTYPE_XCONF_COUNTER;
            eMarker->u.count = 0;
        }
        eMarker->skipFreq = skipFreq;

        Vector_PushBack(profile->eMarkerList, eMarker);
    }
    else //Grep Marker
    {
        char *splitSuffix = NULL;
        // T2Debug("Adding Grep Marker :: Param/Marker Name : %s ref/pattern/Comp : %s fileName : %s skipFreq : %d\n", name, ref, fileName, skipFreq);
        GrepMarker *gMarker = (GrepMarker *)malloc(sizeof(GrepMarker));
        memset(gMarker, 0, sizeof(GrepMarker));
        gMarker->markerName = strdup(name);
        gMarker->searchString = strdup(ref);
        gMarker->logFile = strdup(fileName);
        splitSuffix = strstr(name, SPLITMARKER_SUFFIX);
        if(splitSuffix != NULL && strcmp(splitSuffix, SPLITMARKER_SUFFIX) == 0)
        {
            gMarker->mType = MTYPE_ABSOLUTE;
            gMarker->u.markerValue = NULL;
        }
        else
        {
            gMarker->mType = MTYPE_COUNTER;
            gMarker->u.count = 0;
        }
        gMarker->skipFreq = skipFreq;

        Vector_PushBack(profile->gMarkerList, gMarker);
    }
    profile->paramNumOfEntries++;
    return T2ERROR_SUCCESS;
}

T2ERROR processConfigurationXConf(char* configData, ProfileXConf **localProfile) {
    // configData is the raw data from curl http get request to xconf

    T2Debug("%s ++in\n", __FUNCTION__);

    T2ERROR ret = T2ERROR_SUCCESS;
    int marker_count =0;
    T2Debug("config data = %s\n", configData);
    cJSON *json_root = cJSON_Parse(configData);
    cJSON *telemetry_data = cJSON_GetObjectItem(json_root, "urn:settings:TelemetryProfile");

//    cJSON *jprofileID = cJSON_GetObjectItem(telemetry_data, "id");
    cJSON *jprofileName = cJSON_GetObjectItem(telemetry_data, "telemetryProfile:name");
    cJSON *juploadUrl = cJSON_GetObjectItem(telemetry_data, "uploadRepository:URL");
    cJSON *jschedule = cJSON_GetObjectItem(telemetry_data, "schedule");

    cJSON *profileData = cJSON_GetObjectItem(telemetry_data, "telemetryProfile");
    if(profileData != NULL)
    {
       marker_count = cJSON_GetArraySize(profileData);
    }
//    if(jprofileID)
//        T2Debug("profile id = %s\n", jprofileID->valuestring);
    if(jprofileName)
        T2Debug("profile name = %s\n", jprofileName->valuestring);
    if(juploadUrl)
        T2Debug("upload url = %s\n", juploadUrl->valuestring);
    if(jschedule)
        T2Debug("schedule = %s\n", jschedule->valuestring);
    T2Debug("marker count = %d\n", marker_count);
//    if(jprofileID == NULL || jprofileName == NULL || juploadUrl == NULL || jschedule == NULL || profileData == NULL || marker_count == 0)
    if(jprofileName == NULL || juploadUrl == NULL || jschedule == NULL || profileData == NULL || marker_count == 0)
    {
        T2Error("Incomplete profile information, unable to create profile\n");
        cJSON_Delete(json_root);
        return T2ERROR_FAILURE;
    }

    telemetry_syscfg_set("UploadRepositoryURL",juploadUrl->valuestring);
    int reportIntervalInSec = getScheduleInSeconds(jschedule->valuestring);

    T2Info("Received profile name : %s with interval of : %d secs and upload url : %s \n", jprofileName->valuestring,
            reportIntervalInSec, juploadUrl->valuestring);

    ProfileXConf *profile = (ProfileXConf *)malloc(sizeof(ProfileXConf));
    memset(profile, 0, sizeof(ProfileXConf));
//    profile->id = strdup(jprofileID->valuestring);
    profile->name = strdup(jprofileName->valuestring);
    profile->reportingInterval = reportIntervalInSec;
    profile->isUpdated = false;

    profile->protocol = strdup("HTTP");
    profile->t2HTTPDest = (T2HTTP *)malloc(sizeof(T2HTTP));

    profile->t2HTTPDest->URL = strdup(juploadUrl->valuestring);
    profile->encodingType = strdup("JSON");

    /* Parse daily schedule interval and calculate the final schedule cron time */
    /* default value is "0 3 * * *" */
    cJSON *jdownloadStartTime = cJSON_GetObjectItem(json_root, "urn:settings:DCMSettings:DownloadConfig:StartTime");
    cJSON *jdownloadMaxRandomDelay = cJSON_GetObjectItem(json_root, "urn:settings:DCMSettings:DownloadConfig:MaxRandomDelay");

    char *jstatTime = NULL, *jmaxdelay = NULL;
    int startMinute = 0, finalHour = 0, finalMinute = 0, jmaxDelayInt = 0, random_number = 0;
    int startHour = 3;

    if(jdownloadStartTime != NULL)
    {
        if(jdownloadMaxRandomDelay != NULL)
        {
            jmaxdelay = strdup(jdownloadMaxRandomDelay->valuestring);
        }
        else
        {
            /* default MaxRandomDelay value is 300 ie. 5 hours */
            jmaxdelay = strdup(DEFAULT_MAXRANDOMDELAY);
        }

        /* need to seed the generator to generate a different random number every time */
        srand(time(NULL));
        random_number = rand();
        jmaxDelayInt = atoi(jmaxdelay);

        /* If MaxRandomDelay is specified as 0 in DCMResponse file then make it as 300 */
        if( jmaxDelayInt == 0 )
        {
            jmaxDelayInt = 300;
        }

        jmaxDelayInt = random_number % jmaxDelayInt;

        jstatTime = strdup(jdownloadStartTime->valuestring);
        if(jstatTime)
        {
            if(strlen(jstatTime) >= 5 )
            {
                /* split the time into hour and minute ex: "01:00" */
                jstatTime[2] = '\0';
                startHour = atoi(jstatTime);
                startMinute = atoi(&jstatTime[3]);
            }
        }

        /* If startHour is specified as 0 in DCMResponse file then make it as 3 */
        if(startHour == 0)
        {
            startHour = 3;
        }
        finalMinute = startMinute + jmaxDelayInt;
        finalHour = startHour;

        if(finalMinute >= 60)
        {
            finalHour = finalHour + ( finalMinute / 60 );
            finalMinute = finalMinute % 60;
        }

        if(finalHour >= 24)
        {
            finalHour = finalHour % 24;
        }
    }
    else
    {
        finalHour = 3;
    }

    if(jstatTime)
        free(jstatTime);
    if(jmaxdelay)
        free(jmaxdelay);

    char* scheduleInterval = malloc(SCHEDULE_CRON_SIZE);
    if(scheduleInterval)
    {
        snprintf(scheduleInterval, SCHEDULE_CRON_SIZE, "%d %d * * *", finalMinute, finalHour);
        profile->autoDownloadInterval = strdup(scheduleInterval);
        free(scheduleInterval);
    }

    Vector_Create(&profile->paramList);
    Vector_Create(&profile->eMarkerList);
    Vector_Create(&profile->gMarkerList);
    Vector_Create(&profile->cachedReportList);

    addParameter(profile, "mac", TR181_DEVICE_WAN_MAC, NULL, -1);
#if defined(ENABLE_RDKB_SUPPORT)
    addParameter(profile, "erouterIpv4", TR181_DEVICE_WAN_IPv4, NULL, -1);
    addParameter(profile, "erouterIpv6", TR181_DEVICE_WAN_IPv6, NULL, -1);
#elif defined(ENABLE_STB_SUPPORT)
    addParameter(profile, "StbIp", TR181_DEVICE_WAN_IPv6, NULL, -1);
    addParameter(profile, "receiverId", TR181_DEVICE_RECEIVER_ID, NULL, -1);
#else
    addParameter(profile, "StbIp", TR181_DEVICE_WAN_IPv6, NULL, -1);
#endif
    addParameter(profile, "PartnerId", TR181_DEVICE_PARTNER_ID, NULL, -1);
    addParameter(profile, "Version", TR181_DEVICE_FW_VERSION, NULL, -1);
    addParameter(profile, "AccountId", TR181_DEVICE_ACCOUNT_ID, NULL, -1);

    int markerIndex = 0;
    char* header = NULL;
    char* content = NULL;
    char* logfile = NULL;
    int skipFrequency = 0;
    size_t profileParamCount = 0;
    for (markerIndex = 0; markerIndex < marker_count; markerIndex++) {

        cJSON* pSubitem = cJSON_GetArrayItem(profileData, markerIndex);
        if (pSubitem != NULL) {
            cJSON* jHeader = cJSON_GetObjectItem(pSubitem, "header");
            cJSON* jContent = cJSON_GetObjectItem(pSubitem, "content");
            cJSON* jLogfile = cJSON_GetObjectItem(pSubitem, "type");
            cJSON* jSkipFrequency = cJSON_GetObjectItem(pSubitem, "pollingFrequency");
            if(jHeader)
                header = jHeader->valuestring;

            if(jContent)
                content = jContent->valuestring;

            if(jLogfile)
                logfile = jLogfile->valuestring;

            if(jSkipFrequency)
                skipFrequency = atoi(jSkipFrequency->valuestring);
            else
                skipFrequency = 0 ;

            if(skipFrequency > 0)
            {
                // T2Debug("Skip Frequency is Present, Need to do grep\n");

                ret = addParameter(profile, header, content, logfile, skipFrequency);
            }
            else if(!strncmp(logfile, MT_TR181PARAM_PATTERN, MT_TR181PATAM_PATTERN_LENGTH))
            {
                ret = addParameter(profile, header, content, NULL, -1);
            }
            else if(!strncmp(logfile, MT_EVENT_PATTERN, MT_EVENT_PATTERN_LENGTH))
            {
                ret = addParameter(profile, header, content, NULL, skipFrequency);
            }
            else
            {
                ret = addParameter(profile, header, content, logfile, skipFrequency);
            }

            if (ret != T2ERROR_SUCCESS) {
                T2Error("%s Error in adding parameter to profile %s \n", __FUNCTION__, profile->name);
                continue;
            }
            profileParamCount++;
        }
    }
    // Legacy DCA utils expects the list to be sorted based on logfile names
    Vector_Sort(profile->gMarkerList,  sizeof(GrepMarker*), compareLogFileNames);

    T2Info("Number of tr181params/markers successfully added in profile = %lu \n", (unsigned long)profileParamCount);

    cJSON_Delete(json_root);

    *localProfile = profile;

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}
