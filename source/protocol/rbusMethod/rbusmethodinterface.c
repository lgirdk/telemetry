/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <net/if.h>
#include <netdb.h>
#include <string.h>
#include <ifaddrs.h>
#include <stdbool.h>

#include "reportgen.h"
#include "rbusInterface.h"
#include "rbusmethodinterface.h"
#include "reportprofiles.h"
#include "t2log_wrapper.h"

static pthread_once_t rbusMethodMutexOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t rbusMethodMutex;

static bool isRbusMethod = false ;

static void sendOverRBUSMethodInit( ) {
    pthread_mutex_init(&rbusMethodMutex, NULL);
}

static void asyncMethodHandler(rbusHandle_t handle, char const* methodName, rbusError_t retStatus, rbusObject_t params) {
    (void) handle;
    (void) params;

    T2Debug("T2 asyncMethodHandler called: %s with return error code  = %s \n", methodName, rbusError_ToString(retStatus));
    if(retStatus == RBUS_ERROR_SUCCESS) {
        isRbusMethod = true ;
    } else {
        T2Error("Unable to send data over method %s \n", methodName) ;
        isRbusMethod = false ;
    }
    pthread_mutex_unlock(&rbusMethodMutex);
}

T2ERROR sendReportsOverRBUSMethod(char *methodName, Vector* inputParams, char* payload) {
    T2Debug("%s ++in\n", __FUNCTION__);
    T2ERROR ret = T2ERROR_FAILURE;
    T2Info("methodName = %s payload = %s \n", methodName, payload);

    rbusObject_t inParams;
    rbusValue_t value;
    int i = 0;

    rbusObject_Init(&inParams, NULL);

    for( i = 0; i < Vector_Size(inputParams); i++ ) {
        rbusValue_Init(&value);
        RBUSMethodParam *rbusMethodParam = (RBUSMethodParam *) Vector_At(inputParams, i);
        if(rbusMethodParam) {
            if(rbusMethodParam->name && rbusMethodParam->value) {
                rbusValue_SetString(value, rbusMethodParam->value);
                rbusObject_SetValue(inParams, rbusMethodParam->name, value);
            }
        }
        rbusValue_Release(value);
    }

    // Default parameters associated with a data payload
    rbusValue_Init(&value);
    rbusValue_SetInt32(value, (strlen(payload) + 1));
    rbusObject_SetValue(inParams, "payloadlen", value);
    rbusValue_Release(value);

    rbusValue_Init(&value);
    rbusValue_SetString(value, payload);
    rbusObject_SetValue(inParams, "payload", value);
    rbusValue_Release(value);

    pthread_once(&rbusMethodMutexOnce, sendOverRBUSMethodInit);
    pthread_mutex_lock(&rbusMethodMutex);
    isRbusMethod = false ;
    if ( T2ERROR_SUCCESS == rbusMethodCaller(methodName, &inParams, payload, &asyncMethodHandler)) {
        pthread_mutex_lock(&rbusMethodMutex);
        if (isRbusMethod) {
             T2Info("Return status of send via rbusMethod is success \n " );
             ret = T2ERROR_SUCCESS ;
        } else {
             T2Info("Return status of send via rbusMethod is failure \n " );
        }
        pthread_mutex_unlock(&rbusMethodMutex);
    }
    rbusObject_Release(inParams);

    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

T2ERROR sendCachedReportsOverRBUSMethod(char *methodName, Vector* inputParams, Vector* reportList) {
    T2Debug("%s ++in\n", __FUNCTION__);
    while(Vector_Size(reportList) > 0) {
        char* payload = (char *) Vector_At(reportList, 0);
        if(T2ERROR_FAILURE == sendReportsOverRBUSMethod(methodName, inputParams, payload)) {
            T2Error("Failed to send cached report, left with %zu reports in cache \n", Vector_Size(reportList));
            return T2ERROR_FAILURE;
        }
        Vector_RemoveItem(reportList, payload, NULL);
        free(payload);
    }
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}