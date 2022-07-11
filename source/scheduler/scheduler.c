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

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "t2log_wrapper.h"
#include "scheduler.h"
#include "vector.h"
#include "profile.h"

static TimeoutNotificationCB timeoutNotificationCb;
static ActivationTimeoutCB activationTimeoutCb;
static Vector *profileList = NULL;
static pthread_mutex_t scMutex;
static bool sc_initialized = false;

void freeSchedulerProfile(void *data)
{
    if(data != NULL)
    {
        SchedulerProfile *schProfile = (SchedulerProfile *)data;
        free(schProfile->name);
        pthread_mutex_destroy(&schProfile->tMutex);
        pthread_cond_destroy(&schProfile->tCond);
        pthread_detach(schProfile->tId);
        free(schProfile);
    }
}

/*
 * Function:  getLapsedTime 
 * --------------------
 * calculates the elapsed time between time2 and time1(in seconds and nano seconds)
 * 
 * time2: start time
 * time1: finish time
 *
 * returns: output (calculated elapsed seconds and nano seconds)
 */
int getLapsedTime (struct timespec *output, struct timespec *time1, struct timespec *time2)
{
  
  /* handle the underflow condition, if time2 nsec has higher value */
  int com = time1->tv_nsec < time2->tv_nsec;
  if (com) {
    int nsec = (time2->tv_nsec - time1->tv_nsec) / 1000000000 + 1;
    
    time2->tv_nsec = time2->tv_nsec - 1000000000 * nsec;
    time2->tv_sec = time2->tv_sec + nsec;
  }

  com = time1->tv_nsec - time2->tv_nsec > 1000000000;
  if (com) {
    int nsec = (time1->tv_nsec - time2->tv_nsec) / 1000000000;
    
    time2->tv_nsec = time2->tv_nsec + 1000000000 * nsec;
    time2->tv_sec  = time2->tv_sec - nsec;
  }

  /* Calculate the elapsed time */
  output->tv_sec = time1->tv_sec - time2->tv_sec;

  output->tv_nsec = time1->tv_nsec - time2->tv_nsec;

  if(time1->tv_sec < time2->tv_sec)
    return 1;
  else
    return 0;
}

void* TimeoutThread(void *arg)
{
    SchedulerProfile *tProfile = (SchedulerProfile*)arg;
    struct timespec _ts;
    struct timespec _now;
    struct timespec _MinThresholdTimeTs;
    struct timespec _MinThresholdTimeStart;
    unsigned int minThresholdTime = 0;
    int n;
    T2Debug("%s ++in\n", __FUNCTION__);
    registerTriggerConditionConsumer();

    while(tProfile->repeat && !tProfile->terminated)
    {
        memset(&_ts, 0, sizeof(struct timespec));
        memset(&_now, 0, sizeof(struct timespec));

        pthread_mutex_lock(&tProfile->tMutex);

        clock_gettime(CLOCK_REALTIME, &_now);
        _ts.tv_sec = _now.tv_sec + tProfile->timeOutDuration;

	if(tProfile->timeOutDuration == UINT_MAX){
	     T2Info("Waiting for condition as reporting interval is not configured for profile - %s\n", tProfile->name);
             n = pthread_cond_wait(&tProfile->tCond, &tProfile->tMutex);
	}
	else{
             T2Info("Waiting for %d sec for next TIMEOUT for profile - %s\n", tProfile->timeOutDuration, tProfile->name);
	     n = pthread_cond_timedwait(&tProfile->tCond, &tProfile->tMutex, &_ts);
        }
        if(n == ETIMEDOUT)
        {
            T2Info("TIMEOUT for profile - %s\n", tProfile->name);
            timeoutNotificationCb(tProfile->name, false);
        }
        else if (n == 0)
        {
            /* CID 175316:- Value not atomically updated (ATOMICITY) */
            T2Info("Interrupted before TIMEOUT for profile : %s \n", tProfile->name);
            if(minThresholdTime) 
            {
		 memset(&_MinThresholdTimeTs, 0, sizeof(struct timespec));
                 clock_gettime(CLOCK_REALTIME, &_MinThresholdTimeTs);
		 T2Debug("minThresholdTime left %ld -\n", (long int)(_MinThresholdTimeTs.tv_sec - _MinThresholdTimeStart.tv_sec));
                 if(minThresholdTime < (_MinThresholdTimeTs.tv_sec - _MinThresholdTimeStart.tv_sec)){
                      minThresholdTime = 0;
		      T2Debug("minThresholdTime reset done\n");
		 }     
            }

            if(minThresholdTime == 0)
            {
                 timeoutNotificationCb(tProfile->name, true);
                 if(tProfile->terminated)
                 {
                    T2Error("Profile : %s is being removed from scheduler \n", tProfile->name);
                    pthread_mutex_unlock(&tProfile->tMutex);
                    break;
                 }
                 minThresholdTime = getMinThresholdDuration(tProfile->name);
                 T2Info("minThresholdTime %u\n", minThresholdTime);

                 if(minThresholdTime)
                 {
		     memset(&_MinThresholdTimeStart, 0, sizeof(struct timespec));
                     clock_gettime(CLOCK_REALTIME, &_MinThresholdTimeStart);
                 }
            }
        }
        else
        {
            T2Error("Profile : %s pthread_cond_timedwait ERROR!!!\n", tProfile->name);
        }
        //Update activation timeout
        if (tProfile->timeToLive != INFINITE_TIMEOUT)
        {
            tProfile->timeToLive -= tProfile->timeOutDuration;
        }

        /*
         * If timeToLive < timeOutDuration,
         * invoke activationTimeout callback and
         * exit timeout thread.
         */
        if (tProfile->timeOutDuration == 0 || tProfile->timeToLive < tProfile->timeOutDuration)
        {
            T2Info("Profile activation timeout for %s \n", tProfile->name);
            char *profileName = strdup(tProfile->name);
            tProfile->repeat = false;
            pthread_mutex_unlock(&tProfile->tMutex);
            pthread_detach(pthread_self());
            if(tProfile->deleteonTime) {
               deleteProfile(profileName);
               break;
	    }
            pthread_mutex_lock(&scMutex);
            Vector_RemoveItem(profileList, tProfile, freeSchedulerProfile);
            pthread_mutex_unlock(&scMutex);
            activationTimeoutCb(profileName);
            break;
        }

        pthread_mutex_unlock(&tProfile->tMutex);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return NULL;
}

T2ERROR SendInterruptToTimeoutThread(char* profileName)
{
    SchedulerProfile *tProfile = NULL;
    T2Debug("%s ++in\n", __FUNCTION__);
    if(timeoutNotificationCb == NULL)
    {
        T2Error("Timerout Callback not set - Scheduler is not initialized yet\n");
        return T2ERROR_FAILURE;
    }
    int index = 0;
    pthread_mutex_lock(&scMutex);
    for (; index < profileList->count; ++index)
    {
        tProfile = (SchedulerProfile *)Vector_At(profileList, index);
        if(profileName == NULL || (strcmp(profileName, tProfile->name) == 0)) {
            T2Info("Sending Interrupt signal to Timeout Thread of profile : %s\n", tProfile->name);
            pthread_mutex_lock(&tProfile->tMutex);
            pthread_cond_signal(&tProfile->tCond);
            pthread_mutex_unlock(&tProfile->tMutex);
        }
    }
    pthread_mutex_unlock(&scMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR initScheduler(TimeoutNotificationCB notificationCb, ActivationTimeoutCB activationCB)
{
    T2Debug("%s ++in\n", __FUNCTION__);

    if (sc_initialized)
    {
        T2Info("Scheduler is already initialized \n");
        return T2ERROR_SUCCESS;
    }
    timeoutNotificationCb = notificationCb;
    activationTimeoutCb = activationCB;

    sc_initialized = true;
    pthread_mutex_init(&scMutex, NULL);

    T2Debug("%s --out\n", __FUNCTION__);
    return Vector_Create(&profileList);
}

void uninitScheduler()
{
    int index = 0;
    SchedulerProfile *tProfile = NULL;

    T2Debug("%s ++in\n", __FUNCTION__);

    if (!sc_initialized)
    {
        T2Info("Scheduler is not initialized yet \n");
        return ;
    }
    sc_initialized = false;

    pthread_mutex_lock(&scMutex);
    for (; index < profileList->count; ++index)
    {
        tProfile = (SchedulerProfile *)Vector_At(profileList, index);
        pthread_mutex_lock(&tProfile->tMutex);
        tProfile->terminated = true;
        pthread_cond_signal(&tProfile->tCond);
        pthread_mutex_unlock(&tProfile->tMutex);
        pthread_join(tProfile->tId, NULL);
        T2Info("Profile %s successfully removed from Scheduler\n", tProfile->name);
    }
    Vector_Destroy(profileList, freeSchedulerProfile);
    profileList = NULL;
    pthread_mutex_unlock(&scMutex);
    pthread_mutex_destroy(&scMutex);
    timeoutNotificationCb = NULL;

    T2Debug("%s --out\n", __FUNCTION__);
}

T2ERROR registerProfileWithScheduler(const char* profileName, unsigned int timeInterval, unsigned int activationTimeout, bool deleteonTimeout, bool repeat)
{
    T2ERROR ret;
    T2Debug("%s ++in : profile - %s \n", __FUNCTION__,profileName);
    if(timeoutNotificationCb == NULL)
    {
        T2Error("Timerout Callback not set - Scheduler isn't initialized yet, Unable to register profile\n");
        return T2ERROR_FAILURE;
    }
    else
    {
        int index =  0 ;
        bool isSchedulerAssigned = false ;
        SchedulerProfile *tempSchProfile = NULL;
        if(sc_initialized) { // Check for existing scheduler to avoid duplicate scheduler entries
            pthread_mutex_lock(&scMutex);
            for( ; index < profileList->count; ++index ) {
                tempSchProfile = (SchedulerProfile *) Vector_At(profileList, index);
                if(strcmp(tempSchProfile->name, profileName) == 0) {
                    isSchedulerAssigned = true;
                    break ;
                }
            }
            pthread_mutex_unlock(&scMutex);
            if (isSchedulerAssigned) {
                T2Info("Scheduler already assigned for profile %s , exiting .\n", profileName );
                T2Debug("%s --out\n", __FUNCTION__);
                return T2ERROR_SUCCESS ;
            }
        }

        SchedulerProfile *tProfile = (SchedulerProfile *)malloc(sizeof(SchedulerProfile));
        tProfile->name = strdup(profileName);
        tProfile->repeat = repeat;
        tProfile->timeOutDuration = timeInterval;
        tProfile->timeToLive = activationTimeout;
	tProfile->deleteonTime = deleteonTimeout;
        tProfile->terminated = false;
        pthread_mutex_init(&tProfile->tMutex, NULL);
        pthread_cond_init(&tProfile->tCond, NULL);
        T2Info("Starting TimeoutThread for profile : %s\n", profileName);
        pthread_create(&tProfile->tId, NULL, TimeoutThread, (void*)tProfile);

        T2Debug("%s --out\n", __FUNCTION__);
        pthread_mutex_lock(&scMutex);
        ret = Vector_PushBack(profileList, (void *)tProfile);
        pthread_mutex_unlock(&scMutex);
        return ret;
    }
}

T2ERROR unregisterProfileFromScheduler(const char* profileName)
{
    int index = 0;
    SchedulerProfile *tProfile = NULL;

    T2Debug("%s ++in\n", __FUNCTION__);

    if (!sc_initialized)
    {
        T2Error("scheduler not initialized \n");
        return T2ERROR_SUCCESS;
    }

    pthread_mutex_lock(&scMutex);
    for (; index < profileList->count; ++index)
    {
        tProfile = (SchedulerProfile *)Vector_At(profileList, index);

        if(strcmp(tProfile->name, profileName) == 0)
        {
            pthread_mutex_lock(&tProfile->tMutex);
            tProfile->terminated = true;
            pthread_cond_signal(&tProfile->tCond);
            pthread_mutex_unlock(&tProfile->tMutex);

            pthread_join(tProfile->tId, NULL);

            Vector_RemoveItem(profileList, tProfile, freeSchedulerProfile);
            pthread_mutex_unlock(&scMutex);

            T2Debug("%s --out\n", __FUNCTION__);
            return T2ERROR_SUCCESS;
        }
    }
    pthread_mutex_unlock(&scMutex);
    T2Info("profile: %s, not found in scheduler. Already removed\n", profileName);
    return T2ERROR_FAILURE;
}
