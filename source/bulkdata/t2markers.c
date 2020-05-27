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

#include "t2markers.h"
#include "t2eventreceiver.h"
#include "collection.h"
#include "t2log_wrapper.h"

static hash_map_t *markerCompMap = NULL;

static void freeT2Marker(void *data)
{
    if(data != NULL)
    {
        hash_element_t *element = (hash_element_t *)data;
        free(element->key);
        T2Marker *t2Marker = (T2Marker *)element->data;
        free(t2Marker->componentName);
        free(t2Marker->markerName);
        Vector_Destroy(t2Marker->profileList, free);
        free(t2Marker);
        t2Marker = NULL;
        free(element);
        element = NULL;
    }
}

T2ERROR initT2MarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    markerCompMap = hash_map_create();
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR clearT2MarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    hash_map_clear(markerCompMap, freeT2Marker);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR destroyT2MarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    hash_map_destroy(markerCompMap, freeT2Marker);
    markerCompMap = NULL;
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR addT2EventMarker(const char* markerName, const char* compName, const char *profileName, unsigned int skipFreq)
{
    T2Marker *t2Marker = (T2Marker *)hash_map_get(markerCompMap, markerName);
    if(t2Marker)
    {
        T2Debug("Found a matching T2Marker, adding new profileName to profileList\n");
        Vector_PushBack(t2Marker->profileList, (void *)strdup(profileName));
    }
    else
    {
        t2Marker = (T2Marker *)malloc(sizeof(T2Marker));
        if(t2Marker != NULL)
        {
            t2Marker->markerName = strdup(markerName);
            t2Marker->componentName = strdup(compName);
            Vector_Create(&t2Marker->profileList);
            Vector_PushBack(t2Marker->profileList, (void *)strdup(profileName));

            hash_map_put(markerCompMap, strdup(markerName), (void *)t2Marker);
        }
        else
        {
            T2Error("Unable to add Event Marker to the Map :: Malloc failure\n");
            return T2ERROR_FAILURE;
        }
    }
    return T2ERROR_SUCCESS;
}

T2ERROR getComponentMarkerList(const char* compName, Vector **markerList)
{
    Vector *compMarkers = NULL;
    if(T2ERROR_SUCCESS == Vector_Create(&compMarkers))
    {
        int index = 0;
        for (; index < hash_map_count(markerCompMap); index++)
        {
            T2Marker *t2Marker = (T2Marker *)hash_map_lookup(markerCompMap, index);
            if(t2Marker != NULL && !strcmp(t2Marker->componentName, compName))
            {
                Vector_PushBack(compMarkers, (void *)t2Marker->markerName);
            }
        }
        *markerList = compMarkers;
    }
    else
    {
        T2Error("Unable to create Vector for Component Markers :: Malloc failure\n");
        return T2ERROR_FAILURE;
    }
    return T2ERROR_SUCCESS;
}

T2ERROR getMarkerProfileList(const char* markerName, Vector **profileList)
{
    T2Marker *t2Marker = (T2Marker *)hash_map_get(markerCompMap, markerName);
    if(t2Marker != NULL)
        *profileList = t2Marker->profileList;
    else
    {
        return T2ERROR_FAILURE;
    }

    return T2ERROR_SUCCESS;
}
