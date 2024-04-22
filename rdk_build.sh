#!/bin/bash
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
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
#######################################
#
# Build Framework standard script for
#
#
# use -e to fail on any shell issue
# -e is the requirement from Build Framework
set -e

# default PATHs - use `man readlink` for more info
# the path to combined build
export RDK_PROJECT_ROOT_PATH=${RDK_PROJECT_ROOT_PATH-`readlink -m ..`}
export COMBINED_ROOT=$RDK_PROJECT_ROOT_PATH

# path to build script (this script)
export RDK_SCRIPTS_PATH=${RDK_SCRIPTS_PATH-`readlink -m $0 | xargs dirname`}
export RDK_TOOLCHAIN_PATH=${RDK_TOOLCHAIN_PATH-`readlink -m $RDK_PROJECT_ROOT_PATH/sdk/toolchain/staging_dir`}

# path to components sources and target
export RDK_SOURCE_PATH=${RDK_SOURCE_PATH-`readlink -m .`}
export RDK_TARGET_PATH=${RDK_TARGET_PATH-$RDK_SOURCE_PATH}

# default component name
export RDK_COMPONENT_NAME=${RDK_COMPONENT_NAME-`basename $RDK_SOURCE_PATH`}
export RDK_DIR=$RDK_PROJECT_ROOT_PATH
export RDK_FSROOT_PATH=$RDK_PROJECT_ROOT_PATH/sdk/fsroot/ramdisk
#source $RDK_SCRIPTS_PATH/soc/build/soc_env.sh

# To enable rtMessage
export RTMESSAGE=yes

export STRIP=${RDK_TOOLCHAIN_PATH}/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-strip

if [ "$XCAM_MODEL" == "SCHC2" ]; then
 echo "Configuring for XCAM2"
 source  ${RDK_PROJECT_ROOT_PATH}/build/components/amba/sdk/setenv2
elif [ "$XCAM_MODEL" == "SERXW3" ] || [ "$XCAM_MODEL" == "SERICAM2" ] || [ "$XCAM_MODEL" == "XHB1" ] || [ "$XCAM_MODEL" == "XHC3" ]; then
 echo "Configuring for Other device Model"
  source ${RDK_PROJECT_ROOT_PATH}/build/components/sdk/setenv2
else #No Matching platform
    echo "Source environment that include packages for your platform. The environment variables PROJ_PRERULE_MAK_FILE should refer to the platform s PreRule make"
fi

# parse arguments
INITIAL_ARGS=$@
function usage()
{
    set +x
    echo "Usage: `basename $0` [-h|--help] [-v|--verbose] [action]"
    echo "    -h    --help                  : this help"
    echo "    -v    --verbose               : verbose output"
    echo "    -p    --platform  =PLATFORM   : specify platform for xw4 common"
    echo
    echo "Supported actions:"
    echo "      configure, clean, build (DEFAULT), rebuild, install"
}
# options may be followed by one colon to indicate they have a required argument
if ! GETOPT=$(getopt -n "build.sh" -o hvp: -l help,verbose,platform: -- "$@")
then
    usage
    exit 1
fi
eval set -- "$GETOPT"
while true; do
  case "$1" in
    -h | --help ) usage; exit 0 ;;
    -v | --verbose ) set -x ;;
    -p | --platform ) CC_PLATFORM="$2" ; shift ;;
    -- ) shift; break;;
    * ) break;;
  esac
  shift
done
ARGS=$@
# component-specific vars
if [ "$XCAM_MODEL" == "SCHC2" ]; then
	export CROSS_COMPILE=$RDK_TOOLCHAIN_PATH/bin/arm-linux-gnueabihf-
  export CC=${CROSS_COMPILE}gcc
  export CXX=${CROSS_COMPILE}g++
  export DEFAULT_HOST=arm-linux
  export PKG_CONFIG_PATH="$RDK_PROJECT_ROOT_PATH/opensource/lib/pkgconfig/:$RDK_FSROOT_PATH/img/fs/shadow_root/usr/local/lib/pkgconfig/:$RDK_TOOLCHAIN_PATH/lib/pkgconfig/:$PKG_CONFIG_PATH"
fi

# functional modules
function configure()
{
    echo "Executing configure common code"
    cd $RDK_SOURCE_PATH
    aclocal
    libtoolize --automake
    automake --foreign --add-missing
    rm -f configure
    autoconf
    configure_options="--host=$DEFAULT_HOST --target=$DEFAULT_HOST --enable-deviceextender"

    if [ $XCAM_MODEL == "SCHC2" ]; then
      export LDFLAGS="-L${RDK_PROJECT_ROOT_PATH}/breakpadwrap -lbreakpadwrap -L${RDK_PROJECT_ROOT_PATH}/opensource/lib -lmsgpackc -L${RDK_PROJECT_ROOT_PATH}/rdklogger/src/.libs -lrdkloggers -Wl,-rpath-link=${RDK_PROJECT_ROOT_PATH}/libexchanger/Release/src -Wl,-rpath-link=${RDK_PROJECT_ROOT_PATH}/libexchanger/password/src -L${RDK_PROJECT_ROOT_PATH}/libsyswrapper/source/.libs/  -lsecure_wrapper  -L${RDK_FSROOT_PATH}/usr/lib -lrbus -lrtMessage -lrbuscore -Wl,-rpath-link=${RDK_PROJECT_ROOT_PATH}/opensource/lib -L${RDK_PROJECT_ROOT_PATH}/webconfig-framework/source/.libs"
    else
      export LDFLAGS="-L${RDK_PROJECT_ROOT_PATH}/breakpadwrap -lbreakpadwrap -L${RDK_PROJECT_ROOT_PATH}/opensource/lib -lmsgpackc -L${RDK_PROJECT_ROOT_PATH}/rdklogger/src/.libs -lrdkloggers -L${RDK_PROJECT_ROOT_PATH}/libsyswrapper/source/.libs/ -lsecure_wrapper  -L${RDK_FSROOT_PATH}/usr/lib -lrbus -lrtMessage -lrbuscore -Wl,-rpath-link=${RDK_PROJECT_ROOT_PATH}/opensource/lib -L${RDK_PROJECT_ROOT_PATH}/webconfig-framework/source/.libs"
    fi
    export CFLAGS="-I${RDK_PROJECT_ROOT_PATH}/opensource/include -fPIC  -I${RDK_PROJECT_ROOT_PATH}/opensource/include/glib-2.0/ -I${RDK_PROJECT_ROOT_PATH}/opensource/lib/glib-2.0/include/ -I${RDK_PROJECT_ROOT_PATH}/opensource/include/glib-2.0/glib -I${RDK_PROJECT_ROOT_PATH}opensource/include/cjson/ -I${RDK_PROJECT_ROOT_PATH}/rdklogger/include -I${RDK_PROJECT_ROOT_PATH}/libsyswrapper/source/ -I${RDK_FSROOT_PATH}/usr/include/ -I${RDK_FSROOT_PATH}/usr/include/rbus -I${RDK_PROJECT_ROOT_PATH}/breakpadwrap/ -DENABLE_RDKC_SUPPORT -DINCLUDE_BREAKPAD -DENABLE_RDKC_SUPPORT -DFEATURE_SUPPORT_WEBCONFIG -DMTLS_FROM_ENV "
    export GLIB_CFLAGS="-I${RDK_PROJECT_ROOT_PATH}/opensource/lib/glib-2.0/include/ -I${RDK_PROJECT_ROOT_PATH}/opensource/include/glib-2.0/ -I${RDK_PROJECT_ROOT_PATH}/opensource/include/glib-2.0/glib"
    export ac_cv_func_malloc_0_nonnull=yes
    export ac_cv_func_memset=yes

    ./configure --prefix=${RDK_FSROOT_PATH}/usr --sysconfdir=${RDK_FSROOT_PATH}/etc $configure_options
}

function clean()
{
    echo "Start Clean"
    cd ${RDK_SOURCE_PATH}
    if [ -f Makefile ]; then
      make clean
    fi
    rm -f configure;
    rm -rf aclocal.m4 autom4te.cache config.log config.status libtool
    find . -iname "Makefile.in" -exec rm -f {} \;
    find . -iname "Makefile" | xargs rm -f
}

function build()
{
   echo "Building telemetry common code"
   cd ${RDK_SOURCE_PATH}
   make


    cd ${RDK_SOURCE_PATH}/source
    cp .libs/telemetry2_0 .libs/telemetry2_0_debug
    $RDK_DUMP_SYMS .libs/telemetry2_0 > .libs/telemetry2_0.sym
    cp commonlib/.libs/telemetry2_0_client .libs/telemetry2_0_client_debug
    $RDK_DUMP_SYMS commonlib/.libs/telemetry2_0_client > commonlib/.libs/telemetry2_0_client.sym
    cp dcautil/.libs/libdcautil.so.0.0.0 dcautil/.libs/libdcautil_debug.so.0.0.0
    $RDK_DUMP_SYMS dcautil/.libs/libdcautil.so.0.0.0 > dcautil/.libs/libdcautil.so.0.0.0.sym
    cp reportgen/.libs/libreportgen.so.0.0.0 reportgen/.libs/libreportgen_debug.so.0.0.0
    $RDK_DUMP_SYMS reportgen/.libs/libreportgen.so.0.0.0 > reportgen/.libs/libreportgen.so.0.0.0.sym
    cp interChipHelper/.libs/libinterChipHelper.so.0.0.0 interChipHelper/.libs/libinterChipHelper_debug.so.0.0.0
    $RDK_DUMP_SYMS interChipHelper/.libs/libinterChipHelper.so.0.0.0 > interChipHelper/.libs/libinterChipHelper.so.0.0.0.sym
    cp commonlib/.libs/libtelemetry_msgsender.so.0.0.0 commonlib/.libs/libtelemetry_msgsender_debug.so.0.0.0
    $RDK_DUMP_SYMS commonlib/.libs/libtelemetry_msgsender.so.0.0.0 > commonlib/.libs/libtelemetry_msgsender.so.0.0.0.sym
    cp protocol/http/.libs/libhttp.so.0.0.0 protocol/http/.libs/libhttp_debug.so.0.0.0
    $RDK_DUMP_SYMS protocol/http/.libs/libhttp.so.0.0.0 > protocol/http/.libs/libhttp.so.0.0.0.sym
    cp protocol/rbusMethod/.libs/librbusmethod.so.0.0.0 protocol/rbusMethod/.libs/librbusmethod_debug.so.0.0.0
    $RDK_DUMP_SYMS protocol/rbusMethod/.libs/librbusmethod.so.0.0.0 > protocol/rbusMethod/.libs/librbusmethod.so.0.0.0.sym
    cp t2parser/.libs/libt2parser.so.0.0.0 t2parser/.libs/libt2parser_debug.so.0.0.0
    $RDK_DUMP_SYMS t2parser/.libs/libt2parser.so.0.0.0 > t2parser/.libs/libt2parser.so.0.0.0.sym
    cp scheduler/.libs/libscheduler.so.0.0.0 scheduler/.libs/libscheduler_debug.so.0.0.0
    $RDK_DUMP_SYMS scheduler/.libs/libscheduler.so.0.0.0 > scheduler/.libs/libscheduler.so.0.0.0.sym
    cp ccspinterface/.libs/libccspinterface.so.0.0.0 ccspinterface/.libs/libccspinterface_debug.so.0.0.0
    $RDK_DUMP_SYMS ccspinterface/.libs/libccspinterface.so.0.0.0 > ccspinterface/.libs/libccspinterface.so.0.0.0.sym
    cp xconf-client/.libs/libxconfclient.so.0.0.0 xconf-client/.libs/libxconfclient.so_debug.0.0.0
    $RDK_DUMP_SYMS xconf-client/.libs/libxconfclient.so.0.0.0 > xconf-client/.libs/libxconfclient.so.0.0.0.sym
    cp bulkdata/.libs/libbulkdata.so.0.0.0 bulkdata/.libs/libbulkdata.so_debug.0.0.0
    $RDK_DUMP_SYMS bulkdata/.libs/libbulkdata.so.0.0.0 > bulkdata/.libs/libbulkdata.so.0.0.0.sym
    cp utils/.libs/libt2utils.so.0.0.0 utils/.libs/libt2utils.so_debug.0.0.0
    $RDK_DUMP_SYMS utils/.libs/libt2utils.so.0.0.0 > utils/.libs/libt2utils.so.0.0.0.sym

   
    mv .libs/telemetry2_0.sym  $PLATFORM_SYMBOL_PATH	
    mv commonlib/.libs/telemetry2_0_client.sym $PLATFORM_SYMBOL_PATH
    mv dcautil/.libs/libdcautil.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv reportgen/.libs/libreportgen.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv interChipHelper/.libs/libinterChipHelper.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv commonlib/.libs/libtelemetry_msgsender.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv protocol/http/.libs/libhttp.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv protocol/rbusMethod/.libs/librbusmethod.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv t2parser/.libs/libt2parser.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv scheduler/.libs/libscheduler.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv ccspinterface/.libs/libccspinterface.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv xconf-client/.libs/libxconfclient.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv bulkdata/.libs/libbulkdata.so.0.0.0.sym $PLATFORM_SYMBOL_PATH
    mv utils/.libs/libt2utils.so.0.0.0.sym $PLATFORM_SYMBOL_PATH	

   install
}

function rebuild()
{
    clean
    configure
    build
}

function install()
{
    cd ${RDK_SOURCE_PATH}
    cp include/telemetry_busmessage_sender.h ${RDK_FSROOT_PATH}/usr/include
    cp include/telemetry2_0.h ${RDK_FSROOT_PATH}/usr/include
    cd ${RDK_SOURCE_PATH}/source
    cp dcautil/.libs/libdcautil.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp dcautil/.libs/libdcautil.la ${RDK_FSROOT_PATH}/usr/lib/
    cp reportgen/.libs/libreportgen.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp reportgen/.libs/libreportgen.la ${RDK_FSROOT_PATH}/usr/lib/
    cp interChipHelper/.libs/libinterChipHelper.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp interChipHelper/.libs/libinterChipHelper.a ${RDK_FSROOT_PATH}/usr/lib/
    cp interChipHelper/.libs/libinterChipHelper.la ${RDK_FSROOT_PATH}/usr/lib/
    cp commonlib/.libs/libtelemetry_msgsender.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp commonlib/.libs/libtelemetry_msgsender.la ${RDK_FSROOT_PATH}/usr/lib/
    cp protocol/http/.libs/libhttp.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp protocol/http/.libs/libhttp.la ${RDK_FSROOT_PATH}/usr/lib/
    cp protocol/rbusMethod/.libs/librbusmethod.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp protocol/rbusMethod/.libs/librbusmethod.la ${RDK_FSROOT_PATH}/usr/lib/
    cp t2parser/.libs/libt2parser.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp t2parser/.libs/libt2parser.a ${RDK_FSROOT_PATH}/usr/lib/
    cp t2parser/.libs/libt2parser.la ${RDK_FSROOT_PATH}/usr/lib/
    cp scheduler/.libs/libscheduler.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp scheduler/.libs/libscheduler.la ${RDK_FSROOT_PATH}/usr/lib/
    cp ccspinterface/.libs/libccspinterface.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp ccspinterface/.libs/libccspinterface.la ${RDK_FSROOT_PATH}/usr/lib/
    cp xconf-client/.libs/libxconfclient.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp xconf-client/.libs/libxconfclient.la ${RDK_FSROOT_PATH}/usr/lib
    cp bulkdata/.libs/libbulkdata.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp bulkdata/.libs/libbulkdata.la ${RDK_FSROOT_PATH}/usr/lib/
    cp utils/.libs/libt2utils.so* ${RDK_FSROOT_PATH}/usr/lib/
    cp utils/.libs/libt2utils.la ${RDK_FSROOT_PATH}/usr/lib/
    cp .libs/telemetry2_0 ${RDK_FSROOT_PATH}/usr/bin/
    cp commonlib/.libs/telemetry2_0_client ${RDK_FSROOT_PATH}/usr/bin/
    cp nativeProtocolSimulator/.libs/t2rbusMethodSimulator ${RDK_FSROOT_PATH}/usr/bin/
    cp commonlib/t2Shared_api.sh ${RDK_FSROOT_PATH}/lib/rdk/

}

# run the logic
#these args are what left untouched after parse_args
HIT=false
for i in "$@"; do
    case $i in
        configure)  HIT=true; configure ;;
        clean)      HIT=true; clean ;;
        build)      HIT=true; build ;;
        rebuild)    HIT=true; rebuild ;;
        install)    HIT=true; install ;;
        *)
            #skip unknown
        ;;
    esac
done
# if not HIT do build by default
if ! $HIT; then
  build
fi
