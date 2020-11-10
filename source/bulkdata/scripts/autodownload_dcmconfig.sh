#!/bin/sh
##########################################################################
#
# Copyright 2020 Liberty Global B.V.
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

T2_0_BIN="telemetry2_0"

#TO DO: Once we ported enable dml, uncomment the following

# Download DCMResponse.txt file only if telemetry is enabled.
# telemetryStatus=`syscfg get telemetry_enable`
# if [ "$telemetryStatus" != "true" ]; then
#   exit 0
# fi

T2_0_BIN_PID=`pidof $T2_0_BIN`

if [ -n "${T2_0_BIN_PID}" ]; then
    # Singal 12 (SIGUSR2) is handled in telemetry2_0 process to download DCMResponse.txt file
    kill -12 $T2_0_BIN_PID
else
    echo "Not able to get $T2_0_BIN pid!!!"
fi
