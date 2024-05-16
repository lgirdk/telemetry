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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef DUAL_CORE_XB3
#include <sys/inotify.h>
#include <sys/stat.h>
#include "interChipHelper.h"
#endif

#include "dca.h"
#include "dcautil.h"
#include "telemetry2_0.h"
#include "t2log_wrapper.h"
#include "t2common.h"
#include "legacyutils.h"


#ifdef  _COSA_INTEL_XB3_ARM_
static pthread_mutex_t pInterChipLock = PTHREAD_MUTEX_INITIALIZER;

static T2ERROR getInterChipDCAResult(char* profileName, cJSON** pdcaResultObj, bool isClearSeekMap) {
    T2Debug("%s ++in\n", __FUNCTION__);

    T2ERROR retStatus = T2ERROR_FAILURE;
    pthread_mutex_lock(&pInterChipLock);
    //Adding profile name for AROM
    FILE *profileFp = fopen(TELEMETRY_GREP_PROFILE_NAME, "w");
    if (profileFp == NULL) {
        T2Error("%s: File open error %s\n", __FUNCTION__, TELEMETRY_GREP_PROFILE_NAME);
        pthread_mutex_unlock(&pInterChipLock);
        return T2ERROR_FAILURE;
    }
    fprintf(profileFp, "%s%s%d\n", profileName, DELIMITER, isClearSeekMap ? 1:0);
    fclose(profileFp);
    execNotifier("notifyGetGrepResult");
    // Open up inotify listener for available result file notification .
    int notifyfd = -1;
    int watchfd = -1;

    if( access(INOTIFY_FILE, F_OK) != -1 ){
        // Do not lockout on inotify wait if event is already available
        execNotifier("clearInotifyDir");
        goto cleanup ;
    }

    notifyfd = inotify_init();

    if (notifyfd < 0) {
        T2Error("getInterChipDCAResult : Error on adding inotify_init \n");
        execNotifier("clearInotifyDir");
        pthread_mutex_unlock(&pInterChipLock);
        T2Debug("%s ++out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }
    watchfd = inotify_add_watch(notifyfd, DIRECTORY_TO_MONITOR, IN_CREATE);
    if (watchfd < 0) {
        T2Error("getInterChipDCAResult : Error in adding inotify_add_watch on directory : %s \n", DIRECTORY_TO_MONITOR);
        execNotifier("clearInotifyDir");
        close(notifyfd);
        pthread_mutex_unlock(&pInterChipLock);
        T2Debug("%s ++out\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }
    T2Debug("Successfully added watch on directory %s \n", DIRECTORY_TO_MONITOR);
    if ( 0 == listenForInterProcessorChipEvents(notifyfd, watchfd) ) {
        // Read for data from TELEMTERY_LOG_GREP_RESULT and populate the cJson object
        FILE* grepResultFp = NULL;
        struct stat filestat;
        int status;
        char* line;
        status = stat(TELEMTERY_LOG_GREP_RESULT, &filestat);
        if(status == 0) {

            grepResultFp = fopen(TELEMTERY_LOG_GREP_RESULT, "r");
            if(NULL == grepResultFp) {
                T2Error("Unable to open DCA result file \n");
                goto cleanup;
            }
            line = (char *)malloc((filestat.st_size + 1) * sizeof(char));
            if ( NULL != line ) {
                fread(line, sizeof(char), filestat.st_size, grepResultFp);
                line[filestat.st_size] = '\0';
                *pdcaResultObj = cJSON_Parse(line);
                T2Debug("DCA result from ATOM is : \n %s \n", cJSON_PrintUnformatted(*pdcaResultObj));
                free(line);
            } else {
                T2Info("Unable to allocate memory to read data from %s \n", TELEMTERY_LOG_GREP_RESULT );
            }
            fclose(grepResultFp);
            remove(TELEMTERY_LOG_GREP_RESULT);
            retStatus = T2ERROR_SUCCESS;
        } else {
            T2Info("Unable to get file stats for %s \n", TELEMTERY_LOG_GREP_RESULT );
        }
    } else {
        T2Info("%s %d Unable to get DCA results from ATOM \n", __FUNCTION__, __LINE__);
    }

    cleanup :
    inotify_rm_watch( notifyfd, watchfd);
    close(notifyfd);
    pthread_mutex_unlock(&pInterChipLock);
    T2Debug("%s ++out\n", __FUNCTION__);
    return retStatus;
}

static void sendDeleteProfileEvent(char *profileName) {
    T2Debug("%s ++in\n", __FUNCTION__);

    pthread_mutex_lock(&pInterChipLock);
    //Adding profile name for AROM
    FILE *profileFp = fopen(TELEMETRY_GREP_PROFILE_NAME, "w");
    if (profileFp == NULL) {
        T2Error("%s: File open error %s\n", __FUNCTION__, TELEMETRY_GREP_PROFILE_NAME);
        pthread_mutex_unlock(&pInterChipLock);
        return;
    }
    fprintf(profileFp, "%s", profileName);
    fclose(profileFp);

    execNotifier("notifyProfileRemove");
    pthread_mutex_unlock(&pInterChipLock);

    T2Debug("%s ++out\n", __FUNCTION__);
}

T2ERROR saveGrepConfig(char *name, Vector* grepMarkerList) {
    T2Debug("%s -- _COSA_INTEL_XB3_ARM_  \n", __FUNCTION__);
    int returnStatus = T2ERROR_FAILURE ;
    FILE* dcaLogGrepConf = NULL;

    pthread_mutex_lock(&pInterChipLock);
    remove(TELEMTERY_LOG_GREP_CONF);
    dcaLogGrepConf = fopen(TELEMTERY_LOG_GREP_CONF, "w");
    if(dcaLogGrepConf != NULL) {
        int index = 0;
        int grepMarkerCount = Vector_Size(grepMarkerList);
        int delimiterLen = strlen(DELIMITER) * 4 + 3;
        T2Info("Number of grep markers delegated to interchip = %d\n", grepMarkerCount);
        fprintf(dcaLogGrepConf, "%s%s%s\n", "ProfileName", DELIMITER, name);
        for( index = 0; index < grepMarkerCount; index++ ) {
            int totalDataLen = 0 ;
            GrepMarker* grepMarker = (GrepMarker*) Vector_At(grepMarkerList, index);
            if(NULL == grepMarker) {
                continue;
            }

            totalDataLen = strlen(grepMarker->markerName) + strlen(grepMarker->searchString) + strlen(grepMarker->logFile) + delimiterLen + strlen(USE_ABSOLUTE);
            if ( totalDataLen < MAX_LINE_LEN ) {
                fprintf(dcaLogGrepConf, "%s%s%s%s%s%s%s%s%d\n", grepMarker->markerName, DELIMITER, grepMarker->searchString, DELIMITER, grepMarker->logFile, DELIMITER, ((grepMarker->mType == MTYPE_COUNTER) ? USE_COUNTER : USE_ABSOLUTE), DELIMITER, grepMarker->skipFreq);
            } else {
                T2Info("Total marker info length %d is greater than %d . Ignore adding marker to grep list . \n", totalDataLen, MAX_LINE_LEN );
            }
        }
        fclose(dcaLogGrepConf);
        T2Info("Successfully saved config file %s for interchip \n", TELEMTERY_LOG_GREP_CONF);
        // notify ATOM to pick and load the config file
        execNotifier("notifyConfigUpdate");

        T2Info("Notified interchip to pick latest config \n");
        returnStatus = T2ERROR_SUCCESS ;
    }
    else{
        T2Error("Cannot open the file %s\n",TELEMTERY_LOG_GREP_CONF);
        return T2ERROR_FAILURE;
    }

    pthread_mutex_unlock(&pInterChipLock);
    return returnStatus ;
}
#endif

T2ERROR
getGrepResults (char *profileName, Vector *markerList, Vector **grepResultList, bool isClearSeekMap, bool check_rotated) {
    T2Debug("%s ++in\n", __FUNCTION__);
    if(profileName == NULL || markerList == NULL || grepResultList == NULL){
        T2Error("Invalid Args or Args are NULL\n");
        return T2ERROR_FAILURE;
    }
	#ifdef  _COSA_INTEL_XB3_ARM_  // Get the grep results from atom in case of XB3 platforms
        cJSON* dcaResultObj = NULL;
        // Notify atom and wait for results from atom
        getInterChipDCAResult(profileName, &dcaResultObj, isClearSeekMap);
        //  dcaResultObj can be populated from the search result file from atom  !!!! Rest of the code remains unchanged !!!
        Vector *vgrepResult = NULL;
        if (NULL != dcaResultObj) {
            cJSON* dcaMsgArray = cJSON_GetObjectItem(dcaResultObj, "searchResult");
            int dataCount = cJSON_GetArraySize(dcaMsgArray);
            int count = 0;
            if(dataCount > 0)
                Vector_Create(&vgrepResult);

            for (count = 0; count < dataCount; count++) {
                GrepResult* grepResult = (GrepResult*) malloc(sizeof(GrepResult));
                cJSON* pSubitem = cJSON_GetArrayItem(dcaMsgArray, count);
                if (NULL != pSubitem && NULL != pSubitem->child) {
                    grepResult->markerName = strdup(pSubitem->child->string);
                    grepResult->markerValue = strdup(pSubitem->child->valuestring);
                    Vector_PushBack(vgrepResult, grepResult);
                }
            }
            cJSON_Delete(dcaResultObj);
            T2Info("%s dca Grep results has %d fields . \n", __FUNCTION__, count);

        } else {
            T2Info("%s No data from dcaUtils getDCAResults", __FUNCTION__);
        }
        *grepResultList = vgrepResult;
	#else
        getDCAResultsInVector(profileName, markerList, grepResultList, check_rotated);
        if (isClearSeekMap) {
            removeProfileFromSeekMap(profileName);
        }
	#endif

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

void removeGrepConfig(char* profileName, bool clearSeekMap, bool clearExecMap) {
    T2Debug("%s ++in\n", __FUNCTION__);

#ifdef  _COSA_INTEL_XB3_ARM_
    sendDeleteProfileEvent(profileName);
#else
    if(clearSeekMap)
        removeProfileFromSeekMap(profileName);
#endif
    if (clearExecMap)
        removeProfileFromExecMap(profileName);
    T2Debug("%s ++out\n", __FUNCTION__);
}

