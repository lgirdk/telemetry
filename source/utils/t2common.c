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

#include <stdlib.h>
#include <string.h>
#include "t2common.h"
#include "t2log_wrapper.h"

void freeParam(void *data)
{
    if(data != NULL)
    {
        Param *param = (Param *)data;
        if(param->name)
            free(param->name);
        if(param->alias)
            free((char*)param->alias);
        if(param->paramType)
            free(param->paramType);
        free(param);
    }
}

void freeStaticParam(void *data)
{
    if(data != NULL)
    {
        StaticParam *sparam = (StaticParam *)data;
        if(sparam->name)
            free(sparam->name);
        if (sparam->paramType)
            free(sparam->paramType);
        if (sparam->value)
            free(sparam->value);

        free(sparam);
    }
}

void freeEMarker(void *data)
{
    if(data != NULL)
    {
        EventMarker *eMarker = (EventMarker *)data;
        if(eMarker->alias)
            free(eMarker->alias);
        if(eMarker->compName)
            free(eMarker->compName);
        if(eMarker->markerName)
            free(eMarker->markerName);
        if(eMarker->paramType)
            free(eMarker->paramType);
        if(eMarker->reportTimestampParam == REPORTTIMESTAMP_UNIXEPOCH) {
            if(eMarker->markerName_CT != NULL){
                free(eMarker->markerName_CT);
            }
            if(eMarker->timestamp != NULL){
                free(eMarker->timestamp);
            }
        }
        if(eMarker->mType == MTYPE_ABSOLUTE && eMarker->u.markerValue)
            free(eMarker->u.markerValue);
        if(eMarker->mType == MTYPE_ACCUMULATE && eMarker->u.accumulatedValues) {
            Vector_Destroy(eMarker->u.accumulatedValues, freeAccumulatedParam);
            if(eMarker->accumulatedTimestamp != NULL && eMarker->reportTimestampParam == REPORTTIMESTAMP_UNIXEPOCH) {
                Vector_Destroy(eMarker->accumulatedTimestamp, freeAccumulatedParam);
            }
        }
        free(eMarker);
    }
}

void freeAccumulatedParam(void* data)
{
    if(data != NULL){
        char* dataStr = data;
        free(dataStr);
    }
}

void freeGMarker(void *data)
{
    if(data != NULL)
    {
        GrepMarker *gMarker = (GrepMarker *)data;
        if(gMarker->logFile)
            free(gMarker->logFile);
        if(gMarker->markerName)
            free(gMarker->markerName);
        if(gMarker->searchString)
        	free(gMarker->searchString);
        if(gMarker->paramType)
            free(gMarker->paramType);
        if(gMarker->mType == MTYPE_ABSOLUTE && gMarker->u.markerValue)
            free(gMarker->u.markerValue);
        free(gMarker);
    }
}

int compareLogFileNames(const void *g1, const void *g2)
{
   GrepMarker** p1 = (GrepMarker**) g1 ;
   GrepMarker** p2 = (GrepMarker**) g2 ;

   if ( NULL != p1 && NULL != p2) {
       return strcmp((*p1)->logFile, (*p2)->logFile);
   } else {
       T2Error("compareLogFileNames : either p1 or p2 is NULL \n");
       return -1 ;
   }
}

void freeTriggerCondition(void *data)
{
    if(data != NULL)
    {
        TriggerCondition *tCondition = (TriggerCondition *)data;
        if(tCondition->type)
            free(tCondition->type);
        if(tCondition->oprator)
            free(tCondition->oprator);
        if(tCondition->reference)
            free(tCondition->reference);
        free(tCondition); 
    }
}
