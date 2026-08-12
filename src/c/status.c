#include <errno.h>

#include <arborcore/arborcore.h>

arbor_status arbor_status_from_native(int64_t native)
{
    arbor_status status;
    status.native = native;

    if (native == 0) {
        status.code = ARBOR_STATUS_OK;
    } else if (native == (int64_t)ARBORCORE_SERVER_MORE_WORK) {
        status.code = ARBOR_STATUS_MORE_WORK;
    } else if (native == -ENOENT) {
        status.code = ARBOR_STATUS_NOT_FOUND;
    } else if (native == -EINTR) {
        status.code = ARBOR_STATUS_INTERRUPTED;
    } else if (native == -EIO) {
        status.code = ARBOR_STATUS_IO;
    } else if (native == -EAGAIN) {
        status.code = ARBOR_STATUS_WOULD_BLOCK;
    } else if (native == -EINVAL) {
        status.code = ARBOR_STATUS_INVALID_ARGUMENT;
    } else if (native == -ENOSPC) {
        status.code = ARBOR_STATUS_NO_SPACE;
    } else if (native == -EOVERFLOW) {
        status.code = ARBOR_STATUS_OVERFLOW;
    } else if (native == -ECONNRESET) {
        status.code = ARBOR_STATUS_CONNECTION_RESET;
    } else if (native < 0) {
        status.code = ARBOR_STATUS_NATIVE_ERROR;
    } else {
        status.code = ARBOR_STATUS_NATIVE_SUCCESS;
    }

    return status;
}

bool arbor_status_is_success(arbor_status status)
{
    return status.native >= 0;
}

bool arbor_status_is_error(arbor_status status)
{
    return status.native < 0;
}

const char *arbor_status_name(arbor_status status)
{
    switch (status.code) {
    case ARBOR_STATUS_OK:
        return "ok";
    case ARBOR_STATUS_MORE_WORK:
        return "more_work";
    case ARBOR_STATUS_NOT_FOUND:
        return "not_found";
    case ARBOR_STATUS_INTERRUPTED:
        return "interrupted";
    case ARBOR_STATUS_IO:
        return "io";
    case ARBOR_STATUS_WOULD_BLOCK:
        return "would_block";
    case ARBOR_STATUS_INVALID_ARGUMENT:
        return "invalid_argument";
    case ARBOR_STATUS_NO_SPACE:
        return "no_space";
    case ARBOR_STATUS_OVERFLOW:
        return "overflow";
    case ARBOR_STATUS_CONNECTION_RESET:
        return "connection_reset";
    case ARBOR_STATUS_NATIVE_ERROR:
        return "native_error";
    case ARBOR_STATUS_NATIVE_SUCCESS:
        return "native_success";
    }

    return "unknown";
}
