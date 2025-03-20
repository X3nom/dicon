#pragma once

// TODO MAYBE SOMEDAY ...

#define ASYNCIFY_FUNC_ARGS(_funcname, ...) \
    struct _funcname##_args{ __VA_ARGS__ }

#define ASYNCIFY(_funcname) \
    ASYNCIFY_FUNC_ARGS(_funcname) \
    void *_funcname##_async(){\
    }


