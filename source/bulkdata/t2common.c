/*
 * t2common.c
 *
 *  Created on: Dec 12, 2019
 *      Author: mvalas827
 */
#include "t2common.h"

void freeParam(void *data)
{
    if(data != NULL)
    {
        Param *param = (Param *)data;
        if(param->name)
            free(param->name);
        if(param->alias)
            free(param->alias);
        if(param->paramType)
            free(param->paramType);
        free(param);
        param = NULL;
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
        sparam = NULL;
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
        if(eMarker->mType == MTYPE_ABSOLUTE && eMarker->u.markerValue)
            free(eMarker->u.markerValue);
        free(eMarker);
        eMarker = NULL;
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
        if(gMarker->paramType)
            free(gMarker->paramType);
        if(gMarker->mType == MTYPE_ABSOLUTE && gMarker->u.markerValue)
            free(gMarker->u.markerValue);
        free(gMarker);
        gMarker = NULL;
    }
}
