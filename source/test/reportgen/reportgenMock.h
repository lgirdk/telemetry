#ifndef REPORTGEN_MOCK
#define REPORTGEN_MOCK
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <regex.h>

class ReportgenInterface
{
    public:
        virtual ~ReportgenInterface() {}
        virtual cJSON* cJSON_CreateArray() = 0;
	virtual cJSON* cJSON_CreateObject() = 0;
//	virtual cJSON_bool cJSON_AddItemToArray(cJSON *array, cJSON *item) = 0;
        virtual cJSON*  cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string) = 0;
  //      virtual cJSON_bool cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) = 0;
        virtual char* cJSON_PrintUnformatted(const cJSON *item) = 0;
	virtual void cJSON_Delete(cJSON *root) = 0;
	virtual CURL* curl_easy_init() = 0;
        virtual int regcomp(regex_t *preg, const char *pattern,int cflags) = 0;
        virtual int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags) = 0;
        virtual void regfree(regex_t *preg) = 0;
       // virtual char* curl_easy_escape(CURL *c, const char *string, int len) = 0;
};

class ReportgenMock: public ReportgenInterface
{
    public:
        virtual ~ReportgenMock() {}
        MOCK_METHOD0(cJSON_CreateArray,  cJSON*());
        MOCK_METHOD0(cJSON_CreateObject, cJSON*());
        //MOCK_METHOD2(cJSON_AddItemToArray, cJSON_bool(cJSON *, cJSON *));
        MOCK_METHOD3(cJSON_AddStringToObject, cJSON*(cJSON * const, const char * const, const char * const));
        //MOCK_METHOD3(cJSON_AddItemToObject, cJSON_bool(cJSON *, const char *, cJSON *));
        MOCK_METHOD1(cJSON_PrintUnformatted, char*(const cJSON *));
	MOCK_METHOD1(cJSON_Delete, void(cJSON *));
	MOCK_METHOD0(curl_easy_init, CURL*());
	MOCK_METHOD3(regcomp, int(regex_t*,  const char *, int));
	MOCK_METHOD5(regexec, int(const regex_t *,  const char *, size_t, regmatch_t*, int));
        MOCK_METHOD1(regfree, void(regex_t*));
	//MOCK_METHOD3(curl_easy_escape, char*(CURL *, const char *, int));
};

#endif


