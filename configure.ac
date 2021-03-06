##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
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
AC_PREREQ([2.53])
AC_INIT(telemetry, 2.0)
AC_CONFIG_SRCDIR([source])
AC_CONFIG_SRCDIR([source/bulkdata])
AC_CONFIG_SRCDIR([source/reportgen])
AC_CONFIG_SRCDIR([source/scheduler])
AC_CONFIG_SRCDIR([source/ccspinterface])
AC_CONFIG_SRCDIR([source/t2parser])
AC_CONFIG_SRCDIR([source/xconf-client])
AC_CONFIG_SRCDIR([source/utils])
AC_CONFIG_SRCDIR([source/commonlib])
AC_CONFIG_SRCDIR([source/protocol])
AC_CONFIG_SRCDIR([source/interChipHelper])
AC_CONFIG_SRCDIR([source/protocol/http])
AC_CONFIG_SRCDIR([source/dcautil])
AC_CONFIG_SRCDIR([source/testApp])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
LT_INIT


dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_CPP
AC_PROG_CXXCPP

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])


dnl **********************************
dnl checks for dependencies
dnl **********************************
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
dnl AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT8_T
dnl Checks for library functions.
AC_FUNC_MALLOC

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.0])

ENABLE_TESTS=false
AM_CONDITIONAL([ENABLE_TESTS], [test x$ENABLE_TESTS = xtrue])

ENABLE_CCSP_SUPPORT=false
AC_ARG_ENABLE([ccspsupport],
              AS_HELP_STRING([--enable-ccspsupport],[enable ccsp support (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_CCSP_SUPPORT=true;;
                  no) AC_MSG_ERROR([ccsp support is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-ccspsupport]) ;;
                esac
              ],
              [echo "ccsp support is disabled"])

AM_CONDITIONAL([ENABLE_CCSP_SUPPORT], [test x$ENABLE_CCSP_SUPPORT = xtrue])


AC_CONFIG_FILES([Makefile
                 source/Makefile
                 source/bulkdata/Makefile
                 source/reportgen/Makefile
                 source/scheduler/Makefile
                 source/ccspinterface/Makefile
                 source/t2parser/Makefile
                 source/xconf-client/Makefile
                 source/utils/Makefile
                 source/commonlib/Makefile
                 source/interChipHelper/Makefile
                 source/protocol/Makefile
                 source/dcautil/Makefile
                 source/testApp/Makefile
                 source/protocol/http/Makefile])

AM_COND_IF([ENABLE_CCSP_SUPPORT], [AC_CONFIG_FILES([source/t2dm/Makefile])])
AM_COND_IF([ENABLE_CCSP_SUPPORT], [AC_CONFIG_SRCDIR([source/t2dm])])

AC_OUTPUT
