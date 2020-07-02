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
#include <cjson/cJSON.h>
#include "datamodel.h"
#include "reportprofiles.h"
#include "t2log_wrapper.h"
#include "t2_custom.h"
#include "collection.h"

static bool               stopProcessing = true;
static queue_t            *rpQueue = NULL;
static queue_t            *rpMsgPkgQueue = NULL;
static pthread_t          rpThread;
static pthread_t          rpMsgThread;
static pthread_mutex_t    rpMutex;
static pthread_mutex_t    rpMsgMutex;
static pthread_cond_t     rpCond;
static pthread_cond_t     msg_Cond;

/**
 * Thread function to receive report profiles Json object
 */
static void *process_rp_thread(void *data)
{
    cJSON *reportProfiles = NULL;

    T2Debug("%s ++in\n", __FUNCTION__);

    while(!stopProcessing)
    {
        pthread_mutex_lock(&rpMutex);
        T2Info("%s: Waiting for event from tr-181 \n", __FUNCTION__);
        pthread_cond_wait(&rpCond, &rpMutex);

        T2Debug("%s: Received wake up signal \n", __FUNCTION__);
        if(queue_count(rpQueue) > 0)
        {
            reportProfiles = (cJSON *)queue_pop(rpQueue);
            if (reportProfiles)
            {
                ReportProfiles_ProcessReportProfilesBlob(reportProfiles);
                cJSON_Delete(reportProfiles);
                reportProfiles = NULL;
            }
        }
        pthread_mutex_unlock(&rpMutex);
    }
    T2Debug("%s --out\n", __FUNCTION__);
}

static void *process_msg_thread(void *data)
{
    struct __msgpack__ *msgpack;
    while(!stopProcessing)
    {	
    	pthread_mutex_lock(&rpMsgMutex);
        pthread_cond_wait(&msg_Cond, &rpMsgMutex);
	if(queue_count(rpMsgPkgQueue) > 0)
        {
            msgpack = (struct __msgpack__ *)queue_pop(rpMsgPkgQueue);
            if (msgpack)
            {
        	ReportProfiles_ProcessReportProfilesMsgPackBlob(msgpack->msgpack_blob,msgpack->msgpack_blob_size);
            }
        }
        pthread_mutex_unlock(&rpMsgMutex);
    }
}

/* Description:
 *      The API validate JSON format and check if 'profiles' field is present in JSON data.
 *      It saves the received json data in temporary file and
 *      notify process_rp_thread() for incoming data.
 * Arguments:
 *      char *JsonBlob         List of active profiles.
 */
T2ERROR datamodel_processProfile(char *JsonBlob)
{
    cJSON *rootObj = NULL;
    cJSON *profiles = NULL;

    if((rootObj = cJSON_Parse(JsonBlob)) == NULL)
    {
        T2Error("%s: JSON parsing failure\n", __FUNCTION__);
        return T2ERROR_FAILURE;
    }

    if((profiles = cJSON_GetObjectItem(rootObj, "profiles")) == NULL)
    {
        T2Error("%s: Missing required param 'profiles' \n", __FUNCTION__);
        cJSON_Delete(rootObj);
        return T2ERROR_FAILURE;
    }

    if (MAX_BULKDATA_PROFILES < cJSON_GetArraySize(profiles))
    {
        T2Error("Report profiles in configuration is greater than max supported profiles (%d). Unable to ReportProfiles_ProcessReportProfilesBlob\n", MAX_BULKDATA_PROFILES );
        cJSON_Delete(rootObj);
        return T2ERROR_FAILURE;
    }

    pthread_mutex_lock(&rpMutex);
    if (!stopProcessing)
    {
        queue_push(rpQueue, (void *)rootObj);
        pthread_cond_signal(&rpCond);
    }
    else
    {
        T2Error("Datamodel not initialized, dropping request \n");
        cJSON_Delete(rootObj);
    }
    pthread_mutex_unlock(&rpMutex);

    return T2ERROR_SUCCESS;
}


T2ERROR datamodel_MsgpackProcessProfile(char *str , int strSize)
{	
    struct __msgpack__ *msgpack;
    msgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));

    if (msgpack == NULL)
    {
      return T2ERROR_FAILURE;
    }

    msgpack->msgpack_blob = str;
    msgpack->msgpack_blob_size = strSize;
    pthread_mutex_lock(&rpMsgMutex);
    if (!stopProcessing)
    {
       queue_push(rpMsgPkgQueue, (void *)msgpack);
       pthread_cond_signal(&msg_Cond);
    }
    else
    {
       T2Error("Datamodel not initialized, dropping request \n");
    }
    pthread_mutex_unlock(&rpMsgMutex);
    return T2ERROR_SUCCESS;
}

/* Description:
 *      This API initializes message queue.
 * Arguments:
 *      NULL
 */

T2ERROR datamodel_init(void)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    rpQueue = queue_create();
    if (rpQueue == NULL)
    {
        T2Error("Failed to create report profile Queue\n");
        return T2ERROR_FAILURE;
    }

    rpMsgPkgQueue = queue_create();
    if (rpMsgPkgQueue == NULL)
    {
        T2Error("Failed to create Msg Pck report profile Queue\n");
        return T2ERROR_FAILURE;
    }

    pthread_mutex_init(&rpMutex, NULL);
    pthread_mutex_init(&rpMsgMutex, NULL);
    pthread_cond_init(&rpCond, NULL);
    pthread_cond_init(&msg_Cond, NULL);

    stopProcessing = false;
    pthread_create(&rpThread, NULL, process_rp_thread, (void *)NULL);
    pthread_create(&rpMsgThread, NULL, process_msg_thread, (void *)NULL);

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

void datamodel_unInit(void)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    pthread_mutex_lock(&rpMutex);
    stopProcessing = true;
    pthread_cond_signal(&rpCond);
    pthread_mutex_unlock(&rpMutex);

    pthread_mutex_lock(&rpMsgMutex);
    pthread_cond_signal(&msg_Cond);
    pthread_mutex_unlock(&rpMsgMutex);

    pthread_join(rpThread, NULL);
    pthread_join(rpMsgThread, NULL);
    pthread_mutex_destroy(&rpMutex);
    pthread_mutex_destroy(&rpMsgMutex);
    pthread_cond_destroy(&rpCond);
    pthread_cond_destroy(&msg_Cond);

    T2Debug("%s --out\n", __FUNCTION__);
}
