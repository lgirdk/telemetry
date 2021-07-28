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
#include <malloc.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "profilexconf.h"
#include "reportprofiles.h"
#include "t2eventreceiver.h"
#include "t2markers.h"
#include "t2log_wrapper.h"
#include "busInterface.h"
#include "curlinterface.h"
#include "scheduler.h"
#include "persistence.h"
#include "vector.h"
#include "dcautil.h"
#include "t2parserxconf.h"

#define T2REPORT_HEADER "T2"
#define T2REPORT_HEADERVAL  "1.0"

static bool initialized = false;
static ProfileXConf *singleProfile = NULL;
static pthread_mutex_t plMutex; /* TODO - we can remove plMutex most likely but first check that CollectAndReport doesn't cause issue */

static void freeConfig(void *data)
{
    if(data != NULL)
    {
        Config *config = (Config *)data;

        if(config->name)
            free(config->name);
        if(config->configData)
            free(config->configData);

        free(config);
    }
}

static void freeProfileXConf()
{
    if(singleProfile != NULL)
    {
        if(singleProfile->name)
            free(singleProfile->name);
        if(singleProfile->protocol)
            free(singleProfile->protocol);
        if(singleProfile->encodingType)
            free(singleProfile->encodingType);
        if(singleProfile->t2HTTPDest)
        {
            free(singleProfile->t2HTTPDest->URL);
            free(singleProfile->t2HTTPDest);
        }
        if(singleProfile->eMarkerList)
        {
            Vector_Destroy(singleProfile->eMarkerList, freeEMarker);
        }
        if(singleProfile->gMarkerList)
        {
            Vector_Destroy(singleProfile->gMarkerList, freeGMarker);
        }
        if(singleProfile->paramList)
        {
            Vector_Destroy(singleProfile->paramList, freeParam);
        }

        if(singleProfile->jsonReportObj) {
            cJSON_free(singleProfile->jsonReportObj);
            singleProfile->jsonReportObj = NULL;
        }

        // Data elements from this list is copied in new profile. So do not destroy the vector
        free(singleProfile->cachedReportList);
        free(singleProfile);
        singleProfile = NULL;
    }
}

static T2ERROR initJSONReportXconf(cJSON** jsonObj, cJSON **valArray)
{
    *jsonObj = cJSON_CreateObject();
    if(*jsonObj == NULL)
    {
        T2Error("Failed to create cJSON object\n");
        return T2ERROR_FAILURE;
    }

    cJSON_AddItemToObject(*jsonObj, "searchResult", *valArray = cJSON_CreateArray());

    cJSON *arrayItem = NULL;
    char *currenTime ;

    arrayItem = cJSON_CreateObject();
    cJSON_AddStringToObject(arrayItem, T2REPORT_HEADER, T2REPORT_HEADERVAL);
    cJSON_AddItemToArray(*valArray, arrayItem);

    arrayItem = cJSON_CreateObject();
    // Requirement from field triage to be a fixed string instead of actual profile name .
#if defined(ENABLE_RDKB_SUPPORT)
    cJSON_AddStringToObject(arrayItem, "Profile", "RDKB");
#else
    cJSON_AddStringToObject(arrayItem, "Profile", "RDKV");
#endif
    cJSON_AddItemToArray(*valArray, arrayItem);

    getTimeStamp(&currenTime);
    arrayItem = cJSON_CreateObject( );
    if (NULL != currenTime) {
        cJSON_AddStringToObject(arrayItem, "Time", currenTime);
        free(currenTime);
        currenTime = NULL;
    } else {
        cJSON_AddStringToObject(arrayItem, "Time", "Unknown");
    }
    cJSON_AddItemToArray(*valArray, arrayItem);

    return T2ERROR_SUCCESS;
}

static void* CollectAndReportXconf(void* data)
{
    pthread_mutex_lock(&plMutex);
    ProfileXConf* profile = singleProfile;

    Vector *profileParamVals = NULL;
    Vector *grepResultList = NULL;
    cJSON *valArray = NULL;
    char* jsonReport = NULL;
    FILE *fpReport = NULL;

    struct timespec startTime;
    struct timespec endTime;
    struct timespec elapsedTime;

    T2ERROR ret = T2ERROR_FAILURE;
    T2Info("%s ++in profileName : %s\n", __FUNCTION__, profile->name);


    clock_gettime(CLOCK_REALTIME, &startTime);
    if(!strcmp(profile->encodingType, "JSON"))
    {
        if(T2ERROR_SUCCESS != initJSONReportXconf(&profile->jsonReportObj, &valArray))
        {
            T2Error("Failed to initialize JSON Report\n");
            profile->reportInProgress = false;
            pthread_mutex_unlock(&plMutex);
            return NULL;
        }
        if(Vector_Size(profile->paramList) > 0)
        {

           profileParamVals = getProfileParameterValues(profile->paramList);
           T2Info("Fetch complete for TR-181 Object/Parameter Values for parameters \n");
           if(profileParamVals != NULL)
           {
              encodeParamResultInJSON(valArray, profile->paramList, profileParamVals);
           }
           Vector_Destroy(profileParamVals, freeProfileValues);
        }
        if(Vector_Size(profile->gMarkerList) > 0)
        {
           getGrepResults(profile->name, profile->gMarkerList, &grepResultList, profile->bClearSeekMap);
           T2Info("Grep complete for %lu markers \n", (unsigned long)Vector_Size(profile->gMarkerList));
           encodeGrepResultInJSON(valArray, grepResultList);
           Vector_Destroy(grepResultList, freeGResult);
        }
        if(Vector_Size(profile->eMarkerList) > 0)
        {
           encodeEventMarkersInJSON(valArray, profile->eMarkerList);
        }
        ret = prepareJSONReport(profile->jsonReportObj, &jsonReport);
        destroyJSONReport(profile->jsonReportObj);
        profile->jsonReportObj = NULL;

        if(ret != T2ERROR_SUCCESS)
        {
           T2Error("Unable to generate report for : %s\n", profile->name);
           profile->reportInProgress = false;
           pthread_mutex_unlock(&plMutex);
           return NULL;
        }

        fpReport = fopen(TELEMETRY_REPORT_FILE, "w");
        if(fpReport != NULL) 
        {
            fprintf(fpReport,"cJSON Report = %s\n", jsonReport);
            fclose(fpReport);
        }
        T2Info("cJSON Report written to file %s\n",TELEMETRY_REPORT_FILE);

        long size = strlen(jsonReport);
        T2Info("Report Size = %ld\n", size);
        if(profile->isUpdated)
        {
           T2Info("Profile is udpated, report is cached to send with updated Profile TIMEOUT\n");
           if(Vector_Size(profile->cachedReportList) == MAX_CACHED_REPORTS)
           {
              T2Debug("Max Cached Reports Limit Reached, Overwriting third recent report\n");
              char *thirdCachedReport = (char *)Vector_At(profile->cachedReportList, MAX_CACHED_REPORTS-3);
              Vector_RemoveItem(profile->cachedReportList, thirdCachedReport, NULL);
              free(thirdCachedReport);
           }
           Vector_PushBack(profile->cachedReportList, strdup(jsonReport));
           profile->reportInProgress = false;
           /* CID 187010: Dereference before null check */
           free(jsonReport);
	   jsonReport= NULL;
           T2Debug("%s --out\n", __FUNCTION__);
           pthread_mutex_unlock(&plMutex);
           return NULL;
        }
        if(size > DEFAULT_MAX_REPORT_SIZE)
        {
           T2Warning("Report size is exceeding the max limit : %d\n", DEFAULT_MAX_REPORT_SIZE);
        }
        if(strcmp(profile->protocol, "HTTP") == 0)
        {
           ret = sendReportOverHTTP(profile->t2HTTPDest->URL, jsonReport);

           if(ret == T2ERROR_FAILURE)
           {
              if(Vector_Size(profile->cachedReportList) == MAX_CACHED_REPORTS)
              {
                 T2Debug("Max Cached Reports Limit Reached, Overwriting third recent report\n");
                 char *thirdCachedReport = (char *)Vector_At(profile->cachedReportList, MAX_CACHED_REPORTS-3);
                 Vector_RemoveItem(profile->cachedReportList, thirdCachedReport, NULL);
                 free(thirdCachedReport);
              }
              Vector_PushBack(profile->cachedReportList, strdup(jsonReport));

              T2Info("Report Cached, No. of reportes cached = %lu\n", (unsigned long)Vector_Size(profile->cachedReportList));
           }
           else if(Vector_Size(profile->cachedReportList) > 0)
           {
               T2Info("Trying to send  %lu cached reports\n", (unsigned long)Vector_Size(profile->cachedReportList));
               ret = sendCachedReportsOverHTTP(profile->t2HTTPDest->URL, profile->cachedReportList);
           }
        }
        else
        {
           T2Error("Unsupported report send protocol : %s\n", profile->protocol);
        }
    }
    else
    {
        T2Error("Unsupported encoding format : %s\n", profile->encodingType);
    }
    clock_gettime(CLOCK_REALTIME, &endTime);
    getLapsedTime(&elapsedTime, &endTime, &startTime);
    T2Info("Elapsed Time for : %s = %lu.%lu (Sec.NanoSec)\n", profile->name, (unsigned long)elapsedTime.tv_sec, elapsedTime.tv_nsec);
    if(jsonReport)
    {
        free(jsonReport);
        jsonReport = NULL;
    }

    profile->reportInProgress = false;
    pthread_mutex_unlock(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return NULL;
}

T2ERROR ProfileXConf_init()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        Vector *configList = NULL;
        Config *config = NULL;

        initialized = true;
        pthread_mutex_init(&plMutex, NULL);

        Vector_Create(&configList);
        fetchLocalConfigs(XCONFPROFILE_PERSISTENCE_PATH, configList);

        if(Vector_Size(configList) > 1) // This is to address corner cases where if multiple configs are saved, we ignore them and wait for new config.
        {
          T2Info("Found multiple saved profiles, removing all from the disk and waiting for updated configuration\n");
          clearPersistenceFolder(XCONFPROFILE_PERSISTENCE_PATH);
        }
        else if(Vector_Size(configList) == 1)
        {
          config = Vector_At(configList, 0);
          ProfileXConf *profile = 0;
          T2Debug("Processing config with name : %s\n", config->name);
          T2Debug("Config Size = %lu\n", (unsigned long)strlen(config->configData));
          if(T2ERROR_SUCCESS == processConfigurationXConf(config->configData, &profile))
          {
              if(T2ERROR_SUCCESS == ProfileXConf_set(profile))
              {
                  T2Info("Successfully set new profile: %s\n", profile->name);
              }
              else
              {
                  T2Error("Failed to set new profile: %s\n", profile->name);
              }
          }
        }
        Vector_Destroy(configList, freeConfig);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR ProfileXConf_uninit()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized || !singleProfile)
    {
        T2Info("profile list is not initialized yet, ignoring\n");
        return T2ERROR_SUCCESS;
    }
    initialized = false;

    pthread_mutex_lock(&plMutex);
    if(singleProfile->reportInProgress)
    {
        T2Debug("Waiting for final report before uninit\n");
        pthread_join(singleProfile->reportThread, NULL);
        singleProfile->reportInProgress = false ;
        T2Info("Final report is completed, releasing profile memory\n");
    }
    freeProfileXConf();
    pthread_mutex_unlock(&plMutex);

    pthread_mutex_destroy(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR ProfileXConf_set(ProfileXConf *profile)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    T2ERROR ret = T2ERROR_FAILURE;

    pthread_mutex_lock(&plMutex);

    if(!singleProfile)
    {
        singleProfile = profile;
        singleProfile->reportInProgress = false ;
        int emIndex = 0;
        EventMarker *eMarker = NULL;
        for(;emIndex < Vector_Size(singleProfile->eMarkerList); emIndex++)
        {
            eMarker = (EventMarker *)Vector_At(singleProfile->eMarkerList, emIndex);
            addT2EventMarker(eMarker->markerName, eMarker->compName, singleProfile->name, eMarker->skipFreq);
        }
        if(registerProfileWithScheduler(singleProfile->name, singleProfile->reportingInterval, INFINITE_TIMEOUT, false, true) == T2ERROR_SUCCESS)
        {
            T2Info("Successfully set profile : %s\n", singleProfile->name);
            #ifdef _COSA_INTEL_XB3_ARM_
            if (Vector_Size(profile->gMarkerList) > 0 )
            {
                saveGrepConfig(singleProfile->name, profile->gMarkerList) ;
            }
            #endif
            ret = T2ERROR_SUCCESS;
        }
        else
        {
            T2Error("Unable to register profile : %s with Scheduler\n", singleProfile->name);
        }
    }
    else
    {
        T2Error("XConf profile already added, can't have more then 1 profile\n");
    }

    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

void ProfileXConf_updateMarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return ;
    }
    if(!singleProfile)
    {
        T2Error("Profile not found in %s\n", __FUNCTION__);
        return ;
    }
    int emIndex = 0;
    EventMarker *eMarker = NULL;
    pthread_mutex_lock(&plMutex);
    for(;emIndex < Vector_Size(singleProfile->eMarkerList); emIndex++)
    {
        eMarker = (EventMarker *)Vector_At(singleProfile->eMarkerList, emIndex);
        addT2EventMarker(eMarker->markerName, eMarker->compName, singleProfile->name, eMarker->skipFreq);
    }
    pthread_mutex_unlock(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
}

T2ERROR ProfileXConf_delete(ProfileXConf *profile)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return T2ERROR_FAILURE;
    }

    pthread_mutex_lock(&plMutex);
    if(!singleProfile)
    {
        T2Error("Profile not found in %s\n", __FUNCTION__);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_FAILURE;
    }

    pthread_mutex_unlock(&plMutex);
    if(profile != NULL){
        if(profile->cachedReportList != NULL) {
            singleProfile->isUpdated = true;
        }else {
            if(T2ERROR_SUCCESS != unregisterProfileFromScheduler(singleProfile->name)) {
                T2Error("Profile : %s failed to  unregister from scheduler\n", singleProfile->name);
            }
        }
    }

    pthread_mutex_lock(&plMutex);

    if(singleProfile->reportInProgress)
    {
        T2Info("Waiting for CollectAndReport to be complete : %s\n", singleProfile->name);
        pthread_join(singleProfile->reportThread, NULL);
        singleProfile->reportInProgress = false ;
    }

    int count = Vector_Size(singleProfile->cachedReportList);
    // Copy any cached message present in previous single profile to new profile
    if(profile != NULL) {
        profile->bClearSeekMap = singleProfile->bClearSeekMap ;
        profile->reportInProgress = false ;
        if(count > 0 && profile->cachedReportList != NULL) {
            T2Info("There are %d cached reports in the profile \n", count);
            int index = 0;
            while(index < count) {
                Vector_PushBack(profile->cachedReportList, (void *) Vector_At(singleProfile->cachedReportList, 0));
                Vector_RemoveItem(singleProfile->cachedReportList, (void *) Vector_At(singleProfile->cachedReportList, 0), NULL);/*TODO why this instead of Vector_destroy*/
                index++;
            }
        }
    }
#ifdef _COSA_INTEL_XB3_ARM_
    if (Vector_Size(singleProfile->gMarkerList) > 0 )
    {
        removeGrepConfig(singleProfile->name);
    }
#endif
    T2Info("removing profile : %s\n", singleProfile->name);
    freeProfileXConf();

    pthread_mutex_unlock(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

bool ProfileXConf_isSet()
{
    bool isSet = false;
    pthread_mutex_lock(&plMutex);

    if(singleProfile != NULL)
    {
        T2Debug("ProfileXConf is set\n");
        isSet = true;
    }

    pthread_mutex_unlock(&plMutex);
    return isSet;
}

bool ProfileXConf_isNameEqual(char* profileName)
{
    bool isName = false;
    pthread_mutex_lock(&plMutex);
    if(initialized)
    {
        if(singleProfile && !strcmp(singleProfile->name, profileName))
        { 
            isName = true;
        }
    }
    pthread_mutex_unlock(&plMutex);
    return isName;
}

char* ProfileXconf_getName() {
    char* profileName = NULL ;
    pthread_mutex_lock(&plMutex);
    if(initialized)
    {
        if(singleProfile)
        {
            profileName = strdup(singleProfile->name);
        }
    }
    pthread_mutex_unlock(&plMutex);
    return profileName;
}

void ProfileXConf_notifyTimeout(bool isClearSeekMap)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    int reportThreadStatus = 0 ;
    pthread_mutex_lock(&plMutex);
    if(!singleProfile)
    {
        T2Error("Profile not found in %s\n", __FUNCTION__);
        pthread_mutex_unlock(&plMutex);
        return ;
    }
    if(!singleProfile->reportInProgress)
    {
        singleProfile->bClearSeekMap = isClearSeekMap;
        singleProfile->reportInProgress = true;

        if (singleProfile->reportThread)
            pthread_detach(singleProfile->reportThread);

        reportThreadStatus = pthread_create(&singleProfile->reportThread, NULL, CollectAndReportXconf, NULL);
        if ( reportThreadStatus != 0 )
            T2Error("Failed to create report thread with error code = %d !!! \n", reportThreadStatus);
    }
    else
        T2Warning("Received profileTimeoutCb while previous callback is still in progress - ignoring the request\n");

    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);
}


T2ERROR ProfileXConf_storeMarkerEvent(T2Event *eventInfo)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    pthread_mutex_lock(&plMutex);
    if(!singleProfile)
    {
        T2Error("Profile not found in %s\n", __FUNCTION__);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_FAILURE;
    }
    pthread_mutex_unlock(&plMutex);

    int eventIndex = 0;
    EventMarker *lookupEvent = NULL;
    pthread_mutex_lock(&plMutex);
    for(; eventIndex < Vector_Size(singleProfile->eMarkerList); eventIndex++)
    {
        EventMarker *tempEventMarker = (EventMarker *)Vector_At(singleProfile->eMarkerList, eventIndex);
        if(!strcmp(tempEventMarker->markerName, eventInfo->name))
        {
            lookupEvent = tempEventMarker;
            break;
        }
    }
    pthread_mutex_unlock(&plMutex);

    if(lookupEvent != NULL)
    {
        switch(lookupEvent->mType)
        {
            case MTYPE_XCONF_COUNTER:
                lookupEvent->u.count++;
                T2Debug("Increment marker count to : %d\n", lookupEvent->u.count);
                break;

            case MTYPE_XCONF_ABSOLUTE:
            default:
                if(lookupEvent->u.markerValue)
                    free(lookupEvent->u.markerValue);
                lookupEvent->u.markerValue = strdup(eventInfo->value);
                T2Debug("New marker value saved : %s\n", lookupEvent->u.markerValue);
                break;
        }
    }
    else
    {
        T2Error("Event name : %s value : %s\n", eventInfo->name, eventInfo->value);
        T2Error("Event doens't match any marker information, shouldn't come here\n");
        return T2ERROR_FAILURE;
    }

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

