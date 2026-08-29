# BEGIN makefw-subdirs
SUBDIRS := \
	libsrc \
	src
# END makefw-subdirs

include $(APP_DIR)/c-platform/prod/runtime-bundle.mk

c-platform-runtime-bundle: src

default build: c-platform-runtime-bundle

clean: c-platform-runtime-clean
