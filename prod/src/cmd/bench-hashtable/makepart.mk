# 反復回数の自動調整は bench-io と同じ実装を共有する
ADD_SRCS := \
	$(MYAPP_DIR)/prod/src/cmd/bench-io/bench_timer.c

CPPFLAGS += -I$(MYAPP_DIR)/prod/src/cmd/bench-io
