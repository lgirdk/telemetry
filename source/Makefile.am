##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
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
SUBDIRS = utils ccspinterface t2parser xconf-client protocol reportgen scheduler dcautil commonlib bulkdata interChipHelper

if ENABLE_CCSP_SUPPORT
SUBDIRS += t2dm
endif

AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -DCCSP_INC_no_asm_sigcontext_h

AM_CPPFLAGS = -Wall -Werror
ACLOCAL_AMFLAGS = -I m4
hardware_platform = i686-linux-gnu

bin_PROGRAMS = telemetry2_0
telemetry2_0_SOURCES = telemetry2_0.c
telemetry2_0_CFLAGS = -DFEATURE_SUPPORT_RDKLOG
telemetry2_0_CPPFLAGS = -fPIC -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 \
                                -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include \
                                -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/ \
                                -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus \
                                -I${top_srcdir}/include \
                                -I${top_srcdir}/source/utils \
                                -I${top_srcdir}/source/bulkdata \
                                -I${top_srcdir}/source/ccspinterface \
                                -I${top_srcdir}/source/dcautil \
                                -I${top_srcdir}/source/protocol/http \
                                -I${top_srcdir}/source/reportgen \
                                -I${top_srcdir}/source/scheduler \
                                -I${top_srcdir}/source/t2parser \
                                -I${top_srcdir}/source/utils \
                                -I${top_srcdir}/source/xconf-client \
                                -I${top_srcdir}/source/interChipHelper
telemetry2_0_LDFLAGS = -lrdkloggers -lcjson
if ENABLE_CCSP_SUPPORT
telemetry2_0_LDFLAGS+=-lccsp_common -lwebconfig_framework
telemetry2_0_SOURCES += t2ssp/ssp_main.c t2ssp/ssp_action.c t2ssp/ssp_messagebus_interface.c t2ssp/dm_pack_datamodel.c
telemetry2_0_CPPFLAGS += -I${top_srcdir}/source/t2ssp \
                                -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/ccsp
endif
telemetry2_0_LDADD = ${top_builddir}/source/utils/libutils.la ${top_builddir}/source/dcautil/libdcautil.la \
                  ${top_builddir}/source/bulkdata/libbulkdata.la ${top_builddir}/source/ccspinterface/libccspinterface.la \
                  ${top_builddir}/source/reportgen/libreportgen.la ${top_builddir}/source/scheduler/libscheduler.la \
                  ${top_builddir}/source/protocol/http/libhttp.la \
                  ${top_builddir}/source/interChipHelper/libinterChipHelper.la \
                  ${top_builddir}/source/t2parser/libt2parser.la \
                  ${top_builddir}/source/xconf-client/libxconfclient.la
