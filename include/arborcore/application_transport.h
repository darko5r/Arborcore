#ifndef ARBORCORE_APPLICATION_TRANSPORT_H
#define ARBORCORE_APPLICATION_TRANSPORT_H

#include <stdint.h>

#include <arborcore/application.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_APPLICATION_TRANSPORT_VERSION_MAJOR 0u
#define ARBOR_APPLICATION_TRANSPORT_VERSION_MINOR 1u
#define ARBOR_APPLICATION_TRANSPORT_VERSION_PATCH 0u

/*
 * Parallel rich Application transport. This does not replace arbor_server_step
 * or the frozen Assembly status-only route-handler path.
 */
arbor_status arbor_application_server_step(
    arbor_runtime_storage *storage,
    const arbor_application_capabilities *application,
    int64_t epoll_fd,
    uint64_t *completed_request_count);

#ifdef __cplusplus
}
#endif

#endif
