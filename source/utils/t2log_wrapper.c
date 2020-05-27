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


#include <stdbool.h>

#include "t2log_wrapper.h"

unsigned int rdkLogLevel = RDK_LOG_INFO;

void LOGInit()
{
     rdk_logger_init(DEBUG_INI_NAME);
     if (access( ENABLE_DEBUG_FLAG, F_OK) != -1)
          rdkLogLevel = RDK_LOG_DEBUG;
}

void T2Log(unsigned int level, const char *msg, ...)
{
    va_list arg;
    char *pTempChar = NULL;
    int ret = 0;

    if( level <=  rdkLogLevel)
    {
        pTempChar = (char *)malloc(4096);
        if(pTempChar)
        {
            va_start(arg, msg);
            ret = vsnprintf(pTempChar, 4096, msg,arg);
            if(ret < 0)
            {
                perror(pTempChar);
            }
            va_end(arg);

            RDK_LOG(level, "LOG.RDK.T2", "%s", pTempChar);


            if(pTempChar !=NULL)
            {
                free(pTempChar);
                pTempChar = NULL;
            }
        }
    }
}
