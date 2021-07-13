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

#ifndef _INTERCHIPHELPER_H_
#define _INTERCHIPHELPER_H_

#include <stdbool.h>

#define TELEMTERY_LOG_GREP_CONF "/tmp/dca_sorted_file.conf"
#define TELEMTERY_LOG_GREP_RESULT "/tmp/dcaGrepResult.txt"
#define TELEMETRY_GREP_PROFILE_NAME "/tmp/t2ProfileName"

#define DIRECTORY_TO_MONITOR "/tmp/t2events"
#define INOTIFY_FILE "/tmp/t2events/eventType.cmd"
#define MAX_EVENT_TYPE_BUFFER 50

// Event on ATOM to load/reload new config
#define CONFIG_FETCH_EVENT "loadNewProfile"
#define CONFIG_FETCH_EVENT_LEN 13

// Event on ATOM to generate grep result file for timer schedule
#define T2_TIMER_EVENT "t2Scheduler"
#define T2_TIMER_EVENT_LEN 16

// Event on ATOM to generate grep result file for timer schedule
#define T2_STOP_RUNNING "t2StopRunning"

// Event on ATOM to remove grep config for profile
#define T2_PROFILE_REMOVE_EVENT "t2Remove"

// Event on ARM when dca result file is available for processing
#define T2_CLEAR_SEEK_EVENT "clearSeekValues"
#define T2_CLEAR_SEEK_EVENT_LEN 16

// Event on ARM when dca result file is available for processing
#define T2_DCA_RESULT_EVENT "dcaResult"

// Event on ARM when t2 daemon on atom has successfully initialized
#define T2_DAEMON_START_EVENT "daemonStarted"

// Event on ARM when t2 daemon on atom has failed to initialize
#define T2_DAEMON_FAILED_EVENT "daemonFailed"

// Event on ATOM to remove t2 cache file
#define DEL_T2_CACHE_FILE "t2DeleteCacheFile"

#define NOTIFY_HELPER_UTIL "/lib/rdk/interChipUtils.sh"

#define MIN_NOTIFY_FILE_SIZE 10

#define USE_ABSOLUTE "absolute"
#define USE_COUNTER "count"
#define DELIMITER "<#=#>"
#define MAX_LINE_LEN 1024

int listenForInterProcessorChipEvents (int notifyfd, int watchfd);

/**
 * Notify interchip to start telemetry2_0 daemon start.
 * Will be a blocking call until interchip acknowledges a successful start.
 */
int interchipDaemonStart();

int execNotifier(char *eventType);

bool isHelperEnabled();

int createNotifyDir();

#endif /* _INTERCHIPHELPER_H_ */


