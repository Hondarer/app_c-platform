# BEGIN makefw-subdirs
SUBDIRS := \
	libsrc \
	src
# END makefw-subdirs

include $(call _makefw_escape_path,$(APP_DIR)/cplat/prod/runtime-bundle.mk)

cplat-runtime-bundle: src

default build: cplat-runtime-bundle

clean: cplat-runtime-clean
