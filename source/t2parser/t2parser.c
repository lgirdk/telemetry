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

#include "t2parser.h"

#include <stdio.h>
#include <stdlib.h>

#include "xconfclient.h"
#include "reportprofiles.h"
#include "t2log_wrapper.h"
#include "msgpack.h"

static const int MAX_STATIC_PROP_VAL_LEN = 128 ;


static char * getProfileParameter(Profile * profile, const char *ref) {
    char *pValue = "NULL";
    char *pName = strrchr(ref, '.') + 1;

    pValue = (char*) calloc(MAX_STATIC_PROP_VAL_LEN, sizeof(char));
    if( pValue == NULL) {
        T2Error("Unable to allocate memory for profile parameter value \n");
        return pValue ;
    }
    
    if(!strcmp(pName, "Name")) {
        if(profile->name)
            strncpy(pValue, profile->name, MAX_STATIC_PROP_VAL_LEN);
    }else if(!strcmp(pName, "Version")) {
        if(profile->version)
            strncpy(pValue, profile->version, MAX_STATIC_PROP_VAL_LEN);
    }else if(!strcmp(pName, "Description")) {
        if(profile->Description)
            strncpy(pValue, profile->Description, MAX_STATIC_PROP_VAL_LEN);
    }else if(!strcmp(pName, "Protocol")) {
        if(profile->protocol)
            strncpy(pValue, profile->protocol, MAX_STATIC_PROP_VAL_LEN);
    }else if(!strcmp(pName, "EncodingType")) {
        if(profile->encodingType)
            strncpy(pValue, profile->encodingType, MAX_STATIC_PROP_VAL_LEN);
    }else if(!strcmp(pName, "ReportingInterval")) {
        if(profile->reportingInterval)
            snprintf(pValue, MAX_STATIC_PROP_VAL_LEN, "%d", profile->reportingInterval);
    }else if(!strcmp(pName, "TimeReference")) {
        if(profile->timeRef)
            snprintf(pValue, MAX_STATIC_PROP_VAL_LEN, "%d", profile->timeRef);
    }else if(!strcmp(pName, "ActivationTimeOut")) {
        if(profile->activationTimeoutPeriod)
            snprintf(pValue, MAX_STATIC_PROP_VAL_LEN, "%d", profile->activationTimeoutPeriod);
    }else {
        snprintf(pValue, MAX_STATIC_PROP_VAL_LEN, "No Such Parameter");
    }

    return pValue;
}

static T2ERROR addhttpURIreqParameter(Profile *profile, const char* Hname, const char* Href) {
    T2Debug("%s ++in\n", __FUNCTION__);
    HTTPReqParam *httpreqparam = (HTTPReqParam *) malloc(sizeof(HTTPReqParam));
    if(!httpreqparam) {
        T2Error("failed to allocate memory \n");
        return T2ERROR_FAILURE;

    }
    httpreqparam->HttpRef = strdup(Href);
    httpreqparam->HttpName = strdup(Hname);
    if (strstr(Href, "Profile.") == Href)
        httpreqparam->HttpValue = getProfileParameter(profile, Href);
    else
        httpreqparam->HttpValue = NULL;

    Vector_PushBack(profile->t2HTTPDest->RequestURIparamList, httpreqparam);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

static T2ERROR addParameter(Profile *profile, const char* name, const char* ref, const char* fileName, int skipFreq, const char* ptype,
        const char* use, bool ReportEmpty) {

    T2Debug("%s ++in\n", __FUNCTION__);

    if(!(strcmp(ptype, "dataModel"))) {
        // T2Debug("Adding TR-181 Parameter : %s\n", ref);

        if (strstr(ref, "Profile.") == ref) {        //Static parameters from profile configuration
            StaticParam *sparam = (StaticParam *) malloc(sizeof(StaticParam));
            if(sparam == NULL){
                T2Error("Unable to allocate memory for Static \n");
                return T2ERROR_FAILURE;
            }
            sparam->paramType = strdup(ptype);
            sparam->name = strdup(name);
            sparam->value = getProfileParameter(profile, ref);

            Vector_PushBack(profile->staticParamList, sparam);
        } else {
            Param *param = (Param *) malloc(sizeof(Param));
            if(param == NULL){
                T2Error("Unable to allocate memory for TR-181 Parameter \n");
                return T2ERROR_FAILURE;
            }
            param->name = strdup(name);
            param->alias = strdup(ref);
            param->paramType = strdup(ptype);
            param->reportEmptyParam = ReportEmpty;

            Vector_PushBack(profile->paramList, param);
        }
    }else if(!(strcmp(ptype, "event"))) {
        // T2Debug("Adding Event Marker :: Param/Marker Name : %s ref/pattern/Comp : %s skipFreq : %d\n", name, ref, skipFreq);
        EventMarker *eMarker = (EventMarker *) malloc(sizeof(EventMarker));
        if(eMarker == NULL){
            T2Error("Unable to allocate memory for EventMarker \n");
            return T2ERROR_FAILURE;
        }
        eMarker->markerName = strdup(name);
        eMarker->compName = strdup(ref);
        if(fileName)
            eMarker->alias = strdup(fileName);
        else
            eMarker->alias = NULL;
        eMarker->paramType = strdup(ptype);
        eMarker->reportEmptyParam = ReportEmpty;
        if((use == NULL) || (0 == strcmp(use, "absolute"))) {
            eMarker->mType = MTYPE_ABSOLUTE;
            eMarker->u.markerValue = NULL;
        }else if (0 == strcmp(use, "count")){
            eMarker->mType = MTYPE_COUNTER;
            eMarker->u.count = 0;
        } else {
            T2Info("Unsupported marker type. Defaulting to absolute \n");
            eMarker->mType = MTYPE_ABSOLUTE;
            eMarker->u.markerValue = NULL;
        }
        eMarker->skipFreq = skipFreq;

        Vector_PushBack(profile->eMarkerList, eMarker);
    }else { //Grep Marker

        // T2Debug("Adding Grep Marker :: Param/Marker Name : %s ref/pattern/Comp : %s fileName : %s skipFreq : %d\n", name, ref, fileName, skipFreq);

        GrepMarker *gMarker = (GrepMarker *) malloc(sizeof(GrepMarker));
        if(gMarker == NULL){
            T2Error("Unable to allocate memory for GrepMarker \n");
            return T2ERROR_FAILURE;
        }
        gMarker->markerName = strdup(name);
        gMarker->searchString = strdup(ref);
        if(fileName)
             gMarker->logFile = strdup(fileName);
        else
             gMarker->logFile = NULL;
        gMarker->paramType = strdup(ptype);
        gMarker->reportEmptyParam = ReportEmpty;

        if((use == NULL) || (0 == strcmp(use, "absolute"))) {
            gMarker->mType = MTYPE_ABSOLUTE;
            gMarker->u.markerValue = NULL;
        }else if (0 == strcmp(use, "count")){
            gMarker->mType = MTYPE_COUNTER;
            gMarker->u.count = 0;
        } else {
            T2Info("Unsupported marker type. Defaulting to absolute \n");
            gMarker->mType = MTYPE_ABSOLUTE;
            gMarker->u.markerValue = NULL;
        }
        gMarker->skipFreq = skipFreq;
        Vector_PushBack(profile->gMarkerList, gMarker);
    }

    profile->paramNumOfEntries++;

    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR processConfiguration(char** configData, char *profileName, char* profileHash, Profile **localProfile) {
    T2Debug("%s ++in\n", __FUNCTION__);
    //REPORTPROFILE CJson PARSING
    T2ERROR ret = T2ERROR_SUCCESS;
    int ThisProfileParameter_count = 0;
    cJSON *json_root = cJSON_Parse(*configData);

    cJSON *jprofileHash = cJSON_GetObjectItem(json_root, "Hash");
    if(jprofileHash == NULL) {
        jprofileHash = cJSON_GetObjectItem(json_root, "VersionHash");
    }
    cJSON *jprofileDescription = cJSON_GetObjectItem(json_root, "Description");
    cJSON *jprofileVersion = cJSON_GetObjectItem(json_root, "Version");
    cJSON *jprofileProtocol = cJSON_GetObjectItem(json_root, "Protocol");
    cJSON *jprofileEncodingType = cJSON_GetObjectItem(json_root, "EncodingType");
    cJSON *jprofileReportingInterval = cJSON_GetObjectItem(json_root, "ReportingInterval");
    cJSON *jprofileActivationTimeout = cJSON_GetObjectItem(json_root, "ActivationTimeOut");
    cJSON *jprofileTimeReference = cJSON_GetObjectItem(json_root, "TimeReference");
    cJSON *jprofileParameter = cJSON_GetObjectItem(json_root, "Parameter");
    cJSON *jprofileGenerateNow = cJSON_GetObjectItem(json_root, "GenerateNow");
    if(jprofileParameter) {
        ThisProfileParameter_count = cJSON_GetArraySize(jprofileParameter);
    }
    if(profileName == NULL || jprofileProtocol == NULL || jprofileEncodingType == NULL || jprofileParameter == NULL
            || ThisProfileParameter_count == 0 || (profileHash == NULL  && jprofileHash == NULL)) {
        T2Error("Incomplete profile information, unable to create profile\n");
	cJSON_Delete(json_root);
        return T2ERROR_FAILURE;
    }

    if(!jprofileHash)
    {
        cJSON_AddStringToObject(json_root, "Hash", profileHash);// updating the versionHash to persist the hash along with profile configuration
        free(*configData);
        *configData = NULL;
        *configData = cJSON_PrintUnformatted(json_root);
    }

    cJSON *jprofileHTTP = NULL;
    cJSON *jprofileHTTPURL = NULL;
    cJSON *jprofileHTTPCompression = NULL;
    cJSON *jprofileHTTPMethod = NULL;
    cJSON *jprofileHTTPRequestURIParameter = NULL;
    int ThisprofileHTTPRequestURIParameter_count = 0;

    if((jprofileProtocol) && (strcmp(jprofileProtocol->valuestring, "HTTP") == 0)) {
        jprofileHTTP = cJSON_GetObjectItem(json_root, "HTTP");
        jprofileHTTPURL = cJSON_GetObjectItem(jprofileHTTP, "URL");
        jprofileHTTPCompression = cJSON_GetObjectItem(jprofileHTTP, "Compression");
        jprofileHTTPMethod = cJSON_GetObjectItem(jprofileHTTP, "Method");
        if(jprofileHTTPURL == NULL || jprofileHTTPCompression == NULL || jprofileHTTPMethod == NULL) {
            T2Error("Incomplete profile information, unable to create profile\n");
	        cJSON_Delete(json_root);
            return T2ERROR_FAILURE;
        }
        jprofileHTTPRequestURIParameter = cJSON_GetObjectItem(jprofileHTTP, "RequestURIParameter");
        if(jprofileHTTPRequestURIParameter) {
            ThisprofileHTTPRequestURIParameter_count = cJSON_GetArraySize(jprofileHTTPRequestURIParameter);
        }
    }

    cJSON *jprofileJSONEncoding = NULL;
    cJSON *jprofileJSONReportFormat = NULL;
    cJSON *jprofileJSONReportTimestamp = NULL;

    if((jprofileEncodingType) && (strcmp(jprofileEncodingType->valuestring, "JSON") == 0)) {
        jprofileJSONEncoding = cJSON_GetObjectItem(json_root, "JSONEncoding");
        jprofileJSONReportFormat = cJSON_GetObjectItem(jprofileJSONEncoding, "ReportFormat");
        if(jprofileJSONReportFormat == NULL) {
            T2Error("Incomplete profile information, unable to create profile\n");
	    cJSON_Delete(json_root);
            return T2ERROR_FAILURE;

        }
        jprofileJSONReportTimestamp = cJSON_GetObjectItem(jprofileJSONEncoding, "ReportTimestamp");
    }

    if (jprofileReportingInterval && jprofileActivationTimeout &&
            jprofileActivationTimeout->valueint < jprofileReportingInterval->valueint) {

        T2Error("activationTimeoutPeriod is less than reporting interval. invalid profile: %s \n", profileName);
        cJSON_Delete(json_root);
        return T2ERROR_FAILURE;
    }

    //PROFILE CREATION
    Profile *profile = (Profile *) malloc(sizeof(Profile));
    if(profile == NULL) {
        T2Error("Malloc error\n can not allocate memory to create profile\n exiting \n");
        cJSON_Delete(json_root);
        return T2ERROR_FAILURE;
    }

    memset(profile, 0, sizeof(Profile));

    if((strcmp(jprofileEncodingType->valuestring, "JSON") == 0)) {
        profile->jsonEncoding = (JSONEncoding *) malloc(sizeof(JSONEncoding));
        if(profile->jsonEncoding == NULL) {
            T2Error("jsonEncoding malloc error\n");
            if(profile) {
                free(profile);
            }
	    cJSON_Delete(json_root);
            return T2ERROR_FAILURE;
        }
    }

    if((strcmp(jprofileProtocol->valuestring, "HTTP") == 0)) {
        profile->t2HTTPDest = (T2HTTP *) malloc(sizeof(T2HTTP));
        if(profile->t2HTTPDest == NULL) {
            T2Error("t2HTTPDest malloc error\n");
            if(profile->jsonEncoding) {
                free(profile->jsonEncoding);
            }
            if(profile) {
                free(profile);
            }
	    cJSON_Delete(json_root);
            return T2ERROR_FAILURE;
        }
    }

    profile->name = strdup(profileName);
    if(jprofileDescription) {
        profile->Description = strdup(jprofileDescription->valuestring);
    }
    if(jprofileVersion) {
        profile->version = strdup(jprofileVersion->valuestring);
    }
    if(!jprofileHash)
        profile->hash = strdup(profileHash);
    else
        profile->hash = strdup(jprofileHash->valuestring);

    profile->protocol = strdup(jprofileProtocol->valuestring);
    profile->encodingType = strdup(jprofileEncodingType->valuestring);
    profile->generateNow = false;
    profile->activationTimeoutPeriod = INFINITE_TIMEOUT;

    if (jprofileGenerateNow)
        profile->generateNow = (cJSON_IsTrue(jprofileGenerateNow) == 1);

    if (profile->generateNow) {
        profile->reportingInterval = 0;
    } else {
        if(jprofileReportingInterval) {
            int reportIntervalInSec = jprofileReportingInterval->valueint;
            profile->reportingInterval = reportIntervalInSec;
        }

        if (jprofileActivationTimeout) {
            profile->activationTimeoutPeriod = jprofileActivationTimeout->valueint;
            T2Debug("[[ profile->activationTimeout:%d]]\n", profile->activationTimeoutPeriod);
        }
    }

    if(jprofileTimeReference) {
        // profile->timeRef = strdup(jprofileTimeReference->valuestring);
        /*  MUST TODO: Seems timeref is an unsigned int in profile structure . Handle the scenario accordingly */

    }

    T2Debug("[[profile->name:%s]]\n", profile->name);
    T2Debug("[[ profile->Description:%s]]\n", profile->Description);
    T2Debug("[[profile->version:%s]]\n", profile->version);
    T2Debug("[[profile->protocol:%s]]\n", profile->protocol);
    T2Debug("[[profile->encodingType:%s]]\n", profile->encodingType);

    if(profile->reportingInterval)
        T2Debug("[[ profile->reportingInterval:%d]]\n", profile->reportingInterval);

    if((profile->jsonEncoding) && (strcmp(jprofileEncodingType->valuestring, "JSON") == 0)) {

        if(!(strcmp(jprofileJSONReportFormat->valuestring, "NameValuePair"))) {
            profile->jsonEncoding->reportFormat = JSONRF_KEYVALUEPAIR;
        }else if(!(strcmp(jprofileJSONReportFormat->valuestring, "ObjectHierarchy"))) {
            profile->jsonEncoding->reportFormat = JSONRF_OBJHIERARCHY;
        }else { /* defaulting it to NameValuePair */
            profile->jsonEncoding->reportFormat = JSONRF_KEYVALUEPAIR;
        }

        T2Debug("[[profile->jsonEncoding->reportFormat:%d]]\n", profile->jsonEncoding->reportFormat);
        if(jprofileJSONReportTimestamp) {
            if(!(strcmp(jprofileJSONReportTimestamp->valuestring, "None"))) {
                profile->jsonEncoding->tsFormat = TIMESTAMP_NONE;
            }else if(!(strcmp(jprofileJSONReportTimestamp->valuestring, "Unix-Epoch"))) {
                profile->jsonEncoding->tsFormat = TIMESTAMP_UNIXEPOCH;
            }else if(!(strcmp(jprofileJSONReportTimestamp->valuestring, "ISO-8601"))) {
                profile->jsonEncoding->tsFormat = TIMESTAMP_ISO_8601;
            }else {/*default value for profile->jsonEncoding->tsFormat is None */
                profile->jsonEncoding->tsFormat = TIMESTAMP_NONE;
            }
            T2Debug("[[profile->jsonEncoding->tsFormat:%d]]\n", profile->jsonEncoding->tsFormat);

        }
    }

    if((profile->t2HTTPDest) && (strcmp(jprofileProtocol->valuestring, "HTTP") == 0)) {
        profile->t2HTTPDest->URL = strdup(jprofileHTTPURL->valuestring);
        profile->t2HTTPDest->Compression = COMP_NONE; /*in 1911_sprint default none*/
        profile->t2HTTPDest->Method = HTTP_POST; /*1911_sprint default to POST */

        T2Debug("[[profile->t2HTTPDest->URL:%s]]\n", profile->t2HTTPDest->URL);
        T2Debug("[[profile->t2HTTPDest->Compression:%d]]\n", profile->t2HTTPDest->Compression);
        T2Debug("[[profile->t2HTTPDest->Method:%d]]\n", profile->t2HTTPDest->Method);

        if(jprofileHTTPRequestURIParameter) {

            Vector_Create(&(profile->t2HTTPDest->RequestURIparamList));
            char* href = NULL;
            char* hname = NULL;
            int httpret = 0;
            int httpParamCount = 0;
            int profileHTTPRequestURIParameterIndex = 0;

            for( profileHTTPRequestURIParameterIndex = 0; profileHTTPRequestURIParameterIndex < ThisprofileHTTPRequestURIParameter_count;
                    profileHTTPRequestURIParameterIndex++ ) {
                href = NULL;
                hname = NULL;
                cJSON* pHTTPRPsubitem = cJSON_GetArrayItem(jprofileHTTPRequestURIParameter, profileHTTPRequestURIParameterIndex);
                if(pHTTPRPsubitem != NULL) {
                    cJSON *pHTTPRPsubitemReference = cJSON_GetObjectItem(pHTTPRPsubitem, "Reference");
                    if(pHTTPRPsubitemReference) {
                        href = pHTTPRPsubitemReference->valuestring;
                        hname = pHTTPRPsubitemReference->valuestring; // default value for Name
                        if(!(strcmp(href, ""))) { /*if reference is empty avoid adding this object*/
                            continue;
                        }
                    }else {
                        continue;
                    }

                    cJSON *pHTTPRPsubitemName = cJSON_GetObjectItem(pHTTPRPsubitem, "Name");
                    if(pHTTPRPsubitemName) {
                        hname = pHTTPRPsubitemName->valuestring;
                    }
                    httpret = addhttpURIreqParameter(profile, hname, href);
                    if(httpret != T2ERROR_SUCCESS) {
                        T2Error("%s Error in adding request URIparameterReference: %s for the profile: %s \n", __FUNCTION__, href, profile->name);
                        continue;
                    }else {
                        T2Debug("[[Added  request URIparameterReference: %s]]\n", href);
                    }
                    httpParamCount++;
                }

            }
            T2Info("Profile Name: %s\nConfigured httpURIReqParam count = %d \n", profile->name, ThisprofileHTTPRequestURIParameter_count);
            T2Info("Number of httpURIReqParam added  = %d \n", httpParamCount);
        }
    }

    //Parameter Markers configuration
    Vector_Create(&profile->paramList);
    Vector_Create(&profile->staticParamList);
    Vector_Create(&profile->eMarkerList);
    Vector_Create(&profile->gMarkerList);
    Vector_Create(&profile->cachedReportList);

    char* paramtype = NULL;
    char* use = NULL;
    bool reportEmpty = false;
    char* header = NULL;
    char* content = NULL;
    char* logfile = NULL;
    int skipFrequency = 0;
    size_t profileParamCount = 0;
    int ProfileParameterIndex = 0;

    for( ProfileParameterIndex = 0; ProfileParameterIndex < ThisProfileParameter_count; ProfileParameterIndex++ ) {
        header = NULL;
        content = NULL;
        logfile = NULL;
        skipFrequency = 0;
        paramtype = NULL;
        use = NULL;
        reportEmpty = false;

        cJSON* pSubitem = cJSON_GetArrayItem(jprofileParameter, ProfileParameterIndex);
        if(pSubitem != NULL) {
            cJSON *jpSubitemtype = cJSON_GetObjectItem(pSubitem, "type");
            if(jpSubitemtype) {
                paramtype = jpSubitemtype->valuestring;
            }else { /*TODO:Confirm if type is not configured. For now avoid corresponding marker*/
                continue;
            }

            /* Confirm if "use" is considerable for datamodel type markers.*/
            cJSON *jpSubitemuse = cJSON_GetObjectItem(pSubitem, "use");
            if(jpSubitemuse) {
                use = jpSubitemuse->valuestring;
            }
            cJSON *jpSubitemreportEmpty = cJSON_GetObjectItem(pSubitem, "reportEmpty");
            if(jpSubitemreportEmpty) {
                reportEmpty = cJSON_IsTrue(jpSubitemreportEmpty);
            }
            if(!(strcmp(paramtype, "dataModel"))) { /*Marker is TR181 param*/
                cJSON *jpSubitemname = cJSON_GetObjectItem(pSubitem, "name");
                if(jpSubitemname) {
                    header = jpSubitemname->valuestring;
                }
                cJSON *jpSubitemreference = cJSON_GetObjectItem(pSubitem, "reference");
                if(jpSubitemreference) {
                    content = jpSubitemreference->valuestring;
                    if(!jpSubitemname) {
                        header = jpSubitemreference->valuestring; /*Default Name can be reference*/
                    }
                }
            }else if(!(strcmp(paramtype, "event"))) {

                cJSON *jpSubitemname = cJSON_GetObjectItem(pSubitem, "name"); // Optional repalcement name in report
                cJSON *jpSubitemeventName = cJSON_GetObjectItem(pSubitem, "eventName");
                cJSON *jpSubitemcomponent = cJSON_GetObjectItem(pSubitem, "component");

                if(jpSubitemname) {
                    logfile = jpSubitemname->valuestring;
                }
                if(jpSubitemeventName) {
                    header = jpSubitemeventName->valuestring; /*Default Name can be eventName*/
                }

                if(jpSubitemcomponent) {
                    content = jpSubitemcomponent->valuestring;
                }
            }else if(!(strcmp(paramtype, "grep"))) { //grep Marker

                cJSON *jpSubitemname = cJSON_GetObjectItem(pSubitem, "marker");
                cJSON *jpSubitSearchString = cJSON_GetObjectItem(pSubitem, "search");
                cJSON *jpSubitemLogFile = cJSON_GetObjectItem(pSubitem, "logFile");
                if (jpSubitemname){
                    header = jpSubitemname->valuestring;
                }
                if(jpSubitSearchString){
                    content = jpSubitSearchString->valuestring;
                }
                if(jpSubitemLogFile){
                    logfile = jpSubitemLogFile->valuestring;
                }
            } else {
                T2Error("%s Unknown parameter type %s \n", __FUNCTION__, paramtype);
                continue;
            }
            ret = addParameter(profile, header, content, logfile, skipFrequency, paramtype, use, reportEmpty); //add Multiple Report Profile Parameter
            if(ret != T2ERROR_SUCCESS) {
                T2Error("%s Error in adding parameter to profile %s \n", __FUNCTION__, profile->name);
                continue;
            }else {
                T2Debug("[[Added parameter:%s]]\n", header);
            }
            profileParamCount++;
        }
    }
    // Not included for RDKB-25008 . DCA utils expects the list to be sorted based on logfile names
    T2Info("Number of tr181params/markers successfully added in profile = %d \n", profileParamCount);

    cJSON_Delete(json_root);
    json_root = NULL ;
    *localProfile = profile;
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;

}

char *msgpack_strdup(msgpack_object *obj)
{
    if (NULL == obj || MSGPACK_OBJECT_STR != obj->type)
	return NULL;
    return strndup(obj->via.str.ptr, obj->via.str.size);
}

int msgpack_strcmp(msgpack_object *obj, char *str)
{
    if (NULL == obj || MSGPACK_OBJECT_STR != obj->type)
	return -1;
    return strncmp(str, obj->via.str.ptr, obj->via.str.size);
}

void msgpack_print(msgpack_object *obj, char *obj_name)
{
    if (obj) {
	switch(obj->type) {
	case MSGPACK_OBJECT_BOOLEAN:
	    T2Debug("%s: %s\n", obj_name, obj->via.boolean ? "true" : "false");
	    break;
	case MSGPACK_OBJECT_POSITIVE_INTEGER:
	    T2Debug("%s: %lu\n", obj_name, (unsigned long)obj->via.u64);
	    break;
	case MSGPACK_OBJECT_NEGATIVE_INTEGER:
	    T2Debug("%s: %ld\n", obj_name, (signed long)obj->via.i64);
	    break;
	case MSGPACK_OBJECT_FLOAT:
	    T2Debug("%s: %f\n", obj_name, obj->via.f64);
	    break;
	case MSGPACK_OBJECT_STR:
	    T2Debug("%s: %.*s\n", obj_name, obj->via.str.size, obj->via.str.ptr);
	    break;
	case MSGPACK_OBJECT_ARRAY:
	    T2Debug("%s size: %d\n", obj_name, obj->via.array.size);
            break;
	case MSGPACK_OBJECT_MAP:
	    T2Debug("%s size: %d\n", obj_name, obj->via.map.size);
	    break;
	default:
	    T2Info("%s->type is Invalid\n", obj_name);
	    T2Info("%s->type is %u \n", obj_name, obj->type);
	}
    } else {
	T2Debug("%s Object is NULL\n", obj_name);
    }
}

msgpack_object *msgpack_get_map_value(msgpack_object *obj, char *key)
{
    if (NULL == obj)
	return NULL;
    if (MSGPACK_OBJECT_MAP == obj->type)
	if(obj->via.map.size != 0) {
	    msgpack_object_kv* current = obj->via.map.ptr;
	    msgpack_object_kv* const end = obj->via.map.ptr + obj->via.map.size;
	    for(; current < end; current++)
		if (current->key.type == MSGPACK_OBJECT_STR) {
		    if ( 0 == strncmp(key, current->key.via.str.ptr, current->key.via.str.size))
			return &current->val;
		}
	}
    return NULL;
}

msgpack_object *msgpack_get_array_element(msgpack_object *obj, int index)
{
    if (NULL == obj)
	return NULL;
    if (MSGPACK_OBJECT_ARRAY == obj->type) {
	if(obj->via.array.size != 0) {
	    msgpack_object* current = obj->via.array.ptr;
	    msgpack_object* const end = obj->via.array.ptr + obj->via.array.size;
	    for (; current < end && index; current++, index--)
		;
	    return current;
	}
    } else if (MSGPACK_OBJECT_MAP == obj->type) {
	if(obj->via.map.size != 0) {
	    msgpack_object_kv* current = obj->via.map.ptr;
	    msgpack_object_kv* const end = obj->via.map.ptr + obj->via.map.size;
	    for(; current < end && index; current++, index--)
		;
	    return &current->val;
	}
    }
    return NULL;
}

T2ERROR processMsgPackConfiguration(msgpack_object *profiles_array_map, Profile **profile_dp)
{
    msgpack_object *name_str;
    msgpack_object *hash_str;
    msgpack_object *value_map;
    msgpack_object *Description_str;
    msgpack_object *Version_str;
    msgpack_object *Protocol_str;
    msgpack_object *EncodingType_str;
    msgpack_object *ReportingInterval_u64;
    msgpack_object *TimeReference_str;
    msgpack_object *ActivationTimeout_u64;
    msgpack_object *Parameter_array;
    msgpack_object *Parameter_array_map;
    msgpack_object *Parameter_type_str;
    msgpack_object *Parameter_marker_str;
    msgpack_object *Parameter_search_str;
    msgpack_object *Parameter_logFile_str;
    msgpack_object *Parameter_use_str;
    msgpack_object *Parameter_reference_str;
    msgpack_object *Parameter_eventName_str;
    msgpack_object *Parameter_component_str;
    msgpack_object *Parameter_name_str;
    msgpack_object *Parameter_reportEmpty_boolean;
    msgpack_object *HTTP_map;
    msgpack_object *URL_str;
    msgpack_object *Compression_str;
    msgpack_object *Method_str;
    msgpack_object *RequestURIParameter_array;
    msgpack_object *RequestURIParameter_array_map;
    msgpack_object *RequestURIParameter_Name_str;
    msgpack_object *RequestURIParameter_Reference_str;
    msgpack_object *JSONEncoding_map;
    msgpack_object *ReportFormat_str;
    msgpack_object *ReportTimestamp_str;
    msgpack_object *GenerateNow_boolean;

    int i;
    int ret;

    uint32_t Parameter_array_size = 0;
    uint32_t RequestURIParameter_array_size = 0;
    
    T2Debug("%s --in\n", __FUNCTION__);
    Profile *profile = (Profile *) malloc(sizeof(Profile));
    if(profile == NULL) {
	T2Error("Malloc error exiting: can not allocate memory to create profile \n");
	return T2ERROR_MEMALLOC_FAILED;
    }
    memset(profile, 0, sizeof(Profile));

    name_str = msgpack_get_map_value(profiles_array_map, "name");
    msgpack_print(name_str, msgpack_get_obj_name(name_str));
    profile->name = msgpack_strdup(name_str);

    hash_str = msgpack_get_map_value(profiles_array_map, "hash");
    if(NULL == hash_str)
        hash_str = msgpack_get_map_value(profiles_array_map, "VersionHash");
    msgpack_print(hash_str, msgpack_get_obj_name(hash_str));
    profile->hash = msgpack_strdup(hash_str);

    value_map = msgpack_get_map_value(profiles_array_map, "value");

    Protocol_str = msgpack_get_map_value(value_map, "Protocol");
    msgpack_print(Protocol_str, msgpack_get_obj_name(Protocol_str));
    profile->protocol = msgpack_strdup(Protocol_str);
    if (0 == msgpack_strcmp(Protocol_str, "HTTP")) {
	profile->t2HTTPDest = (T2HTTP *) malloc(sizeof(T2HTTP));
	if(NULL == profile->t2HTTPDest) {
	    free(profile->name);
	    free(profile->hash);
	    free(profile);
	    T2Error("Malloc error exiting: can not allocate memory to create profile->t2HTTPDest \n");
	    return T2ERROR_MEMALLOC_FAILED;
	}
    }

    EncodingType_str = msgpack_get_map_value(value_map, "EncodingType");
    msgpack_print(EncodingType_str, msgpack_get_obj_name(EncodingType_str));
    profile->encodingType = msgpack_strdup(EncodingType_str);
    if (0 == msgpack_strcmp(EncodingType_str, "JSON")) {
	profile->jsonEncoding = (JSONEncoding *) malloc(sizeof(JSONEncoding));
	if(NULL == profile->jsonEncoding) {
	    free(profile->name);
	    free(profile->hash);
	    free(profile->t2HTTPDest);
	    free(profile);
	    T2Error("Malloc error exiting: can not allocate memory to create profile->jsonEncoding \n");
	    return T2ERROR_MEMALLOC_FAILED;
	}
    }

    Description_str = msgpack_get_map_value(value_map, "Description");
    msgpack_print(Description_str, msgpack_get_obj_name(Description_str));
    profile->Description = msgpack_strdup(Description_str);

    Version_str = msgpack_get_map_value(value_map, "Version");
    msgpack_print(Version_str, msgpack_get_obj_name(Version_str));
    profile->version = msgpack_strdup(Version_str);

    profile->activationTimeoutPeriod = INFINITE_TIMEOUT;
    profile->generateNow = false;

    GenerateNow_boolean = msgpack_get_map_value(value_map, "GenerateNow");
    msgpack_print(GenerateNow_boolean, msgpack_get_obj_name(GenerateNow_boolean));
    MSGPACK_GET_NUMBER(GenerateNow_boolean, profile->generateNow);
    T2Debug("profile->generateNow: %u\n", profile->generateNow);
    if (profile->generateNow) {
	profile->reportingInterval = 0;
    } else {

	ReportingInterval_u64 = msgpack_get_map_value(value_map, "ReportingInterval");
	msgpack_print(ReportingInterval_u64, msgpack_get_obj_name(ReportingInterval_u64));
	MSGPACK_GET_NUMBER(ReportingInterval_u64, profile->reportingInterval);
	T2Debug("profile->reportingInterval: %u\n", profile->reportingInterval);

	ActivationTimeout_u64 = msgpack_get_map_value(value_map, "ActivationTimeout");
	msgpack_print(ActivationTimeout_u64, msgpack_get_obj_name(ActivationTimeout_u64));
	MSGPACK_GET_NUMBER(ActivationTimeout_u64, profile->activationTimeoutPeriod);
	T2Debug("profile->activationTimeoutPeriod: %u\n", profile->activationTimeoutPeriod);
    }

    TimeReference_str = msgpack_get_map_value(value_map, "TimeReference");
    msgpack_print(TimeReference_str, msgpack_get_obj_name(TimeReference_str));
    /* profile->timeRef = msgpack_strdup(TimeReference_str); */
    /* MUST TODO: Seems timeref is an unsigned int in profile structure. */
    /* Handle the scenario accordingly */

    /* Parameter Markers configuration */
    Vector_Create(&profile->paramList);
    Vector_Create(&profile->staticParamList);
    Vector_Create(&profile->eMarkerList);
    Vector_Create(&profile->gMarkerList);
    Vector_Create(&profile->cachedReportList);

    Parameter_array = msgpack_get_map_value(value_map, "Parameter");
    int profileParamCount = 0;
    MSGPACK_GET_ARRAY_SIZE(Parameter_array, Parameter_array_size);
    for (i = 0; i < Parameter_array_size; i++) {

	char* paramtype;
	char* use;
	char* header;
	char* content;
	char* logfile;
	bool reportEmpty;
	int skipFrequency;

	header = NULL;
	content = NULL;
	logfile = NULL;
	skipFrequency = 0;
	paramtype = NULL;
	use = NULL;
	reportEmpty = false;

	Parameter_array_map = msgpack_get_array_element(Parameter_array, i);

	Parameter_type_str = msgpack_get_map_value(Parameter_array_map, "type");
	if (NULL == Parameter_type_str)
	    /*TODO:Confirm if type is not configured. For now avoid corresponding marker*/
	    continue;
	msgpack_print(Parameter_type_str, msgpack_get_obj_name(Parameter_type_str));
	paramtype = msgpack_strdup(Parameter_type_str);

	Parameter_use_str = msgpack_get_map_value(Parameter_array_map, "use");
	msgpack_print(Parameter_use_str, msgpack_get_obj_name(Parameter_use_str));
	use = msgpack_strdup(Parameter_use_str);

	Parameter_reportEmpty_boolean = msgpack_get_map_value(Parameter_array_map, "reportEmpty");
	msgpack_print(Parameter_reportEmpty_boolean, msgpack_get_obj_name(Parameter_reportEmpty_boolean));
	MSGPACK_GET_NUMBER(Parameter_reportEmpty_boolean, reportEmpty);
	T2Debug("reportEmpty: %u\n", reportEmpty);

	if (0 == msgpack_strcmp(Parameter_type_str, "dataModel")) {

	    Parameter_name_str = msgpack_get_map_value(Parameter_array_map, "name");
	    msgpack_print(Parameter_name_str, msgpack_get_obj_name(Parameter_name_str));
	    header = msgpack_strdup(Parameter_name_str);

	    Parameter_reference_str = msgpack_get_map_value(Parameter_array_map, "reference");
	    msgpack_print(Parameter_reference_str, msgpack_get_obj_name(Parameter_reference_str));
	    content = msgpack_strdup(Parameter_reference_str);

	    if (NULL == header)
		header = msgpack_strdup(Parameter_reference_str);

	} else if (0 == msgpack_strcmp(Parameter_type_str, "event")) {

	    Parameter_name_str = msgpack_get_map_value(Parameter_array_map, "name");
	    msgpack_print(Parameter_name_str, msgpack_get_obj_name(Parameter_name_str));
	    logfile = msgpack_strdup(Parameter_name_str);

	    Parameter_eventName_str = msgpack_get_map_value(Parameter_array_map, "eventName");
	    msgpack_print(Parameter_eventName_str, msgpack_get_obj_name(Parameter_eventName_str));
	    header = msgpack_strdup(Parameter_eventName_str);

	    Parameter_component_str = msgpack_get_map_value(Parameter_array_map, "component");
	    msgpack_print(Parameter_component_str, msgpack_get_obj_name(Parameter_component_str));
	    content = msgpack_strdup(Parameter_component_str);

	} else if (0 == msgpack_strcmp(Parameter_type_str, "grep")) {

	    Parameter_marker_str = msgpack_get_map_value(Parameter_array_map, "marker");
	    msgpack_print(Parameter_marker_str, msgpack_get_obj_name(Parameter_marker_str));
	    header = msgpack_strdup(Parameter_marker_str);

	    Parameter_search_str = msgpack_get_map_value(Parameter_array_map, "search");
	    msgpack_print(Parameter_search_str, msgpack_get_obj_name(Parameter_search_str));
	    content = msgpack_strdup(Parameter_search_str);

	    Parameter_logFile_str = msgpack_get_map_value(Parameter_array_map, "logFile");
	    msgpack_print(Parameter_logFile_str, msgpack_get_obj_name(Parameter_logFile_str));
	    logfile = msgpack_strdup(Parameter_logFile_str);

	}
        else {
            T2Error("%s Unknown parameter type %s \n", __FUNCTION__, paramtype);
            free(paramtype);
            free(use);
            continue;
        }
	ret = addParameter(profile, header, content, logfile, skipFrequency, paramtype, use, reportEmpty);
	/* Add Multiple Report Profile Parameter */
	if(T2ERROR_SUCCESS != ret) {
	    T2Error("%s Error in adding parameter to profile %s \n", __FUNCTION__, profile->name);
	} else {
	    T2Debug("Added parameter:%s \n", header);
	    profileParamCount++;
	}
	free(header);
	free(content);
	free(logfile);
	free(use);
	free(paramtype);
    }
    T2Debug("Added parameter count:%d \n", profileParamCount);

    HTTP_map = msgpack_get_map_value(value_map, "HTTP");

    URL_str = msgpack_get_map_value(HTTP_map, "URL");
    msgpack_print(URL_str, msgpack_get_obj_name(URL_str));
    profile->t2HTTPDest->URL = msgpack_strdup(URL_str);

    Compression_str = msgpack_get_map_value(HTTP_map, "Compression");
    msgpack_print(Compression_str, msgpack_get_obj_name(Compression_str));
    profile->t2HTTPDest->Compression = COMP_NONE; /*in 1911_sprint default none*/

    Method_str = msgpack_get_map_value(HTTP_map, "Method");
    msgpack_print(Method_str, msgpack_get_obj_name(Method_str));
    profile->t2HTTPDest->Method = HTTP_POST; /*1911_sprint default to POST */

    Vector_Create(&(profile->t2HTTPDest->RequestURIparamList));
    RequestURIParameter_array = msgpack_get_map_value(HTTP_map, "RequestURIParameter");
    int httpParamCount = 0;
    MSGPACK_GET_ARRAY_SIZE(RequestURIParameter_array, RequestURIParameter_array_size);
    for ( i = 0; i < RequestURIParameter_array_size; i++) {
	char* href;
	char* hname;
	int httpret = 0;

	href = NULL;
	hname = NULL;
	RequestURIParameter_array_map = msgpack_get_array_element(RequestURIParameter_array, i);

	RequestURIParameter_Reference_str = msgpack_get_map_value(RequestURIParameter_array_map,
								  "Reference");
	msgpack_print(RequestURIParameter_Reference_str,
		      msgpack_get_obj_name(RequestURIParameter_Reference_str));
	href = msgpack_strdup(RequestURIParameter_Reference_str);

	RequestURIParameter_Name_str = msgpack_get_map_value(RequestURIParameter_array_map, "Name");
	msgpack_print(RequestURIParameter_Name_str, msgpack_get_obj_name(RequestURIParameter_Name_str));
	hname = msgpack_strdup(RequestURIParameter_Name_str);
	if(NULL == hname)
	    hname = msgpack_strdup(RequestURIParameter_Reference_str);

	httpret = addhttpURIreqParameter(profile, hname, href);
	if(T2ERROR_SUCCESS != httpret) {
	    T2Error("%s Error in adding request URIparameterReference: %s for the profile: %s \n",
		    __FUNCTION__, href, profile->name);
	} else {
	    T2Debug("Added  request URIparameterReference: %s \n", href);
	    httpParamCount++;
	}
	free(href);
	free(hname);
    }
    T2Debug("Added URI parameter count:%d \n", httpParamCount);

    JSONEncoding_map = msgpack_get_map_value(value_map, "JSONEncoding");

    ReportFormat_str = msgpack_get_map_value(JSONEncoding_map, "ReportFormat");
    msgpack_print(ReportFormat_str, msgpack_get_obj_name(ReportFormat_str));
    if(0 == msgpack_strcmp(ReportFormat_str, "NameValuePair")) {
	profile->jsonEncoding->reportFormat = JSONRF_KEYVALUEPAIR;
    }else if(0 == msgpack_strcmp(ReportFormat_str, "ObjectHierarchy")) {
	profile->jsonEncoding->reportFormat = JSONRF_OBJHIERARCHY;
    }else { /* defaulting it to NameValuePair */
	profile->jsonEncoding->reportFormat = JSONRF_KEYVALUEPAIR;
    }

    ReportTimestamp_str = msgpack_get_map_value(JSONEncoding_map, "ReportTimestamp");
    msgpack_print(ReportTimestamp_str, msgpack_get_obj_name(ReportTimestamp_str));
    if(0 == msgpack_strcmp(ReportTimestamp_str, "None")) {
	profile->jsonEncoding->tsFormat = TIMESTAMP_NONE;
    }else if(0 == msgpack_strcmp(ReportTimestamp_str, "Unix-Epoch")) {
	profile->jsonEncoding->tsFormat = TIMESTAMP_UNIXEPOCH;
    }else if(0 == msgpack_strcmp(ReportTimestamp_str, "ISO-8601")) {
	profile->jsonEncoding->tsFormat = TIMESTAMP_ISO_8601;
    }else {/*default value for profile->jsonEncoding->tsFormat is None */
	profile->jsonEncoding->tsFormat = TIMESTAMP_NONE;
    }

    *profile_dp = profile;
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}
