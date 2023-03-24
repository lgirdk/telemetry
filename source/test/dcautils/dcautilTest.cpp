#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <iostream>
#include <stdexcept>
#include "test/mocks/SystemMock.h"
#include "test/mocks/FileioMock.h"

extern "C" {
#include <utils/vector.h>
#include <utils/t2log_wrapper.h>
#include <dcautil/dca.h>
#include <dcautil/legacyutils.h>
#include <dcautil/dcautil.h>
#include <dcautil/dcalist.h>
#include <telemetry2_0.h>
}
using namespace std;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

FileIOMock * g_ffileIOMock = NULL;
SystemMock * g_SSystemMock = NULL;

class dcaFileTestFixture : public ::testing::Test {
    protected:
        FileIOMock mockeddcaFileIO;

        dcaFileTestFixture()
        {
            g_ffileIOMock = &mockeddcaFileIO;

        }
        virtual ~dcaFileTestFixture()
        {
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

class dcaSystemTestFixture : public ::testing::Test {
    protected:
        SystemMock mockedSSystem;

        dcaSystemTestFixture()
        {
            g_SSystemMock = &mockedSSystem;

        }
        virtual ~dcaSystemTestFixture()
        {
            g_SSystemMock = NULL;
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

class dcaTestFixture : public ::testing::Test {
    protected:
        FileIOMock mockeddcaFileIO;
        SystemMock mockedSSystem;

        dcaTestFixture()
        {
            g_ffileIOMock = &mockeddcaFileIO;
            g_SSystemMock = &mockedSSystem;

        }
        virtual ~dcaTestFixture()
        {
            g_ffileIOMock = NULL;
            g_SSystemMock = NULL;
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


//dcalist.c

GList *pch = NULL;
TEST(INSERTPCNODE, PATT_NULL)
{
   EXPECT_EQ(0, insertPCNode(&pch, NULL, "SYS_INFO", STR, 0, "SYS_INFO_DATA"));
}

TEST(INSERTPCNODE, HEAD_NULL)
{
   EXPECT_EQ(0, insertPCNode(&pch, "info is", NULL, STR, 0, "SYS_INFO_DATA"));
}

TEST(INSERTPCNODE, DATA_NULL)
{
   EXPECT_EQ(0, insertPCNode(&pch, "info is", "SYS_INFO", STR, 0, NULL));
}

TEST(INSERTPCNODE, APPEND_CHECK)
{
   EXPECT_EQ(0, insertPCNode(&pch,  NULL, NULL, STR, 0, NULL));
}

TEST(COMPAREPATTERN, NP_SP_NULL_CHECK)
{
   pcdata_t *sp = (pcdata_t*)"SEARCH_DATA_LIST";
   pcdata_t *np = (pcdata_t*)"POINTER TO BE SEARCHED";
   EXPECT_EQ(-1, comparePattern(NULL, (gpointer)sp));
   EXPECT_EQ(-1, comparePattern(np, NULL));
}

TEST(SEARCHPCNODE, NULL_CHECK)
{
   GList *fnode = NULL;
   EXPECT_EQ(NULL, searchPCNode(NULL, "info is"));
   fnode = g_list_append(fnode, (gpointer)"Hello world!");
   EXPECT_EQ(NULL, searchPCNode(fnode, NULL));
}


//dcaproc.c

TEST(GETPROCUSAGE, GREPRESULTLIST_NULL)
{
   EXPECT_EQ(-1, getProcUsage("telemetry2_0", NULL));
}

TEST(GETPROCUSAGE, PROCESS_NULL)
{
   Vector* gresulist = NULL;
   Vector_Create(&gresulist);
   EXPECT_EQ(-1, getProcUsage(NULL, gresulist));
   Vector_Destroy(gresulist, free);
}

TEST(GETPROCPIDSTAT, PINFO_NULL)
{
   EXPECT_EQ(0, getProcPidStat(12345, NULL));
}

TEST(GETPROCINFO, PMINFO_NULL)
{
   EXPECT_EQ(0, getProcInfo(NULL));
}

TEST(GETMEMINFO, PMINFO_NULL)
{
   EXPECT_EQ(0, getMemInfo(NULL));
}

TEST(GETCPUINFO, PINFO_NULL)
{
   EXPECT_EQ(0, getCPUInfo(NULL));
}

//dcautil.c

char* gmarker1 = "SYS_INFO_BOOTUP";
char* dcamarker1 = "SYS_INFO1";
char* dcamarker2 = "SYS_INFO2";


/*TEST(SAVEGREPCONFIG, ARGS_NULL)
{
   Vector* greplist = NULL;
   Vector_Create(&greplist);
   Vector_PushBack(greplist, (void*) strdup(gmarker1));
   EXPECT_EQ(T2ERROR_FAILURE, saveGrepConfig(NULL, greplist));
   EXPECT_EQ(T2ERROR_FAILURE, saveGrepConfig("RDKB_Profile1", NULL));
   Vector_Destroy(greplist, free);
}
*/
TEST(GETGREPRESULTS, PROFILENAME_NULL)
{
   Vector* markerlist = NULL;
   Vector_Create(&markerlist);
   Vector_PushBack(markerlist, (void*) strdup(dcamarker1));
   Vector_PushBack(markerlist, (void*) strdup(dcamarker2));
   Vector* grepResultlist = NULL;
   Vector_Create(&grepResultlist);
   EXPECT_EQ(T2ERROR_FAILURE, getGrepResults(NULL, markerlist, &grepResultlist, false));
   EXPECT_EQ(T2ERROR_FAILURE, getGrepResults("RDKB_Profile1", NULL, &grepResultlist, false));
   EXPECT_EQ(T2ERROR_FAILURE, getGrepResults("RDKB_Profile1", markerlist, NULL, false));
   Vector_Destroy(markerlist, NULL);
}

//legacyutils.c

TEST(ADDTOPROFILESEEK, PROFILENM_NULL)
{
    EXPECT_EQ(NULL, addToProfileSeekMap(NULL));
}

TEST(ADDTOPROFILESEEK, PROFILENM_INVALID)
{
   EXPECT_EQ(NULL, addToProfileSeekMap("RDKB_INVALID_PROFILE1"));
}

TEST(GETLOGSEEKMAP, PROFILENM_NULL)
{
   EXPECT_EQ(NULL, getLogSeekMapForProfile(NULL));
}

TEST(GETLOGSEEKMAP, PROFILENM_INVALID)
{
   EXPECT_EQ(NULL, getLogSeekMapForProfile("RDKB_INVALID_PROFILE1"));
}

TEST(UPDATELOGSEEK, LOGSEEKMAP_NULL)
{
   char* logFileName = "t2_log.txt";
   hash_map_t* logseekmap = (hash_map_t *)malloc(512);
   EXPECT_EQ(T2ERROR_FAILURE,  updateLogSeek(NULL, logFileName));
   EXPECT_EQ(T2ERROR_FAILURE,  updateLogSeek(logseekmap, NULL));
   free(logseekmap);
   logseekmap = NULL;
}

TEST(GETLOADAVG, VECTOR_NULL)
{
    EXPECT_EQ(0, getLoadAvg(NULL));
}

TEST(GETLOGLINE, NAME_NULL)
{
    hash_map_t* logSeekMap = (hash_map_t *)malloc(512);
    char* buf = (char *)malloc(512);
    char* name = "Consolelog.txt.0";
    EXPECT_EQ(NULL, getLogLine(NULL, buf, 512, name));
    EXPECT_EQ(NULL, getLogLine(logSeekMap, NULL, 512, name));
    EXPECT_EQ(NULL, getLogLine(logSeekMap, buf, 512, NULL));
    free(buf);
    buf = NULL;
}

TEST(ISPROPSINITIALIZED, TESTING)
{
    EXPECT_EQ(false, isPropsInitialized());
}

//dca.c

TEST(PROCESSTOPPATTERN, VECTOR_NULL)
{
    char* logfile = "Consolelog.txt.0";
     GList *tlist = NULL;
     tlist =  g_list_append(tlist, (gpointer)"Hello world!");
    Vector* grepResultlist = NULL;
    Vector_Create(&grepResultlist);
    EXPECT_EQ(-1, processTopPattern(NULL, tlist, 0, grepResultlist));
    EXPECT_EQ(-1, processTopPattern(logfile, NULL, 0, grepResultlist));
    EXPECT_EQ(-1, processTopPattern(logfile, tlist, 0, NULL));
}

TEST(GETERRORCODE, STR_NULL)
{
    char* str = "telemetry-dcautil";
    EXPECT_EQ(0, getErrorCode(str, NULL));
}

TEST(STRSPLIT, STR_DLI_NULL)
{
    EXPECT_EQ(NULL, strSplit(NULL, "##"));
    EXPECT_EQ(NULL, strSplit("this## is ## a ## test string", NULL));
}

#if 0
TEST_F(dcaFileTestFixture, getProcPidStat)
{
    procinfo pinfo;
    int fd = (int)0xffffffff;
    EXPECT_CALL(*g_ffileIOMock, open(_,_))
            .Times(1)
            .WillOnce(Return(-1));
    ASSERT_EQ(0, getProcPidStat(123, &pinfo));
}

TEST_F(dcaFileTestFixture, getProcPidStat1)
{
    procinfo pinfo;
    int fd = (int)0xffffffff;
    EXPECT_CALL(*g_ffileIOMock, open(_,_))
            .Times(1)
            .WillOnce(Return(fd));
    EXPECT_CALL(*g_ffileIOMock, read(_,_,_))
            .Times(1)
            .WillOnce(Return(-1));
    EXPECT_CALL(*g_ffileIOMock, close(_))
            .Times(1)
            .WillOnce(Return(0));
    ASSERT_EQ(0, getProcPidStat(123, &pinfo));
}

TEST_F(dcaFileTestFixture, getTotalCpuTimes)
{
    FILE* mockfp = (FILE *)NULL;
    int t[2];
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(mockfp));

    ASSERT_EQ(0, getTotalCpuTimes(&t[0]));
}

TEST_F(dcaFileTestFixture, getTotalCpuTimes1)
{
    FILE* mockfp = (FILE *)0xffffffff;
    int t[2];
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(mockfp));
    EXPECT_CALL(*g_ffileIOMock, fclose(_))
            .Times(1)
            .WillOnce(Return(-1));
    ASSERT_EQ(0, getTotalCpuTimes(&t[0]));
}

TEST_F(dcaTestFixture, getCPUInfo)
{
    FILE *inFp = (FILE*)NULL;
    #ifdef LIBSYSWRAPPER_BUILD
       EXPECT_CALL(*g_SSystemMock, v_secure_system(_))
                .Times(1)
                .WillOnce(Return(0));
    #else
       EXPECT_CALL(*g_SSystemMock, system(_))
                .Times(1)
                .WillOnce(Return(0));
    #endif
    #ifdef INTEL
       #ifdef LIBSYSWRAPPER_BUILD

           EXPECT_CALL(*g_ffileIOMock, v_secure_popen(_,_))
                .Times(1)
                .WillOnce(Return(inFp));
       #else
           EXPECT_CALL(*g_ffileIOMock, popen(_,_))
                .Times(1)
                .WillOnce(Return(inFp));
       #endif
    #endif
         procMemCpuInfo pinfo;
         char* processName = "telemetry";
         memset(&pinfo, '\0', sizeof(procMemCpuInfo));
         memcpy(pinfo.processName, processName, strlen(processName) + 1);
         ASSERT_EQ(0,  getCPUInfo(&pinfo));
}

TEST_F(dceTestFixture, saveGrepConfig)
{
    char* marker = "SYS_INFO";
    Vector* grepMarkerlist = NULL;
    Vector_Create(&grepMarkerlist);
    FILE* inFp = (FILE*)NULL;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
                .Times(1)
                .WillOnce(Return(inFp));

    ASSERT_EQ(T2ERROR_FAILURE, saveGrepConfig(marker, grepMarkerlist));
}

TEST_F(dcaFileTestFixture, saveGrepConfig1)
{
    char* marker = "SYS_INFO";
    Vector* grepMarkerlist = NULL;
    Vector_Create(&grepMarkerlist);
    FILE* inFp = (FILE*)NULL;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
                .Times(1)
                .WillOnce(Return(inFp));

    ASSERT_EQ(T2ERROR_FAILURE, saveGrepConfig(marker, grepMarkerlist));
}

TEST_F(dcaFileTestFixture, saveGrepConfig2)
{
    char* marker = "SYS_INFO";
    Vector* grepMarkerlist = NULL;
    Vector_Create(&grepMarkerlist);
    FILE* inFp = (FILE*)0xffffffff;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
                .Times(1)
                .WillOnce(Return(inFp));
    EXPECT_CALL(*g_ffileIOMock, fclose(_))
                .Times(1)
                .WillOnce(Return(-1));

    ASSERT_EQ(T2ERROR_FAILURE, saveGrepConfig(marker, grepMarkerlist));
}


TEST_F(dcaFileTestFixture,  getProcUsage)
{
    Vector* gresulist = NULL;
    Vector_Create(&gresulist);
    char* processName = "telemetry2_0";
    FILE* fp = (FILE*)NULL;
    EXPECT_CALL(*g_ffileIOMock, popen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    EXPECT_EQ(0, getProcUsage(processName, gresulist));
}

TEST_F(dcaFileTestFixture,  getProcUsage1)
{
    Vector* gresulist = NULL;
    Vector_Create(&gresulist);
    char* processName = "telemetry2_0";
    FILE* fp = (FILE*)0xffffffff;
     EXPECT_CALL(*g_ffileIOMock, popen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
     EXPECT_CALL(*g_ffileIOMock, pclose(_))
             .Times(1)
             .WillOnce(Return(-1));
    EXPECT_EQ(0, getProcUsage(processName, gresulist));
}

TEST_F( dcaFileTestFixture, addToProfileSeekMap)
{
    char* profileName = "Profile1";
    EXPECT_CALL(*g_ffileIOMock, stat(_,_))
           .Times(1)
           .WillOnce(Return(-1));
    addToProfileSeekMap(profileName);
}

TEST_F(dcaFileTestFixture, getLoadAvg)
{
    Vector* grepResultList;
    Vector_Create(&grepResultList);
    FILE* fp = (FILE*)NULL;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    EXPECT_EQ(0, getLoadAvg(grepResultList));
}

TEST_F(dcaFileTestFixture, getLoadAvg1)
{
    Vector* grepResultList;
    Vector_Create(&grepResultList);
    FILE* fp = (FILE*)0xffffffff;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    EXPECT_CALL(*g_ffileIOMock, fread(_,_,_,_))
            .Times(1)
            .WillOnce(Return(-1));
    EXPECT_CALL(*g_ffileIOMock, fclose(_))
            .Times(1)
            .WillOnce(Return(0));
    EXPECT_EQ(0, getLoadAvg(grepResultList));
}

TEST_F(dcaFileTestFixture, getLogLine)
{
    hash_map_t* logSeekMap = (hash_map_t *)malloc(512);
    char* buf = (char *)malloc(512);
    char* name = "Consolelog.txt.0";
    FILE* fp = (FILE*)NULL;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    EXPECT_EQ(NULL, getLogLine(logSeekMap, buf, 10, name));
}

TEST_F(dcaFileTestFixture, updateLastSeekval)
{
    hash_map_t *logSeekMap = ( hash_map_t*)malloc(512);
    char* prev_file = NULL;
    char* filename = "Consolelog.txt.0";
    FILE* fp = (FILE*)NULL;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    updateLastSeekval(logSeekMap, &prev_file, filename);
}

TEST_F(dcaFileTestFixture, updateLastSeekval1)
{
    hash_map_t *logSeekMap = ( hash_map_t*)malloc(512);
    char* prev_file = NULL;
    char* filename = "Consolelog.txt.0";
    FILE* fp = (FILE*)0xffffffff;
    EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    EXPECT_CALL(*g_ffileIOMock, fclose(_))
            .Times(1)
            .WillOnce(Return(0));
    updateLastSeekval(logSeekMap, &prev_file, filename);
}

TEST_F(dcaFileTestFixture, getLogLine1)
{
    hash_map_t* logSeekMap = (hash_map_t *)malloc(512);
    char* buf = (char *)malloc(512);
    char* name = "core_log.txt";
     FILE* fp = (FILE*)0xffffffff;
     EXPECT_CALL(*g_ffileIOMock, fopen(_,_))
            .Times(1)
            .WillOnce(Return(fp));
    EXPECT_CALL(*g_ffileIOMock, fseek(_,_,_))
            .Times(1)
            .WillOnce(Return(-1));
     EXPECT_EQ(NULL, getLogLine(logSeekMap, buf, 10, name));

}

#endif
