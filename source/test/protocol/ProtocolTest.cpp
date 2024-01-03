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
#include <protocol/http/curlinterface.c>
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

SystemMock * g_psystemMock = NULL; 
FileIOMock * g_pfileIOMock = NULL;

class ProtocolTestFixture : public ::testing::Test {
    protected:
        SystemMock mockedpsystem;
        FileIOMock mockedpfileIO;

        ProtocolTestFixture()
        {
            g_psystemMock = &mockedpsystem;
            g_pfileIOMock = &mockedpfileIO;

        }
        virtual ~ProtocolTestFixture()
        {
            g_psystemMock = NULL;
            g_pfileIOMock = NULL;
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

class ProtocolFileTestFixture : public ::testing::Test {
    protected:
        FileIOMock mockedpfileIO;

        ProtocolFileTestFixture()
        {
            g_pfileIOMock = &mockedpfileIO;

        }
        virtual ~ProtocolFileTestFixture()
        {
            g_pfileIOMock = NULL;
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

/*
TEST(SETHEADER, CURL_NULL)
{
    char* destURL = "https://google.com";
    EXPECT_EQ(T2ERROR_FAILURE, setHeader(NULL, destURL, NULL));
}

TEST(SETHEADER, DEST_NULL)
{
    CURL* curl = curl_easy_init();
     EXPECT_EQ(T2ERROR_FAILURE, setHeader(curl, NULL, NULL));
}

const char* certFile = "certs/etyeu.txt";
const char* passwd = "euyeurywi";
CURL* curl = curl_easy_init();
TEST(SETMTLSHEADERS, NULL_CHECK)
{
   EXPECT_EQ(T2ERROR_FAILURE,  setMtlsHeaders(NULL, certFile, passwd));
   EXPECT_EQ(T2ERROR_FAILURE,  setMtlsHeaders(curl, NULL, passwd));
   EXPECT_EQ(T2ERROR_FAILURE,  setMtlsHeaders(curl, certFile, NULL));
}

const char* payload = "This is a payload string";
TEST(SETPAYLOAD, NULL_CHECK)
{
   EXPECT_EQ(T2ERROR_FAILURE, setPayload(NULL, payload));
   EXPECT_EQ(T2ERROR_FAILURE, setPayload(curl, NULL));
}
*/

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

TEST(SENDRBUDREPORTOVERRBUS, 2_NULL_CHECK)
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
#if 0

TEST_F(ProtocolFileTestFixture, WRITETOFILE)
{
     char str[] = "This is tutorialspoint.com";
     FILE* fp = NULL;
     fp = fopen( "/tmp/file.txt" , "w" );
     EXPECT_CALL(*g_pfileIOMock, fwrite(_, _, _, _))
           .Times(1)
           .WillOnce(Return(0));
     EXPECT_EQ(0, writeToFile(str, 0, 0, fp));
}


TEST_F(ProtocolFileTestFixture, SENDREPORTOVERHTTP)
{
     char* httpURL = "https://stbrtl.r53.xcal.tv";
     char* payload = "This is a payload string";
     EXPECT_CALL(*g_pfileIOMock, pipe(_))
             .Times(1)
             .WillOnce(Return(-1));
     EXPECT_EQ(T2ERROR_FAILURE, sendReportOverHTTP(httpURL, payload, NULL));
}

TEST_F(ProtocolFileTestFixture, SENDREPORTOVERHTTP1)
{
     char* httpURL = "https://stbrtl.r53.xcal.tv";
     char* payload = "This is a payload string";
     CURL *curl = (CURL*)NULL;
     EXPECT_CALL(*g_pfileIOMock, pipe(_))
             .Times(1)
             .WillOnce(Return(0));
     EXPECT_CALL(*g_pfileIOMock, curl_easy_init())
             .Times(1)
             .WillOnce(::testing::ReturnNull());
     EXPECT_EQ(T2ERROR_FAILURE, sendReportOverHTTP(httpURL, payload, NULL));
}

TEST_F(ProtocolFileTestFixture, SENDREPORTOVERHTTP2)
{
     char* httpURL = "https://stbrtl.r53.xcal.tv";
     char* payload = "This is a payload string";
     CURL *curl = (CURL*)0xffffffff;
     FILE* fp = (FILE*)0xffffffff;
      CURLcode code = CURLE_OK;
     
     EXPECT_CALL(*g_pfileIOMock, pipe(_))
             .Times(1)
             .WillOnce(Return(0));
     EXPECT_CALL(*g_pfileIOMock, curl_easy_init())
             .Times(1)
             .WillOnce(Return(curl));

     EXPECT_CALL(*g_pfileIOMock, fopen(_,_))
             .Times(1)
             .WillOnce(Return(fp));
     EXPECT_CALL(*g_pfileIOMock, curl_easy_perform(_))
             .Times(1)
             .WillOnce(Return(code));
     EXPECT_CALL(*g_pfileIOMock, fclose(_))
              .Times(1)
              .WillOnce(Return(0));
     sendReportOverHTTP(httpURL, payload, NULL);
}
#endif
