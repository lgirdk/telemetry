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
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <fcntl.h>

#include "interChipHelper.h"

#include "t2collection.h"
#include "telemetry2_0.h"
#include "t2log_wrapper.h"
#include "vector.h"
#include "t2common.h"
#include "dca.h"
#include "legacyutils.h"
#ifdef LIBSYSWRAPPER_BUILD
#include "secure_wrapper.h"
#endif

#define LOOP_SLEEP 100

static bool isEnabled = true;
// Aligning with max line supported by legacy dca
static Vector *grepMarkerList;
static hash_map_t *grepMarkerListMap = NULL;
static bool bGrepMarkerMapInitialized = false;



T2ERROR initGrepMarkerMap() {
    T2Debug("++in %s \n", __FUNCTION__);
    if (bGrepMarkerMapInitialized) {
        T2Info("profile list is already initialized\n");
        return T2ERROR_SUCCESS;
    }

    grepMarkerListMap = hash_map_create();
    bGrepMarkerMapInitialized = true;
    T2Debug("--out %s \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

void freegrepMarkerListMap(void* data){
     T2Debug("++in %s \n", __FUNCTION__);
     if(data != NULL)
     {
         GrepMarker *gMarker = (GrepMarker *)data;
         if(gMarker->logFile != NULL)
             free(gMarker->logFile);
         if(gMarker->markerName != NULL)
             free(gMarker->markerName);
         if(gMarker->searchString != NULL)
             free(gMarker->searchString);
         if(gMarker->paramType != NULL)
             free(gMarker->paramType);
         if(gMarker->mType == MTYPE_ABSOLUTE && gMarker->u.markerValue)
             free(gMarker->u.markerValue);

         free(gMarker);
     }
     T2Debug("--out %s \n", __FUNCTION__);
}

static T2ERROR addGrepMarkersToMap () {
    T2Debug("++in %s \n", __FUNCTION__);

    FILE *gMarkerFptr = NULL;
    char *ProfileName = NULL;
    char line[MAX_LINE_LEN];
    Vector *markerList = NULL;

    initGrepMarkerMap();

    gMarkerFptr = fopen(TELEMTERY_LOG_GREP_CONF, "r");
    if (NULL == gMarkerFptr) {
        T2Info("Unable to open file %s \n", TELEMTERY_LOG_GREP_CONF);
        return T2ERROR_FAILURE;
    }

    if (NULL != fgets(line, MAX_LINE_LEN, gMarkerFptr)) {
        char *nameHeader = (char*) strSplit(line, DELIMITER);
        char *nameValue = (char*) strSplit(NULL, DELIMITER);
        int len = strlen(nameValue);
        if (0 == strcmp(nameHeader, "ProfileName") && len > 0) {
            if (nameValue[len - 1] == '\n')
                nameValue[len - 1] = '\0';
            ProfileName = strdup(nameValue);
        } else {
            T2Error("%s: ProfileName parameter missing in grepMarker list \n", __FUNCTION__);
        }
    } else {
        T2Error("%s: received empty conf file %s from ARM \n", __FUNCTION__, TELEMTERY_LOG_GREP_CONF);
    }

    if (ProfileName == NULL) {
        T2Error("%s: Received invalid conf from ARM \n", __FUNCTION__);
        fclose(gMarkerFptr);
        return T2ERROR_FAILURE;
    }

    Vector_Create(&markerList);

    T2Debug("%s: Preparing marker list for profile - %s \n", __FUNCTION__, ProfileName);
    while(NULL != fgets(line, MAX_LINE_LEN, gMarkerFptr)) {
        // Remove new line
        int len = strlen(line);
        if(len > 0 && line[len - 1] == '\n')
            line[--len] = '\0';

        // multiple string split
        char *header = (char*) strSplit(line, DELIMITER);
        char *grepPattern = (char*) strSplit(NULL, DELIMITER);
        char *grepFile = (char*) strSplit(NULL, DELIMITER);
        char *useProperty = (char*) strSplit(NULL, DELIMITER);
        char *skipIntervalStr = (char*) strSplit(NULL, DELIMITER);
        int skipInterval;
        if(NULL == grepFile || NULL == grepPattern || NULL == header || NULL == skipIntervalStr || NULL == useProperty)
            continue;

        if((0 == strcmp(grepPattern, "")) || (0 == strcmp(grepFile, "")))
            continue;

        skipInterval = atoi(skipIntervalStr);
        if(skipInterval <= 0)
            skipInterval = 0;

        GrepMarker *gMarker = (GrepMarker *) malloc(sizeof(GrepMarker));
        gMarker->markerName = strdup(header);
        gMarker->searchString = strdup(grepPattern);
        gMarker->logFile = strdup(grepFile);
        if (strstr(useProperty, USE_ABSOLUTE)) {
            gMarker->mType = MTYPE_ABSOLUTE;
            gMarker->u.markerValue = NULL;
        }else {
            gMarker->mType = MTYPE_COUNTER;
            gMarker->u.count = 0;
        }
        gMarker->skipFreq = skipInterval;
        T2Debug("Adding maker to grep list : %s %s %s %s %d  \n", gMarker->markerName, gMarker->searchString, gMarker->logFile, gMarker->mType == MTYPE_COUNTER ? USE_COUNTER:USE_ABSOLUTE,  gMarker->skipFreq);
        Vector_PushBack(markerList, gMarker);
    }
    hash_map_put(grepMarkerListMap, ProfileName, markerList, freegrepMarkerListMap);

    fclose(gMarkerFptr);
    T2Debug("--out %s \n", __FUNCTION__);
    return 0;
}

static void removeProfileMarkers(char *profileName) {
    T2Debug("++in %s \n", __FUNCTION__);

    Vector *markerList = (Vector *)hash_map_remove(grepMarkerListMap, profileName);

    if (NULL != markerList) {
        removeProfileFromSeekMap(profileName);
    }
    removeProfileFromExecMap(profileName);

    T2Debug("--out %s \n", __FUNCTION__);
}

static void freeInterChipResources() {
    if(grepMarkerList)
    {
        Vector_Destroy(grepMarkerList, freeGMarker);
    }
    isEnabled = false ;
}
/**
 *  Gets grep results on ATOM and saves in file TELEMTERY_LOG_GREP_RESULT "/tmp/dcaGrepResult.txt"
 */
static T2ERROR saveDcaGrepResults() {
    T2Debug("%s ++in\n", __FUNCTION__);

    char line[MAX_LINE_LEN] = { '\0' };
    char *profileName = NULL;
    bool isClearSeekMap = false;
    cJSON* dcaResultObj = NULL;
    FILE *profileFp = fopen(TELEMETRY_GREP_PROFILE_NAME, "r");
    if (profileFp) {
        if (NULL != fgets(line, MAX_LINE_LEN, profileFp)) {
            profileName = strSplit(line, DELIMITER);
            char *clearSeekMapPtr = strSplit(NULL, DELIMITER);
            if (clearSeekMapPtr) {
                isClearSeekMap = (atoi(clearSeekMapPtr) == 1 ? true : false);
            }
            T2Debug("%s: for Profile: %s, clearValue: %d \n", __func__, profileName, isClearSeekMap);
        } else {
            T2Error("%s: profileName missing for grep result \n", __FUNCTION__);
        }
        fclose(profileFp);
        if(remove(TELEMETRY_GREP_PROFILE_NAME) != 0){
            T2Error("Cannot remove the file %s\n",TELEMETRY_GREP_PROFILE_NAME);
        }
    }

    if (NULL != profileName) {
        execNotifier("copyLogs");

        initGrepMarkerMap();
        Vector *gMarkerList = (Vector *)hash_map_get(grepMarkerListMap, profileName);

        if(NULL != gMarkerList) {
            T2Debug("Get grep results from legacy dca utils \n");
            getDCAResultsInJson(profileName, gMarkerList, &dcaResultObj);
        }else {
            T2Info("Grep marker list is empty \n");
        }
    }

    if(NULL != dcaResultObj) {
        T2Debug("ATOM Data from dca : \n %s \n", cJSON_PrintUnformatted(dcaResultObj));
    }else {
        dcaResultObj = cJSON_CreateObject();
        if(NULL != dcaResultObj)
            cJSON_AddItemToObject(dcaResultObj, "searchResult", cJSON_CreateArray());
        T2Info("No data from dcaUtils getDCAResults \n");
    }

    if(remove(TELEMTERY_LOG_GREP_RESULT) != 0){
        T2Error("Cannot remove the file %s\n",TELEMTERY_LOG_GREP_RESULT);
    }
    FILE* dcaLogGrepResult = NULL;
    dcaLogGrepResult = fopen(TELEMTERY_LOG_GREP_RESULT, "w+");
    if(dcaLogGrepResult != NULL) {
        fprintf(dcaLogGrepResult, "%s", cJSON_PrintUnformatted(dcaResultObj));
        usleep(LOOP_SLEEP);
        fclose(dcaLogGrepResult);
        T2Debug("Saved grepsult file %s for interchip \n", TELEMTERY_LOG_GREP_RESULT);
    }
    execNotifier("copyJsonResultToArm");
    T2Info("Notified interchip to pick results \n");
    cJSON_Delete(dcaResultObj);
    dcaResultObj = NULL;

    if (isClearSeekMap)
        removeProfileFromSeekMap(profileName);

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static char *getProfileName() {
    T2Debug("++in %s\n", __FUNCTION__);

    char *name = NULL;
    int profileFp = open(TELEMETRY_GREP_PROFILE_NAME, O_RDONLY);
    struct stat filestat;
    if (profileFp == -1) {
        T2Error("%s: File open error %s\n", __FUNCTION__, TELEMETRY_GREP_PROFILE_NAME);
        return NULL;
    }

    int status = fstat(profileFp, &filestat);
    if(status == 0 && filestat.st_size > 0) {
        name = (char *)malloc((filestat.st_size + 1) * sizeof(char));
        if (name) {
            if(read(profileFp, name, filestat.st_size)  == -1){
                close(profileFp);
                free(name);
                if(remove(TELEMETRY_GREP_PROFILE_NAME) != 0){
                     T2Error("Cannot remove the file %s\n", TELEMETRY_GREP_PROFILE_NAME);
                }
                T2Debug("--out %s\n", __FUNCTION__);
                return NULL;
            }
            name[filestat.st_size] = '\0';
        }
    }
    close(profileFp);
    if(remove(TELEMETRY_GREP_PROFILE_NAME) != 0){
       T2Error("Cannot remove the file %s\n", TELEMETRY_GREP_PROFILE_NAME);
    }
    T2Debug("--out %s\n", __FUNCTION__);
    return name;
}

static int processEventType(char *eventType) {
    T2Debug("++in processEventType for event %s \n", eventType);
    int returnStatus = -1;
    if(eventType == NULL) {
        return returnStatus;
    }

    returnStatus = 0 ;
    if(strncasecmp(eventType, CONFIG_FETCH_EVENT, MAX_EVENT_TYPE_BUFFER) == 0) { // On ATOM
        addGrepMarkersToMap();
        T2Info("processEventType : %s\n", eventType);
    }else if(strncasecmp(eventType, T2_TIMER_EVENT, MAX_EVENT_TYPE_BUFFER) == 0) { //On ATOM
        T2Debug("%s\n", T2_TIMER_EVENT);
        saveDcaGrepResults();
    }else if(strncasecmp(eventType, T2_CLEAR_SEEK_EVENT, MAX_EVENT_TYPE_BUFFER) == 0) { //On ATOM
        T2Debug("%s Clear seek events from interchip received\n", T2_CLEAR_SEEK_EVENT);
        execNotifier(T2_CLEAR_SEEK_EVENT);
    }else if(strncasecmp(eventType, T2_STOP_RUNNING, MAX_EVENT_TYPE_BUFFER) == 0) { //On ATOM
        T2Info("%s Stop execution event from interchip received\n", eventType);
        freeInterChipResources();
    }else if(strncasecmp(eventType, T2_DCA_RESULT_EVENT, MAX_EVENT_TYPE_BUFFER) == 0) { //On ARM
        T2Debug("%s DCA data results from interchip received\n", eventType);
    }else if(strncasecmp(eventType, T2_DAEMON_START_EVENT, MAX_EVENT_TYPE_BUFFER) == 0) { //On ARM
        T2Debug("%s T2 Daemon init successful on interchip \n", eventType);
    }else if(strncasecmp(eventType, T2_DAEMON_FAILED_EVENT, MAX_EVENT_TYPE_BUFFER) == 0) { //On ARM
        T2Debug("%s T2 Daemon init failed on interchip \n", eventType);
        returnStatus = -1 ;
    }else if (strncasecmp(eventType, T2_PROFILE_REMOVE_EVENT, MAX_EVENT_TYPE_BUFFER) == 0){ //On ATOM
        char *profileName = getProfileName();
        if (profileName) {
            T2Debug("%s: Remove Grep config for %s \n", __FUNCTION__, profileName);
            removeProfileMarkers(profileName);
            free(profileName);
        } else {
            T2Error("%s: ProfileName not available \n", __FUNCTION__);
        }
    }else if(strncasecmp(eventType, DEL_T2_CACHE_FILE, MAX_EVENT_TYPE_BUFFER) == 0) { //On ATOM
        T2Info("%s Remove t2 cache file from interchip received\n", eventType);
        if(remove(T2_CACHE_FILE) != 0){
          T2Error("Cannot remove the file %s\n",T2_CACHE_FILE);
        }
    }else {
        // Unknown event type
        T2Info("Unknown event %s !!!! Ignore and return \n", eventType);
        // Safe return to unblock wait on grep results in case of any rare condition
        #ifdef _COSA_INTEL_USG_ATOM_
        T2Debug("Initiating saveDcaGrepResults for avoiding data loss \n");
        saveDcaGrepResults();
        #endif
    }
    T2Debug("--out processEventType \n");
    return returnStatus;
}


bool isHelperEnabled() {

    return isEnabled ;
}

/**
 * Listen for inotify events over /telemetry
 */
int listenForInterProcessorChipEvents(int notifyfd, int watchfd) {
    T2Debug("++in listenForInterProcessorChipEvents \n");

    int ret = -1;
    char buffer[sizeof(struct inotify_event) + MAX_EVENT_TYPE_BUFFER + 1] = { '0' };
    struct inotify_event * event_ptr;
    char absEventFilePath[MAX_EVENT_TYPE_BUFFER] = { '0' };

    ssize_t count = read(notifyfd, buffer, sizeof(buffer));
    if(count < 0) {
        T2Info("Failure in reading buffer \n");
        return -1;
    }
    /*CID 137537: String not null terminated*/
    buffer[sizeof(buffer)-1] = '\0';
    event_ptr = (struct inotify_event *) buffer;

    if(event_ptr != NULL) {
        if((event_ptr->mask & IN_CREATE) && event_ptr->len > 0) {
            memset(absEventFilePath, sizeof(char), MAX_EVENT_TYPE_BUFFER);
            if(event_ptr->name[0] != '\0') {
                ret = 0;
                snprintf(absEventFilePath, MAX_EVENT_TYPE_BUFFER, "%s/%s", DIRECTORY_TO_MONITOR, event_ptr->name);
                processEventType(event_ptr->name);
                T2Debug("Clear off the event file = %s for next events \n", absEventFilePath);
                if(remove(absEventFilePath) != 0){
                    T2Error("Cannot remove the file %s\n",absEventFilePath);
                }
            }else {
                T2Debug("Event file name is null \n");
            }
        }else {
            T2Debug("Unknown Mask 0x%.8x\n", event_ptr->mask);
        }

    }

    T2Debug("--out listenForInterProcessorChipEvents \n");
    return ret;
}


int execNotifier(char *eventType) {
    int ret = 0;
    T2Debug("++in execNotifier with eventType =  %s  \n", eventType);
    // Use secure system call if the userstory is ready across platforms
    T2Debug("--out execNotifier, exec command is %s %s", NOTIFY_HELPER_UTIL, eventType);
    #ifdef LIBSYSWRAPPER_BUILD
        ret = v_secure_system(NOTIFY_HELPER_UTIL " %s",eventType);
    #else
        char command[128] = { '0' };
        snprintf(command, sizeof(command),"%s %s", NOTIFY_HELPER_UTIL, eventType);
        ret = system(command);
    #endif
    if(ret !=0)
    {
        T2Debug("Failed in executing v_secure_system command ret %d \n",ret);
    }
    return ret;
}

int interchipDaemonStart( ) {

    T2Debug("++in interchipDaemonStart \n");
    // Open up inotify listener for available result file notification .
    int notifyfd;
    int watchfd;
    int ret = -1 ;
    notifyfd = inotify_init();

    if(notifyfd < 0) {
        T2Error("Error on adding inotify_init \n");
        return ret ;
    }
    watchfd = inotify_add_watch(notifyfd, DIRECTORY_TO_MONITOR, IN_CREATE);
    if(watchfd < 0) {
        T2Error("Error in adding inotify_add_watch on directory : %s \n", DIRECTORY_TO_MONITOR);
        return ret ;
    }
    execNotifier("notifyDaemonStart");
    ret = listenForInterProcessorChipEvents(notifyfd, watchfd);
    T2Info("Event for telemetry2_0 daemon start on interchip received \n");
    T2Debug("--out interchipDaemonStart \n");
    return ret ;

}

int createNotifyDir() {
    struct stat dirSt = {0};
    int status = -1 ;

    // Clear any unflushed event files from previous stale execution
    execNotifier("clearInotifyDir");

    if ( stat(DIRECTORY_TO_MONITOR, &dirSt) == 0 ) {
    	status = 0 ;
    } else {
    	T2Info("Failed to create %s \n", DIRECTORY_TO_MONITOR);
    }

    return status;

}
