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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "reportprofiles.h"

#include "t2collection.h"
#include "persistence.h"
#include "t2log_wrapper.h"
#include "profile.h"
#include "profilexconf.h"
#include "t2eventreceiver.h"
#include "t2_custom.h"
#include "scheduler.h"
#include "t2markers.h"
#include "datamodel.h"
#include "msgpack.h"
#include "busInterface.h"

#define MAX_PROFILENAMES_LENGTH 2048
#define T2_VERSION_DATAMODEL_PARAM  "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Version"

static BulkData bulkdata;
static bool rpInitialized = false;
static char *t2Version = NULL;

pthread_mutex_t rpMutex = PTHREAD_MUTEX_INITIALIZER;

void ReportProfiles_Interrupt()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    char* xconfProfileName = NULL ;
    if (ProfileXConf_isSet()) {
        xconfProfileName = ProfileXconf_getName();
        if (xconfProfileName) {
            SendInterruptToTimeoutThread(xconfProfileName);
            free(xconfProfileName);
        }
    }

    sendLogUploadInterruptToScheduler();
    T2Debug("%s --out\n", __FUNCTION__);
}

void ReportProfiles_TimeoutCb(char* profileName, bool isClearSeekMap)
{
    T2Info("%s ++in\n", __FUNCTION__);

    if(ProfileXConf_isNameEqual(profileName)) {
        ProfileXConf_notifyTimeout(isClearSeekMap);
    }else {
        NotifyTimeout(profileName, isClearSeekMap);
    }

    T2Info("%s --out\n", __FUNCTION__);
}

void ReportProfiles_ActivationTimeoutCb(char* profileName)
{
    T2Info("%s ++in\n", __FUNCTION__);

    bool isDeleteRequired = false;
    if(ProfileXConf_isNameEqual(profileName)) {
        T2Error("ActivationTimeout received for Xconf profile. Ignoring!!!! \n");
    } else {
        if (T2ERROR_SUCCESS != disableProfile(profileName, &isDeleteRequired)) {
            T2Error("Failed to disable profile after timeout: %s \n", profileName);
            return;
        }

        if (isDeleteRequired) {
            removeProfileFromDisk(REPORTPROFILES_PERSISTENCE_PATH, profileName);
            if (T2ERROR_SUCCESS != deleteProfile(profileName)) {
                T2Error("Failed to delete profile after timeout: %s \n", profileName);
            }
        }

        T2ER_StopDispatchThread();
        clearT2MarkerComponentMap();

        if(ProfileXConf_isSet())
            ProfileXConf_updateMarkerComponentMap();
        updateMarkerComponentMap();

        /* Restart DispatchThread */
        if (ProfileXConf_isSet() || getProfileCount() > 0)
            T2ER_StartDispatchThread();
    }

    T2Info("%s --out\n", __FUNCTION__);
}

T2ERROR ReportProfiles_storeMarkerEvent(char *profileName, T2Event *eventInfo) {
    T2Debug("%s ++in\n", __FUNCTION__);

    if(ProfileXConf_isNameEqual(profileName)) {
        ProfileXConf_storeMarkerEvent(eventInfo);
    }else {
        Profile_storeMarkerEvent(profileName, eventInfo);
    }

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR ReportProfiles_setProfileXConf(ProfileXConf *profile) {
    T2Debug("%s ++in\n", __FUNCTION__);
    if(T2ERROR_SUCCESS != ProfileXConf_set(profile)) {
        T2Error("Failed to set XConf profile\n");
        return T2ERROR_FAILURE;
    }

    T2ER_StartDispatchThread();

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR ReportProfiles_deleteProfileXConf(ProfileXConf *profile) {
    T2Debug("%s ++in\n", __FUNCTION__);
    if(ProfileXConf_isSet()) {
        T2ER_StopDispatchThread();

        clearT2MarkerComponentMap();

        updateMarkerComponentMap();

        return ProfileXConf_delete(profile);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR ReportProfiles_addReportProfile(Profile *profile) {
    T2Debug("%s ++in\n", __FUNCTION__);

    if(T2ERROR_SUCCESS != addProfile(profile)) {
        T2Error("Failed to create/add new report profile : %s\n", profile->name);
        return T2ERROR_FAILURE;
    }
    if(T2ERROR_SUCCESS != enableProfile(profile->name)) {
        T2Error("Failed to enable profile : %s\n", profile->name);
        return T2ERROR_FAILURE;
    }

    T2ER_StartDispatchThread(); //Error case can be ignored as Dispatch thread may be running already

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR ReportProfiles_deleteProfile(const char* profileName) {
    T2Debug("%s ++in\n", __FUNCTION__);

    if(T2ERROR_SUCCESS != deleteProfile(profileName))
    {
        T2Error("Failed to delete profile : %s\n", profileName);
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    T2ER_StopDispatchThread();

    clearT2MarkerComponentMap();

    if(ProfileXConf_isSet())
        ProfileXConf_updateMarkerComponentMap();
    updateMarkerComponentMap();

    /* Restart DispatchThread */
    if (ProfileXConf_isSet() || getProfileCount() > 0)
        T2ER_StartDispatchThread();

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static void createComponentDataElements() {
    T2Debug("%s ++in\n", __FUNCTION__);
    Vector* componentList = NULL ;
    int i = 0;
    int length = 0 ;
    getComponentsWithEventMarkers(&componentList);
    length = Vector_Size(componentList);
    for (i = 0; i < length; ++i) {
        char *compName = (char*) Vector_At(componentList,i);
        if(compName)
            regDEforCompEventList(compName, getComponentMarkerList);
    }
    T2Debug("%s --out\n", __FUNCTION__);
}

T2ERROR initReportProfiles()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(rpInitialized) {
        T2Error("%s ReportProfiles already initialized - ignoring\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }
    rpInitialized = true;

    bulkdata.enable = false;
    bulkdata.minReportInterval = 10;
    bulkdata.protocols = strdup("HTTP");
    bulkdata.encodingTypes = strdup("JSON");
    bulkdata.parameterWildcardSupported = true;
    bulkdata.maxNoOfParamReferences = MAX_PARAM_REFERENCES;
    bulkdata.maxReportSize = DEFAULT_MAX_REPORT_SIZE;

    initScheduler(ReportProfiles_TimeoutCb, ReportProfiles_ActivationTimeoutCb);
    initT2MarkerComponentMap();
    T2ER_Init();

    ProfileXConf_init();
    if(T2ERROR_SUCCESS == getParameterValue(T2_VERSION_DATAMODEL_PARAM, &t2Version) && !strcmp(t2Version, "2.0.1")) {
        T2Debug("T2 Version = %s\n", t2Version);
        initProfileList();
        free(t2Version);
        // Init datamodel processing thread
        if (T2ERROR_SUCCESS == datamodel_init())
        {
            if(isRbusEnabled()) {
                T2Debug("Enabling datamodel for report profiles in RBUS mode \n");
                regDEforProfileDataModel(datamodel_processProfile);
            }else {
                // Register TR-181 DM for T2.0
                T2Debug("Enabling datamodel for report profiles in DBUS mode \n");
                if(0 != initTR181_dm()) {
                    T2Error("Unable to initialize TR181!!! \n");
                    datamodel_unInit();
                }
            }
        }
        else
        {
            T2Error("Unable to start message processing thread!!! \n");
        }

    }

    if(ProfileXConf_isSet() || getProfileCount() > 0) {
        if(isRbusEnabled())
            createComponentDataElements();
        T2ER_StartDispatchThread();
    }

    T2Debug("%s --out\n", __FUNCTION__);
    T2Info("Init ReportProfiles Successful\n");
    return T2ERROR_SUCCESS;
}

T2ERROR ReportProfiles_uninit( ) {
    T2Debug("%s ++in\n", __FUNCTION__);
    if(!rpInitialized) {
        T2Error("%s ReportProfiles is not initialized yet - ignoring\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }
    rpInitialized = false;

    T2ER_Uninit();
    destroyT2MarkerComponentMap();
    uninitScheduler();

    if(t2Version && !strcmp(t2Version, "2.0.1")) {
        // Unregister TR-181 DM
        unInitTR181_dm();

        // Stop datamodel processing thread;
        datamodel_unInit();

        uninitProfileList();
    }

    ProfileXConf_uninit();
    free(bulkdata.protocols);
    bulkdata.protocols = NULL ;
    free(bulkdata.encodingTypes);
    bulkdata.encodingTypes = NULL ;

#ifdef _COSA_INTEL_XB3_ARM_
    execNotifier("notifyStopRunning");
#endif

    T2Debug("%s --out\n", __FUNCTION__);
    T2Info("Uninit ReportProfiles Successful\n");
    return T2ERROR_SUCCESS;
}

static void freeProfilesHashMap(void *data) {
    T2Debug("%s ++in\n", __FUNCTION__);
    if(data != NULL) {
        hash_element_t *element = (hash_element_t *) data;
        if(element->key) {
            T2Debug("Freeing hash entry element for Profiles object Name:%s\n", element->key);
            free(element->key);
        }
        if (element->data)
            free(element->data);
        free(element);
    }
    T2Debug("%s --out\n", __FUNCTION__);
}

static void freeReportProfileHashMap(void *data) {
    T2Debug("%s ++in\n", __FUNCTION__);
    if (data != NULL) {
        hash_element_t *element = (hash_element_t *) data;

        if (element->key) {
            T2Debug("Freeing hash entry element for Profiles object Name:%s\n", element->key);
            free(element->key);
        }

        if (element->data) {
            ReportProfile *entry = (ReportProfile *)element->data;

            if (entry->hash)
                free(entry->hash);

            if (entry->config)
                free(entry->config);

            free(entry);
        }
    }
    T2Debug("%s --out\n", __FUNCTION__);
}

static T2ERROR deleteAllReportProfiles() {
    T2Debug("%s ++in\n", __FUNCTION__);

    if (T2ERROR_SUCCESS != deleteAllProfiles()) {
        T2Error("Error while deleting all report profiles \n");
    }

    T2ER_StopDispatchThread();

    clearT2MarkerComponentMap();

    if(ProfileXConf_isSet())
        ProfileXConf_updateMarkerComponentMap();

    /* Restart DispatchThread */
    if (ProfileXConf_isSet())
        T2ER_StartDispatchThread();

    T2Debug("%s --out\n", __FUNCTION__);

    return T2ERROR_SUCCESS;
}

void ReportProfiles_ProcessReportProfilesBlob(cJSON *profiles_root) {

    T2Debug("%s ++in\n", __FUNCTION__);
    if(profiles_root == NULL) {
        T2Error("Profile profiles_root is null . Unable to ReportProfiles_ProcessReportProfilesBlob \n");
        T2Debug("%s --out\n", __FUNCTION__);
        return;
    }

    cJSON *profilesArray = cJSON_GetObjectItem(profiles_root, "profiles");
    int profiles_count = cJSON_GetArraySize(profilesArray);

    T2Info("Number of report profiles in current configuration is %d \n", profiles_count);
    if(profiles_count == 0) {
        T2Debug("Empty report profiles in configuration. Delete all active profiles. \n");
        if (T2ERROR_SUCCESS != deleteAllReportProfiles())
            T2Error("Failed to delete all profiles \n");

        T2Debug("%s --out\n", __FUNCTION__);
        return;
    }

    char* profileName = NULL;
    int profileIndex = 0;

    hash_map_t *profileHashMap = getProfileHashMap();
    hash_map_t *receivedProfileHashMap = hash_map_create();

    // Populate profile hash map for current configuration
    for( profileIndex = 0; profileIndex < profiles_count; profileIndex++ ) {
        cJSON* singleProfile = cJSON_GetArrayItem(profilesArray, profileIndex);
        if(singleProfile == NULL) {
            T2Error("Incomplete profile information, unable to create profile for index %d \n", profileIndex);
            continue;
        }

        cJSON* nameObj = cJSON_GetObjectItem(singleProfile, "name");
        cJSON* hashObj = cJSON_GetObjectItem(singleProfile, "hash");
        if(hashObj == NULL) {
            hashObj = cJSON_GetObjectItem(singleProfile, "VersionHash");
        }
        cJSON* profileObj = cJSON_GetObjectItem(singleProfile, "value");

        if(nameObj == NULL || hashObj == NULL || profileObj == NULL) {
            T2Error("Incomplete profile object information, unable to create profile\n");
            continue;
        }

        ReportProfile *profileEntry = (ReportProfile *)malloc(sizeof(ReportProfile));
        profileName = strdup(nameObj->valuestring);
        profileEntry->hash = strdup(hashObj->valuestring);
        profileEntry->config = cJSON_PrintUnformatted(profileObj);
        hash_map_put(receivedProfileHashMap, profileName, profileEntry);

    } // End of looping through report profiles

    // Delete profiles not present in the new profile list
    char *profileNameKey = NULL;
    int count = hash_map_count(profileHashMap) - 1;
    bool rm_flag = false;
    while(count >= 0) {
        profileNameKey = hash_map_lookupKey(profileHashMap, count--);
        T2Debug("%s Map content from disk = %s \n", __FUNCTION__ , profileNameKey);
        if(NULL == hash_map_get(receivedProfileHashMap, profileNameKey)) {
            T2Debug("%s Profile %s not present in current config . Remove profile from disk \n", __FUNCTION__, profileNameKey);
            removeProfileFromDisk(REPORTPROFILES_PERSISTENCE_PATH, profileNameKey);
            T2Debug("%s Terminate profile %s \n", __FUNCTION__, profileNameKey);
            ReportProfiles_deleteProfile(profileNameKey);
	    rm_flag = true;
        }
    }

    if(isRbusEnabled())
        unregisterDEforCompEventList();

    for( profileIndex = 0; profileIndex < hash_map_count(receivedProfileHashMap); profileIndex++ ) {
        ReportProfile *profileEntry = (ReportProfile *)hash_map_lookup(receivedProfileHashMap, profileIndex);
        profileName = hash_map_lookupKey(receivedProfileHashMap, profileIndex);

        char *existingProfileHash = hash_map_remove(profileHashMap, profileName);
        if(existingProfileHash != NULL)
        {
            if(!strcmp(existingProfileHash, profileEntry->hash)) {
                T2Debug("%s Profile hash for %s is same as previous profile, ignore processing config\n", __FUNCTION__, profileName);
                free(existingProfileHash);
                continue;
            } else {
                Profile *profile = 0;
                free(existingProfileHash);
                if(T2ERROR_SUCCESS == processConfiguration(&(profileEntry->config), profileName, profileEntry->hash, &profile)) { //CHECK if process configuration should have locking mechanism

                    if(T2ERROR_SUCCESS != saveConfigToFile(REPORTPROFILES_PERSISTENCE_PATH, profile->name, profileEntry->config))
                    {
                        T2Error("Unable to save profile : %s to disk\n", profile->name);
                    }
                    if(T2ERROR_SUCCESS == ReportProfiles_deleteProfile(profile->name))
                    {
                        ReportProfiles_addReportProfile(profile);
			rm_flag = true;
                    }
                }else {
                    T2Error("Unable to parse the profile: %s, invalid configuration\n", profileName);
                }
            }
        }
        else
        {
            T2Debug("%s Previous entry for profile %s not found . Adding new profile.\n", __FUNCTION__, profileName);
            Profile *profile = 0;

            if(T2ERROR_SUCCESS == processConfiguration(&(profileEntry->config), profileName, profileEntry->hash, &profile)) { //CHECK if process configuration should have locking mechanism

                if(T2ERROR_SUCCESS != saveConfigToFile(REPORTPROFILES_PERSISTENCE_PATH, profile->name, profileEntry->config))
                {
                    T2Error("Unable to save profile : %s to disk\n", profile->name);
                }
                ReportProfiles_addReportProfile(profile);
		rm_flag = true;
            }else {
                T2Error("Unable to parse the profile: %s, invalid configuration\n", profileName);
            }
        }
    } // End of looping through report profiles

    if (rm_flag) {
	removeProfileFromDisk(REPORTPROFILES_PERSISTENCE_PATH, MSGPACK_REPORTPROFILES_PERSISTENT_FILE);
	T2Info("%s is removed from disk \n", MSGPACK_REPORTPROFILES_PERSISTENT_FILE);
    }

    if(isRbusEnabled()) {
        createComponentDataElements();
        // Notify registered components that profile has received an update
        publishEventsProfileUpdates();
    }
    hash_map_destroy(receivedProfileHashMap, freeReportProfileHashMap);
    hash_map_destroy(profileHashMap, freeProfilesHashMap);
    T2Debug("%s --out\n", __FUNCTION__);
    return;
}

pErr Process_Telemetry_WebConfigRequest(void *Data)
{
     T2Info("FILE:%s\t FUNCTION:%s\t LINE:%d\n", __FILE__, __FUNCTION__, __LINE__);
     pErr execRetVal=NULL;
     execRetVal = (pErr ) malloc (sizeof(Err));
     memset(execRetVal,0,(sizeof(Err)));
     T2Info("FILE:%s\t FUNCTION:%s\t LINE:%d Execution in Handler, excuted \n", __FILE__, __FUNCTION__, __LINE__);
     int retval=__ReportProfiles_ProcessReportProfilesMsgPackBlob(Data);
     if(retval == T2ERROR_SUCCESS)
     {
     	execRetVal->ErrorCode=BLOB_EXEC_SUCCESS;
     	return execRetVal;
     }
     execRetVal->ErrorCode=TELE_BLOB_PROCESSES_FAILURE;
     return execRetVal;
}

static void __msgpack_free_blob(void *user_data)
{
    struct __msgpack__ *msgpack = (struct __msgpack__ *)user_data;
    free(msgpack->msgpack_blob);
    free(msgpack);
}

void msgpack_free_blob(void *exec_data)
{
    execData *execDataPf = (execData *)exec_data;
    __msgpack_free_blob((void *)execDataPf->user_data);
    free(execDataPf);
}

void ReportProfiles_ProcessReportProfilesMsgPackBlob(char *msgpack_blob , int msgpack_blob_size)
{
    uint64_t subdoc_version=0;
    uint16_t transac_id=0;
    int entry_count=0;
    struct __msgpack__ *msgpack = malloc(sizeof(struct __msgpack__));
    if (NULL == msgpack) {
	T2Error("Insufficient memory at Line %d on %s \n", __LINE__, __FILE__);
	return;
    }
    msgpack->msgpack_blob = msgpack_blob;
    msgpack->msgpack_blob_size = msgpack_blob_size;
    
    T2Debug("%s ++in\n", __FUNCTION__);
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_unpack_return ret;

    int profiles_count;
    msgpack_object *profiles_root;
    msgpack_object *profilesArray;

    msgpack_object *subdoc_name, *transaction_id, *version;
    execData *execDataPf = NULL ;
    msgpack_unpacked_init(&result);
    ret = msgpack_unpack_next(&result, msgpack_blob, msgpack_blob_size, &off);
    if (ret != MSGPACK_UNPACK_SUCCESS) {
	T2Error("The data in the buf is invalid format.\n");
	__msgpack_free_blob((void *)msgpack);
	msgpack_unpacked_destroy(&result);
	T2Debug("%s --out\n", __FUNCTION__);
	return;
    }
    profiles_root =  &result.data;
    if(profiles_root == NULL) {
        T2Error("Profile profiles_root is null . Unable to ReportProfiles_ProcessReportProfilesBlob \n");
	__msgpack_free_blob((void *)msgpack);
	msgpack_unpacked_destroy(&result);
	T2Debug("%s --out\n", __FUNCTION__);
	return;
    }

    subdoc_name = msgpack_get_map_value(profiles_root, "subdoc_name");
    transaction_id = msgpack_get_map_value(profiles_root, "transaction_id");
    version = msgpack_get_map_value(profiles_root, "version");

    msgpack_print(subdoc_name, msgpack_get_obj_name(subdoc_name));
    msgpack_print(transaction_id, msgpack_get_obj_name(transaction_id));
    msgpack_print(version, msgpack_get_obj_name(version));

    profilesArray = msgpack_get_map_value(profiles_root, "profiles");
    MSGPACK_GET_ARRAY_SIZE(profilesArray, profiles_count);

    if (NULL == subdoc_name && NULL == transaction_id && NULL == version) {
        /* dmcli flow */
        __ReportProfiles_ProcessReportProfilesMsgPackBlob((void *)msgpack);
	__msgpack_free_blob((void *)msgpack);
	msgpack_unpacked_destroy(&result);
	T2Debug("%s --out\n", __FUNCTION__);
	return;
    }
    /* webconfig flow */
    subdoc_version=(uint64_t)version->via.u64;
    transac_id=(uint16_t)transaction_id->via.u64;
    T2Debug("subdocversion is %llu transac_id in integer is %u"
            " entry_count is %d \n",subdoc_version,transac_id,entry_count);

    execDataPf = (execData*) malloc (sizeof(execData));
    if ( NULL == execDataPf ) {
        T2Error("execData memory allocation failed\n");
	__msgpack_free_blob((void *)msgpack);
	msgpack_unpacked_destroy(&result);
	T2Debug("%s --out\n", __FUNCTION__);
	return;
    }
    memset(execDataPf, 0, sizeof(execData));
    strncpy(execDataPf->subdoc_name,"telemetry",sizeof(execDataPf->subdoc_name)-1);
    execDataPf->txid = transac_id;
    execDataPf->version = (uint32_t)subdoc_version;
    execDataPf->numOfEntries = 1;
    execDataPf->user_data = (void*)msgpack;
    execDataPf->calcTimeout = NULL;
    execDataPf->executeBlobRequest = Process_Telemetry_WebConfigRequest;
    execDataPf->rollbackFunc = NULL;
    execDataPf->freeResources = msgpack_free_blob;
    T2Debug("subdocversion is %d transac_id in integer is %d entry_count is %d subdoc_name is %s"
            " calcTimeout is %p\n",execDataPf->version,execDataPf->txid,execDataPf->numOfEntries,
            execDataPf->subdoc_name,execDataPf->calcTimeout);

    PushBlobRequest(execDataPf);
    T2Debug("PushBlobRequest complete\n");
    msgpack_unpacked_destroy(&result);
    T2Debug("%s --out\n", __FUNCTION__);
    return;
}

int __ReportProfiles_ProcessReportProfilesMsgPackBlob(void *msgpack)
{
    char *msgpack_blob = ((struct __msgpack__ *)msgpack)->msgpack_blob;
    int msgpack_blob_size = ((struct __msgpack__ *)msgpack)->msgpack_blob_size;
    
    T2Debug("%s ++in\n", __FUNCTION__);
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_unpack_return ret;

    int profiles_count = 0;
    msgpack_object *profiles_root;
    msgpack_object *profilesArray;

    msgpack_unpacked_init(&result);
    ret = msgpack_unpack_next(&result, msgpack_blob, msgpack_blob_size, &off);
    if (ret != MSGPACK_UNPACK_SUCCESS) {
	T2Error("The data in the buf is invalid format.\n");
        return T2ERROR_INVALID_ARGS;
    }
    profiles_root =  &result.data;
    if(profiles_root == NULL) {
        T2Error("Profile profiles_root is null . Unable to ReportProfiles_ProcessReportProfilesBlob \n");
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_INVALID_ARGS;
    }

    profilesArray = msgpack_get_map_value(profiles_root, "profiles");
    MSGPACK_GET_ARRAY_SIZE(profilesArray, profiles_count);

    T2Info("Number of report profiles in current configuration is %d \n", profiles_count);
    if(profiles_count == 0) {
        T2Debug("Empty report profiles in configuration. Delete all active profiles. \n");
        if (T2ERROR_SUCCESS != deleteAllReportProfiles())
            T2Error("Failed to delete all profiles \n");
        T2Debug("%s --out\n", __FUNCTION__);
        return T2ERROR_PROFILE_NOT_FOUND;
    }
    hash_map_t *profileHashMap;
    int count;
    char *profileNameKey = NULL;
    int profileIndex;
    msgpack_object *singleProfile;
    bool profile_found_flag = false;
    bool save_flag = false;
    
    profileHashMap = getProfileHashMap();

/* Delete profiles not present in the new profile list */
    count = hash_map_count(profileHashMap) - 1;
    while(count >= 0) {
	profile_found_flag = false;
	profileNameKey = hash_map_lookupKey(profileHashMap, count--);
	for( profileIndex = 0; profileIndex < profiles_count; profileIndex++ ) {
	    singleProfile = msgpack_get_array_element(profilesArray, profileIndex);
	    msgpack_object* nameObj = msgpack_get_map_value(singleProfile, "name");
	    if (0 == msgpack_strcmp(nameObj, profileNameKey)) {
		T2Info("%s is found \n",profileNameKey);
		profile_found_flag = true;
		break;
	    }
	}
	if (false == profile_found_flag) {
	    ReportProfiles_deleteProfile(profileNameKey);
	    save_flag = true;
	}
    }

/* Populate profile hash map for current configuration */
    for( profileIndex = 0; profileIndex < profiles_count; profileIndex++ ) {
	singleProfile = msgpack_get_array_element(profilesArray, profileIndex);
        if(singleProfile == NULL) {
            T2Error("Incomplete profile information, unable to create profile for index %d \n", profileIndex);
            continue;
        }
        msgpack_object* nameObj = msgpack_get_map_value(singleProfile, "name");
        msgpack_object* hashObj = msgpack_get_map_value(singleProfile, "hash");
        msgpack_object* profileObj = msgpack_get_map_value(singleProfile, "value");
	if(nameObj == NULL || hashObj == NULL || profileObj == NULL) {
            T2Error("Incomplete profile object information, unable to create profile\n");
            continue;
        }

	char* profileName = NULL;
	char *existingProfileHash = NULL;
	Profile *profile = NULL;
        profileName = msgpack_strdup(nameObj);
	existingProfileHash = hash_map_remove(profileHashMap, profileName);
        if(NULL == existingProfileHash) {
	    if(T2ERROR_SUCCESS == processMsgPackConfiguration(singleProfile, &profile)) {
		ReportProfiles_addReportProfile(profile);
		save_flag = true;
	    }
	} else {
            if(0 == msgpack_strcmp(hashObj, existingProfileHash)) {
		T2Info("Profile %s with %s hash already exist \n", profileName, existingProfileHash);
		continue;
            } else {
		if(T2ERROR_SUCCESS == processMsgPackConfiguration(singleProfile, &profile))
                    if(T2ERROR_SUCCESS == ReportProfiles_deleteProfile(profile->name)) {
                        ReportProfiles_addReportProfile(profile);
			save_flag = true;
		    }
	    }
	}
	free(profileName);
    } /* End of looping through report profiles */
    if (save_flag) {
	clearPersistenceFolder(REPORTPROFILES_PERSISTENCE_PATH);
	T2Debug("Persistent folder is cleared\n");
	MsgPackSaveConfig(REPORTPROFILES_PERSISTENCE_PATH, MSGPACK_REPORTPROFILES_PERSISTENT_FILE,
			  msgpack_blob , msgpack_blob_size);
	T2Debug("%s is saved on disk \n", MSGPACK_REPORTPROFILES_PERSISTENT_FILE);
    }
    msgpack_unpacked_destroy(&result);
    hash_map_destroy(profileHashMap, freeProfilesHashMap);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}
