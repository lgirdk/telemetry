extern "C" 
{
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
#include <scheduler/scheduler.h>
#include <utils/vector.h>
#include <bulkdata/profile.h>
#include <bulkdata/reportprofiles.h>
#include <utils/t2log_wrapper.h>	
sigset_t blocking_signal;
}
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <iostream>
#include <stdexcept>

using namespace std;


TEST(GET_LOGDEMAND, TEST1)
{
   EXPECT_EQ( false, get_logdemand());
}

TEST(GETLAPSEDTIME, T1_GT_T2)
{
    struct timespec time1;
    struct timespec time2;
    struct timespec output;
    time1.tv_nsec = 110196606;
    time1.tv_sec = 1683887703;
    time2.tv_nsec = 2919488;
    time2.tv_sec = 1683887687;
    EXPECT_EQ(0, getLapsedTime(&output, &time1, &time2));
}

TEST(GETLAPSEDTIME, T2_GT_T1)
{
    struct timespec time1;
    struct timespec time2;
    struct timespec output;
    time2.tv_nsec = 110196606;
    time2.tv_sec = 1683887703;
    time1.tv_nsec = 2919488;
    time1.tv_sec = 1683887687; 
    EXPECT_EQ(1, getLapsedTime(&output, &time1, &time2));
}

TEST(REGISTERSCHEWITHPROFILE, TEST1)
{
   EXPECT_EQ(T2ERROR_INVALID_ARGS,  registerProfileWithScheduler(NULL, 50, 3600, true, true, true, 10, "2022-12-20T11:05:56Z"));
}

TEST(UNREGISTERPROFILEFROMSCH, TEST1)
{
    EXPECT_EQ(T2ERROR_SUCCESS,  unregisterProfileFromScheduler("RDKB_PROFILE1"));
}

TEST(UNREGISTERPROFILEFROMSCH, TEST2)
{
    EXPECT_EQ(T2ERROR_INVALID_ARGS,  unregisterProfileFromScheduler(NULL));
}


void ReportProfiles_ToutCb(const char* profileName, bool isClearSeekMap)
{
    printf("ReportProfiles_ToutCb is done\n");
}

void ReportProfiles_ActivationToutCb(const char* profileName)
{
    printf("ReportProfiles_ActivationTimeoutCb is done\n");
}

void schedulercb(char* profileName, bool isschedulerstarted)
{
    printf("schedulercb is done\n");
}

TEST(initScheduler, NULL_CALLBACK)
{
    EXPECT_EQ(T2ERROR_SUCCESS,  initScheduler((TimeoutNotificationCB)NULL, (ActivationTimeoutCB)NULL, (NotifySchedulerstartCB)NULL));
}

TEST(initScheduler, NON_NULL_CALLBACK)
{
    EXPECT_EQ(T2ERROR_SUCCESS,  initScheduler((TimeoutNotificationCB)ReportProfiles_ToutCb, (ActivationTimeoutCB)ReportProfiles_ActivationToutCb, (NotifySchedulerstartCB)schedulercb));
}

TEST(SendInterruptToTimeoutThread, NULL_CHECK)
{
    EXPECT_EQ(T2ERROR_INVALID_ARGS, SendInterruptToTimeoutThread(NULL));
}
