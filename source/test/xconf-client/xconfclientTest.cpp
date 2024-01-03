extern "C" {
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <bulkdata/reportprofiles.h>
#include <bulkdata/profilexconf.h>
#include <xconf-client/xconfclient.h>
#include <xconf-client/xconfclient.c>
#include <utils/t2MtlsUtils.h>
#include <t2parser/t2parserxconf.h>
#include <utils/vector.h>
#include <utils/persistence.h>
#include <telemetry2_0.h>
#include <ccspinterface/busInterface.h>
sigset_t blocking_signal;
}

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xconfclientMocks/fileMock.h"
#include <iostream>
#include <stdexcept>
using namespace std;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

FileioMock *m_fileIOMock = NULL;

class XconfcliTestFixture : public ::testing::Test {
    protected:
 
        FileioMock mockedpfileIO;

        XconfcliTestFixture()
        {
            m_fileIOMock = &mockedpfileIO;

        }
        virtual ~XconfcliTestFixture()
        {
            m_fileIOMock = NULL;
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

TEST(GETBUILDTYPE, NULL_CHECK)
{
    EXPECT_EQ(T2ERROR_FAILURE, getBuildType(NULL));
}

TEST(GETTIMEZONE, NULL_CHECK)
{
    EXPECT_EQ(NULL, getTimezone());
}

TEST(APPENDREQUEST, NULL_CHECK)
{
    char *buf = NULL;
    EXPECT_EQ(T2ERROR_FAILURE, appendRequestParams(buf,256));
    buf = (char*)malloc(256);
    EXPECT_EQ(T2ERROR_FAILURE, appendRequestParams(buf,0));
    free(buf);
    buf = NULL;
}


TEST(DOHTTPGET, HTTPURL_CHECK)
{
    char* data = NULL;
    EXPECT_EQ(T2ERROR_FAILURE,  doHttpGet(NULL, &data));
}

TEST(FETCHREMOTECONFIG, CONFIGURL_NULL_INVALID)
{
    char* configURL = "https://test.com";
    char* configData = NULL;
    EXPECT_EQ(T2ERROR_FAILURE, fetchRemoteConfiguration(configURL, &configData));
    EXPECT_EQ(T2ERROR_INVALID_ARGS, fetchRemoteConfiguration(NULL, &configData));
}


TEST_F(XconfcliTestFixture, getBuildType)
{
     FILE* mockfp = NULL;
     char build_type[256] = {0};
     EXPECT_CALL(*m_fileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(mockfp));
     ASSERT_EQ(T2ERROR_FAILURE, getBuildType(build_type));
}

TEST_F(XconfcliTestFixture, getTimezone)
{
    FILE* mockfp = NULL;
    EXPECT_CALL(*m_fileIOMock, fopen(_,_))
            .Times(1)
	    .WillOnce(Return(mockfp));
    EXPECT_CALL(*m_fileIOMock, access(_,_))
                .Times(11)
                .WillOnce(Return(-1))
                .WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1))
		.WillOnce(Return(-1));


    EXPECT_EQ(NULL, getTimezone());
}

TEST_F(XconfcliTestFixture, doHttpGet)
{
     char* data = NULL;
     EXPECT_CALL(*m_fileIOMock, pipe(_))
             .Times(1)
             .WillOnce(Return(-1));
     EXPECT_EQ(T2ERROR_FAILURE, doHttpGet("https://test.com", &data));
}

TEST_F(XconfcliTestFixture,  getRemoteConfigURL)
{
     char* configURL = NULL;
     #if defined(ENABLE_RDKB_SUPPORT)
     EXPECT_CALL(*m_fileIOMock, access(_,_))
                .Times(1)
                .WillOnce(Return(-1));
     EXPECT_EQ(T2ERROR_SUCCESS, getRemoteConfigURL(&configURL));
     #endif
}

TEST_F(XconfcliTestFixture, doHttpGet1)
{
     char* data = NULL;
     EXPECT_CALL(*m_fileIOMock, pipe(_))
             .Times(2)
             .WillOnce(Return(0))
	     .WillOnce(Return(-1));
     EXPECT_EQ(T2ERROR_FAILURE, doHttpGet("https://test.com", &data));
}
