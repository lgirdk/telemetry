/*
* Copyright 2020 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

extern "C" {
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <telemetry2_0.h>
#include <utils/vector.h>
#include <utils/t2MtlsUtils.h>
#include <signal.h>
#include <protocol/http/curlinterface.h>
#include <protocol/rbusMethod/rbusmethodinterface.h>
#include <bulkdata/reportprofiles.h>
#include <ccspinterface/busInterface.h>
#include <ccspinterface/rbusInterface.h>
#include <reportgen/reportgen.h>
sigset_t blocking_signal;
}

#include "gmock/gmock.h"
#include "gtest/gtest.h"
using namespace std;

#include <iostream>
#include <stdexcept>
#include "test/mocks/SystemMock.h"
#include "test/mocks/FileioMock.h"


using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

SystemMock * g_ssystemMock = NULL; 
FileIOMock * g_ffileIOMock = NULL;

class ProtocolTestFixture : public ::testing::Test {
    protected:
        SystemMock mockedSsystem;
        FileIOMock mockedFfileIO;

        ProtocolTestFixture()
        {
            g_ssystemMock = &mockedSsystem;
            g_ffileIOMock = &mockedFfileIO;

        }
        virtual ~ProtocolTestFixture()
        {
            g_ssystemMock = NULL;
            g_ffileIOMock = NULL;
        }

         virtual void SetUp()
        {
            printf("%s\n", __func__);
        }

        virtual void TearDown()
        {
            printf("%s\n", __func__);
        }

        static void SetUpTestCase()
        {
            printf("%s\n", __func__);
        }

        static void TearDownTestCase()
        {
            printf("%s\n", __func__);
        }

};

TEST(SENDREPORTOVERHTTP, 1_NULL_CHECK)
{
    char *payload = "This is a payload string";
    EXPECT_EQ(T2ERROR_FAILURE, sendReportOverHTTP(NULL, payload, NULL));
}

TEST(SENDREPORTOVERHTTP, 2_NULL_CHECK)
{
    char *url = "https://test.com";
    EXPECT_EQ(T2ERROR_FAILURE, sendReportOverHTTP(url, NULL, NULL));
}

TEST(SENDCACREPOVERHTTP, 1_NULL_CHECK)
{
    Vector* reportlist;
    Vector_Create(&reportlist);
    EXPECT_EQ(T2ERROR_FAILURE, sendCachedReportsOverHTTP(NULL, reportlist));
    Vector_Destroy(reportlist, free);
}

TEST(SENDCACREPOVERHTTP, 2_NULL_CHECK)
{
    char *url = "https://test.com";
    EXPECT_EQ(T2ERROR_FAILURE, sendCachedReportsOverHTTP(url, NULL));
}

TEST(SENDRBUDREPORTOVERRBUS, 1_NULL_CHECK)
{    
    Vector* inputParams = NULL;
    Vector_Create(&inputParams);
    char* payload = "This is a payload string";
    EXPECT_EQ(T2ERROR_FAILURE, sendReportsOverRBUSMethod(NULL, inputParams, payload));
    Vector_Destroy(inputParams, free);
}

TEST(SENDRBUDREPORTOVERRBUS,2_NULL_CHECK)
{
     char* method = "RBUS_METHOD";
     char* payload = "This is a payload string";
     EXPECT_EQ(T2ERROR_FAILURE, sendReportsOverRBUSMethod(method, NULL, payload));
}

TEST(SENDRBUDREPORTOVERRBUS,3_NULL_CHECK)
{
    char* method = "RBUS_METHOD";
    Vector* inputParams = NULL;
    Vector_Create(&inputParams);
    EXPECT_EQ(T2ERROR_FAILURE, sendReportsOverRBUSMethod(method, inputParams, NULL));
    EXPECT_EQ(T2ERROR_FAILURE, sendReportsOverRBUSMethod(NULL, NULL, NULL));
    Vector_Destroy(inputParams, free);
}

TEST(SENDRBUSCACHEREPORTOVERRBUS, NULL_CHECK)
{
    char* method = "RBUS_METHOD";
    Vector* inputParams = NULL;
    Vector_Create(&inputParams);
    Vector* reportList = NULL;
    Vector_Create(&reportList);
    EXPECT_EQ(T2ERROR_FAILURE, sendCachedReportsOverRBUSMethod(NULL, inputParams, reportList));
    EXPECT_EQ(T2ERROR_FAILURE, sendCachedReportsOverRBUSMethod(method, NULL, reportList));
    EXPECT_EQ(T2ERROR_FAILURE, sendCachedReportsOverRBUSMethod(method, inputParams, NULL));
    EXPECT_EQ(T2ERROR_FAILURE, sendCachedReportsOverRBUSMethod(NULL,NULL,NULL));
    Vector_Destroy(inputParams, free);
    Vector_Destroy(reportList, free);
}
