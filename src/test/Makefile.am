test_tsmapi_CFLAGS = -m64 -DLINUX_CLIENT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -I$(top_srcdir)/src/lib -I@TSM_SRC_DIR@/ -I@LUSTRE_SRC_DIR@/lustre/include

bin_PROGRAMS = test_tsmapi
test_tsmapi_SOURCES = test_tsmapi.c CuTest.c test_qarray.c test_dsstruct64_off_t.c
test_tsmapi_LDADD = $(top_srcdir)/src/lib/libtsmapi.a