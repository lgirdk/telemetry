#include <iostream>
#include "reportgenMock.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
using namespace std;

extern ReportgenMock *m_reportgenMock;

extern "C" cJSON* cJSON_CreateArray()
{
    if (!m_reportgenMock)
    {
         return NULL;
    }
    return m_reportgenMock->cJSON_CreateArray();
}

extern "C" cJSON* cJSON_CreateObject()
{
    if (!m_reportgenMock)
    {
         return NULL;
    }
    return m_reportgenMock->cJSON_CreateObject();
}

/*extern "C" cJSON_bool cJSON_AddItemToArray(cJSON *array, cJSON *item)
{
    if (!m_reportgenMock)
    {
         return false;
    }
    return m_reportgenMock->cJSON_AddItemToArray(array, item);
}

extern "C" cJSON_bool cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item)
{
    if (!m_reportgenMock)
    {
         return false;
    }
    return m_reportgenMock->cJSON_AddItemToObject(object, string, item);
}*/

extern "C" char* cJSON_PrintUnformatted(const cJSON *item)
{
    if (!m_reportgenMock)
    {
         return NULL;
    }
    return m_reportgenMock->cJSON_PrintUnformatted(item);
}

extern "C" cJSON*  cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string)
{
    if (!m_reportgenMock)
    {
         return NULL;
    }
    return m_reportgenMock->cJSON_AddStringToObject(object, name, string);
}

extern "C" CURL* curl_easy_init()
{
        if(!m_reportgenMock)
        {
                return NULL;
        }
        return m_reportgenMock->curl_easy_init();
}

/*extern "C" char *curl_easy_escape( CURL *curl, const char *url, int length )
{
        if(!m_reportgenMock)
        {
                return NULL;
        }
        return m_reportgenMock->curl_easy_escape(curl, url, length);
}
*/
extern "C" void cJSON_Delete(cJSON * root)
{
        if(!m_reportgenMock)
        {
                return;
        }
        m_reportgenMock->cJSON_Delete(root);
}

extern "C" int regcomp(regex_t *preg, const char *pattern, int cflags)
{
        if(!m_reportgenMock)
        {
               return -1;
        }
        m_reportgenMock->regcomp(preg, pattern, cflags);
}

extern "C" int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags)
{
        if(!m_reportgenMock)
        {
               return -1;
        }
        m_reportgenMock->regexec(preg, string, nmatch, pmatch, eflags);
}

extern "C"  void regfree(regex_t *preg)
{
        if(!m_reportgenMock)
        {
               return;
        }
         m_reportgenMock->regfree(preg);
}
