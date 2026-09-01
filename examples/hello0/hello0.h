#ifndef ARBORCORE_EXAMPLES_HELLO0_H
#define ARBORCORE_EXAMPLES_HELLO0_H

#include <stdint.h>

#include <arborcore/http_mvc.h>
#include <arborcore/view.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HELLO0_ROUTE_COUNT 2u
#define HELLO0_MIDDLEWARE_COUNT 1u
#define HELLO0_TEMPLATE_PART_CAPACITY 4u
#define HELLO0_TEMPLATE_LITERAL_CAPACITY 1024u
#define HELLO0_RESPONSE_FIELD_CAPACITY 2u
#define HELLO0_TEMPLATE_SOURCE_CAPACITY 4096u
#define HELLO0_CONNECTION_BUFFER_CAPACITY 16384u
#define HELLO0_CONNECTION_SLOT_COUNT 8u
#define HELLO0_LISTEN_BACKLOG 16

typedef enum hello0_action {
    HELLO0_ACTION_SHOW_PAGE = 1,
    HELLO0_ACTION_REDIRECT = 2
} hello0_action;

typedef enum hello0_outcome_code {
    HELLO0_OUTCOME_PAGE = 1,
    HELLO0_OUTCOME_REDIRECT = 2
} hello0_outcome_code;

typedef struct hello0_service {
    arbor_span page_message;
} hello0_service;

typedef struct hello0_service_result {
    uint32_t outcome_code;
    uint32_t reserved0;
    arbor_span message;
} hello0_service_result;

typedef struct hello0_page_model {
    arbor_span message;
} hello0_page_model;

typedef struct hello0_metrics {
    uint64_t middleware_calls;
    uint64_t controller_calls;
    uint64_t service_calls;
    uint64_t presenter_calls;
} hello0_metrics;

struct hello0_web_application;

typedef struct hello0_route_context {
    struct hello0_web_application *application;
    hello0_action action;
} hello0_route_context;

typedef struct hello0_web_application {
    hello0_service service;
    hello0_metrics metrics;

    arbor_view_html_template_part template_parts[HELLO0_TEMPLATE_PART_CAPACITY];
    uint8_t template_literals[HELLO0_TEMPLATE_LITERAL_CAPACITY];
    arbor_view_html_template_storage template_storage;
    arbor_view_html_template template_view;

    hello0_route_context route_contexts[HELLO0_ROUTE_COUNT];
    uint64_t middleware_indices[HELLO0_MIDDLEWARE_COUNT];
    arbor_mvc_middleware_descriptor middlewares[HELLO0_MIDDLEWARE_COUNT];
    arbor_mvc_route routes[HELLO0_ROUTE_COUNT];
    arbor_mvc_catalog catalog;
    arbor_mvc_application mvc_application;
    arbor_http_mvc_application http_application;
    uint64_t prepared_guard;
} hello0_web_application;

arbor_status hello0_service_prepare(hello0_service *service);

arbor_status hello0_service_execute(
    const hello0_service *service,
    hello0_action action,
    hello0_service_result *result_out);

arbor_status hello0_web_application_prepare(
    arbor_span template_source,
    uint64_t response_field_capacity,
    hello0_web_application *out);

arbor_status hello0_web_application_validate(
    const hello0_web_application *application);

#ifdef __cplusplus
}
#endif

#endif
