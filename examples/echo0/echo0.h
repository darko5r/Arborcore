#ifndef ARBORCORE_EXAMPLES_ECHO0_H
#define ARBORCORE_EXAMPLES_ECHO0_H

#include <stdint.h>

#include <arborcore/http_mvc.h>
#include <arborcore/view.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECHO0_ROUTE_COUNT 2u
#define ECHO0_MIDDLEWARE_COUNT 1u
#define ECHO0_TEMPLATE_PART_CAPACITY 4u
#define ECHO0_TEMPLATE_LITERAL_CAPACITY 1024u
#define ECHO0_RESPONSE_FIELD_CAPACITY 2u
#define ECHO0_TEMPLATE_SOURCE_CAPACITY 4096u
#define ECHO0_CONNECTION_BUFFER_CAPACITY 16384u
#define ECHO0_CONNECTION_SLOT_COUNT 8u
#define ECHO0_LISTEN_BACKLOG 16
#define ECHO0_SERVICE_PREPARED_GUARD UINT64_C(0x4543484f30535631)

typedef enum echo0_action {
    ECHO0_ACTION_SHOW_PAGE = 1,
    ECHO0_ACTION_REDIRECT = 2
} echo0_action;

typedef enum echo0_outcome_code {
    ECHO0_OUTCOME_PAGE = 1,
    ECHO0_OUTCOME_REDIRECT = 2
} echo0_outcome_code;

typedef struct echo0_service {
    uint64_t prepared_guard;
} echo0_service;

typedef struct echo0_service_result {
    uint32_t outcome_code;
    uint32_t reserved0;
    arbor_span value;
} echo0_service_result;

typedef struct echo0_page_model {
    arbor_span value;
} echo0_page_model;

typedef struct echo0_metrics {
    uint64_t middleware_calls;
    uint64_t controller_calls;
    uint64_t service_calls;
    uint64_t presenter_calls;
} echo0_metrics;

struct echo0_web_application;

typedef struct echo0_route_context {
    struct echo0_web_application *application;
    echo0_action action;
} echo0_route_context;

typedef struct echo0_web_application {
    echo0_service service;
    echo0_metrics metrics;

    arbor_view_html_template_part template_parts[ECHO0_TEMPLATE_PART_CAPACITY];
    uint8_t template_literals[ECHO0_TEMPLATE_LITERAL_CAPACITY];
    arbor_view_html_template_storage template_storage;
    arbor_view_html_template template_view;

    echo0_route_context route_contexts[ECHO0_ROUTE_COUNT];
    uint64_t middleware_indices[ECHO0_MIDDLEWARE_COUNT];
    arbor_mvc_middleware_descriptor middlewares[ECHO0_MIDDLEWARE_COUNT];
    arbor_mvc_route routes[ECHO0_ROUTE_COUNT];
    arbor_mvc_catalog catalog;
    arbor_mvc_application mvc_application;
    arbor_http_mvc_application http_application;
    uint64_t prepared_guard;
} echo0_web_application;

arbor_status echo0_service_prepare(echo0_service *service);

arbor_status echo0_service_execute(
    const echo0_service *service,
    echo0_action action,
    arbor_span value,
    echo0_service_result *result_out);

arbor_status echo0_web_application_prepare(
    arbor_span template_source,
    uint64_t response_field_capacity,
    echo0_web_application *out);

arbor_status echo0_web_application_validate(
    const echo0_web_application *application);

#ifdef __cplusplus
}
#endif

#endif
