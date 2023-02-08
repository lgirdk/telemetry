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

#include "t2MtlsUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef LIBSYSWRAPPER_BUILD
#include <secure_wrapper.h>
#endif
#include <stdbool.h>

#include "t2log_wrapper.h"

#define MAX_DOMAIN_NAME_LEN 253
#define XCONF_MTLS_DOMAN "secure.xconfds.coast.xcal.tv"
#define DATA_LAKE_MTLS_DOMAIN "stbrtl-oi.stb.r53.xcal.tv"

#if !defined(ENABLE_RDKC_SUPPORT)
static const char* staticMtlsCert = "/etc/ssl/certs/staticXpkiCrt.pk12";
static const char* staticMtlsDestFile = "/tmp/.cfgStaticxpki";

static const char* dynamicMtlsDestFile = "/tmp/.cfgDynamicxpki";
#if defined(ENABLE_RDKB_SUPPORT)
static const char* dynamicMtlsCert = "/nvram/certs/devicecert_1.pk12";
#else
static const char* dynamicMtlsCert = "/opt/certs/devicecert_1.pk12";
#endif

#else
static char* staticMtlsCert = "";
static char* staticPassPhrase = "";
static char* dynamicMtlsCert = "";
static char* dynamicPassPhrase = "";
#endif

void initMtls() {
    T2Debug("%s ++in\n", __FUNCTION__);
    // Prepare certs required for mTls commmunication
    // CPG doesn't support api's - will have to use v_secure_system calls
#if !defined (ENABLE_RDKC_SUPPORT)
    #ifdef LIBSYSWRAPPER_BUILD
    v_secure_system("/usr/bin/GetConfigFile %s", staticMtlsDestFile);
    v_secure_system("/usr/bin/GetConfigFile %s", dynamicMtlsDestFile);
    #else
    char command[256] = { '\0' };
    snprintf(command, sizeof(command), "/usr/bin/GetConfigFile %s", staticMtlsDestFile);
    if(system(command) != 0) {
        T2Error("%s,%d: %s command failed\n", __FUNCTION__, __LINE__, command);
    }
    memset(command, '\0', 256);
    snprintf(command, sizeof(command), "/usr/bin/GetConfigFile %s", dynamicMtlsDestFile);
    if(system(command) != 0) {
        T2Error("%s,%d: %s command failed\n", __FUNCTION__, __LINE__, command);
    }
    #endif
#endif
    T2Debug("%s --out\n", __FUNCTION__);

}

void uninitMtls() {
    T2Debug("%s ++in\n", __FUNCTION__);
#if !defined (ENABLE_RDKC_SUPPORT)
    #ifdef LIBSYSWRAPPER_BUILD
    v_secure_system("rm -f %s", staticMtlsDestFile);
    v_secure_system("rm -f %s", dynamicMtlsDestFile);
    #else
    char command[256] = { '\0' };
    snprintf(command, sizeof(command), "rm -f %s", staticMtlsDestFile);
    if(system(command) != 0) {
        T2Error("%s,%d: %s command failed\n", __FUNCTION__, __LINE__, command);
    }
    memset(command, '\0', 256);
    snprintf(command, sizeof(command), "rm -f %s", dynamicMtlsDestFile);
    if(system(command) != 0) {
        T2Error("%s,%d: %s command failed\n", __FUNCTION__, __LINE__, command);
    }
    #endif
#endif
    T2Debug("%s --out\n", __FUNCTION__);

}

/**
 * Retrieves the certs and keys associated to respective end points
 */
T2ERROR getMtlsCerts(char **certName, char **phrase) {

    T2ERROR ret = T2ERROR_FAILURE;

    T2Debug("%s ++in\n", __FUNCTION__);
    char buf[124];
    memset(buf, 0, sizeof(buf));
#if !defined (ENABLE_RDKC_SUPPORT)
    FILE *filePointer;
    if( certName == NULL || phrase == NULL ){
        T2Error("Input args are NULL \n");
        T2Debug("%s --out\n", __FUNCTION__);
        return ret;
    }

    if(access(dynamicMtlsCert, F_OK) != -1) { // Dynamic cert
#else
    if(getenv("XPKI") != NULL) {
      dynamicMtlsCert = getenv("XPKI_CERT");
      if (dynamicMtlsCert != NULL) { // Dynamic cert
#endif
        *certName = strdup(dynamicMtlsCert);
        T2Info("Using xpki Dynamic Certs connection certname: %s\n", dynamicMtlsCert);
#if !defined (ENABLE_RDKC_SUPPORT)
        FILE *fp;
	#ifdef LIBSYSWRAPPER_BUILD
        fp = v_secure_popen("r", "/usr/bin/rdkssacli \"{STOR=GET,SRC=kquhqtoczcbx,DST=/dev/stdout}\"");
        #else
        fp = popen("/usr/bin/rdkssacli \"{STOR=GET,SRC=kquhqtoczcbx,DST=/dev/stdout}\"", "r");
        #endif
        if(fp == NULL) {
            T2Error("v_secure_popen failed\n");
        }else {
            if(fgets(buf, sizeof(buf) - 1, fp) == NULL) {
                T2Error("v_secure_popen read error\n");
            }else {
                buf[strcspn(buf, "\n")] = '\0';
                filePointer = fopen(dynamicMtlsDestFile, "w");
                if(filePointer) {
                    fprintf(filePointer, "%s", buf);
                    fclose(filePointer);
                }
                *phrase = strdup(buf);
            }
	    #ifdef LIBSYSWRAPPER_BUILD
            v_secure_pclose(fp);
            #else
            pclose(fp);
            #endif
        }
#else
        dynamicPassPhrase = getenv("XPKI_PASS");
        if (dynamicPassPhrase != NULL)
          *phrase = strdup(dynamicPassPhrase);
      }
#endif
        ret = T2ERROR_SUCCESS;
#if !defined (ENABLE_RDKC_SUPPORT)
    }else if(access(staticMtlsCert, F_OK) != -1) { // Static cert
#else
    }else if (getenv("STATICXPKI") != NULL) {
        staticMtlsCert = getenv("STATIC_XPKI_CERT");
        if (staticMtlsCert != NULL) { // Static cert
#endif
        T2Info("Using xpki Static Certs connection certname: %s\n", staticMtlsCert);
        *certName = strdup(staticMtlsCert);

	/* CID: 189984 Time of check time of use (TOCTOU) */
#if !defined (ENABLE_RDKC_SUPPORT)
	filePointer = fopen(staticMtlsDestFile, "r");
        if( !filePointer ) {
            T2Info("%s Destination file %s is not present. Generate now.\n", __FUNCTION__, *phrase);
            #ifdef LIBSYSWRAPPER_BUILD
            v_secure_system("/usr/bin/GetConfigFile %s", staticMtlsDestFile);
            #else
            char command[256] = { '\0' };
            snprintf(command, sizeof(command), "/usr/bin/GetConfigFile %s", staticMtlsDestFile);
            if(system(command) != 0) {
                T2Error("%s,%d: %s command failed\n", __FUNCTION__, __LINE__, command);
            }
            #endif
            filePointer = fopen(staticMtlsDestFile, "r");
        }

        if(!filePointer) {
	   T2Error("Certs %s not found\n", *certName);
	}
	else {
           if(NULL != fgets(buf, sizeof(buf) - 1, filePointer)) {
              buf[strcspn(buf, "\n")] = '\0';
              *phrase = strdup(buf);
           }
           fclose(filePointer);
        }
#else
          staticPassPhrase = getenv("STATIC_XPKI_PASS");
          if(staticPassPhrase != NULL)
            *phrase = strdup(staticPassPhrase);
        }
#endif
        ret = T2ERROR_SUCCESS;
    }else {
        T2Error("Certs not found\n");
    }

    T2Debug("Using Cert = %s Pass = %s \n", *certName, *phrase);

    T2Debug("%s --out\n", __FUNCTION__);
    return ret;
}

