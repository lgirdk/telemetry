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
#include "t2markers.h"

#include "t2collection.h"
#include "t2eventreceiver.h"
#include "t2log_wrapper.h"

/**
 * Store event markers associated with a component
 */
static hash_map_t *markerCompMap = NULL;

/**
 * Store component names that has targeted event markers from any of the profile
 */
static Vector* componentList = NULL ;

static pthread_mutex_t t2MarkersMutex;
static pthread_mutex_t t2CompListMutex;


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
        free(element);
    }
}

static void freeT2ComponentList(void *data){
    if(data != NULL)
    {
        free(data);
        data = NULL ;
    }
}

T2ERROR initT2MarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    markerCompMap = hash_map_create();
    pthread_mutex_init(&t2MarkersMutex, NULL);
    pthread_mutex_init(&t2CompListMutex, NULL);
    Vector_Create(&componentList);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR clearT2MarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    pthread_mutex_lock(&t2MarkersMutex);
    hash_map_clear(markerCompMap, freeT2Marker);
    pthread_mutex_unlock(&t2MarkersMutex);  

    Vector_Destroy(componentList, freeT2ComponentList);
    componentList = NULL ;
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR destroyT2MarkerComponentMap()
{
    T2Debug("%s ++in\n", __FUNCTION__);
    pthread_mutex_lock(&t2MarkersMutex);
    hash_map_destroy(markerCompMap, freeT2Marker);
    markerCompMap = NULL;
    pthread_mutex_unlock(&t2MarkersMutex);
    pthread_mutex_destroy(&t2MarkersMutex);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR updateEventMap(const char* markerName , T2Marker* t2Marker )
{
    char *eventName = hash_map_get(markerCompMap, markerName );
    if( NULL == eventName ) {
        if(!markerCompMap)
            markerCompMap = hash_map_create();
        hash_map_put(markerCompMap, strdup(markerName), (void *)t2Marker);
    }
    return T2ERROR_SUCCESS;
}

static void updateComponentList(const char* componentName) {

    //T2Debug("%s ++in \n", __FUNCTION__);
    if(!componentName) {
        T2Error("componentName is null\n");
        T2Debug("%s --out\n", __FUNCTION__);
        return;
    }
    if(!componentList) {
        T2Error("Component name list is not initialized . Re-initializing \n");
        Vector_Create(&componentList);
    }

    int length = Vector_Size(componentList);
    if(0 == length) {
        Vector_PushBack(componentList, (void*) strdup(componentName));
    }else {
        int i = 0;
        for( ; i < length; i++ ) {
            char* tempName = Vector_At(componentList, i);
            if(strncmp(tempName, componentName, MAX_EVENT_MARKER_NAME_LEN) == 0) {
                T2Debug("%s --out\n", __FUNCTION__);
                return;
            }
        }
        Vector_PushBack(componentList, (void*) strdup(componentName));
    }
    //T2Debug("%s --out\n", __FUNCTION__);
    return;
}

T2ERROR addT2EventMarker(const char* markerName, const char* compName, const char *profileName, unsigned int skipFreq)
{
    pthread_mutex_lock(&t2MarkersMutex);
    T2Marker *t2Marker = (T2Marker *)hash_map_get(markerCompMap, markerName);
    if(t2Marker)
    {
        T2Debug("Found a matching T2Marker \n");
        if(t2Marker->profileList) {
            int i = 0;
            int length = Vector_Size(t2Marker->profileList);
            bool isPresent = false;
            for( i = 0; i < length; ++i ) {
                char* profNameInlist = (char *) Vector_At(t2Marker->profileList, i);
                if(!strncmp(profileName, profNameInlist, length)) {
                    isPresent = true;
                    break;
                }
            }
            if(isPresent != true) {
                T2Debug("Found a matching T2Marker, adding new profileName %s to profileList or marker %s \n", profileName, markerName);
                Vector_PushBack(t2Marker->profileList, (void *) strdup(profileName));
            }else {
                T2Debug("%s already present in eventlist of %s . Ignore updates \n", profileName, markerName);
            }
        }
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
            updateEventMap(markerName, t2Marker);
            updateComponentList(compName);
        }
        else
        {
            T2Error("Unable to add Event Marker to the Map :: Malloc failure\n");
	    pthread_mutex_unlock(&t2MarkersMutex);
            return T2ERROR_FAILURE;
        }
    }
    pthread_mutex_unlock(&t2MarkersMutex);
    return T2ERROR_SUCCESS;
}

void getComponentsWithEventMarkers(Vector **eventComponentList){
    *eventComponentList = componentList ;
}

/**
 * Populates the list of markers associated with a component
 */
void getComponentMarkerList(const char* compName, void **markerList)
{
    Vector *compMarkers = NULL;
    if(T2ERROR_SUCCESS == Vector_Create(&compMarkers))
    {
        int index = 0;

	pthread_mutex_lock(&t2MarkersMutex);
        for (; index < hash_map_count(markerCompMap); index++)
        {
            T2Marker *t2Marker = (T2Marker *)hash_map_lookup(markerCompMap, index);
            if(t2Marker != NULL && !strcmp(t2Marker->componentName, compName))
            {
                Vector_PushBack(compMarkers, (void *)strdup(t2Marker->markerName));
            }
        }
        *markerList = (Vector*) compMarkers;
	pthread_mutex_unlock(&t2MarkersMutex);
    }
    else
    {
        T2Error("Unable to create Vector for Component Markers :: Malloc failure\n");
        return ;
    }
    return;
}

/**
 * Returns the list of profiles to which markerName is associated with
 */

T2ERROR getMarkerProfileList(const char* markerName, Vector **profileList)
{
    pthread_mutex_lock(&t2MarkersMutex);
    T2Marker *t2Marker = (T2Marker *) hash_map_get(markerCompMap, markerName);
    int i = 0 ;
    int count = 0 ;
    if(t2Marker != NULL && t2Marker->profileList != NULL) {
        count = Vector_Size(t2Marker->profileList);
        for (; i < count; ++i) {
            Vector_PushBack(*profileList, strdup(Vector_At(t2Marker->profileList, i)));
        }
    } else {
        pthread_mutex_unlock(&t2MarkersMutex);
        return T2ERROR_FAILURE;
    }

    pthread_mutex_unlock(&t2MarkersMutex);
    return T2ERROR_SUCCESS;
}
