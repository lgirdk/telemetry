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

static char * getProfileParameter(Profile * profile, char *ref) {
    char *pValue = "NULL";
    char *pName = strrchr(ref, '.') + 1;

    if (!strcmp(pName, "Name"))
    {
        if (profile->name)
            pValue = strdup(profile->name);
    }
    else if(!strcmp(pName, "Version"))
    {
        if (profile->version)
            pValue = strdup(profile->version);
    }
    else if(!strcmp(pName, "Description"))
    {
        if (profile->Description)
            pValue = strdup(profile->Description);
    }
    else
    {
        //TODO: Extend it for any other static Profile parameter
        pValue = strdup("NULL");
    }

    return pValue;
}

static T2ERROR addhttpURIreqParameter(Profile *profile, const char* Hname, const char* Href) {
    T2Debug("%s ++in\n", __FUNCTION__);
    HTTPReqParam *httpreqparam = (HTTPReqParam *) malloc(sizeof(Param));
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

    if(profile->paramNumOfEntries == MAX_PROFILE_PARAM_ENTRIES) {
        T2Error("Max Profile Param Entries Reached, ignoring addParameter request for profileID : %s\n", profile->name);
        return T2ERROR_FAILURE;
    }

    if(!(strcmp(ptype, "dataModel"))) {
        T2Debug("Adding TR-181 Parameter : %s\n", ref);

        if (strstr(ref, "Profile.") == ref) {        //Static parameters from profile configuration
            StaticParam *sparam = (StaticParam *) malloc(sizeof(StaticParam));
            sparam->paramType = strdup(ptype);
            sparam->name = strdup(name);
            sparam->value = getProfileParameter(profile, ref);

            Vector_PushBack(profile->staticParamList, sparam);
        } else {
            Param *param = (Param *) malloc(sizeof(Param));
            param->name = strdup(name);
            param->alias = strdup(ref);
            param->paramType = strdup(ptype);
            param->reportEmptyParam = ReportEmpty;

            Vector_PushBack(profile->paramList, param);
        }
    }else if(!(strcmp(ptype, "event"))) {
        T2Debug("Adding Event Marker :: Param/Marker Name : %s ref/pattern/Comp : %s skipFreq : %d\n", name, ref, skipFreq);
        EventMarker *eMarker = (EventMarker *) malloc(sizeof(EventMarker));

        eMarker->markerName = strdup(name);
        eMarker->compName = strdup(ref);
        if(fileName)
            eMarker->alias = strdup(fileName);
        else
            eMarker->alias = NULL;
        eMarker->paramType = strdup(ptype);
        eMarker->reportEmptyParam = ReportEmpty;

        if(!(strcmp(use, "absolute"))) {
            eMarker->mType = MTYPE_ABSOLUTE;
            eMarker->u.markerValue = NULL;
        }else if(!(strcmp(use, "count"))) {
            eMarker->mType = MTYPE_COUNTER;
            eMarker->u.count = 0;
        }else { //default to absolute type
            eMarker->mType = MTYPE_ABSOLUTE;
            eMarker->u.markerValue = NULL;
        }
        eMarker->skipFreq = skipFreq;

        Vector_PushBack(profile->eMarkerList, eMarker);
    }else { //Grep Marker

        T2Debug("Adding Grep Marker :: Param/Marker Name : %s ref/pattern/Comp : %s fileName : %s skipFreq : %d\n", name, ref, fileName, skipFreq);

        GrepMarker *gMarker = (GrepMarker *) malloc(sizeof(GrepMarker));
        gMarker->markerName = strdup(name);
        gMarker->searchString = strdup(ref);
        gMarker->logFile = strdup(fileName);
        gMarker->paramType = strdup(ptype);
        gMarker->reportEmptyParam = ReportEmpty;

        if(!(strcmp(use, "absolute"))) {
            gMarker->mType = MTYPE_ABSOLUTE;
            gMarker->u.markerValue = NULL;
        }else if(!(strcmp(use, "count"))) {
            gMarker->mType = MTYPE_COUNTER;
            gMarker->u.count = 0;
        }else {  //default to absolute type
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
    T2ERROR ret = 0;
    int ThisProfileParameter_count = 0;
    cJSON *json_root = cJSON_Parse(*configData);

    cJSON *jprofileName = cJSON_GetObjectItem(json_root, "Name");
    cJSON *jprofileHash = cJSON_GetObjectItem(json_root, "Hash");
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

                header = cJSON_GetObjectItem(pSubitem, "marker")->valuestring;
                content = cJSON_GetObjectItem(pSubitem, "search")->valuestring;
                logfile = cJSON_GetObjectItem(pSubitem, "logFile")->valuestring;

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
    *localProfile = profile;
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;

}

