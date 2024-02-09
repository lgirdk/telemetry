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
#include <unistd.h>

#include "t2log_wrapper.h"
#include "t2commonutils.h"

int getcurrenttime (char *current_time_string, int timestampparams)
{
    time_t current_time;
    struct tm *c_time_string;

    /* Obtain current time */
    current_time = time(NULL);
    if (current_time == ((time_t)-1))
    {
        T2Error("Failed to obtain the current time\n");
        current_time_string = NULL;
        return 1;
    }

    /* Convert to local time format. */
    c_time_string = localtime(&current_time);
    if (c_time_string == NULL)
    {
        T2Error("Failure to obtain the current time\n");
        current_time_string = NULL;
        return 1;
    }

    strftime(current_time_string, timestampparams, "%Y-%m-%d %H:%M:%S", c_time_string);

    return 0;
}

int telemetry_syscfg_get (char *temp, char *buf, int buf_size)
{
    FILE *value = NULL;
    char cmd[256];
    int ret = -1;

    snprintf(cmd, sizeof(cmd), "syscfg get %s", temp);

    value = popen(cmd, "r");
    if (value)
    {
        fgets(buf, buf_size, value);
        buf[strlen(buf) - 1] = '\0';
        pclose(value);
        ret = 0;
    }

    return ret;
}

int telemetry_syscfg_set (char *temp, char *buf)
{
    FILE *value = NULL;
    char cmd[256];
    int ret = -1;

    snprintf(cmd, sizeof(cmd), "syscfg set %s \"%s\"", temp, buf);

    value = popen(cmd, "w");
    if (value)
    {
        pclose(value);
        ret = 0;
    }

    return ret;
}

void _get_shell_output (char *cmd, char *buf, size_t len)
{
    FILE *fp;

    if (len > 0)
        buf[0] = 0;
    fp = popen (cmd, "r");
    if (fp == NULL)
        return;
    buf = fgets (buf, len, fp);
    pclose (fp);
    if ((len > 0) && (buf != NULL)) {
        len = strlen (buf);
        if ((len > 0) && (buf[len - 1] == '\n'))
            buf[len - 1] = 0;
    }
}
