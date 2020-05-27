/*
 * persistence.h
 *
 *  Created on: Sep 25, 2019
 *      Author: mvalas827
 */

#ifndef _PERSISTENCE_H_
#define _PERSISTENCE_H_

#include "telemetry2_0.h"
#include "vector.h"

#define XCONFPROFILE_PERSISTENCE_PATH "/nvram/.t2persistentfolder/"
#define REPORTPROFILES_PERSISTENCE_PATH "/nvram/.t2reportprofiles/"

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
