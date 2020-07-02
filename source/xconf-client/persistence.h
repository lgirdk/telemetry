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

#ifndef _PERSISTENCE_H_
#define _PERSISTENCE_H_

#include "telemetry2_0.h"
#include "vector.h"

#define XCONFPROFILE_PERSISTENCE_PATH "/nvram/.t2persistentfolder/"
#define REPORTPROFILES_PERSISTENCE_PATH "/nvram/.t2reportprofiles/"
#define MSGPACK_REPORTPROFILES_PERSISTENT_FILE "profiles.msgpack"
#define REPORTPROFILES_FILE_PATH_SIZE 256

typedef struct _Config
{
    char* name;
    char* configData;
}Config;

T2ERROR fetchLocalConfigs(const char* path, Vector *configList);

T2ERROR saveConfigToFile(const char* path, const char *profileName, const char* configuration);

void clearPersistenceFolder(const char* path);

void removeProfileFromDisk(const char* path, const char* profileName);

#endif /* _PERSISTENCE_H_ */
