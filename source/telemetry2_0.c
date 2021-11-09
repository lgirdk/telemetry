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
#ifdef __GNUC__
#ifndef _BUILD_ANDROID
#include <execinfo.h>
#endif
#endif

#include "telemetry2_0.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <curl/curl.h>
#include <execinfo.h>
#include <errno.h>
#include <sys/file.h>
#include <fcntl.h>
#if defined(DUAL_CORE_XB3) || defined (_COSA_INTEL_USG_ATOM_)
#include <sys/inotify.h>
#endif
#include "t2log_wrapper.h"
#include "syslog.h"
#include "reportprofiles.h"
#include "xconfclient.h"
#if defined(DUAL_CORE_XB3) || defined (_COSA_INTEL_USG_ATOM_)
#include "interChipHelper.h"
#endif
#include "t2eventreceiver.h"

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif
#include "t2commonutils.h"

#define MAX_PARAMETERNAME_LEN    512
/*Define signals properly to make sure they don't get overide anywhere*/
#define LOG_UPLOAD 10
#define EXEC_RELOAD 12


static bool isDebugEnabled = true;


T2ERROR initTelemetry()
{
    T2ERROR ret = T2ERROR_FAILURE;
    T2Debug("%s ++in\n",__FUNCTION__);

    if(T2ERROR_SUCCESS == initReportProfiles())
    {
        if(T2ERROR_SUCCESS == initXConfClient())
        {
            ret = T2ERROR_SUCCESS;
            T2Debug("%s --out\n", __FUNCTION__);
        }
        else
            T2Error("Failed to initializeXConfClient\n");
    }
    else
        T2Error("Failed to initialize ReportProfiles\n");


    T2Debug("%s --out\n",__FUNCTION__);
    return ret;
}


static void terminate() {
    uninitXConfClient();
    ReportProfiles_uninit();
    curl_global_cleanup();
    if(0 != remove("/tmp/.t2ReadyToReceiveEvents")){
        T2Info("%s Unable to remove ready to receive event flag \n", __FUNCTION__);
    }
    if(0 != remove("/tmp/telemetry_initialized_bootup")){
        T2Info("%s Unable to remove initialization complete flag \n", __FUNCTION__);
    }
    if(0 != remove(T2_CONFIG_READY)){
        T2Info("%s Unable to remove config initialization complete flag \n", __FUNCTION__);
    }
    rdk_logger_deinit();
}

static void _print_stack_backtrace(void)
{
#ifdef __GNUC__
#ifndef _BUILD_ANDROID
    void* tracePtrs[100];
    char** funcNames = NULL;
    int i, count = 0;

    count = backtrace( tracePtrs, 100 );
    backtrace_symbols_fd( tracePtrs, count, 2 );

    funcNames = backtrace_symbols( tracePtrs, count );

    if ( funcNames ) {
            // Print the stack trace
        for( i = 0; i < count; i++ )
        printf("%s\n", funcNames[i] );

            // Free the string pointers
            free( funcNames );
    }
#endif
#endif
}

void sig_handler(int sig)
{

    if ( sig == SIGINT ) {
        signal(SIGINT, sig_handler); /* reset it to this function */
        T2Info(("SIGINT received!\n"));
        uninitXConfClient();
        ReportProfiles_uninit();
        exit(0);
    }
    else if ( sig == SIGUSR1 || sig == LOG_UPLOAD ) {
        signal(LOG_UPLOAD, sig_handler); /* reset it to this function */
        T2Info(("LOG_UPLOAD received!\n"));
        ReportProfiles_Interrupt();
    }
    else if ( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        T2Info(("SIGCHLD received!\n"));
    }
    else if ( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        T2Info(("SIGPIPE received!\n"));
    }
    else if(sig == SIGUSR2 || sig == EXEC_RELOAD)
    {
        T2Info(("EXEC_RELOAD received!\n"));
        stopXConfClient();
        if(T2ERROR_SUCCESS == startXConfClient()) {
            T2Info("XCONF config reload - SUCCESS \n");
        }else {
            T2Info("XCONF config reload - IN PROGRESS ... Ignore current reload request \n");
        }

    }
    else if ( sig == SIGTERM || sig == SIGKILL )
    {
        T2Info(("SIGTERM received!\n"));
        terminate();
        exit(0);
    }
    else if ( sig == SIGALRM )
    {
        signal(SIGALRM, sig_handler); /* reset it to this function */
        T2Warning("SIGALRM received!\n");

    }
    else {
        /* get stack trace first */
        _print_stack_backtrace();
        T2Warning("Signal %d received, exiting!\n", sig);
        terminate();
        exit(0);
    }
}


#ifndef _COSA_INTEL_USG_ATOM_
static void t2DaemonMainModeInit( ) {

    /**
     * Signal handling is being used as way to handle log uploads . Double check whether we get minidump events for crashes
     */
#ifdef INCLUDE_BREAKPAD
   breakpad_ExceptionHandler();
#endif
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(LOG_UPLOAD, sig_handler);
    signal(EXEC_RELOAD, sig_handler);

    #ifdef _COSA_INTEL_XB3_ARM_
    if ( createNotifyDir() != 0 ) {
        T2Error("Failed to initialize Telemetry inotify directory .. exiting the process\n");
        exit(0);
    }
    // Notify ATOM to start telemetry 2.0 daemon and quit from dca inotify watcher
    if ( interchipDaemonStart() != 0 ) {
    	T2Error("Failed to initialize for inotify events .. exiting the process\n");
    	exit(0);
    }
    #endif

    if(T2ERROR_SUCCESS != initTelemetry()) {
        T2Error("Failed to initialize Telemetry.. exiting the process\n");
        exit(0);
    }


    T2Info("Telemetry 2.0 Component Init Success\n");

    while(1) {
        sleep(30);
    }
    T2Info("Telemetry 2.0 Process Terminated\n");
}
#endif // End of _COSA_INTEL_USG_ATOM_

#ifdef _COSA_INTEL_USG_ATOM_
static void t2DaemonHelperModeInit( ) {

    T2Info("Init inotify event watcher for ATOM  CHIP");
    int notifyfd = -1;
    int watchfd = -1;

    if ( createNotifyDir() != 0 ) {
        T2Error("Failed to initialize Telemetry inotify directory .. exiting the process\n");
        execNotifier("daemonFailed");
        exit(0);
    }

    notifyfd = inotify_init();
    if (notifyfd < 0) {
        T2Error("t2DaemonHelperModeInit : Error on adding inotify_init \n");
        execNotifier("daemonFailed");
        exit(0);
    }
    watchfd = inotify_add_watch(notifyfd, DIRECTORY_TO_MONITOR, IN_CREATE);
    if (watchfd < 0) {
        T2Error("t2DaemonHelperModeInit : Error in adding inotify_add_watch on directory : %s \n", DIRECTORY_TO_MONITOR);
        execNotifier("daemonFailed");
        exit(0);
    }
    // Notify ARM for successful start of daemon on atom .
    execNotifier("daemonStarted");
    T2Info("Successfully added watch on directory %s \n", DIRECTORY_TO_MONITOR);
    while(isHelperEnabled())
    {
        listenForInterProcessorChipEvents(notifyfd, watchfd);
    }
    inotify_rm_watch(notifyfd, watchfd);
    T2Info("Telemetry 2.0 Process Terminated\n");
}

/*
   Note that there are two versions of _get_shell_output() used with RDKB.
   This version, which accepts a char * command as the first argument, is
   the older version. The newer version accepts a FILE pointer as created
   by a call to v_secure_popen().
*/
static void _get_shell_output (char *cmd, char *buf, size_t len)
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

#endif //End of  _COSA_INTEL_USG_ATOM_

static int getBridgeMode (void)
{
    T2Debug("%s ++in\n",__FUNCTION__);
    char buf[8] = {0};
    int isBridgeMode = 0;

#if defined(_COSA_INTEL_USG_ATOM_)
    _get_shell_output ("/usr/bin/rpcclient2 \"syscfg get bridge_mode\" | sed '/RPC/d;/^$/d'", buf, sizeof(buf));
    if (strcmp(buf, "0") != 0)
    {
        isBridgeMode = 1;
    }
#else
    if( 0 == telemetry_syscfg_get("bridge_mode", buf, sizeof( buf )))
    {
        if (strcmp(buf, "0") != 0)
        {
            // in bridge mode
            isBridgeMode = 1;
        }          
    }
    else
    {
        T2Error(("syscfg_get failed in %s\n",__FUNCTION__));
    }
#endif

    return isBridgeMode;
}

static int checkTelemetryStatus (void)
{
    T2Debug("%s ++in\n",__FUNCTION__);

    char buf[6];
    int res = 0;

    if (telemetry_syscfg_get("telemetry_enable", buf, sizeof(buf)) == 0)
    {
        if ((strcmp(buf, "true") == 0) && (getBridgeMode() == 0))
        {
            res = 1;
        }
        else
        {
            T2Info("Device is in Bridge mode.So exiting telemetry.\n");
        }
        T2Info("telemetry_enable value is : %s \n",buf);
    }
    else
    {
        T2Error("syscfg_get failed in %s\n",__FUNCTION__);
    }

    T2Debug("%s --out\n",__FUNCTION__);

    return res;
}

static int checkAnotherTelemetryInstance()
{
    T2Debug("%s ++in\n",__FUNCTION__);

    int res = 0;
    int pid_file = open("/tmp/telemetry2_0.pid", O_CREAT | O_RDWR, 0666);
    if(pid_file != -1)
    {
        int rc = flock(pid_file, LOCK_EX | LOCK_NB);
        if (rc)
        {
            if (EWOULDBLOCK == errno)
            {
                T2Error("Another instance of telemetry2_0 is running. so exiting....\n");
                res = 1;
            }
            else
            {
                T2Error("flock returned an error other than EWOULDBLOCK. Proceeding futher..\n");
            }
        }
        else
        {
            T2Info("First instance of telemetry2_0 process\n");
        }
    }
    else
    {
        T2Error("File open failed in func - %s\n",__FUNCTION__);
    }

    T2Debug("%s --out\n",__FUNCTION__);
    return res;
}

int main(int argc, char* argv[])
{
    pid_t process_id = 0;
    pid_t sid = 0;
    LOGInit();

    /* Check whether telemetry is enabled */
    if(!checkTelemetryStatus())
    {
        T2Info("Telemetry is disabled. Existing!!!\n");
        return 0;
    }

    /* Check another instance of telemetry2_0 is running in background */
    if(checkAnotherTelemetryInstance())
    {
        return 1;
    }

    printf("Starting Telemetry 2.0 Process\n");

    curl_global_init(CURL_GLOBAL_ALL);
    // Create child process
    process_id = fork();
    if (process_id < 0) {
        T2Error("fork failed!\n");
        return 1;
    } else if (process_id > 0) {
        return 0;
    }

    //unmask the file mode
    umask(0);

    //set new session
    sid = setsid();
    if (sid < 0) {
        T2Error("setsid failed!\n");
        return 1;
    }

    // Change the current working directory to root.
    chdir("/");

    if (isDebugEnabled != true) {
        // Close stdin. stdout and stderr
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }


    T2Info("Initializing Telemetry 2.0 Component\n");
    #ifndef _COSA_INTEL_USG_ATOM_   // This doesn't apply for helper daemon on atom
    t2DaemonMainModeInit();
    #endif

    #ifdef _COSA_INTEL_USG_ATOM_
    t2DaemonHelperModeInit();
    #endif

    return 0;
}
