#include <mock_stdlib.h>

#define atexit(__function) mock_atexit(__FILE__, __LINE__, __func__, __function)

#include "../../../../../prod/libsrc/com_util/runtime/shutdown.c"
