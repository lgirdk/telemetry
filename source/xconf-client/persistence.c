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

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "persistence.h"
#include "t2log_wrapper.h"
#include "telemetry2_0.h"

#define MAX_FILENAME_LENGTH 128

T2ERROR fetchLocalConfigs(const char* path, Vector *configList)
{
    struct dirent *entry;
    T2Debug("%s ++in\n", __FUNCTION__);
    DIR *dir = opendir(path);
    if (dir == NULL) {
        T2Error("Failed to open persistence folder : %s, creating folder\n", path);

        char command[256] = {'\0'};
        snprintf(command, sizeof(command), "mkdir %s", path);
        T2Debug("Executing command : %s\n", command);
        if (system(command) != 0) {
            T2Error("%s,%d: Failed to make directory : %s  \n", __FUNCTION__ , __LINE__, path);
        }

        return T2ERROR_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        struct stat filestat;
        int         status;
        char absfilepath[256] = {'\0'};

        if(entry->d_name[0] == '.')
            continue;

        snprintf(absfilepath,sizeof(absfilepath), "%s%s", path, entry->d_name);
        T2Debug("Config file : %s\n", absfilepath);
        int fp = open(absfilepath, O_RDONLY);
        if(fp == -1)
        {
            T2Error("Failed to open file : %s\n", entry->d_name);
            continue;
        }

        status = fstat(fp, &filestat);
        if(status == 0) {
            T2Info("Filename : %s Size : %ld\n", entry->d_name, (long int)filestat.st_size);

            Config *config = (Config *)malloc(sizeof(Config));
            memset(config, 0 , sizeof(Config));
            config->name = strdup(entry->d_name);
            config->configData = (char *)malloc((filestat.st_size + 1) * sizeof(char));
            memset( config->configData, 0, (filestat.st_size + 1 ));
            int read_size = read(fp, config->configData, filestat.st_size);
            config->configData[filestat.st_size] = '\0';

            if(read_size != filestat.st_size)
                T2Error("read size = %d filestat.st_size = %lu\n", read_size, (unsigned long)filestat.st_size);
            close(fp);
            Vector_PushBack(configList, config);

            T2Debug("Config data size = %lu\n", (unsigned long)strlen(config->configData));
            T2Debug("Config data = %s\n", config->configData);
        }
        else
        {
            T2Error("Unable to stat, Invalid file : %s\n", entry->d_name);
            close(fp);
            continue;
        }
    }

    T2Info("Returning %lu local configurations \n", (unsigned long)Vector_Size(configList));
    closedir(dir);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR saveConfigToFile(const char* path, const char *profileName, const char* configuration)
{
    FILE *fp = NULL;
    char filePath[256] = {'\0'};
    T2Debug("%s ++in\n", __FUNCTION__);

    if(strlen(profileName) > MAX_FILENAME_LENGTH)
    {
        T2Error("profileID exceeds max limit of %d chars\n", MAX_FILENAME_LENGTH);
        return T2ERROR_FAILURE;
    }
    snprintf(filePath, sizeof(filePath), "%s%s", path, profileName);

    fp = fopen(filePath, "w");
    if(fp == NULL)
    {
        T2Error("Unable to write to file : %s\n", filePath);
        return T2ERROR_FAILURE;
    }
    fprintf(fp, "%s", configuration);
    fclose(fp);
    T2Debug("%s --out\n", __FUNCTION__);
    return T2ERROR_SUCCESS;
}

T2ERROR MsgPackSaveConfig(const char* path, const char *fileName, const char *msgpack_blob, size_t blob_size)
{
    FILE *fp;
    char filePath[256] = {'\0'};

    if(strlen(fileName) > MAX_FILENAME_LENGTH)
    {
        T2Error("fileName exceeds max limit of %d chars\n", MAX_FILENAME_LENGTH);
        return T2ERROR_FAILURE;
    }
    snprintf(filePath, sizeof(filePath), "%s%s", path, fileName);
    fp = fopen(filePath, "wb");
    if (NULL == fp) {
        T2Error("%s file open is failed \n", filePath);
	return T2ERROR_FAILURE;
    }
    fwrite(msgpack_blob, sizeof(char), blob_size, fp);
    fclose(fp); 
    return T2ERROR_SUCCESS;
}

void clearPersistenceFolder(const char* path)
{
    char command[256] = {'\0'};
    T2Debug("%s ++in\n", __FUNCTION__);

    snprintf(command, sizeof(command), "rm -f %s*", path);
    T2Debug("Executing command : %s\n", command);
    if (system(command) != 0) {
        T2Error("%s,%d: %s command failed\n", __FUNCTION__ , __LINE__, command);
    }

    T2Debug("%s --out\n", __FUNCTION__);
}

void removeProfileFromDisk(const char* path, const char* fileName)
{
    char command[256] = {'\0'};
    T2Debug("%s ++in\n", __FUNCTION__);

    snprintf(command, sizeof(command), "rm -f %s%s", path, fileName);
    T2Debug("Executing command : %s\n", command);
    if (system(command) != 0) {
        T2Error("%s,%d: %s command failed\n", __FUNCTION__ , __LINE__, command);
    }


    T2Debug("%s --out\n", __FUNCTION__);
}
