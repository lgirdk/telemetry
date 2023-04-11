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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>

#include "reportgen.h"
#include "t2log_wrapper.h"
#include "telemetry2_0.h"
#include "t2common.h"
#include "dcautil.h"
#include "busInterface.h"

static bool checkForEmptyString( char* valueString ){
    bool isEmpty = false ;
    T2Debug("%s ++in \n", __FUNCTION__);

    if(valueString){
        // rbusInterface implementation explicitly adds string as "NULL" for parameters which doesn't exist on device
        // Consider "NULL" string value as qualified for empty string
        if(strlen(valueString) < 1 || !strncmp(valueString, " ", 1) || !strncmp(valueString, "NULL", 4)){
            isEmpty = true ;
        }
    }else{
        isEmpty = true;
    }
    T2Debug("%s --Out with return status as %s \n", __FUNCTION__, (isEmpty ? "true" : "false"));
    return isEmpty;
}

void freeProfileValues(void *data)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    if(data != NULL)
    {
        profileValues *profVal = (profileValues *) data;
        freeParamValueSt(profVal->paramValues, profVal->paramValueCount);
        free(profVal);
    }
    T2Debug("%s --Out\n", __FUNCTION__);
}

// convert vector to json array it is responsbility of
// caller function to free the vector 
void convertVectorToJson(cJSON *output, Vector *input) {
    if(!output)
        output = cJSON_CreateArray();
    int i,vectorSize = (int) Vector_Size(input);
    for(i = 0; i < vectorSize; i++){
        char* eventValue = Vector_At(input, i);
        cJSON_AddItemToArray(output, cJSON_CreateString(eventValue));                   
    } 
}

T2ERROR destroyJSONReport(cJSON *jsonObj)
{
    cJSON_Delete(jsonObj);
    return T2ERROR_SUCCESS;
}

T2ERROR encodeParamResultInJSON(cJSON *valArray, Vector *paramNameList, Vector *paramValueList)
{
    int index = 0;
    T2Debug("%s ++in \n", __FUNCTION__);

    for(; index < Vector_Size(paramNameList); index++)
    {
        Param* param = (Param *)Vector_At(paramNameList, index);
        tr181ValStruct_t **paramValues = ((profileValues *)Vector_At(paramValueList, index))->paramValues;
        if(param == NULL || paramValues == NULL ) {
            // Ignore tr181 params returning null values in report
            continue ;
        }
        int paramValCount = ((profileValues *)Vector_At(paramValueList, index))->paramValueCount;
        T2Debug("Parameter Name : %s valueCount = %d\n", param->name, paramValCount);
        if(paramValCount == 0)
        {
            if(param->reportEmptyParam){
                cJSON *arrayItem = cJSON_CreateObject();
                T2Info("Paramter was not successfully retrieved... \n");
                cJSON_AddStringToObject(arrayItem, param->name, "NULL");
                cJSON_AddItemToArray(valArray, arrayItem);
            }else{
                continue ;
            }
        }
        else if(paramValCount == 1) // Single value
        {
            if(paramValues[0]) {
                if(param->reportEmptyParam || !checkForEmptyString(paramValues[0]->parameterValue)) {
                    cJSON *arrayItem = cJSON_CreateObject();
                    cJSON_AddStringToObject(arrayItem, param->name, paramValues[0]->parameterValue);
                    cJSON_AddItemToArray(valArray, arrayItem);
                }
            }
        }
        else
        {
            cJSON *valList = NULL;
            cJSON *valItem = NULL;
            int valIndex = 0;
            bool isTableEmpty = true ;
            cJSON *arrayItem = cJSON_CreateObject();
            cJSON_AddItemToObject(arrayItem, param->name, valList = cJSON_CreateArray());
            for (; valIndex < paramValCount; valIndex++)
            {
                if(paramValues[valIndex]){
                    if(param->reportEmptyParam || !checkForEmptyString(paramValues[0]->parameterValue)) {
                        valItem = cJSON_CreateObject();
                        cJSON_AddStringToObject(valItem, paramValues[valIndex]->parameterName, paramValues[valIndex]->parameterValue);
                        cJSON_AddItemToArray(valList, valItem);
                        isTableEmpty = false ;
                    }
                }
            }

            if(!isTableEmpty) {
                cJSON_AddItemToArray(valArray, arrayItem);
            } else {
                cJSON_free(arrayItem);
            }
        }
    }
    T2Debug("%s --Out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR encodeStaticParamsInJSON(cJSON *valArray, Vector *staticParamList)
{
    T2Debug("%s ++in \n", __FUNCTION__);

    int index = 0;
    cJSON *arrayItem = NULL;
    for(; index < Vector_Size(staticParamList); index++)
    {
        StaticParam *sparam = (StaticParam *)Vector_At(staticParamList, index);
        if(sparam) {
            if(sparam->name == NULL || sparam->value == NULL )
                continue ;
            arrayItem = cJSON_CreateObject();
            cJSON_AddStringToObject(arrayItem, sparam->name, sparam->value);
            cJSON_AddItemToArray(valArray, arrayItem);
        }
    }

    T2Debug("%s --Out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR encodeGrepResultInJSON(cJSON *valArray, Vector *grepResult)
{
    T2Debug("%s ++in \n", __FUNCTION__);
    int index = 0;
    cJSON *arrayItem = NULL;
    for(; index < Vector_Size(grepResult); index++)
    {
        GrepResult* grep = (GrepResult *)Vector_At(grepResult, index);
        if(grep) {
            if(grep->markerName == NULL || grep->markerValue == NULL ) // Ignore null values
                continue ;
            arrayItem = cJSON_CreateObject();
            cJSON_AddStringToObject(arrayItem, grep->markerName, grep->markerValue);
            cJSON_AddItemToArray(valArray, arrayItem);
        }
    }
    T2Debug("%s --Out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR encodeEventMarkersInJSON(cJSON *valArray, Vector *eventMarkerList)
{
    T2Debug("%s ++in \n", __FUNCTION__);
    int index = 0;
    cJSON *arrayItem = NULL;
    for(; index < Vector_Size(eventMarkerList); index++)
    {
        EventMarker* eventMarker = (EventMarker *)Vector_At(eventMarkerList, index);
        switch(eventMarker->mType)
        {
            case MTYPE_COUNTER:
                if(eventMarker->u.count > 0)
                {
                    char stringValue[10] = {'\0'};
                    sprintf(stringValue, "%d", eventMarker->u.count);
                    arrayItem = cJSON_CreateObject();
                    if (eventMarker->alias) {
                        cJSON_AddStringToObject(arrayItem, eventMarker->alias, stringValue);
                    } else {
                        cJSON_AddStringToObject(arrayItem, eventMarker->markerName, stringValue);
                    }
                    if(eventMarker->reportTimestampParam == REPORTTIMESTAMP_UNIXEPOCH){
                        cJSON_AddStringToObject(arrayItem, eventMarker->markerName_CT, eventMarker->timestamp);
                    }
                    cJSON_AddItemToArray(valArray, arrayItem);

                    T2Debug("Marker value for : %s is %d\n", eventMarker->markerName, eventMarker->u.count);
                    eventMarker->u.count = 0;
                }
                break;

            case MTYPE_ACCUMULATE:
                if( eventMarker->u.accumulatedValues != NULL && Vector_Size(eventMarker->u.accumulatedValues))
                {
                    arrayItem = cJSON_CreateObject();
                    cJSON *vectorToarray = cJSON_CreateArray();
                    convertVectorToJson(vectorToarray, eventMarker->u.accumulatedValues);
                    Vector_Clear(eventMarker->u.accumulatedValues, freeAccumulatedParam);
                    T2Debug("eventMarker->reportTimestampParam type is %d \n", eventMarker->reportTimestampParam);

                    if(eventMarker->alias) {
                        cJSON_AddItemToObject(arrayItem, eventMarker->alias, vectorToarray);
                    }else {
                        cJSON_AddItemToObject(arrayItem, eventMarker->markerName, vectorToarray);
                    }
                    if(eventMarker->reportTimestampParam == REPORTTIMESTAMP_UNIXEPOCH) {
                        cJSON *TimevectorToarray = cJSON_CreateArray();
                        convertVectorToJson(TimevectorToarray, eventMarker->accumulatedTimestamp);
                        T2Debug("convertVectorToJson is successful\n");
                        Vector_Clear(eventMarker->accumulatedTimestamp, freeAccumulatedParam);
                        cJSON_AddItemToObject(arrayItem, eventMarker->markerName_CT, TimevectorToarray);
                    }                  
                    cJSON_AddItemToArray(valArray, arrayItem);
                    T2Debug("Marker value Array for : %s is %s\n", eventMarker->markerName, cJSON_Print(vectorToarray));
                }
                break;

            case MTYPE_ABSOLUTE:
            default:
                if(eventMarker->u.markerValue != NULL)
                {
                    arrayItem = cJSON_CreateObject();
                    if (eventMarker->alias) {
                        cJSON_AddStringToObject(arrayItem, eventMarker->alias, eventMarker->u.markerValue);
                    } else {
                        cJSON_AddStringToObject(arrayItem, eventMarker->markerName, eventMarker->u.markerValue);
                    }
                    if(eventMarker->reportTimestampParam == REPORTTIMESTAMP_UNIXEPOCH){
                        cJSON_AddStringToObject(arrayItem, eventMarker->markerName_CT, eventMarker->timestamp);
                    }
                    cJSON_AddItemToArray(valArray, arrayItem);
                    T2Debug("Marker value for : %s is %s\n", eventMarker->markerName, eventMarker->u.markerValue);
                    free(eventMarker->u.markerValue);
                    eventMarker->u.markerValue = NULL;
                }
        }
    }
    T2Debug("%s --Out \n", __FUNCTION__);
    return T2ERROR_SUCCESS;

}

T2ERROR prepareJSONReport(cJSON* jsonObj, char** reportBuff)
{
    T2Debug("%s ++in\n", __FUNCTION__);
    *reportBuff = cJSON_PrintUnformatted(jsonObj);
    if(*reportBuff == NULL)
    {
        T2Error("Failed to get unformatted json\n");
        return T2ERROR_FAILURE;
    }
    T2Debug("%s --Out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

char *prepareHttpUrl(T2HTTP *http)
{
    CURL *curl = curl_easy_init();
    char *httpUrl = strdup(http->URL);
    T2Debug("%s: Default URL: %s \n", __FUNCTION__, httpUrl);

    if (http->RequestURIparamList && http->RequestURIparamList->count)
    {
        unsigned int index = 0;
        int params_len = 0;
        char *url_params = NULL;

        for(; index < http->RequestURIparamList->count; index++)
        {
            HTTPReqParam * httpParam = (HTTPReqParam *)(Vector_At)(http->RequestURIparamList, index);
            char *httpParamVal = NULL;
            char *paramValue = NULL;

            if (httpParam->HttpValue)		//Static parameter
            {
                httpParamVal = curl_easy_escape(curl, httpParam->HttpValue, 0);
            }
            else				//Dynamic parameter
            {
                if(T2ERROR_SUCCESS != getParameterValue(httpParam->HttpRef, &paramValue))
                {
                    T2Error("Failed to retrieve param : %s\n", httpParam->HttpRef);
                    continue;
                }
                else
                {
                    if (paramValue[0] == '\0') {
                        free(paramValue);
                        T2Error("Param value is empty for : %s\n", httpParam->HttpRef);
                        continue;
                    }

                    httpParamVal = curl_easy_escape(curl, paramValue, 0);
                    free(paramValue);
                    paramValue = NULL;
                }
            }

            int new_params_len = params_len /* current params length */
                           + strlen(httpParam->HttpName) /* Length of parameter 'Name' */ + strlen("=")
                           + strlen("&") /* Add '&' for next paramter */ + 1;
            if(httpParamVal)
            {
                new_params_len += strlen(httpParamVal);
            }
            url_params = realloc(url_params, new_params_len);
            if(url_params == NULL){
                T2Error("Unable to allocate %d bytes of memory at Line %d on %s \n",new_params_len, __LINE__, __FILE__);
                curl_free(httpParamVal);
                continue;
            }
            params_len += snprintf(url_params+params_len, new_params_len-params_len, "%s=%s&", httpParam->HttpName, httpParamVal);

            curl_free(httpParamVal);
        }

        if (params_len > 0 && url_params != NULL)
        {
            url_params[params_len-1] = '\0';

            int url_len = strlen(httpUrl);
            int modified_url_len = url_len + strlen("?") + strlen(url_params) + 1;

            httpUrl = realloc(httpUrl, modified_url_len);
            snprintf(httpUrl+url_len, modified_url_len-url_len, "?%s", url_params);

        }
       if(url_params){
           free(url_params);
       }
    }

    curl_easy_cleanup(curl);
    T2Debug("%s: Modified URL: %s \n", __FUNCTION__, httpUrl);

    return httpUrl;
}
