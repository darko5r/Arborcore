#ifndef ARBORCORE_EXAMPLES_COUNTER1_H
#define ARBORCORE_EXAMPLES_COUNTER1_H

#include <stdint.h>

#include <arborcore/application_service.h>
#include <arborcore/ddd_support.h>
#include <arborcore/http_mvc.h>
#include <arborcore/view.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COUNTER1_REPOSITORY_ABI_VERSION 1u
#define COUNTER1_REPOSITORY_FLAGS_NONE UINT64_C(0)
#define COUNTER1_ROUTE_COUNT 2u
#define COUNTER1_TEMPLATE_FIELD_COUNT 2u
#define COUNTER1_RESPONSE_FIELD_CAPACITY 1u

typedef enum counter1_repository_get_outcome {
    COUNTER1_REPOSITORY_GET_FOUND = 1,
    COUNTER1_REPOSITORY_GET_NOT_FOUND = 2
} counter1_repository_get_outcome;

typedef enum counter1_repository_increment_outcome {
    COUNTER1_REPOSITORY_INCREMENTED = 1,
    COUNTER1_REPOSITORY_INCREMENT_NOT_FOUND = 2,
    COUNTER1_REPOSITORY_INCREMENT_LIMIT_REACHED = 3
} counter1_repository_increment_outcome;

typedef enum counter1_service_outcome {
    COUNTER1_SERVICE_FOUND = 1,
    COUNTER1_SERVICE_INCREMENTED = 2,
    COUNTER1_SERVICE_NOT_FOUND = 3,
    COUNTER1_SERVICE_LIMIT_REACHED = 4
} counter1_service_outcome;

typedef enum counter1_controller_outcome {
    COUNTER1_CONTROLLER_FOUND = 1,
    COUNTER1_CONTROLLER_INCREMENTED = 2,
    COUNTER1_CONTROLLER_NOT_FOUND = 3,
    COUNTER1_CONTROLLER_LIMIT_REACHED = 4,
    COUNTER1_CONTROLLER_INVALID_REQUEST = 5
} counter1_controller_outcome;

typedef struct counter1_record {
    uint64_t id;
    uint64_t value;
} counter1_record;

typedef struct counter1_repository_get_result {
    uint32_t outcome_code;
    uint32_t reserved0;
    uint64_t id;
    uint64_t value;
} counter1_repository_get_result;

typedef struct counter1_repository_increment_result {
    uint32_t outcome_code;
    uint32_t reserved0;
    uint64_t id;
    uint64_t value;
} counter1_repository_increment_result;

typedef int64_t (*counter1_repository_get_fn)(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_get_result *result_out);

typedef int64_t (*counter1_repository_increment_fn)(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_increment_result *result_out);

typedef struct counter1_repository_v1 {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    counter1_repository_get_fn get;
    counter1_repository_increment_fn increment;
} counter1_repository_v1;

typedef struct counter1_repository_provider {
    const counter1_repository_v1 *repository;
    const arbor_ddd_transaction_interface *transaction;
    void *provider_context;
} counter1_repository_provider;

typedef struct counter1_transaction_state {
    uint64_t guard;
    uint64_t staged_index;
    uint64_t staged_value;
    uint64_t has_staged_mutation;
} counter1_transaction_state;

typedef struct counter1_in_memory_repository {
    counter1_record records[3];
    uint64_t record_count;
    uint64_t prepared_guard;
} counter1_in_memory_repository;

typedef struct counter1_service_request {
    arbor_asm_arena *arena;
    uint64_t id;
} counter1_service_request;

typedef struct counter1_service_result {
    uint32_t outcome_code;
    uint32_t reserved0;
    uint64_t id;
    uint64_t value;
} counter1_service_result;

typedef int64_t (*counter1_service_get_fn)(
    void *provider_context,
    const counter1_service_request *request,
    counter1_service_result *result_out);

typedef int64_t (*counter1_service_increment_fn)(
    void *provider_context,
    const counter1_service_request *request,
    counter1_service_result *result_out);

typedef struct counter1_service_v1 {
    arbor_application_service_interface_header header;
    counter1_service_get_fn get_counter;
    counter1_service_increment_fn increment_counter;
} counter1_service_v1;

typedef struct counter1_service_module_state {
    arbor_capability_binding repository_binding;
    arbor_capability_binding transaction_binding;
    uint64_t prepared_guard;
} counter1_service_module_state;

typedef struct counter1_service_provider_context {
    counter1_service_module_state *module;
} counter1_service_provider_context;

typedef struct counter1_incremented_event_v1 {
    uint64_t id;
    uint64_t value;
} counter1_incremented_event_v1;

typedef struct counter1_page_model {
    arbor_span id_text;
    arbor_span value_text;
} counter1_page_model;

typedef struct counter1_application {
    counter1_repository_provider provider;
    counter1_service_module_state service_module_state;
    counter1_service_provider_context service_provider_context;
    arbor_capability_export provider_exports[2];
    arbor_capability_export service_exports[1];
    arbor_capability_requirement service_requirements[2];
    arbor_module_descriptor modules[2];
    arbor_capability_binding catalog_bindings[3];
    arbor_capability_resolution catalog_resolutions[2];
    uint64_t catalog_module_order[2];
    arbor_capability_catalog catalog;
    uint64_t service_export_indices[1];
    arbor_application_service_module_descriptor service_module_descriptor;
    arbor_application_service_runtime_record runtime_records[1];
    arbor_application_runtime runtime;
    arbor_capability_binding service_binding;
    uint64_t prepared_guard;
} counter1_application;

typedef struct counter1_route_context {
    counter1_application *application;
    uint32_t action;
    uint32_t reserved0;
} counter1_route_context;

typedef struct counter1_web_application {
    counter1_application *application;
    arbor_view_html_template_part template_parts[64];
    uint8_t template_literals[4096];
    arbor_view_html_template_storage template_storage;
    arbor_view_html_template template_view;
    counter1_route_context route_contexts[2];
    arbor_mvc_route routes[2];
    arbor_mvc_catalog catalog;
    arbor_mvc_application mvc_application;
    arbor_http_mvc_application http_application;
    uint64_t prepared_guard;
} counter1_web_application;

arbor_status counter1_in_memory_repository_prepare(
    counter1_in_memory_repository *repository,
    counter1_repository_provider *provider_out);

arbor_status counter1_in_memory_repository_validate(
    const counter1_in_memory_repository *repository);

arbor_status counter1_repository_provider_validate(
    const counter1_repository_provider *provider);

arbor_status counter1_application_prepare(
    const counter1_repository_provider *provider,
    counter1_application *out);

arbor_status counter1_application_validate(
    const counter1_application *application);

arbor_status counter1_application_stop(counter1_application *application);

arbor_status counter1_web_application_prepare(
    counter1_application *application,
    arbor_span template_source,
    uint64_t response_field_capacity,
    counter1_web_application *out);

arbor_status counter1_web_application_validate(
    const counter1_web_application *application);

#ifdef __cplusplus
}
#endif

#endif
