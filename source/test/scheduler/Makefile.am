##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
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
AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS = -std=c++11

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = scheduler_gtest.bin

scheduler_gtest_bin_CPPFLAGS = -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/gtest -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/gtest/internal -I${top_srcdir}/gtest/include -I${top_srcdir}/source/include -I${top_srcdir}/source -I${top_srcdir}/include -I${top_srcdir}/source/utils -I${top_srcdir}/source/protocol -I${top_srcdir}/source/t2parser -I${top_srcdir}/source/xconf-client -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus -I${top_srcdir}/source/bulkdata -I${top_srcdir}/source/ccspinterface -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include  -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/ccsp -I${top_srcdir}/source/reportgen  -I${PKG_CONFIG_SYSROOT_DIR}$(includedir) -I${top_srcdir}/source/t2ssp -I${top_srcdir}/source/dcautil -I${RDK_PROJECT_ROOT_PATH}/opensource/include -I${RDK_PROJECT_ROOT_PATH}/opensource/include/glib-2.0/ -I${RDK_PROJECT_ROOT_PATH}/opensource/lib/glib-2.0/include/ -I${RDK_PROJECT_ROOT_PATH}/opensource/include/glib-2.0/glib -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/glib-2.0 -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/glib-2.0/include

if ENABLE_CCSP_SUPPORT
scheduler_gtest_bin_SOURCES = gtest_main.cpp schedulerTest.cpp ../../t2ssp/ssp_main.c ../../t2ssp/ssp_action.c ../../t2ssp/ssp_messagebus_interface.c ../../t2ssp/dm_pack_datamodel.c
else
scheduler_gtest_bin_SOURCES = gtest_main.cpp schedulerTest.cpp
endif

scheduler_gtest_bin_LDFLAGS = -lgtest -lgmock -lgcov -lcurl -lrbus $(GLIB_LIBS) -lcjson -pthread

scheduler_gtest_bin_LDADD = ${top_builddir}/source/scheduler/libscheduler.la ${top_builddir}/source/bulkdata/libbulkdata.la ${top_builddir}/source/utils/libt2utils.la ${top_builddir}/source/t2parser/libt2parser.la

scheduler_gtest_bin_DEPENDENCIES = ${top_builddir}/source/scheduler/libscheduler.la ${top_builddir}/source/bulkdata/libbulkdata.la ${top_builddir}/source/utils/libt2utils.la ${top_builddir}/source/t2parser/libt2parser.la

scheduler_gtest_bin_la_DEPENDENCIES = ${top_builddir}/source/scheduler/libscheduler.la ${top_builddir}/source/bulkdata/libbulkdata.la ${top_builddir}/source/utils/libt2utils.la ${top_builddir}/source/t2parser/libt2parser.la

