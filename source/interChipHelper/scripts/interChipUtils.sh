#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#

############ Start include property files ############
. /etc/include.properties
. /etc/device.properties

if [ -f /etc/telemetry2_0.properties ]; then
    . /etc/telemetry2_0.properties
fi

if [ -f /lib/rdk/utils.sh  ]; then
    . /lib/rdk/utils.sh
fi

if [ -f /etc/mount-utils/getConfigFile.sh ]; then
    . /etc/mount-utils/getConfigFile.sh
fi

if [ -f /etc/logFiles.properties ]; then
    . /etc/logFiles.properties
fi
############ End include property files ############




############ Start global variables ############

TELEMETRY_INOTIFY_FOLDER=/tmp/t2events
TELEMETRY_INOTIFY_EVENT="$TELEMETRY_INOTIFY_FOLDER/eventType.cmd"
TELEMETRY_PATH="$PERSISTENT_PATH/.telemetry"
TELEMETRY_PATH_TEMP="$TELEMETRY_PATH/tmp"

TELEMETRY_EXEC_COMPLETE="/tmp/.dca_done"

RTL_LOG_FILE="$LOG_PATH/dcmProcessing.log"
RTL_DELTA_LOG_FILE="$RAMDISK_PATH/.rtl_temp.log"
MAP_PATTERN_CONF_FILE="$TELEMETRY_PATH/dcafile.conf"
TEMP_PATTERN_CONF_FILE="$TELEMETRY_PATH/temp_dcafile.conf"


# Should match with interChipHelper.h
TELEMTERY_LOG_GREP_CONF="/tmp/dca_sorted_file.conf"
TELEMTERY_LOG_GREP_RESULT="/tmp/dcaGrepResult.txt"
TELEMETRY_EVENT_CACHE_FILE="/tmp/t2_caching_file"
TELEMETRY_EVENT_CACHE_ATOM_FILE="/tmp/t2_atom_caching_file"

NOTIFY_EVENT_DCA_RESULT="dcaResult"

# SSH/SCP related variables
SCP_COMPLETE="/tmp/.scp_done"
PEER_COMM_ID="/tmp/elxrretyt-dca.swr"
IDLE_TIMEOUT=30

MAX_SCP_RETRIES=3
TMP_SCP_PATH="/tmp/scp_logs"

MAX_SSH_RETRY=3  

TELEMETRY_ER_READY="/tmp/.t2ReadyToReceiveEvents"
TELEMETRY_GREP_PROFILE_NAME="/tmp/t2ProfileName"

############ End global variables ############


############ Start of functions ############

echo_t()
{
        echo "`date +"%y%m%d-%T.%6N"` $1"
}

## Recovery from daemon execution doesn't make sense as this is no longer a cron execution from atom and is purely based on events from atom
dropbearRecovery()
{
   dropbearPid=`ps | grep -i dropbear | grep "$ATOM_INTERFACE_IP" | grep -v grep`
   if [ -z "$dropbearPid" ]; then
       DROPBEAR_PARAMS_1="/tmp/.dropbear/dropcfg1_t2"
       DROPBEAR_PARAMS_2="/tmp/.dropbear/dropcfg2_t2"
       if [ ! -d '/tmp/.dropbear' ]; then
          echo "wan_ssh.sh: need to create dropbear dir !!! " >> $RTL_LOG_FILE
          mkdir -p /tmp/.dropbear
       fi
       echo "wan_ssh.sh: need to create dropbear files !!! " >> $RTL_LOG_FILE
       if [ ! -f $DROPBEAR_PARAMS_1 ]; then
          getConfigFile $DROPBEAR_PARAMS_1
       fi
       if [ ! -f $DROPBEAR_PARAMS_2 ]; then
          getConfigFile $DROPBEAR_PARAMS_2
       fi
       dropbear -r $DROPBEAR_PARAMS_1 -r $DROPBEAR_PARAMS_2 -E -s -p $ATOM_INTERFACE_IP:22 &
       sleep 2
   fi
}

clearTelemetrySeekValues()
{
    # Clear markers with XCONF update as logs will be flushed in case of maintenance window case as well. 
    # During boot-up no need of maintaining old markers.
    if [ -d $TELEMETRY_PATH_TEMP ]; then
        rm -rf $TELEMETRY_PATH_TEMP
    fi
    mkdir -p $TELEMETRY_PATH_TEMP
}

sshCmdOnAtom() {

    command=$1
    if [ ! -f $PEER_COMM_ID ]; then
        GetConfigFile $PEER_COMM_ID
    fi
    count=0
    isCmdExecFail="true"
    while [ $count -lt $MAX_SSH_RETRY ]
    do
        ssh -I $IDLE_TIMEOUT -i $PEER_COMM_ID root@$ATOM_INTERFACE_IP "rm -f $TELEMETRY_INOTIFY_FOLDER/* ; touch $TELEMETRY_INOTIFY_FOLDER/$command"  > /dev/null 2>&1
        ret=$?
        sleep 1
        if [ $ret -ne 0 ]; then
            echo_t "$count : SSH failure to interchip for $command" >> $T2_0_LOGFILE
            sleep 10
        else
            count=$MAX_SSH_RETRY
            isCmdExecFail="false"
        fi
        count=$((count + 1))
    done

    if [ "x$isCmdExecFail" == "xtrue" ]; then
        echo_t "Failed to exec command $command on atom . Enable default option in interchip for recovery ... " >> $T2_0_LOGFILE
        echo 'dcaResult' > $TELEMETRY_INOTIFY_EVENT
    fi

}

sshCmdOnArm(){

    command=$1
    if [ ! -f $PEER_COMM_ID ]; then
        GetConfigFile $PEER_COMM_ID
    fi
    count=0
    isCmdExecFail="true"
    while [ $count -lt $MAX_SSH_RETRY ]
    do
        ssh -I $IDLE_TIMEOUT -i $PEER_COMM_ID root@$ARM_INTERFACE_IP "rm -f $TELEMETRY_INOTIFY_FOLDER/* ; touch $TELEMETRY_INOTIFY_FOLDER/$command"  > /dev/null 2>&1
        ret=$?
        sleep 1
        if [ $ret -ne 0 ]; then
            echo_t "$count : SSH command execution failure to interchip for $command" >> $T2_0_ATOM_LOGFILE
            sleep 10
        else
            count=$MAX_SSH_RETRY
            isCmdExecFail="false"
        fi
        count=$((count + 1))
    done

    if [ "x$isCmdExecFail" == "xtrue" ]; then
        echo_t "Failed to exec command $command on arm with $MAX_SSH_RETRY retries" >> $T2_0_ATOM_LOGFILE
    fi

}

startInterChipDaemon() {
    if [ ! -f $PEER_COMM_ID ]; then
        GetConfigFile $PEER_COMM_ID
    fi
    count=0
    isInterChipFail="true"
    echo "Notified atom for starting the daemon" >> $T2_0_LOGFILE
    while [ $count -lt $MAX_SSH_RETRY ]
    do
        echo "$count : Starting ${T2_0_BIN} on interchip" >> $T2_0_LOGFILE
        ssh -I $IDLE_TIMEOUT -i $PEER_COMM_ID root@$ATOM_INTERFACE_IP "/lib/rdk/interChipUtils.sh 'runT2Daemon' "  > /dev/null 2>&1
        sleep 1
        t2AtomPid=`ssh -I $IDLE_TIMEOUT -i $PEER_COMM_ID root@$ATOM_INTERFACE_IP "pidof ${T2_0_APP}"`
        if [ -z "$t2AtomPid" ]; then
           echo "$count : Failed to start ${T2_0_APP} on interchip" >> $T2_0_LOGFILE
           sleep 10
        else 
            count=$MAX_SSH_RETRY
            isInterChipFail="false"
        fi               
        count=$((count + 1))
    done
    if [ "x$isInterChipFail" == "xtrue" ]; then 
        echo "Failed to start ${T2_0_APP} on atom . Terminate telemetry 2.0 execution for self heal recovery ... " >> $T2_0_LOGFILE
        echo 'daemonFailed' > $TELEMETRY_INOTIFY_EVENT
    else
        echo "${T2_0_BIN} on interchip started successfully" >> $T2_0_LOGFILE
    fi 

}

stopInterChipDaemon() {
    sshCmdOnAtom 't2StopRunning'
    echo "Notified atom for stopping the daemon execution" >> $T2_0_LOGFILE
}

copyProfileToAtom() {
    if [ -f $TELEMTERY_LOG_GREP_CONF ]; then
        if [ ! -f $PEER_COMM_ID ]; then
            GetConfigFile $PEER_COMM_ID
        fi
        scp -i $PEER_COMM_ID -r $TELEMTERY_LOG_GREP_CONF $ATOM_INTERFACE_IP:$TELEMTERY_LOG_GREP_CONF > /dev/null 2>&1
        echo "Copied profile file $TELEMTERY_LOG_GREP_CONF to $ATOM_INTERFACE_IP" >> $T2_0_LOGFILE
        sleep 1
        sshCmdOnAtom 'loadNewProfile'
    else
        echo "Unable to locate config file $TELEMTERY_LOG_GREP_CONF" >> $T2_0_LOGFILE
    fi
}

notifyGetGrepResultFromAtom() {
    if [ -f $TELEMETRY_GREP_PROFILE_NAME ]; then
        echo "Notify atom for legacy dca results" >> $T2_0_LOGFILE
        if [ ! -f $PEER_COMM_ID ]; then
            GetConfigFile $PEER_COMM_ID
        fi
        scp -i $PEER_COMM_ID $TELEMETRY_GREP_PROFILE_NAME $ATOM_INTERFACE_IP:$TELEMETRY_GREP_PROFILE_NAME > /dev/null 2>&1
        sleep 1
        sshCmdOnAtom 't2Scheduler'
    else
        echo "Unable to locate profile name for grep result" >> $T2_0_LOGFILE
    fi
}

notifyProfileRemoveToAtom() {
    if [ -f $TELEMETRY_GREP_PROFILE_NAME ]; then
        echo "Notify atom to remove a profile" >> $T2_0_LOGFILE
        if [ ! -f $PEER_COMM_ID ]; then
            GetConfigFile $PEER_COMM_ID
        fi
        scp -i $PEER_COMM_ID $TELEMETRY_GREP_PROFILE_NAME $ATOM_INTERFACE_IP:$TELEMETRY_GREP_PROFILE_NAME > /dev/null 2>&1
        sleep 1
        sshCmdOnAtom 't2Remove'
    else
        echo "Unable to locate profile name to be removed" >> $T2_0_LOGFILE
    fi
}

startT2DaemonOnAtom() {
    rm -rf $TELEMETRY_PATH_TEMP
    rm -rf $TELEMETRY_INOTIFY_EVENT
    mkdir -p $TELEMETRY_PATH_TEMP
    t2Pid=`pidof ${T2_0_APP}`
    if [ ! -z "$t2Pid" ]; then
        echo "`date +"%y%m%d-%T.%6N"` telemetry2_0 daemon is already running. Clearing previous obsolete instances ." >> $T2_0_ATOM_LOGFILE
        killall -9 ${T2_0_APP}
    fi
        
    echo "Telemetry is now operating in version 2.0 mode." >> $RTL_LOG_FILE
    telemetryNotifyPid=`ps -ww | grep -i inotify-minidump-watcher | grep 'telemetry' | cut -d ' ' -f2 | tr -s ' '`
    if [ ! -z "$telemetryNotifyPid" ]; then
        echo "Delegating the responsibility to telemetry2_0 daemon and shutdown legacy dca supporting utils" >> $RTL_LOG_FILE
        kill -9 $telemetryNotifyPid
    fi 
    echo "`date +"%y%m%d-%T.%6N"` Starting telemetry2_0 daemon" >> $T2_0_ATOM_LOGFILE
    ${T2_0_BIN}
}

notifyERReadyToAtom() {
    if [ -f $TELEMETRY_ER_READY ]; then
        if [ ! -f $PEER_COMM_ID ]; then
            GetConfigFile $PEER_COMM_ID
        fi
        scp -i $PEER_COMM_ID -r $TELEMETRY_ER_READY $ATOM_INTERFACE_IP:$TELEMETRY_ER_READY
        echo "Notified ATOM about Event receiver ready" >> $T2_0_LOGFILE
    else
        #BUG: we shouldn't be here
        echo "Error: $TELEMETRY_ER_READY not available" >> $T2_0_LOGFILE
    fi
}

copyLogsFromArm() {
    
    mkdir -p $LOG_PATH
    mkdir -p $TMP_SCP_PATH
    mkdir -p $TELEMETRY_PATH_TEMP
    if [ ! -f $PEER_COMM_ID ]; then
        GetConfigFile $PEER_COMM_ID
    fi
    scp -i $PEER_COMM_ID -r $ARM_INTERFACE_IP:$LOG_PATH/* $TMP_SCP_PATH/ > /dev/null 2>&1
    scp -i $PEER_COMM_ID -r $ARM_INTERFACE_IP:$LOG_SYNC_PATH/$SelfHealBootUpLogFile  $ARM_INTERFACE_IP:$LOG_SYNC_PATH/$PcdLogFile  $TMP_SCP_PATH/ > /dev/null 2>&1

    rpcRes=`rpcclient $ARM_ARPING_IP "touch $SCP_COMPLETE"`
    rpcOk=`echo $rpcRes | grep "RPC CONNECTED"`
    if [ "$rpcOk" == "" ]; then
        echo_t "RPC touch failed : attemp 1" >> $RTL_LOG_FILE
        rpcRes=`rpcclient $ARM_ARPING_IP "touch $SCP_COMPLETE"`
    rpcOk=`echo $rpcRes | grep "RPC CONNECTED"`
        if [ "$rpcOk" == "" ]; then
            echo_t "RPC touch failed : attemp 2" >> $RTL_LOG_FILE
        fi
    fi

    # Exclude atom files which are copied from ARM which will be obsolete ones copied from atom for logupload previously
    ATOM_FILE_LIST=`echo ${ATOM_FILE_LIST} | sed -e "s/{//g" -e "s/}//g" -e "s/,/ /g"`
    # File list is lengthy so have to loop and remove files
    if [ -d $TMP_SCP_PATH ]; then
        for file in $ATOM_FILE_LIST
        do
            rm -f $TMP_SCP_PATH/$file
        done
    fi

    if [ -d $TMP_SCP_PATH ]; then
        cp -r $TMP_SCP_PATH/* $LOG_PATH/
        rm -rf $TMP_SCP_PATH
    fi
    sleep 1
}

copyProfileFromArm() {
    if [ ! -f $PEER_COMM_ID ]; then
    GetConfigFile $PEER_COMM_ID
    fi
    scp -i $PEER_COMM_ID -r $ARM_INTERFACE_IP:$TELEMTERY_LOG_GREP_CONF $TELEMTERY_LOG_GREP_CONF
}

notifyDaemonStarted(){
    
    mkdir -p $TELEMETRY_PATH_TEMP
    sshCmdOnArm 'daemonStarted'
}

notifyDaemonFailed(){
    
    mkdir -p $TELEMETRY_PATH_TEMP
    sshCmdOnArm 'daemonFailed'
}

copyJsonResultToArm(){
    if [ ! -f $PEER_COMM_ID ]; then
        GetConfigFile $PEER_COMM_ID
    fi
    scp -i $PEER_COMM_ID $TELEMTERY_LOG_GREP_RESULT root@$ARM_INTERFACE_IP:$TELEMTERY_LOG_GREP_RESULT > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        scp -i $PEER_COMM_ID $TELEMTERY_LOG_GREP_RESULT root@$ARM_INTERFACE_IP:$TELEMTERY_LOG_GREP_RESULT > /dev/null 2>&1
    fi
    sshCmdOnArm 'dcaResult'
}

copyT2CacheFileToArm(){
    if [ ! -f $PEER_COMM_ID ]; then
        GetConfigFile $PEER_COMM_ID
    fi
    scp -i $PEER_COMM_ID root@$ATOM_INTERFACE_IP:$TELEMETRY_EVENT_CACHE_FILE $TELEMETRY_EVENT_CACHE_ATOM_FILE > /dev/null 2>&1
    sshCmdOnAtom 't2DeleteCacheFile'
}

############ End functions ############


######  Start of main execution #####

# Sanity checks for interchip copy supporting utils 
if [ "x$DCA_MULTI_CORE_SUPPORTED" = "xyes" ]; then
    if [ ! -f /usr/bin/GetConfigFile ];then
        echo "Error: GetConfigFile Not Found"
        exit 127
    fi
fi

## Consider forced exec because of logupload, so may not have to flush out seek values in case of unknown reboots. 
## Flush seek values only if its a log upload case
if [ ! -f /tmp/.dca_bootup ]; then
    rm -f $RTL_LOG_FILE
    echo "First dca execution after bootup." >> $T2_0_LOGFILE
    touch /tmp/.dca_bootup
fi

eventType=$1
echo "telemetry2_0 event is $eventType ..." >> $RTL_LOG_FILE

case "$eventType" in
    *notifyDaemonStart* )
        # Will run on ARM to create inotify event on ATOM for starting t2 daemon
        startInterChipDaemon
        ;;
    *notifyConfigUpdate* )
        # Will run on ARM to copy config to ATOM and create inotify event on ATOM
        copyProfileToAtom
        ;;
    *notifyGetGrepResult* )
        # Will run on ARM to notify ATOM for scheduled reporting
        notifyGetGrepResultFromAtom
        ;;
    *notifyProfileRemove* )
        # Will run on ARM to notify ATOM about deletion of profile
        notifyProfileRemoveToAtom
        ;;
    *notifyStopRunning* )
        # Will run on ARM to notify ATOM for scheduled reporting
        stopInterChipDaemon
        ;;
    *copyJsonResultToArm* )
        # Will run on ATOM to notify ARM for available grep results
        copyJsonResultToArm
        ;;
    *daemonStarted* )
        # Will run on ATOM to notify ARM for available grep results
        notifyDaemonStarted
        ;;
    *daemonFailed* )
        # Will run on ATOM to notify ARM for available grep results
        notifyDaemonFailed
        ;;
    *copyLogs* )
        # Will run on ATOM
        copyLogsFromArm
        ;;
    *runT2Daemon* )
        startT2DaemonOnAtom
        ;;
    *clearSeekValues* )
        # Will run on ATOM
        clearTelemetrySeekValues
        ;;
    *copyProfileFromArm* )
        # Will run on ATOM
        copyProfileFromArm
        ;;
    *clearInotifyDir* )
        rm -rf $TELEMETRY_INOTIFY_FOLDER/*
        mkdir -p $TELEMETRY_INOTIFY_FOLDER
        ;;
    *notifyEventReceiverReady* )
        # will run on ARM to notify ATOM when event receiver is ready
        notifyERReadyToAtom
        ;;
    *copyT2CacheFileToArm* )
        # Will run on ATOM to copy cached file to ARM
        copyT2CacheFileToArm 
        ;;

esac

######  End of main execution #####

