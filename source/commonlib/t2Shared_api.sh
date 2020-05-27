#!/bin/sh
#masking dmcli usage when rbus service is not running
T2_MSG_CLIENT=/usr/bin/telemetry2_0_client
if [ -e /nvram/rbus ]; then
    rbus_alive=`ps | grep /usr/bin/rtrouted | grep -vc grep`
    if [ "$rbus_alive" -eq "1" ];then
        IS_T2_ENABLED=`dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable | grep value | awk '{print $5}'`
    else
        IS_T2_ENABLED=false
    fi
else
    IS_T2_ENABLED=`dmcli eRT getv Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable | grep value | awk '{print $5}'`
fi

t2CountNotify() {
    if [ "$IS_T2_ENABLED" == "true" ]; then
        marker=$1
        $T2_MSG_CLIENT  "$marker" "1"
    fi
}

t2ValNotify() {
    if [ "$IS_T2_ENABLED" == "true" ]; then
        marker=$1
        shift
        $T2_MSG_CLIENT "$marker" "$*"
    fi    
}
