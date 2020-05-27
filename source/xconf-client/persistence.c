/*
 * persistence.c
 *
 *  Created on: Sep 25, 2019
 *      Author: mvalas827
 */

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

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
        sprintf(command, "mkdir %s", path);
        T2Debug("Executing command : %s\n", command);
        system(command);

        return T2ERROR_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        struct stat filestat;
        int         status;
        char absfilepath[256] = {'\0'};

        if(entry->d_name[0] == '.')
            continue;

        sprintf(absfilepath, "%s%s", path, entry->d_name);
        T2Debug("Config file : %s\n", absfilepath);
        status = stat(absfilepath, &filestat);
        if(status == 0) {
            T2Info("Filename : %s Size : %ld\n", entry->d_name, filestat.st_size);

            FILE *fp = fopen(absfilepath, "r");
            if(fp == NULL)
            {
                T2Error("Failed to open file : %s\n", entry->d_name);
                continue;
            }

            Config *config = (Config *)malloc(sizeof(Config));
            config->name = strdup(entry->d_name);
            config->configData = (char *)malloc((filestat.st_size + 1) * sizeof(char));
            int read_size = fread(config->configData, sizeof(char), filestat.st_size, fp);
            config->configData[filestat.st_size] = '\0';

            if(read_size != filestat.st_size)
                T2Error("read size = %d filestat.st_size = %d\n", read_size, filestat.st_size);
            fclose(fp);
            Vector_PushBack(configList, config);

            T2Debug("Config data size = %d\n", strlen(config->configData));
            T2Debug("Config data = %s\n", config->configData);
        }
        else
        {
            T2Error("Unable to stat, Invalid file : %s\n", entry->d_name);
            continue;
        }
    }
    T2Info("Returning %d local configurations \n", Vector_Size(configList));

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
    sprintf(filePath, "%s%s", path, profileName);

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

void clearPersistenceFolder(const char* path)
{
    char command[256] = {'\0'};
    T2Debug("%s ++in\n", __FUNCTION__);

    sprintf(command, "rm -f %s*", path);
    T2Debug("Executing command : %s\n", command);
    system(command);

    T2Debug("%s --out\n", __FUNCTION__);
}

void removeProfileFromDisk(const char* path, const char* fileName)
{
    char command[256] = {'\0'};
    T2Debug("%s ++in\n", __FUNCTION__);

    sprintf(command, "rm -f %s%s", path, fileName);
    T2Debug("Executing command : %s\n", command);
    system(command);

    T2Debug("%s --out\n", __FUNCTION__);
}
