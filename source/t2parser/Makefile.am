##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
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
AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_

lib_LTLIBRARIES = libt2parser.la
libt2parser_la_SOURCES = t2parser.c t2parserxconf.c
libt2parser_la_LIBADD = ${top_builddir}/source/ccspinterface/libccspinterface.la ${top_builddir}/source/utils/libutils.la
libt2parser_la_CPPFLAGS = -fPIC -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/dbus-1.0 \
                                -I${PKG_CONFIG_SYSROOT_DIR}$(libdir)/dbus-1.0/include \
                                -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/ccsp \
                                -I${top_srcdir}/include \
                                -I${top_srcdir}/source/xconf-client \
                                -I${top_srcdir}/source/bulkdata \
                                -I${top_srcdir}/source/reportgen \
                                -I${top_srcdir}/source/ccspinterface \
                                -I${top_srcdir}/source/utils

libt2parser_la_DEPENDENCIES = ${top_builddir}/source/ccspinterface/libccspinterface.la ${top_builddir}/source/utils/libutils.la

libt2parser_la_LDFLAGS = -lmsgpackc
