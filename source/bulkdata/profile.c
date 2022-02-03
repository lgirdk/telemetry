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
#include <unistd.h>

#include "profile.h"
#include "reportprofiles.h"
#include "t2eventreceiver.h"
#include "t2markers.h"
#include "t2log_wrapper.h"
#include "busInterface.h"
#include "curlinterface.h"
#include "rbusmethodinterface.h"
#include "scheduler.h"
#include "persistence.h"
#include "vector.h"
#include "dcautil.h"
#include "t2parser.h"
#include "rbusInterface.h"

static bool initialized = false;
static Vector *profileList;
static pthread_mutex_t plMutex;
static pthread_mutex_t reportLock;

static void freeRequestURIparam(void *data)
{
    if(data != NULL)
    {
        HTTPReqParam *hparam = (HTTPReqParam *)data;
        if(hparam->HttpName)
            free(hparam->HttpName);
        if(hparam->HttpRef)
            free(hparam->HttpRef);
        if(hparam->HttpValue)
            free(hparam->HttpValue);
        free(hparam);
    }
}

static void freeReportProfileConfig(void *data)
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

static void freeProfile(void *data)
{
    T2Debug("%s ++in \n", __FUNCTION__);
    if(data != NULL)
    {
        Profile *profile = (Profile *)data;
        if(profile->name)
            free(profile->name);
        if(profile->hash)
            free(profile->hash);
        if(profile->protocol)
            free(profile->protocol);
        if(profile->encodingType)
            free(profile->encodingType);
        if(profile->Description)
             free(profile->Description);
        if(profile->version)
             free(profile->version);
        if(profile->jsonEncoding)
            free(profile->jsonEncoding);
        if(profile->t2HTTPDest)
        {
            free(profile->t2HTTPDest->URL);
            if(profile->t2HTTPDest->RequestURIparamList) {
                Vector_Destroy(profile->t2HTTPDest->RequestURIparamList, freeRequestURIparam);
            }
            free(profile->t2HTTPDest);
        }
        if(profile->t2RBUSDest){
            if(profile->t2RBUSDest->rbusMethodName){
                memset(profile->t2RBUSDest->rbusMethodName , 0 , strlen(profile->t2RBUSDest->rbusMethodName));
                free(profile->t2RBUSDest->rbusMethodName);
                profile->t2RBUSDest->rbusMethodName = NULL ;
            }
            if(profile->t2RBUSDest->rbusMethodParamList){
                // TBD determine whether data is simple string before passing free as cleanup function
                Vector_Destroy(profile->t2RBUSDest->rbusMethodParamList, free);
            }
            free(profile->t2RBUSDest);
        }
        if(profile->eMarkerList)
        {
            Vector_Destroy(profile->eMarkerList, freeEMarker);
        }
        if(profile->gMarkerList)
        {
            Vector_Destroy(profile->gMarkerList, freeGMarker);
        }
        if(profile->paramList)
        {
            Vector_Destroy(profile->paramList, freeParam);
        }
        if (profile->staticParamList)
        {
            Vector_Destroy(profile->staticParamList, freeStaticParam);
        }
        if(profile->triggerConditionList)
        {
            Vector_Destroy(profile->triggerConditionList, freeTriggerCondition);
        }
        free(profile);
    }
    T2Debug("%s ++out \n", __FUNCTION__);
}

static T2ERROR getProfile(const char *profileName, Profile **profile)
{
    int profileIndex = 0;
    Profile *tempProfile = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);
    if(profileName == NULL)
    {
        T2Error("profileName is null\n");
        return T2ERROR_FAILURE;
    }
    for(; profileIndex < Vector_Size(profileList); profileIndex++)
    {
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        if(strcmp(tempProfile->name, profileName) == 0)
        {
            *profile = tempProfile;
            T2Debug("%s --out\n", __FUNCTION__);
            return T2ERROR_SUCCESS;
        }
    }
    T2Error("Profile with Name : %s not found\n", profileName);
    return T2ERROR_PROFILE_NOT_FOUND;
}

static T2ERROR initJSONReportProfile(cJSON** jsonObj, cJSON **valArray)
{
    *jsonObj = cJSON_CreateObject();
    if(*jsonObj == NULL)
    {
        T2Error("Failed to create cJSON object\n");
        return T2ERROR_FAILURE;
    }

    cJSON_AddItemToObject(*jsonObj, "Report", *valArray = cJSON_CreateArray());


    return T2ERROR_SUCCESS;
}

T2ERROR profileWithNameExists(const char *profileName, bool *bProfileExists)
{
    int profileIndex = 0;
    Profile *tempProfile = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return T2ERROR_FAILURE;
    }
    if(profileName == NULL)
    {
        T2Error("profileName is null\n");
        *bProfileExists = false;
        return T2ERROR_FAILURE;
    }
    pthread_mutex_lock(&plMutex);
    for(; profileIndex < Vector_Size(profileList); profileIndex++)
    {
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        if(strcmp(tempProfile->name, profileName) == 0)
        {
            *bProfileExists = true;
            pthread_mutex_unlock(&plMutex);
            return T2ERROR_SUCCESS;
        }
    }
    *bProfileExists = false;
    pthread_mutex_unlock(&plMutex);
    T2Error("Profile with Name : %s not found\n", profileName);
    return T2ERROR_PROFILE_NOT_FOUND;
}

void getMarkerCompRbusSub(bool subscription){
    T2Debug("%s ++in\n", __FUNCTION__);
    Vector* eventMarkerListForComponent = NULL;
    getComponentMarkerList(T2REPORTCOMPONENT, (void**)&eventMarkerListForComponent);
    int length = Vector_Size(eventMarkerListForComponent);
    int i;
        if(length > 0)  {
            for(i = 0; i < length; ++i ) {
                char* markerName = (char *) Vector_At(eventMarkerListForComponent, i);
                if(markerName) {
                    int ret = T2RbusReportEventConsumer(markerName,subscription);
                    T2Debug("%d T2RbusEventReg with name = %s: subscription = %s ret %d \n", i, markerName,(subscription?"Subscribe":"Un-Subscribe"),ret);
                }
                else
                    T2Error("Error while retrieving Marker Name at index : %d \n",i);
            }
            Vector_Destroy(eventMarkerListForComponent, free);
        }
    T2Debug("%s --out\n", __FUNCTION__);
}

static void* CollectAndReport(void* data)
{
    if(data == NULL)
    {
        T2Error("data passed is NULL can't identify the profile, existing from CollectAndReport\n");
        return NULL;
    }
    Profile* profile = (Profile *)data;
    profile->reportInProgress = true;

    Vector *profileParamVals = NULL;
    Vector *grepResultList = NULL;
    cJSON *valArray = NULL;
    char* jsonReport = NULL;
    cJSON *triggercondition = NULL;

    struct timespec startTime;
    struct timespec endTime;
    struct timespec elapsedTime;

    T2ERROR ret = T2ERROR_FAILURE;
    T2Info("%s ++in profileName : %s\n", __FUNCTION__, profile->name);


    clock_gettime(CLOCK_REALTIME, &startTime);
    if(!strcmp(profile->encodingType, "JSON") || !strcmp(profile->encodingType, "MessagePack"))
    {
        JSONEncoding *jsonEncoding = profile->jsonEncoding;
        if (jsonEncoding->reportFormat != JSONRF_KEYVALUEPAIR)
        {
            //TODO: Support 'ObjectHierarchy' format in RDKB-26154.
            T2Error("Only JSON name-value pair format is supported \n");
            profile->reportInProgress = false;
            return NULL;
        }
        if(profile->triggerReportOnCondition && (profile->jsonReportObj != NULL))
        {
            triggercondition=profile->jsonReportObj;
            profile->jsonReportObj = NULL;
        }
        if(T2ERROR_SUCCESS != initJSONReportProfile(&profile->jsonReportObj, &valArray))
        {
            T2Error("Failed to initialize JSON Report\n");
            profile->reportInProgress = false;
            return NULL;
        }
        else
        {
            if(Vector_Size(profile->staticParamList) > 0)
            {
                T2Debug(" Adding static Parameter Values to Json report\n");
                encodeStaticParamsInJSON(valArray, profile->staticParamList);
            }
            if(Vector_Size(profile->paramList) > 0)
            {
                T2Debug("Fetching TR-181 Object/Parameter Values\n");
                profileParamVals = getProfileParameterValues(profile->paramList);
                if(profileParamVals != NULL)
                {
                    encodeParamResultInJSON(valArray, profile->paramList, profileParamVals);
                }
                Vector_Destroy(profileParamVals, freeProfileValues);
            }
            if(Vector_Size(profile->gMarkerList) > 0)
            {
                getGrepResults(profile->name, profile->gMarkerList, &grepResultList, profile->bClearSeekMap);
                encodeGrepResultInJSON(valArray, grepResultList);
                Vector_Destroy(grepResultList, freeGResult);
            }
            if(Vector_Size(profile->eMarkerList) > 0)
            {
                encodeEventMarkersInJSON(valArray, profile->eMarkerList);
            }
            if(profile->triggerReportOnCondition && (triggercondition != NULL))
                cJSON_AddItemToArray(valArray, triggercondition);
            ret = prepareJSONReport(profile->jsonReportObj, &jsonReport);
	    destroyJSONReport(profile->jsonReportObj);
            profile->jsonReportObj = NULL;

            if(ret != T2ERROR_SUCCESS)
            {
                T2Error("Unable to generate report for : %s\n", profile->name);
                profile->reportInProgress = false;
                return NULL;
            }
            long size = strlen(jsonReport);
            T2Info("cJSON Report = %s\n", jsonReport);
	    cJSON *root = cJSON_Parse(jsonReport);
	    if(root != NULL){
		     cJSON *array = cJSON_GetObjectItem(root, "Report");
		     if(cJSON_GetArraySize(array) == 0){
			     T2Warning("Array size of Report is %d. Report is empty. Cannot send empty report\n", cJSON_GetArraySize(array));
			     profile->reportInProgress = false;
			     cJSON_Delete(root);
			     return NULL;
		     }
		     cJSON_Delete(root);
	    }

            T2Info("Report Size = %ld\n", size);
            if(size > DEFAULT_MAX_REPORT_SIZE) {
                T2Warning("Report size is exceeding the max limit : %d\n", DEFAULT_MAX_REPORT_SIZE);
            }
            if(strcmp(profile->protocol, "HTTP") == 0 || strcmp(profile->protocol, "RBUS_METHOD") == 0 ) {
                char *httpUrl = NULL ;
                if ( strcmp(profile->protocol, "HTTP") == 0 ) {
                    httpUrl = prepareHttpUrl(profile->t2HTTPDest); /* Append URL with http properties */
                    ret = sendReportOverHTTP(httpUrl, jsonReport);
                } else {
                    ret = sendReportsOverRBUSMethod(profile->t2RBUSDest->rbusMethodName, profile->t2RBUSDest->rbusMethodParamList, jsonReport);
                }
                if(strcmp(profile->protocol, "HTTP") == 0 ) { /* Adding this condition to avoid caching of reports and retry attempts for RBUS_METHOD protocol */
                    if(ret == T2ERROR_FAILURE) {
                        if(Vector_Size(profile->cachedReportList) == MAX_CACHED_REPORTS) {
                            T2Debug("Max Cached Reports Limit Reached, Overwriting third recent report\n");
                            char *thirdCachedReport = (char *) Vector_At(profile->cachedReportList, MAX_CACHED_REPORTS - 3);
                            Vector_RemoveItem(profile->cachedReportList, thirdCachedReport, NULL);
                            free(thirdCachedReport);
                        }
                        Vector_PushBack(profile->cachedReportList, jsonReport);

                        T2Info("Report Cached, No. of reportes cached = %lu\n", (unsigned long)Vector_Size(profile->cachedReportList));
                    }else if(Vector_Size(profile->cachedReportList) > 0) {
                        T2Info("Trying to send  %lu cached reports\n", (unsigned long)Vector_Size(profile->cachedReportList));
                        if ( strcmp(profile->protocol, "HTTP") == 0 ) {
                            ret = sendCachedReportsOverHTTP(httpUrl, profile->cachedReportList);
                        } else {
                            ret = sendCachedReportsOverRBUSMethod(profile->t2RBUSDest->rbusMethodName, profile->t2RBUSDest->rbusMethodParamList, profile->cachedReportList);
                        }
                    }
	        }
                if (httpUrl) {
                    free(httpUrl);
                    httpUrl = NULL ;
                }
            }
            else
            {
                T2Error("Unsupported report send protocol : %s\n", profile->protocol);
            }
        }
    }
    else
    {
        T2Error("Unsupported encoding format : %s\n", profile->encodingType);
    }
    clock_gettime(CLOCK_REALTIME, &endTime);
    getLapsedTime(&elapsedTime, &endTime, &startTime);
    T2Info("Elapsed Time for : %s = %lu.%lu (Sec.NanoSec)\n", profile->name, elapsedTime.tv_sec, elapsedTime.tv_nsec);
    if(ret == T2ERROR_SUCCESS && jsonReport)
    {
        free(jsonReport);
        jsonReport = NULL;
    }

    profile->reportInProgress = false;
    T2Info("%s --out\n", __FUNCTION__);
    return NULL;
}

void NotifyTimeout(const char* profileName, bool isClearSeekMap)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    pthread_mutex_lock(&plMutex);

    Profile *profile = NULL;
    if(T2ERROR_SUCCESS != getProfile(profileName, &profile))
    {
        T2Error("Profile : %s not found\n", profileName);
        pthread_mutex_unlock(&plMutex);
        return ;
    }

    pthread_mutex_unlock(&plMutex);

    T2Info("%s: profile %s is in %s state\n", __FUNCTION__, profileName, profile->enable ? "Enabled" : "Disabled");
    if(profile->enable && !profile->reportInProgress)
    {
        /* To avoid previous report thread to go into zombie state, mark it detached. */
        if (profile->reportThread)
            pthread_detach(profile->reportThread);

        profile->bClearSeekMap = isClearSeekMap;
        pthread_create(&profile->reportThread, NULL, CollectAndReport, (void*)profile);
    }
    else
    {
        T2Warning("Either profile is disabled or report generation still in progress - ignoring the request\n");
    }

    T2Debug("%s --out\n", __FUNCTION__);
}


T2ERROR Profile_storeMarkerEvent(const char *profileName, T2Event *eventInfo)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    pthread_mutex_lock(&plMutex);
    Profile *profile = NULL;
    if(T2ERROR_SUCCESS != getProfile(profileName, &profile))
    {
        T2Error("Profile : %s not found\n", profileName);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_FAILURE;
    }
    pthread_mutex_unlock(&plMutex);
    if(!profile->enable)
    {
        T2Warning("Profile : %s is disabled, ignoring the event\n", profileName);
        return T2ERROR_FAILURE;
    }
    int eventIndex = 0;
    EventMarker *lookupEvent = NULL;
    for(; eventIndex < Vector_Size(profile->eMarkerList); eventIndex++)
    {
        EventMarker *tempEventMarker = (EventMarker *)Vector_At(profile->eMarkerList, eventIndex);
        if(!strcmp(tempEventMarker->markerName, eventInfo->name))
        {
            lookupEvent = tempEventMarker;
            break;
        }
    }
    if(lookupEvent != NULL)
    {
        switch(lookupEvent->mType)
        {
            case MTYPE_COUNTER:
                lookupEvent->u.count++;
                T2Debug("Increment marker count to : %d\n", lookupEvent->u.count);
                break;

            case MTYPE_ABSOLUTE:
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

T2ERROR addProfile(Profile *profile)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return T2ERROR_FAILURE;
    }
    pthread_mutex_lock(&plMutex);
    Vector_PushBack(profileList, profile);

#ifdef _COSA_INTEL_XB3_ARM_
    if (Vector_Size(profile->gMarkerList) > 0)
        saveGrepConfig(profile->name, profile->gMarkerList) ;
#endif
    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR enableProfile(const char *profileName)
{
    T2Debug("%s ++in \n", __FUNCTION__);
    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return T2ERROR_FAILURE;
    }
    pthread_mutex_lock(&plMutex);
    Profile *profile = NULL;
    if(T2ERROR_SUCCESS != getProfile(profileName, &profile))
    {
        T2Error("Profile : %s not found\n", profileName);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_FAILURE;
    }
    if(profile->enable)
    {
        T2Info("Profile : %s is already enabled - ignoring duplicate request\n", profileName);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_SUCCESS;
    }
    else
    {
        profile->enable = true;

        int emIndex = 0;
        EventMarker *eMarker = NULL;
        for(;emIndex < Vector_Size(profile->eMarkerList); emIndex++)
        {
            eMarker = (EventMarker *)Vector_At(profile->eMarkerList, emIndex);
            addT2EventMarker(eMarker->markerName, eMarker->compName, profile->name, eMarker->skipFreq);
        }
        if(registerProfileWithScheduler(profile->name, profile->reportingInterval, profile->activationTimeoutPeriod, true) != T2ERROR_SUCCESS)
        {
            profile->enable = false;
            T2Error("Unable to register profile : %s with Scheduler\n", profileName);
            pthread_mutex_unlock(&plMutex);
            return T2ERROR_FAILURE;
        }

        T2ER_StartDispatchThread();

        T2Info("Successfully enabled profile : %s\n", profileName);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    pthread_mutex_unlock(&plMutex);
    return T2ERROR_SUCCESS;
}


void updateMarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);

    int profileIndex = 0;
    Profile *tempProfile = NULL;

    for(; profileIndex < Vector_Size(profileList); profileIndex++)
    {
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        if(tempProfile->enable)
        {
            T2Debug("Updating component map for profile %s \n", tempProfile->name);
            int emIndex = 0;
            EventMarker *eMarker = NULL;
            for(;emIndex < Vector_Size(tempProfile->eMarkerList); emIndex++)
            {
                eMarker = (EventMarker *)Vector_At(tempProfile->eMarkerList, emIndex);
                addT2EventMarker(eMarker->markerName, eMarker->compName, tempProfile->name, eMarker->skipFreq);
            }
        }
    }
    T2Debug("%s --out\n", __FUNCTION__);
}

T2ERROR disableProfile(const char *profileName, bool *isDeleteRequired) {
    T2Debug("%s ++in \n", __FUNCTION__);

    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return T2ERROR_FAILURE;
    }

    pthread_mutex_lock(&plMutex);
    Profile *profile = NULL;
    if(T2ERROR_SUCCESS != getProfile(profileName, &profile))
    {
        T2Error("Profile : %s not found\n", profileName);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_FAILURE;
    }

    if (profile->generateNow) {
        *isDeleteRequired = true;
    } else {
        profile->enable = false;
    }
    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);

    return T2ERROR_SUCCESS;
}

T2ERROR deleteAllProfiles(bool delFromDisk) {
    T2Debug("%s ++in\n", __FUNCTION__);

    int count = 0;
    int profileIndex = 0;
    Profile *tempProfile = NULL;

    if(profileList == NULL)
    {
        T2Error("profile list is not initialized yet or profileList is empty, ignoring\n");
        return T2ERROR_FAILURE;
    }

    pthread_mutex_lock(&plMutex);
    count = Vector_Size(profileList);
    pthread_mutex_unlock(&plMutex);

    for(; profileIndex < count; profileIndex++)
    {
        pthread_mutex_lock(&plMutex);
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        tempProfile->enable = false;
        pthread_mutex_unlock(&plMutex);

	if(T2ERROR_SUCCESS != unregisterProfileFromScheduler(tempProfile->name))
        {
            T2Error("Profile : %s failed to  unregister from scheduler\n", tempProfile->name);
        }

        pthread_mutex_lock(&plMutex);
        if (tempProfile->reportThread)
            pthread_join(tempProfile->reportThread, NULL);

        if (Vector_Size(tempProfile->gMarkerList) > 0)
            removeGrepConfig(tempProfile->name);
        pthread_mutex_unlock(&plMutex);
        if(delFromDisk == true){
           removeProfileFromDisk(REPORTPROFILES_PERSISTENCE_PATH, tempProfile->name);
	}   
    }
    if(delFromDisk == true){
       removeProfileFromDisk(REPORTPROFILES_PERSISTENCE_PATH, MSGPACK_REPORTPROFILES_PERSISTENT_FILE);
    }   

    pthread_mutex_lock(&plMutex);
    T2Debug("Deleting all profiles from the profileList\n");
    Vector_Destroy(profileList, freeProfile);
    profileList = NULL;
    Vector_Create(&profileList);
    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);

    return T2ERROR_SUCCESS;
}

bool isProfileEnabled(const char *profileName)
{
     bool is_profile_enable = false; 
     Profile *get_profile = NULL;
     pthread_mutex_lock(&plMutex);
     if(T2ERROR_SUCCESS != getProfile(profileName, &get_profile))
     {
         T2Error("Profile : %s not found\n", profileName);
         T2Debug("%s --out\n", __FUNCTION__);
         pthread_mutex_unlock(&plMutex);
         return false;
     }
     is_profile_enable = get_profile->enable;
     T2Debug("is_profile_enable = %d \n",is_profile_enable);
     pthread_mutex_unlock(&plMutex);
     return is_profile_enable;
}


T2ERROR deleteProfile(const char *profileName)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        T2Error("profile list is not initialized yet, ignoring\n");
        return T2ERROR_FAILURE;
    }

    Profile *profile = NULL;
    pthread_mutex_lock(&plMutex);
    if(T2ERROR_SUCCESS != getProfile(profileName, &profile))
    {
        T2Error("Profile : %s not found\n", profileName);
        pthread_mutex_unlock(&plMutex);
        return T2ERROR_FAILURE;
    }

    if(profile->enable)
        profile->enable = false;
    
    pthread_mutex_unlock(&plMutex);
    if(T2ERROR_SUCCESS != unregisterProfileFromScheduler(profileName))
    {
        T2Info("Profile : %s already removed from scheduler\n", profileName);
    }

    T2Info("Waiting for CollectAndReport to be complete : %s\n", profileName);
    pthread_mutex_lock(&plMutex);
    if (profile->reportThread) {
        pthread_join(profile->reportThread, NULL);
    }

    if(Vector_Size(profile->triggerConditionList) > 0){
        rbusT2ConsumerUnReg(profile->triggerConditionList);
    }

    if (Vector_Size(profile->gMarkerList) > 0)
        removeGrepConfig((char*)profileName);

    T2Info("removing profile : %s from profile list\n", profile->name);
    Vector_RemoveItem(profileList, profile, freeProfile);

    pthread_mutex_unlock(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

void sendLogUploadInterruptToScheduler()
{
    int profileIndex = 0;
    Profile *tempProfile = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);

    pthread_mutex_lock(&plMutex);
    for(; profileIndex < Vector_Size(profileList); profileIndex++)
    {
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        if (Vector_Size(tempProfile->gMarkerList) > 0)
        {
            SendInterruptToTimeoutThread(tempProfile->name);
        }
    }
    pthread_mutex_unlock(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
}

static void loadReportProfilesFromDisk()
{
#if defined(FEATURE_SUPPORT_WEBCONFIG)
    T2Info("loadReportProfilesFromDisk \n");
    char filePath[REPORTPROFILES_FILE_PATH_SIZE] = {'\0'};
    snprintf(filePath, sizeof(filePath), "%s%s", REPORTPROFILES_PERSISTENCE_PATH, MSGPACK_REPORTPROFILES_PERSISTENT_FILE);
    /* CID: 157386 Time of check time of use (TOCTOU) */
    FILE *fp;
    fp = fopen (filePath, "rb");
    if(fp != NULL){
        T2Info("Msgpack: loadReportProfilesFromDisk \n");
        struct __msgpack__ msgpack;
        fseek(fp, 0L, SEEK_END);
        msgpack.msgpack_blob_size = ftell(fp);
        if(msgpack.msgpack_blob_size < 0)
        {
          T2Error("Unable to detect the file pointer position for file %s\n", filePath);
          fclose(fp);
          return;
        }
        msgpack.msgpack_blob = malloc(sizeof(char) * msgpack.msgpack_blob_size);
        if (NULL == msgpack.msgpack_blob) {
          T2Error("Unable to allocate %d bytes of memory at Line %d on %s \n",
                    msgpack.msgpack_blob_size, __LINE__, __FILE__);
          fclose (fp);
          return;
        }
        fseek(fp, 0L, SEEK_SET);
        if(fread(msgpack.msgpack_blob, sizeof(char), msgpack.msgpack_blob_size, fp) < msgpack.msgpack_blob_size)
        {
          T2Error("fread is returning fewer bytes than expected from the file %s\n", filePath);
          free(msgpack.msgpack_blob);
          fclose(fp);
          return;
        }
        fclose (fp);
        __ReportProfiles_ProcessReportProfilesMsgPackBlob((void *)&msgpack);
        free(msgpack.msgpack_blob);
        return;
    }
    T2Info("JSON: loadReportProfilesFromDisk \n");
#endif
	
    int configIndex = 0;
    Vector *configList = NULL;
    Config *config = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);

    Vector_Create(&configList);
    fetchLocalConfigs(REPORTPROFILES_PERSISTENCE_PATH, configList);

     for(; configIndex < Vector_Size(configList); configIndex++)
     {
         config = Vector_At(configList, configIndex);
         Profile *profile = 0;
         T2Debug("Processing config with name : %s\n", config->name);
         T2Debug("Config Size = %lu\n", (unsigned long)strlen(config->configData));
         if(T2ERROR_SUCCESS == processConfiguration(&config->configData, config->name, NULL, &profile))
         {
             if(T2ERROR_SUCCESS == addProfile(profile))
             {
                 T2Info("Successfully created/added new profile : %s\n", profile->name);
                 if(T2ERROR_SUCCESS != enableProfile(profile->name))
                 {
                     T2Error("Failed to enable profile name : %s\n", profile->name);
                 }
             }
             else
             {
                 T2Error("Unable to create and add new profile for name : %s\n", config->name);
             }
         }
     }
     T2Info("Completed processing %lu profiles on the disk,trying to fetch new/updated profiles\n", (unsigned long)Vector_Size(configList));
     Vector_Destroy(configList, freeReportProfileConfig);

    T2Debug("%s --out\n", __FUNCTION__);
}

T2ERROR initProfileList()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(initialized)
    {
        T2Info("profile list is already initialized\n");
        return T2ERROR_SUCCESS;
    }
    initialized = true;
    pthread_mutex_init(&plMutex, NULL);
    pthread_mutex_init(&reportLock, NULL);

    Vector_Create(&profileList);

    loadReportProfilesFromDisk();

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

int getProfileCount()
{
    int count = 0;
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!initialized)
    {
        T2Info("profile list isn't initialized\n");
        return count;
    }
    pthread_mutex_lock(&plMutex);
    count = Vector_Size(profileList);
    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);
    return count;
}

hash_map_t *getProfileHashMap()
{
    int profileIndex = 0;
    hash_map_t *profileHashMap = NULL;
    Profile *tempProfile = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);

    pthread_mutex_lock(&plMutex);
    profileHashMap = hash_map_create();
    for(; profileIndex < Vector_Size(profileList); profileIndex++)
    {
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        char *profileName = strdup(tempProfile->name);
        char *profileHash = strdup(tempProfile->hash);
        hash_map_put(profileHashMap, profileName, profileHash);
    }
    pthread_mutex_unlock(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);
    return profileHashMap;
}

T2ERROR uninitProfileList()
{
    T2Debug("%s ++in\n", __FUNCTION__);

    if(!initialized)
    {
        T2Info("profile list is not initialized yet, ignoring\n");
        return T2ERROR_SUCCESS;
    }

    initialized = false;
    deleteAllProfiles(false); // avoid removing multiProfiles from Disc

    pthread_mutex_destroy(&reportLock);
    pthread_mutex_destroy(&plMutex);

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR registerTriggerConditionConsumer()
{

    T2Debug("%s ++in\n", __FUNCTION__);
    #define MAX_RETRY_COUNT 3
    int profileIndex = 0;
    int retry_count = 0;
    int retry = 0;
    int timer = 16;
    int ret = T2ERROR_SUCCESS;
    Profile *tempProfile = NULL;

    while(retry_count <= MAX_RETRY_COUNT){
        pthread_mutex_lock(&plMutex);
	profileIndex = 0;
        for(; profileIndex < Vector_Size(profileList); profileIndex++)
        {
            tempProfile = (Profile *)Vector_At(profileList, profileIndex);
            if(tempProfile->triggerConditionList)
            {
               ret = rbusT2ConsumerReg(tempProfile->triggerConditionList);
               T2Debug("rbusT2ConsumerReg return = %d\n", ret);
	       if(ret != T2ERROR_SUCCESS){
	           retry = 1;
	       }   
            }

        }
        pthread_mutex_unlock(&plMutex);
	if(retry == 1){
	   if(retry_count >= MAX_RETRY_COUNT)
	      break;
           T2Debug("Retry Consumer Registration in %d sec\n", timer);		
	   retry_count++;
           retry = 0;
	   sleep(timer);
	   timer = timer/2;
        }
        else{
	   break;  
        }		
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

void appendTriggerCondition (Profile *tempProfile, const char *referenceName, const char *referenceValue){
    T2Debug("%s ++in\n", __FUNCTION__);
    cJSON *temparrayItem = cJSON_CreateObject();
    cJSON_AddStringToObject(temparrayItem, "reference",referenceName);
    cJSON_AddStringToObject(temparrayItem, "value", referenceValue);
    cJSON *temparrayItem1 = cJSON_CreateObject();
    cJSON_AddItemToObject(temparrayItem1, "TriggerConditionResult", temparrayItem);
    tempProfile->jsonReportObj=temparrayItem1;
    T2Debug("%s --out\n", __FUNCTION__);
}

T2ERROR triggerReportOnCondtion(const char *referenceName, const char *referenceValue)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    int j, profileIndex = 0;
    Profile *tempProfile = NULL;
   
    pthread_mutex_lock(&plMutex);
    for(; profileIndex < Vector_Size(profileList); profileIndex++)
    {
        tempProfile = (Profile *)Vector_At(profileList, profileIndex);
        if(tempProfile->triggerConditionList && (tempProfile->triggerConditionList->count > 0))
        {
             for( j = 0; j < tempProfile->triggerConditionList->count; j++ ) {
                TriggerCondition *triggerCondition = ((TriggerCondition *) Vector_At(tempProfile->triggerConditionList, j));
                if(strcmp(triggerCondition->reference,referenceName) == 0)
                {
	             T2Debug("Triggering report on condition for %s with %s operator, %d threshold\n",
				     triggerCondition->reference, triggerCondition->oprator, triggerCondition->threshold);
                     tempProfile->triggerReportOnCondition = true;
                     if(triggerCondition->report)
                         appendTriggerCondition(tempProfile, referenceName, referenceValue);
                     tempProfile->minThresholdDuration = triggerCondition->minThresholdDuration;
                     SendInterruptToTimeoutThread(tempProfile->name);
                     T2Debug("%s --out\n", __FUNCTION__);
                     pthread_mutex_unlock(&plMutex);
                     return T2ERROR_SUCCESS;
                }
             }
        }
    }
    pthread_mutex_unlock(&plMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

unsigned int getMinThresholdDuration(char *profileName)
{
     unsigned int minThresholdDuration = 0;
     Profile *get_profile = NULL;
     T2Debug("%s --in\n", __FUNCTION__);
     pthread_mutex_lock(&plMutex);
     if(T2ERROR_SUCCESS != getProfile(profileName, &get_profile))
     {
         T2Error("Profile : %s not found\n", profileName);
         T2Debug("%s --out\n", __FUNCTION__);
         pthread_mutex_unlock(&plMutex);
         return 0;
     }
     minThresholdDuration = get_profile->minThresholdDuration;
     get_profile->minThresholdDuration = 0; // reinit the value
     T2Debug("minThresholdDuration = %u \n",minThresholdDuration);
     pthread_mutex_unlock(&plMutex);
     T2Debug("%s --out\n", __FUNCTION__);
     return minThresholdDuration;
}


