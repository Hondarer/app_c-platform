# BEGIN makefw-subdirs
SUBDIRS := \
	libsrc \
	src
# END makefw-subdirs

include $(APP_DIR)/com_util/prod/runtime-bundle.mk

com-util-runtime-bundle: src

default build: com-util-runtime-bundle

clean: com-util-runtime-clean
