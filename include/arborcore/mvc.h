#ifndef ARBORCORE_MVC_H
#define ARBORCORE_MVC_H

#include <stdint.h>

#include <arborcore/application.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_MVC_ABI_VERSION 1u
#define ARBOR_MVC_APPLICATION_FLAGS_NONE UINT64_C(0)
#define ARBOR_MVC_CATALOG_FLAGS_NONE UINT64_C(0)
#define ARBOR_MVC_ROUTE_FLAGS_NONE UINT64_C(0)
#define ARBOR_MVC_MIDDLEWARE_FLAGS_NONE UINT64_C(0)
#define ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE UINT32_C(0)

#define ARBOR_MVC_MIDDLEWARE_CONTINUE UINT32_C(0)
#define ARBOR_MVC_MIDDLEWARE_RESPOND UINT32_C(1)

typedef struct arbor_mvc_requirements {
    uint64_t route_validation_param_capacity;
} arbor_mvc_requirements;

typedef struct arbor_mvc_prepare_workspace {
    arbor_route_param *route_params;
    uint64_t route_param_capacity;
} arbor_mvc_prepare_workspace;

typedef struct arbor_mvc_controller_result {
    uint32_t outcome_code;
    uint32_t flags;
    const void *model_data;
    uint64_t model_size;
} arbor_mvc_controller_result;

typedef struct arbor_mvc_middleware_before_result {
    uint32_t action;
    uint32_t reserved0;
    arbor_response_plan response;
} arbor_mvc_middleware_before_result;

typedef struct arbor_mvc_request arbor_mvc_request;

typedef int64_t (*arbor_mvc_controller_fn)(
    const arbor_mvc_request *request,
    void *controller_context,
    arbor_mvc_controller_result *result_out);

typedef int64_t (*arbor_mvc_presenter_fn)(
    const arbor_mvc_request *request,
    void *presenter_context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out);

typedef int64_t (*arbor_mvc_middleware_before_fn)(
    const arbor_mvc_request *request,
    void *middleware_context,
    arbor_mvc_middleware_before_result *result_out);

typedef int64_t (*arbor_mvc_middleware_after_fn)(
    const arbor_mvc_request *request,
    void *middleware_context,
    const arbor_response_plan *current,
    arbor_response_plan *response_out);

typedef struct arbor_mvc_middleware_descriptor {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    void *middleware_context;
    arbor_mvc_middleware_before_fn before;
    arbor_mvc_middleware_after_fn after;
} arbor_mvc_middleware_descriptor;

typedef struct arbor_mvc_route {
    const uint8_t *method_data;
    uint64_t method_length;
    const uint8_t *pattern_data;
    uint64_t pattern_length;
    arbor_mvc_controller_fn controller;
    void *controller_context;
    arbor_mvc_presenter_fn presenter;
    void *presenter_context;
    const uint64_t *middleware_indices;
    uint64_t middleware_count;
} arbor_mvc_route;

typedef struct arbor_mvc_catalog {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_mvc_route *routes;
    uint64_t route_count;
    const arbor_mvc_middleware_descriptor *middlewares;
    uint64_t middleware_count;
} arbor_mvc_catalog;

typedef struct arbor_mvc_application {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_mvc_catalog *catalog;
    uint64_t max_route_parameter_count;
    /* Prepared-integrity guard. Catalog storage remains caller-owned immutable. */
    uint64_t max_route_parameter_count_guard;
} arbor_mvc_application;

struct arbor_mvc_request {
    const arbor_request_scope *scope;
    const arbor_mvc_route *route;
    const arbor_route_param *params;
    uint64_t parameter_count;
    uint64_t route_index;
};

arbor_status arbor_mvc_catalog_measure(
    const arbor_mvc_catalog *catalog,
    arbor_mvc_requirements *out);

arbor_status arbor_mvc_application_prepare(
    const arbor_mvc_catalog *catalog,
    arbor_mvc_prepare_workspace *workspace,
    arbor_mvc_application *out);

arbor_status arbor_mvc_application_validate(
    const arbor_mvc_application *application);

arbor_status arbor_mvc_application_capabilities_make(
    arbor_mvc_application *application,
    arbor_application_capabilities *out);

arbor_status arbor_mvc_request_validate(const arbor_mvc_request *request);

arbor_status arbor_mvc_controller_result_validate(
    const arbor_mvc_controller_result *result);

arbor_status arbor_mvc_middleware_before_result_validate(
    const arbor_mvc_middleware_before_result *result);

#ifdef __cplusplus
}
#endif

#endif
