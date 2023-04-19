#include "gtest/gtest.h"
#include <iostream>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

extern "C" {
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <cjson/cJSON.h>
#include <bulkdata/reportprofiles.h>
#include <bulkdata/profilexconf.h>
#include <bulkdata/profile.h>
#include <utils/t2common.h>
#include <xconf-client/xconfclient.h>
#include <t2parser/t2parser.h>
#include <t2parser/t2parserxconf.h>
#include <telemetry2_0.h>
#include <ccspinterface/busInterface.h>
#include <glib.h>
#include <glib/gi18n.h>
sigset_t blocking_signal;
}

TEST(PROCESSXCONFCONFIGURATION, TEST_NULL_INVALID)
{
     ProfileXConf *profile = 0;
     fstream new_file;
     char* data = NULL;
     new_file.open("xconfInputfile.txt", ios::in);
     if (new_file.is_open()) {
        string sa;
        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE, processConfigurationXConf(data, &profile));
        getline(new_file, sa);

	std::copy(sa.begin(), sa.end(), data);
	EXPECT_EQ(T2ERROR_FAILURE, processConfigurationXConf(data, &profile));
	getline(new_file, sa);

        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE, processConfigurationXConf(data, &profile));
	getline(new_file, sa);

        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE, processConfigurationXConf(data, &profile));
	getline(new_file, sa);

        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE, processConfigurationXConf(data, &profile));
     }
     new_file.close();
}

TEST(PROCESSCONFIGURATION_CJSON, TEST_NULL_INVALID_PARAM)
{
    Profile *profile = 0;
    fstream new_file;
    string sa;
    new_file.open("rpInputfile.txt", ios::in);
    char* data = NULL;
    if (new_file.is_open()) {
	getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //Profilename NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, NULL, "hash1", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //Protocol NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash2", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //Encoding NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash3", &profile));

        getline(new_file, sa); 
        std::copy(sa.begin(), sa.end(), data);
        //Parameter NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash4", &profile));

       	getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //HASH NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash5", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //RI<ML NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash6", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//!RI!TC
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash7", &profile));

	getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash8", &profile));
       //Protocol HTTP
       getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash9", &profile));
	getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash10", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //RBUS_METHOD_PARAM_NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash11", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//Unsupported protocol
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash12", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	 //ReportTimeformat NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash13", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //RI_GT_AT_NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash14", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//TC type
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash15", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//Type!=datamodel
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash16", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//operator NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash17", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//operator invalid
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash18", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //threshold
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash19", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
        //Ref NULL
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash20", &profile));

        getline(new_file, sa);
        std::copy(sa.begin(), sa.end(), data);
	//Ref Invalid
        EXPECT_EQ(T2ERROR_FAILURE,  processConfiguration(&data, "RDKB_Profile", "hash21", &profile));
    }
    new_file.close();
}

TEST(PROCESSCONFIGURATION_MSGPACK, PRONAME_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6kTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQl9Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCsRW5jb2RpbmdUeXBlpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWw8sUFjdGl2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRlTm93wqhSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAyMi0xMi0xOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKR0eXBlqWRhdGFNb2RlbKRuYW1lplVQVElNRalyZWZlcmVuY2W4RGV2aWNlLkRldmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEpHR5cGWlZXZlbnSpZXZlbnROYW1lr1VTRURfTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW50o3VzZahhYnNvbHV0Zd8AAAAFpHR5cGWkZ3JlcKZtYXJrZXLZIlNZU19JTkZPX0NyYXNoUG9ydGFsVXBsb2FkX3N1Y2Nlc3Omc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADrlJlcG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRlct0AAAAB3wAAAAKkTmFtZapyZXBvcnROYW1lqVJlZmVyZW5jZaxQcm9maWxlLk5hbWWsSlNPTkVuY29kaW5n3wAAAAKsUmVwb3J0Rm9ybWF0rU5hbWVWYWx1ZVBhaXKvUmVwb3J0VGltZXN0YW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, PROTOCOL_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUKRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29soKxFbmNvZGluZ1R5CGWKSINPTIFSZXBvcnRpbmdJbnRlcnZhbDyxQWN0aXZhdGlvblRpbWVvdXTNDhCrR2VuZXJhdGVOb3fCqFJvb3ROYW1lqkZSMI9VU19UQZKtVGIZVJIZmVyZW5jZbQyMDlyLTEYLTE5VDA5OjMzOjU2WqlQYXJhbWVOZXLdAAAAA98AAAAEрHR5CGWPZGF0YU1VZGVspG5hbWWmVVBUSU1FqXJlZmVyZW5jZbhEZXZpY2UURGV2aWNISW5mby5VcFRpbWWjdXNlqGFic29sdXRI3wAAAASkdHlwZavidmVudKidmvadE5hbWWvVVNFRF9NRUOXX3NwbGI0qWNvbXBvbmVudKzzeXNpbnSjdxNfqGFIc29sdXRI3wAAAAWkdHlwZaRncmVwpm1hcmtctkiU1TXOIORk9f03Jhc2hQb3J0YWXVcGxvYWRfc3VjY2Vzc6ZzZWFyY2ixU3VjY2VzcyB1cGxvYWRpbmenbG9nRmlsZaxjb3JIX2xvZy50eHSjdXNlpWNvdW50tFJIcG9ydGluZOFkanVzdG11bnRz3wAAAAOuUmVwb3JÓT25VcGRhdGXCtkZpcnN0UmVwb3J0aW5nSW50ZXJ2YWWPSE1heFVwbG9hZExhdGVuY3nNw1CKSFRUUN8AAAAE01VSTLtodHRwczovL3N0YnJ0bC5yNTMueGNhbC50di+rQ29tcHJlc3Npb26kTm9uZaZNZXRob2SkUE9TVLNSZXF1ZXNOVVJJUGFyYW1ldGVy3QAAAAHfAAAAAQROYW1lgnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVIUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, ENCODING_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIOLFSZXBvcnRpbmdJbnRlcnZhbDyxQWN0aXZhdGlvblRpbWVvdXTNDhCrR2VuZXJhdGVOb3fCqFJvb3ROYW1lqkZSMI9VU19UQZKtVGIZVJIZmVyZW5jZbQyMDlyLTEYLTE5VDA5OjMzOjU2WqlQYXJhbWVOZXLdAAAAA98AAAAEрHR5CGWPZGF0YU1VZGVspG5hbWWmVVBUSU1FqXJlZmVyZW5jZbhEZXZpY2UURGV2aWNISW5mby5VcFRpbWWjdXNlqGFic29sdXRI3wAAAASkdHlwZavidmVudKidmvadE5hbWWvVVNFRF9NRUOXX3NwbGI0qWNvbXBvbmVudKzzeXNpbnSjdxNfqGFIc29sdXRI3wAAAAWkdHlwZaRncmVwpm1hcmtctkiU1TXOIORk9f03Jhc2hQb3J0YWXVcGxvYWRfc3VjY2Vzc6ZzZWFyY2ixU3VjY2VzcyB1cGxvYWRpbmenbG9nRmlsZaxjb3JIX2xvZy50eHSjdXNlpWNvdW50tFJIcG9ydGluZOFkanVzdG11bnRz3wAAAAOuUmVwb3JÓT25VcGRhdGXCtkZpcnN0UmVwb3J0aW5nSW50ZXJ2YWWPSE1heFVwbG9hZExhdGVuY3nNw1CKSFRUUN8AAAAE01VSTLtodHRwczovL3N0YnJ0bC5yNTMueGNhbC50di+rQ29tcHJlc3Npb26kTm9uZaZNZXRob2SkUE9TVLNSZXF1ZXNOVVJJUGFyYW1ldGVy3QAAAAHfAAAAAQROYW1lgnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVIUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, PARAM_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEPTT06xUmVwb3J0aW5nSW50ZXJ2YWwesUFjdGI2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRITm93wghSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAYMIOXMIOXOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAACOUmVwb3J0aW5nQWRqdXN0bWVudHPfAAAAA65SZXBvcnRPbIVwZGF0ZcK2Rmlyc3RSZXBvcnRpbmdJbnRlcnZhbA+WTWF4VXBsb2FkTGF0ZW5jec3DUKRIVFRQ&wAAAASVVJMu2h0dHBzOi8vc3RicnRsLn11My54Y2FsLnR2L6tDb21wcmVzc2lvbqROD251pk1ldGhvZKRQT1NUS1JlcXVIC3RVUKIQYXJhbWVOZXLdAAAAABAAAACpE5hbWWqcmVwb3J0TmFtZaISZWZlcmVuY2WSUHJvZmlsZS5OYW1lrEpTTOSFbmNvZGluZ98AAAACrFJlcG9ydEZvcm1hdK10YW1lVmFsdWVQYWlyr1JlcG9ydFRpbWVzdGFtcKROb25l";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, HASH_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWwesUFjdGI2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRITm93wghSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAYMIOXMIOXOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKROEXBIqWRhdGFNb2RlbKRuYW1lp/VQVEINRalyZWZlcmVuY2W4RGV2aWNILKRIdmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEрHRSCGWIZXZionSpZXZIbnROYW1l1VTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW5003VzZahhYnNvbHVOZd8AAAAFpHR5cGWkZ3JICKZtYXJrZXLZIINZU19JTKZPXONYYXNOUG9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADriJicG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZOludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUky7aHROcHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SZUmVxdWVzdFVSSVBhcmFtZXRlct0AAAAB3wAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxILk5hbWWsSINPTkVuY29kaW5n3wAAAAKSUmVwb3J0Rm9ybWF0rU5hbWVWYWx1ZVBhaXKvUmVwb3J0VGItZXNOYW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}


TEST(PROCESSCONFIGURATION_MSGPACK, RL_LT_ML)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWwesUFjdGI2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRITm93wghSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAYMIOXMIOXOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKROEXBIqWRhdGFNb2RlbKRuYW1lp/VQVEINRalyZWZlcmVuY2W4RGV2aWNILKRIdmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEрHRSCGWIZXZionSpZXZIbnROYW1l1VTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW5003VzZahhYnNvbHVOZd8AAAAFpHR5cGWkZ3JICKZtYXJrZXLZIINZU19JTKZPXONYYXNOUG9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADriJicG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZOludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUky7aHROcHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SZUmVxdWVzdFVSSVBhcmFtZXRlct0AAAAB3wAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxILk5hbWWsSINPTkVuY29kaW5n3wAAAAKSUmVwb3J0Rm9ybWF0rU5hbWVWYWx1ZVBhaXKvUmVwb3J0VGItZXNOYW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, NT_RI_NT_TC)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA2KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCrR2VuZXJhdGVOb3fCqFJvb3ROYW1lqkZSMI9VU19UQzKtVGIZVJIZmVyZW5jZbQyMDIyLTEyLTE5VDA5OjMzOjU2WqlQYXJhbWVOZXLdAAAAA98AAAAEpHRSCGWPZGF0YU1vZGVspG5hbWWmVVBUSU1FqXJIZmVyZW5jZbhEZXZpY2UURGV2aWNISW5mby5VcFRpbWWjdXNlqGFic29sdXRI3wAAAASkdHlwZaVidmVudKildmVudE5hbWwyVVNFRF9NRUOXX3NWbGI0qWNvbXBvbmVudKZzeXNpbnSjdXNlqGFic29sdXRI3WAAAAWkdHlwZaRncmVwpm1hcmtlctkiU1ITX0IORk9fQ3Jhc2hQb3J0YWxveGxvYWRfc3VjY2VZc6ZzZWFyY2ixU3VjY2VzcyB1cGxvYWRpbmenbG9nRmlsZaxjb3JlX2xvZy50eHSjdXNpWNvdW50tFJIcG9ydGluZOFkanVzdG1lbnRz3wAAAAOuUmVwb3J0T25VcGRhdGXCtkZpcnN0UmVwb3J0aW5nSW50ZXJ2YWwPSE1heFVwbG9hZExhdGVuY3nNw1CkSFRUUN8AAAAE01VSTLtodHRwczovL3N0YnJobC5yNTMueGNhbC50di+rQ29tcHJlc3Npb26kTm9uZaZNZXRob2SkUE9TVLNSZXF1ZXNOVVJJUGFyYW1ldGVy3QAAAAHfAAAAAqROYW1lqnJicG9ydE5hbWWpUmVmZXJlbmNIrFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVIUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, PROTOCOL_HTTP)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEPTT06xUmVwb3J0aW5nSW50ZXJ2YWw8sUFjdGI2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRITm93wghSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAYMIOXMIOXOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKROEXBIqWRhdGFNb2RlbKRuYW1lp/VQVEINRalyZWZlcmVuY2W4RGV2aWNILKRIdmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEрHRSCGWIZXZionSpZXZIbnROYW1l1VTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW5003VzZahhYnNvbHVOZd8AAAAFpHR5cGWkZ3JICKZtYXJrZXLZIINZU19JTKZPXONYYXNOUG9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADriJlcG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZOludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUkygqONVbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRIct0AAAAB3WAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxlLk5hbWWSSINPTkVuY29kaW5n3wAAAAKsUmVwb3J0Rm9ybWF0rU5hbWVWYWx'ZVBhaXKVUmVwb3J0VGItZXNOYW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, PROTOCOL_HTTP1)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6kTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQl9Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCsRW5jb2RpbmdUeXBlpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWw8sUFjdGl2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRlTm93wqhSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAyMi0xMi0xOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKR0eXBlqWRhdGFNb2RlbKRuYW1lplVQVElNRalyZWZlcmVuY2W4RGV2aWNlLkRldmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEpHR5cGWlZXZlbnSpZXZlbnROYW1lr1VTRURfTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW50o3VzZahhYnNvbHV0Zd8AAAAFpHR5cGWkZ3JlcKZtYXJrZXLZIlNZU19JTkZPX0NyYXNoUG9ydGFsVXBsb2FkX3N1Y2Nlc3Omc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADrlJlcG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9uoKZNZXRob2SkUE9TVLNSZXF1ZXN0VVJJUGFyYW1ldGVy3QAAAAHfAAAAAqROYW1lqnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVlUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, PROTOCOL_HTTP2)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6kTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQl9Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCsRW5jb2RpbmdUeXBlpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWw8sUFjdGl2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRlTm93wqhSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAyMi0xMi0xOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKR0eXBlqWRhdGFNb2RlbKRuYW1lplVQVElNRalyZWZlcmVuY2W4RGV2aWNlLkRldmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEpHR5cGWlZXZlbnSpZXZlbnROYW1lr1VTRURfTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW50o3VzZahhYnNvbHV0Zd8AAAAFpHR5cGWkZ3JlcKZtYXJrZXLZIlNZU19JTkZPX0NyYXNoUG9ydGFsVXBsb2FkX3N1Y2Nlc3Omc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADrlJlcG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9koLNSZXF1ZXN0VVJJUGFyYW1ldGVy3QAAAAHfAAAAAqROYW1lqnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVlUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, RBUS_METHOD_PARAM_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAAykTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb263Q2hlY2sgTXVsdGkgcHJvZmxpZSBvbmWnVmVyc2lvbqExqFByb3RvY29sq1JCVVNFTUVUSE9ETEVuY29kaW5nVHlwZaRKU090q0dlbmVyYXRITm93wrFSZXBvcnRpbmdJbnRlcnZhbAqxQWN0aXZhdGlvblRpbWVPdXTM8K1UaW1IUmVmZXJlbmNitDAwMDETMDEtMDFUMDA6MDA6MDBaqVBhcmFtZXRlct0AAAAF3wAAAAWkdHlwZaVldmVudKildmVudE5hbWW0U1ITX1NIX2xpZ2h0dHBkQ3Jhc2ipY29tcG9uZW50s3RIc3QtYW5kLWRpYWdub3N0aWOjdXNpWNvdW50q3JICG9ydEVtcHR5wt8AAAAFpHR5cGWKZ3JickZtYXJrZXK2U1ITXOIOR9TTE9HU19VUEPOURFRKZZZWFYY2jZLEXPR1MgVVBMTOFERUQgU1VDQOVTUOZVTEXZLCBSRVRVUk4gQ09ERTogMjAwp2xvZ0ZpbGWwQ29uc29sZWxvZy50eHQuMKN1c2WIY291bnTfAAAAA6R0eXBigWRhdGFNb2RlbKRuYW1l001BQ6lyZWZlcmVuY2XZJKRldmljZS5EZXZpY2VJbmZvLIhfQ09NQOFTVC1DT01fQ0ffTUFD3WAAAAKkdHlwZálkYXRhTW9kZWypcmVmZXJlbmNlrFByb2ZpbGUuTmFtZd8AAAACpHR5cGWPZGF0YU1vZGVsqXJlZmVyZW5jZbhEZXZpY2UuRGV2aWNISW5mby5VcFRpbWWrUKJVU19NRVRITOTFAAAAAQZNZXRob2Sgg|BhcmFtZXRlcnPdAAAAAt8AAAACpG5hbWWrY29udGVudFR5cGWldmFsdWWOUHJvZmlsZS5FbmNvZGluZ1R5cGXfAAAAAqRuYW1lpXRvcGljpXZhbHVIFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxS ZXBvcnRGb3JtYXStTmFtZVZhbHVIUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, UNSUPPORTED_PROTOCOL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6kTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQl9Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29srFJCVVNfSU5WQUxJRKxFbmNvZGluZ1R5cGWkSlNPTrFSZXBvcnRpbmdJbnRlcnZhbDyxQWN0aXZhdGlvblRpbWVvdXTNDhCrR2VuZXJhdGVOb3fCqFJvb3ROYW1lqkZSMl9VU19UQzKtVGltZVJlZmVyZW5jZbQyMDIyLTEyLTE5VDA5OjMzOjU2WqlQYXJhbWV0ZXLdAAAAA98AAAAEpHR5cGWpZGF0YU1vZGVspG5hbWWmVVBUSU1FqXJlZmVyZW5jZbhEZXZpY2UuRGV2aWNlSW5mby5VcFRpbWWjdXNlqGFic29sdXRl3wAAAASkdHlwZaVldmVudKlldmVudE5hbWWvVVNFRF9NRU0xX3NwbGl0qWNvbXBvbmVudKZzeXNpbnSjdXNlqGFic29sdXRl3wAAAAWkdHlwZaRncmVwpm1hcmtlctkiU1lTX0lORk9fQ3Jhc2hQb3J0YWxVcGxvYWRfc3VjY2Vzc6ZzZWFyY2ixU3VjY2VzcyB1cGxvYWRpbmenbG9nRmlsZaxjb3JlX2xvZy50eHSjdXNlpWNvdW50tFJlcG9ydGluZ0FkanVzdG1lbnRz3wAAAAOuUmVwb3J0T25VcGRhdGXCtkZpcnN0UmVwb3J0aW5nSW50ZXJ2YWwPsE1heFVwbG9hZExhdGVuY3nNw1CkSFRUUN8AAAAEo1VSTLtodHRwczovL3N0YnJ0bC5yNTMueGNhbC50di+rQ29tcHJlc3Npb26kTm9uZaZNZXRob2SkUE9TVLNSZXF1ZXN0VVJJUGFyYW1ldGVy3QAAAAHfAAAAAqROYW1lqnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2ZpbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVlUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, REPORTTIMEFORMAT_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char *data = "3wAAAA6kTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQl9Qcm9maWxlp1ZlcnNpb26jMC4xqFByb3RvY29spEhUVFCsRW5jb2RpbmdUeXBlpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWw8sUFjdGl2YXRpb25UaW1lb3V0zQ4Qq0dlbmVyYXRlTm93wqhSb290TmFtZapGUjJfVVNfVEMyrVRpbWVSZWZlcmVuY2W0MjAyMi0xMi0xOVQwOTozMzo1NlqpUGFyYW1ldGVy3QAAAAPfAAAABKR0eXBlqWRhdGFNb2RlbKRuYW1lplVQVElNRalyZWZlcmVuY2W4RGV2aWNlLkRldmljZUluZm8uVXBUaW1lo3VzZahhYnNvbHV0Zd8AAAAEpHR5cGWlZXZlbnSpZXZlbnROYW1lr1VTRURfTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW50o3VzZahhYnNvbHV0Zd8AAAAFpHR5cGWkZ3JlcKZtYXJrZXLZIlNZU19JTkZPX0NyYXNoUG9ydGFsVXBsb2FkX3N1Y2Nlc3Omc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADrlJlcG9ydE9uVXBkYXRlwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zcNQpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRlct0AAAAB3wAAAAKkTmFtZapyZXBvcnROYW1lqVJlZmVyZW5jZaxQcm9maWxlLk5hbWWsSlNPTkVuY29kaW5n3wAAAAKsUmVwb3J0Rm9ybWF0oK9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, RI_GT_AT_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06xUmVwb3J0aW5nSW50ZXJ2YWZNF3CXQWN0aXZhdGlvblRpbWVvdXTNDhCrR2VuZXJhdGVOb3fCqFJvb3ROYW1lqkZSMI9VU19UQzKtVGITZVJIZmVyZW5jZbQyMDIyLTEyLTESVDA5OjMzOjU2WqlQYXJhbWVOZXLdAAAAA98AAAAEPHR5CGWPZGF0YU1vZGVspG5hbWWmVVBUSUIFqXJIZmVyZW5jZbhEZXZpY2UURGV2aWNISW5mby5VcFRpbWWjdXNlqGFic29sdXR3WAAAAŞKHlwZaVidmVudkidmVudE5hbWWVVVNFRF9NRUOXX3NwbGl0qWNvbXBvbmVudkZzexNponSjdXNIqGFic29sdXRI3wAAAAWkdHlwZaRncmVwpm1hcmtictkiU11TX0IORK91Q3Jhc2hQb3J0YWxVcGxvYWRfc3VjY2Vzc6ZzZWFyY2ixU3VjY2VzcyB1cGxvYWRpbmenbG9nRmlsZaxjb3JIX2xvZy50eHSjdXNpWNvdW50tFJlcG9ydGluZOFKanVzdG1lbnRz3wAAAAOuUmVwb3J0T25VCGRhdGXCtkZpcnNOUmVwb3J0aW5nSW50ZXJ2YWWPSE1heFVwbG9hZExhdGVuY3nNw1CKSFRUUN8AAAAE01VSTLtodHRwczovL3N0YnJobC5yNTMueGNhbC50di+rQ29tcHJlc3Npb26kTm9uZaZNZXRob2SKUE9TVLNSZXF1ZXNOVVJJUGFyYW1ldGVy3QAAAAHfAAAAAGROYW1lgnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2ZpbGUuTmFtZaxKU090RW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVIUGFpcq9SZXBvcnRUaW11c3RhbXCKTm9uZQ==";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, TC_TYPE)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WqtHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRílyX1VTX1RDNalQYXJhbWVOZXLdAAAAAt8AAAAEPHR5cGWIZXZIbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc31zaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBloklyZWZlcmVuY2XZOURIdmljZS5EZXZpY2VJbmZvLIhfUKRLQOVOVFJBTC1DT01fUKZDLKZIYXR1cmUuT1ZTLkVuYWJsZahvcGVyYXRvcgNhbnm0UmVwb3J0aW5nQWRqdXN0bWVudHPfAAAAA65SZXBvcnRPbIVwZGF0ZcK2Rmlyc3RSZXBvcnRpbmdJbnRlcnZhbA+WTWF4VXBsb2FkTGF0ZW5jec1OIKRIVFRQ3WAAAASJVVJMu2h0dHBzOi8vc3RicnRsLnl1My54Y2FsLnR2L6tDb21wcmVzc2lvbqRO625lpk1ldGhvZKRQT1NUS1JlcXVIc3RVUKIQYXJhbWVOZXLdAAAAAd8AAAACpE5hbWWqcmVwb3J0TmFtZalSZWZlcmVuY2WSUHJvZmlsZS50YW1lrEpTT05FbmNvZGluZ98AAAACrFJlcG9ydEZvcm1hdK10YW1lVmFsdWVQYWlyr1JlcG9ydFRpbWVzdGFtcKROb251";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, TYPE_NT_DATAMODEL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WqtHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRílyX1VTX1RDNalQYXJhbWVOZXLdAAAAAt8AAAAEPHR5cGWIZXZIbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc31zaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBIpUV2ZW50qXJIZmVyZW5jZdk5RGV2aWNILKRIdmljZUluZm8uWF9SRETDRU5UUKFMLUNPTV9SRkMuRmVhdHVyZS5PVIMURW5hYmxlqG9wZXJhdG9yo2FuebRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADrIJIcG9ydE9uVXBKYXRĺwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zU4gpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvg0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRICTOAAAAB3wAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxILk5hbWWSSINPTKVuY29kaW5n3wAAAAKsUmVwb3J0Rm9ybWF0rU5hbWVWYWx1ZVBhaXKvUmVwb3JOVGItZXN0YW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, OPERATOR_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WqtHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRílyX1VTX1RDNalQYXJhbWVOZXLdAAAAAt8AAAAEPHR5cGWIZXZIbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc31zaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBlqWRhdGFNb2RlbKlyZWZlcmVuY2XZOURIdmljZS5EZXZpY2VJbmZvLIhfUKRLQOVOVFJBTC1DT01fUKZDLKZIYXR1cmUuT1ZTLkVuYWJsZahvcGVyYXRvcqCOUmVwb3J0aW5nQWRqdXN0bWVudHPfAAAAA65SZXBvcnRPbIVwZGFOZCK2Rmlyc3RSZXBvcnRpbmdJbnRlcnZhbA+wTWF4VXBsb2FkTGF0ZW5jec1OIKRIVFRQ3wAAAASJVVJMu2h0dHBzOi8vc3RicnRsLnl1My54Y2FsLnR2L6tDb21wcmVzc2lvbqR0b25lpk1ldGhvZKRQT1NUS1JlcXVIC3RVUKIQYXJhbWVOZXLdAAAAAd8AAAACpE5hbWWgcmVwb3J0TmFtZalSZWZlcmVuY2WsUHJvZmlsZS5OYW1lrEpTT05FbmNvZGluZ98AAAACrFJlcG9ydEZvcm1hdK10YW11VmFsdWVQYWlyr1JlcG9ydFRpbWVzdGFtcKROb25!";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, OPERATOR_INVALID)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WatHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRilyX1VTX1RDNalQYXJhbWVOZXLdAAAAAt8AAAAEPHR5cGWIZXZIbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc31zaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBlqWRhdGFNb2RlbKlyZWZlcmVuY2XZOURIdmljZS5EZXZpY2VJbmZvLIhfUKRLQOVOVFJBTC1DT01fUKZDLKZIYXR1cmUuT1ZTLkVuYWJsZahvcGVyYXRvcqJsZLRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADriJlcG9ydE9uVXBKYXRlwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRIbmN5zU4gpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRICt0AAAAB3wAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxlLk5hbWWSSINPTkVuY29kaW5n3wAAAAKSUmVwb3JORm9ybWFOrUshbWVWYWx1ZVBhaXKvUmVwb3J0VGItZXNOYW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, THRESHOLD)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WqtHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRílyX1VTX1RDNalQYXJhbWV0ZXLdAAAAAt8AAAAEPHR5CGWIZXZlbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc31zaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBlqWRhdGFNb2RlbKlyZWZlcmVuY2WgqG9wZXJhdG9yo2FuebRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADriJicG9ydE9uVXBkYXRlwrZGaXJzdFJicG9ydGluZOludGVydmFsD7BNYXhVcGxvYWRMYXRlbmN5zU4gpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvq0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRlctÓAAAAB3wAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxILk5hbWWsSINPTKVUY29kaW5n3wAAAAKsUmVwb3JORm9ybWF0rÜ5hbWVWYWx1ZVBhaXKvUmVwb3J0VGItZXNOYW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, REF_NULL)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WatHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRilyX1VTX1RDNalQYXJhbWVOZXLdAAAAAt8AAAAEPHR5cGWIZXZIbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc3lzaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBlqWRhdGFNb2RlIbKlyZWZlcmVuY2XZNURIdmljZS5EZXZpY2VJbmZvLIhfUKRLQOVOVFJBTC1DT01fUKZDLKZIYXR1cmUURW5hYmxlqG9wZXJhdG9yo2FuebRSZXBvcnRpbmdBZGp1c3RtZW50c98AAAADriJlcG9ydE9uVXBKYXRĺwrZGaXJzdFJlcG9ydGluZ0ludGVydmFsD7BNYXhVcGxvYWRMYXRIbmN5zU4gpEhUVFDfAAAABKNVUky7aHR0cHM6Ly9zdGJydGwucjUzLnhjYWwudHYvg0NvbXByZXNzaW9upE5vbmWmTWV0aG9kpFBPU1SzUmVxdWVzdFVSSVBhcmFtZXRICTOAAAAB3wAAAAKKTmFtZapyZXBvcnROYW1lqVJIZmVyZW5jZaxQcm9maWxILk5hbWWSSINPTKVuY29kaW5n3wAAAAKsUmVwb3J0Rm9ybWF0rU5hbWVWYWx1ZVBhaXKvUmVwb3JOVGItZXN0YW1wpE5vbmU=";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}

TEST(PROCESSCONFIGURATION_MSGPACK, REF_INVALID)
{
    Profile *profile = 0;
    struct __msgpack__ msgpack;
    msgpack.msgpack_blob_size = 1200;
    msgpack.msgpack_blob = (char *)malloc(sizeof(char) * msgpack.msgpack_blob_size);
    struct __msgpack__ *messgpack = (struct __msgpack__ *)malloc(sizeof(struct __msgpack__));
    char* data = "3wAAAA6KTmFtZaxSREtCX1Byb2ZpbGWrRGVzY3JpcHRpb26sUkRLQ19Qcm9maWxlp1ZlcnNpb26jMC4xqFByб3RvY29spEhUVFCSRW5jb2RpbmdUeXBIpEpTT06XQWN0aXZhdGlvblRpbWVvdXTNDhCtVGltZVJIZmVyZW5jZbQyMDIyLTEyLTIWVDA5OjMzOjU2WatHZW5lcmF0ZU5vd8KoUm9vdE5hbWWqRilyX1VTX1RDNalQYXJhbWVOZXLdAAAAAt8AAAAEPHR5cGWIZXZIbnSpZXZlbnROYW1lMVTRURFTUVNMV9zcGxpdKljb21wb25lbnSmc31zaW5003VzZahhYnNvbHV0Zd8AAAAFPHR5CGWkZ3JickZtYXJrZXLZIINZU19JTKZPXONYYXNo@G9ydGFsVXBsb2FkX3N1Y2Nlc30mc2VhcmNosVN1Y2Nlc3MgdXBsb2FkaW5np2xvZ0ZpbGWsY29yZV9sb2cudHh0o3VzZaVjb3VudLBUeminZ2VyQ29uZGI0aW9u3QAAAAHAAAAA6R0eXBlqWRhdGFNb2RlbKlyZWZlcmVuY2XZNURIdmljZS5EZXZpY2VJbmZvLIhfUKRLQOVOVFJBTC1DT01fUkZDLkZIYXR1cmUuRW5hYmxlqG9wZXJhdG9yomd0tFJlcG9ydGluZOFkanVzdG1lbnRz3wAAAAOuUmVwb3J0T25VCGRhdGXCtkZpcnN0UmVwb3J0aW5nSW50ZXJ2YWWPSE1heFVwbG9hZExhdGVuY3nNTICKSFRUUN8AAAAE01VSTLtodHRwczovL3N0YnJobC5yNTMueGNhbC50di+rQ29tcHJlc3Npb26kTm9uZaZNZXRob2SKUE9TVLNSZXF1ZXNOVVJJUGFyYW1lbGUuTmFtZaxKU09ORW5jb2RpbmffAAAAAqxSZXBvcnRGb3JtYXStTmFtZVZhbHVIUGFpcq9SZXBvcnRUaW1lc3RhbXCkTm9uZQ==dGVy3QAAAAHfAAAAAqROYW1lqnJlcG9ydE5hbWWpUmVmZXJlbmNlrFByb2Zp";
    strncpy(msgpack.msgpack_blob, data, msgpack.msgpack_blob_size);
    messgpack->msgpack_blob = msgpack.msgpack_blob;
    msgpack_unpacked result;
    size_t off = 0;
    msgpack_object *profiles_root;
    msgpack_unpacked_init(&result);
    msgpack_unpack_next(&result,  messgpack->msgpack_blob,  strlen(messgpack->msgpack_blob), &off);
    profiles_root = &result.data;

    EXPECT_EQ(T2ERROR_FAILURE,  processMsgPackConfiguration(profiles_root, &profile));
    free(messgpack->msgpack_blob);
    free(messgpack);
}
