include $(top_srcdir)/common/check.mak

CHECK_REGISTRY = $(top_builddir)/check/test-registry.xml

REGISTRY_ENVIRONMENT = \
	GST_REGISTRY=$(CHECK_REGISTRY)

TESTS_ENVIRONMENT = \
	$(REGISTRY_ENVIRONMENT)					\
	GST_PLUGIN_PATH_ONLY=yes				\
	GST_PLUGIN_PATH=$(top_builddir)/gst:$(top_builddir)/ext

# ths core dumps of some machines have PIDs appended
CLEANFILES = core.* test-registry.xml

clean-local: clean-local-check

$(CHECK_REGISTRY):
	$(TESTS_ENVIRONMENT)					\
	$(GST_TOOLS_DIR)/gst-register-@GST_MAJORMINOR@

TESTS = $(GST_TOOLS_DIR)/gst-register-@GST_MAJORMINOR@		\
	$(check_PROGRAMS)

if USE_VORBIS
check_vorbis = elements/vorbisdec
else
check_vorbis =
endif

check_PROGRAMS = \
	elements/volume \
	$(check_vorbis)

AM_CFLAGS = $(GST_OBJ_CFLAGS) $(GST_CHECK_CFLAGS)
LDADD = $(GST_OBJ_LIBS) $(GST_CHECK_LIBS)

# valgrind testing
VALGRIND_TESTS_DISABLE = 			\
	$(GST_TOOLS_DIR)/gst-register-@GST_MAJORMINOR@

SUPPRESSIONS = $(top_srcdir)/common/gst.supp