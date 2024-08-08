AM_CFLAGS =

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = http_send

http_send_SOURCES = send.c  ../t2ssp/ssp_main.c ../t2ssp/ssp_action.c ../t2ssp/ssp_messagebus_interface.c ../t2ssp/dm_pack_datamodel.c

http_send_LDFLAGS = -shared -fPIC -lcurl -lpthread -lssl -lcrypto -lccsp_common -lrdkloggers -lcjson -lwebconfig_framework
http_send_LDADD = -lrbus ${top_builddir}/source/utils/libt2utils.la ${top_builddir}/source/dcautil/libdcautil.la ${top_builddir}/source/protocol/http/libhttp.la ${top_builddir}/source/protocol/rbusMethod/librbusmethod.la ${top_builddir}/source/ccspinterface/libccspinterface.la ${top_builddir}/source/bulkdata/libbulkdata.la  ${top_builddir}/source/reportgen/libreportgen.la ${top_builddir}/source/scheduler/libscheduler.la ${top_builddir}/source/t2parser/libt2parser.la ${top_builddir}/source/interChipHelper/libinterChipHelper.la

http_send_CPPFLAGS = -fPIC -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/ccsp \
                                -I${top_srcdir}/include \
                                -I${top_srcdir}/source/ccspinterface \
                                -I${top_srcdir}/source/bulkdata \
								-I${top_srcdir}/source/protocol/http \
                                -I${top_srcdir}/source/utils \
								-I${top_srcdir}/source/reportgen \
                                -I${top_srcdir}/source/scheduler \
                                -I${top_srcdir}/source/t2parser \
                                -I${top_srcdir}/source/utils \
                                -I${top_srcdir}/source/xconf-client \
								-I${top_srcdir}/source/t2ssp \
                                -I${top_srcdir}/source/interChipHelper


http_send_DEPENDENCIES = ${top_builddir}/source/ccspinterface/libccspinterface.la ${top_builddir}/source/utils/libt2utils.la ${top_builddir}/source/dcautil/libdcautil.la ${top_builddir}/source/protocol/http/libhttp.la ${top_builddir}/source/protocol/rbusMethod/librbusmethod.la ${top_builddir}/source/bulkdata/libbulkdata.la ${top_builddir}/source/reportgen/libreportgen.la ${top_builddir}/source/scheduler/libscheduler.la ${top_builddir}/source/t2parser/libt2parser.la ${top_builddir}/source/interChipHelper/libinterChipHelper.la
