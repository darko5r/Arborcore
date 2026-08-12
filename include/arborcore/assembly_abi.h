#ifndef ARBORCORE_ASSEMBLY_ABI_H
#define ARBORCORE_ASSEMBLY_ABI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__linux__) || !defined(__x86_64__)
#error "Arborcore Assembly ABI v1 requires Linux x86-64"
#endif

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "Arborcore Assembly ABI v1 requires little-endian x86-64"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ARBORCORE_ASSEMBLY_ABI_VERSION_MAJOR 1u
#define ARBORCORE_ASSEMBLY_ABI_VERSION_MINOR 0u
#define ARBORCORE_SERVER_MORE_WORK 1u
#define ARBORCORE_SERVER_REQUEST_BUDGET 8u

/*
 * Several frozen Assembly functions return a primary result/status in RAX and
 * a secondary value in RDX. Under the System V AMD64 ABI, a 16-byte aggregate
 * consisting of two INTEGER eightbytes is returned in exactly RAX:RDX. These
 * result types model that native contract directly; no shim ABI is introduced.
 */
typedef struct arbor_asm_result_u64 {
    int64_t status;
    uint64_t value;
} arbor_asm_result_u64;

typedef struct arbor_asm_result_i64 {
    int64_t status;
    int64_t value;
} arbor_asm_result_i64;

typedef struct arbor_asm_result_ptr {
    int64_t status;
    void *value;
} arbor_asm_result_ptr;

typedef struct arbor_asm_span_result {
    const uint8_t *data;
    uint64_t length;
} arbor_asm_span_result;

typedef struct arbor_asm_match_result {
    int64_t match;
    uint64_t parameter_count;
} arbor_asm_match_result;

/* Frozen data layouts from abi/arborcore-1.layout. */
typedef struct arbor_asm_buffer {
    uint8_t *data;
    uint64_t length;
    uint64_t capacity;
} arbor_asm_buffer;

typedef struct arbor_asm_arena {
    uint8_t *base;
    uint64_t capacity;
    uint64_t offset;
} arbor_asm_arena;

typedef enum arbor_asm_connection_state {
    ARBOR_ASM_CONNECTION_ACCEPTED = 1,
    ARBOR_ASM_CONNECTION_READING = 2,
    ARBOR_ASM_CONNECTION_REQUEST_READY = 3,
    ARBOR_ASM_CONNECTION_DISPATCHING = 4,
    ARBOR_ASM_CONNECTION_WRITING = 5,
    ARBOR_ASM_CONNECTION_KEEP_ALIVE = 6,
    ARBOR_ASM_CONNECTION_CLOSING = 7,
    ARBOR_ASM_CONNECTION_CLOSED = 8
} arbor_asm_connection_state;

typedef struct arbor_asm_connection {
    int64_t fd;
    uint64_t state;
    uint64_t flags;
    arbor_asm_buffer *input_buffer;
    arbor_asm_buffer *output_buffer;
    arbor_asm_arena *arena;
    uint64_t read_bytes;
    uint64_t write_bytes;
    uint64_t request_count;
    int64_t last_error;
    uint64_t frame_scan;
    uint64_t frame_needed;
} arbor_asm_connection;

typedef struct arbor_asm_http_request {
    const uint8_t *method_ptr;
    uint64_t method_len;
    const uint8_t *target_ptr;
    uint64_t target_len;
    const uint8_t *version_ptr;
    uint64_t version_len;
    const uint8_t *headers_ptr;
    uint64_t headers_len;
    const uint8_t *body_ptr;
    uint64_t body_available;
    uint64_t content_length;
    uint64_t message_length;
} arbor_asm_http_request;

typedef struct arbor_asm_request_target {
    const uint8_t *path_ptr;
    uint64_t path_len;
    const uint8_t *query_ptr;
    uint64_t query_len;
} arbor_asm_request_target;

typedef struct arbor_asm_route_param {
    const uint8_t *name_ptr;
    uint64_t name_len;
    const uint8_t *value_ptr;
    uint64_t value_len;
} arbor_asm_route_param;

typedef int64_t (*arbor_asm_route_handler)(
    const arbor_asm_http_request *request,
    void *context,
    const arbor_asm_route_param *params,
    uint64_t parameter_count);

typedef struct arbor_asm_route {
    const uint8_t *method_ptr;
    uint64_t method_len;
    const uint8_t *pattern_ptr;
    uint64_t pattern_len;
    arbor_asm_route_handler handler;
} arbor_asm_route;

/* Linux epoll_event layout used by the frozen Assembly ABI: 4 + 8 = 12. */
typedef struct __attribute__((packed)) arbor_asm_epoll_event {
    uint32_t events;
    uint64_t data;
} arbor_asm_epoll_event;

_Static_assert(sizeof(void *) == 8u, "Assembly ABI requires 64-bit pointers");
_Static_assert(sizeof(arbor_asm_result_u64) == 16u, "RAX:RDX result ABI drift");
_Static_assert(sizeof(arbor_asm_result_i64) == 16u, "RAX:RDX result ABI drift");
_Static_assert(sizeof(arbor_asm_result_ptr) == 16u, "RAX:RDX result ABI drift");
_Static_assert(sizeof(arbor_asm_span_result) == 16u, "RAX:RDX span ABI drift");
_Static_assert(sizeof(arbor_asm_match_result) == 16u, "RAX:RDX match ABI drift");

_Static_assert(sizeof(arbor_asm_buffer) == 24u, "BUFFER size ABI drift");
_Static_assert(offsetof(arbor_asm_buffer, data) == 0u, "BUFFER.data ABI drift");
_Static_assert(offsetof(arbor_asm_buffer, length) == 8u, "BUFFER.length ABI drift");
_Static_assert(offsetof(arbor_asm_buffer, capacity) == 16u, "BUFFER.capacity ABI drift");

_Static_assert(sizeof(arbor_asm_arena) == 24u, "ARENA size ABI drift");
_Static_assert(offsetof(arbor_asm_arena, base) == 0u, "ARENA.base ABI drift");
_Static_assert(offsetof(arbor_asm_arena, capacity) == 8u, "ARENA.capacity ABI drift");
_Static_assert(offsetof(arbor_asm_arena, offset) == 16u, "ARENA.offset ABI drift");

_Static_assert(sizeof(arbor_asm_connection) == 96u, "CONNECTION size ABI drift");
_Static_assert(offsetof(arbor_asm_connection, fd) == 0u, "CONNECTION.fd ABI drift");
_Static_assert(offsetof(arbor_asm_connection, state) == 8u, "CONNECTION.state ABI drift");
_Static_assert(offsetof(arbor_asm_connection, flags) == 16u, "CONNECTION.flags ABI drift");
_Static_assert(offsetof(arbor_asm_connection, input_buffer) == 24u, "CONNECTION.input_buffer ABI drift");
_Static_assert(offsetof(arbor_asm_connection, output_buffer) == 32u, "CONNECTION.output_buffer ABI drift");
_Static_assert(offsetof(arbor_asm_connection, arena) == 40u, "CONNECTION.arena ABI drift");
_Static_assert(offsetof(arbor_asm_connection, read_bytes) == 48u, "CONNECTION.read_bytes ABI drift");
_Static_assert(offsetof(arbor_asm_connection, write_bytes) == 56u, "CONNECTION.write_bytes ABI drift");
_Static_assert(offsetof(arbor_asm_connection, request_count) == 64u, "CONNECTION.request_count ABI drift");
_Static_assert(offsetof(arbor_asm_connection, last_error) == 72u, "CONNECTION.last_error ABI drift");
_Static_assert(offsetof(arbor_asm_connection, frame_scan) == 80u, "CONNECTION.frame_scan ABI drift");
_Static_assert(offsetof(arbor_asm_connection, frame_needed) == 88u, "CONNECTION.frame_needed ABI drift");

_Static_assert(sizeof(arbor_asm_http_request) == 96u, "HTTP_REQUEST size ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, method_ptr) == 0u, "HTTP_REQUEST.method_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, target_ptr) == 16u, "HTTP_REQUEST.target_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, version_ptr) == 32u, "HTTP_REQUEST.version_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, headers_ptr) == 48u, "HTTP_REQUEST.headers_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, body_ptr) == 64u, "HTTP_REQUEST.body_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, content_length) == 80u, "HTTP_REQUEST.content_length ABI drift");
_Static_assert(offsetof(arbor_asm_http_request, message_length) == 88u, "HTTP_REQUEST.message_length ABI drift");

_Static_assert(sizeof(arbor_asm_request_target) == 32u, "REQUEST_TARGET size ABI drift");
_Static_assert(offsetof(arbor_asm_request_target, path_ptr) == 0u, "REQUEST_TARGET.path_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_request_target, query_ptr) == 16u, "REQUEST_TARGET.query_ptr ABI drift");

_Static_assert(sizeof(arbor_asm_route) == 40u, "ROUTE size ABI drift");
_Static_assert(offsetof(arbor_asm_route, method_ptr) == 0u, "ROUTE.method_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_route, pattern_ptr) == 16u, "ROUTE.pattern_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_route, handler) == 32u, "ROUTE.handler ABI drift");

_Static_assert(sizeof(arbor_asm_route_param) == 32u, "ROUTE_PARAM size ABI drift");
_Static_assert(offsetof(arbor_asm_route_param, name_ptr) == 0u, "ROUTE_PARAM.name_ptr ABI drift");
_Static_assert(offsetof(arbor_asm_route_param, value_ptr) == 16u, "ROUTE_PARAM.value_ptr ABI drift");

_Static_assert(sizeof(arbor_asm_epoll_event) == 12u, "EPOLL_EVENT size ABI drift");
_Static_assert(offsetof(arbor_asm_epoll_event, events) == 0u, "EPOLL_EVENT.events ABI drift");
_Static_assert(offsetof(arbor_asm_epoll_event, data) == 4u, "EPOLL_EVENT.data ABI drift");

/* Marker used by tools/c_assembly_abi_header_verify.sh. */
#define ARBOR_ASM_EXTERN extern

/* Arena / VM. */
ARBOR_ASM_EXTERN arbor_asm_result_ptr arena_alloc(arbor_asm_arena *arena, uint64_t size);
ARBOR_ASM_EXTERN arbor_asm_result_ptr arena_alloc_aligned(arbor_asm_arena *arena, uint64_t size, uint64_t alignment);
ARBOR_ASM_EXTERN arbor_asm_result_u64 arena_init(arbor_asm_arena *arena, void *base, uint64_t capacity);
ARBOR_ASM_EXTERN uint64_t arena_mark(const arbor_asm_arena *arena);
ARBOR_ASM_EXTERN arbor_asm_result_u64 arena_reset(arbor_asm_arena *arena);
ARBOR_ASM_EXTERN arbor_asm_result_u64 arena_rewind(arbor_asm_arena *arena, uint64_t mark);
ARBOR_ASM_EXTERN intptr_t vm_map_rw(uint64_t length);
ARBOR_ASM_EXTERN int64_t vm_unmap(void *address, uint64_t length);

/* ASCII. */
ARBOR_ASM_EXTERN uint64_t ascii_is_alpha(uint64_t value);
ARBOR_ASM_EXTERN uint64_t ascii_is_digit(uint64_t value);
ARBOR_ASM_EXTERN uint64_t ascii_is_hex_digit(uint64_t value);
ARBOR_ASM_EXTERN uint64_t ascii_is_space(uint64_t value);
ARBOR_ASM_EXTERN uint64_t ascii_to_lower(uint64_t value);
ARBOR_ASM_EXTERN uint64_t ascii_to_upper(uint64_t value);

/* Base64. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 base64_decode(const void *source, uint64_t source_length, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN arbor_asm_result_u64 base64_decoded_max_length(uint64_t encoded_length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 base64_encode(const void *source, uint64_t source_length, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN arbor_asm_result_u64 base64_encoded_length(uint64_t source_length);

/* Bounded buffers. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 buffer_append(arbor_asm_buffer *buffer, const void *source, uint64_t length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 buffer_append_byte(arbor_asm_buffer *buffer, uint64_t value);
ARBOR_ASM_EXTERN arbor_asm_result_u64 buffer_consume(arbor_asm_buffer *buffer, uint64_t count);
ARBOR_ASM_EXTERN arbor_asm_result_u64 buffer_init(arbor_asm_buffer *buffer, void *data, uint64_t capacity);
ARBOR_ASM_EXTERN uint64_t buffer_length(const arbor_asm_buffer *buffer);
ARBOR_ASM_EXTERN uint64_t buffer_remaining(const arbor_asm_buffer *buffer);
ARBOR_ASM_EXTERN arbor_asm_result_u64 buffer_reset(arbor_asm_buffer *buffer);

/* Byte spans and codecs. */
ARBOR_ASM_EXTERN int64_t bytes_compare(const void *left, uint64_t left_length, const void *right, uint64_t right_length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 bytes_decode_hex(const void *source, uint64_t source_length, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN arbor_asm_result_u64 bytes_encode_hex(const void *source, uint64_t source_length, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN uint64_t bytes_ends_with(const void *span, uint64_t span_length, const void *suffix, uint64_t suffix_length);
ARBOR_ASM_EXTERN uint64_t bytes_equal(const void *left, uint64_t left_length, const void *right, uint64_t right_length);
ARBOR_ASM_EXTERN uint64_t bytes_equal_ascii_ci(const void *left, uint64_t left_length, const void *right, uint64_t right_length);
ARBOR_ASM_EXTERN const uint8_t *bytes_find(const void *haystack, uint64_t haystack_length, const void *needle, uint64_t needle_length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 bytes_parse_u64_decimal(const void *buffer, uint64_t length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 bytes_parse_u64_hex(const void *buffer, uint64_t length);
ARBOR_ASM_EXTERN uint64_t bytes_skip_ascii_space(const void *buffer, uint64_t length);
ARBOR_ASM_EXTERN uint64_t bytes_skip_byte(const void *buffer, uint64_t length, uint64_t value);
ARBOR_ASM_EXTERN uint64_t bytes_starts_with(const void *span, uint64_t span_length, const void *prefix, uint64_t prefix_length);
ARBOR_ASM_EXTERN arbor_asm_span_result bytes_trim_ascii_space(const void *buffer, uint64_t length);
ARBOR_ASM_EXTERN uint64_t cstr_length(const char *c_string);

/* Connection state. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 connection_complete_request(arbor_asm_connection *connection);
ARBOR_ASM_EXTERN arbor_asm_result_u64 connection_init(arbor_asm_connection *connection, int64_t fd, arbor_asm_buffer *input_buffer, arbor_asm_buffer *output_buffer, arbor_asm_arena *arena);
ARBOR_ASM_EXTERN arbor_asm_result_u64 connection_note_read(arbor_asm_connection *connection, uint64_t count);
ARBOR_ASM_EXTERN arbor_asm_result_u64 connection_note_write(arbor_asm_connection *connection, uint64_t count);
ARBOR_ASM_EXTERN arbor_asm_result_u64 connection_set_error(arbor_asm_connection *connection, int64_t error);
ARBOR_ASM_EXTERN uint64_t connection_state(const arbor_asm_connection *connection);
ARBOR_ASM_EXTERN arbor_asm_result_u64 connection_transition(arbor_asm_connection *connection, uint64_t new_state);

/* Event engine. */
ARBOR_ASM_EXTERN uint64_t event_deadline_remaining_ms(uint64_t deadline_ms, uint64_t now_ms);
ARBOR_ASM_EXTERN int64_t event_epoll_add(int64_t epfd, int64_t fd, uint64_t events, void *data);
ARBOR_ASM_EXTERN int64_t event_epoll_create(void);
ARBOR_ASM_EXTERN int64_t event_epoll_modify(int64_t epfd, int64_t fd, uint64_t events, void *data);
ARBOR_ASM_EXTERN int64_t event_epoll_remove(int64_t epfd, int64_t fd);
ARBOR_ASM_EXTERN int64_t event_epoll_wait(int64_t epfd, arbor_asm_epoll_event *events, uint64_t maxevents, int64_t timeout_ms);
ARBOR_ASM_EXTERN int64_t event_monotonic_ms(void);

/* HTTP. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 http_parse_request(const void *buffer, uint64_t length, arbor_asm_http_request *request_out);
ARBOR_ASM_EXTERN arbor_asm_result_u64 http_response_serialize(arbor_asm_buffer *buffer, uint64_t status, const void *body, uint64_t body_length, uint64_t keep_alive);

/* Memory / security. */
ARBOR_ASM_EXTERN int64_t memory_compare(const void *left, const void *right, uint64_t length);
ARBOR_ASM_EXTERN void *memory_copy(void *destination, const void *source, uint64_t length);
ARBOR_ASM_EXTERN uint64_t memory_equal_constant_time(const void *left, const void *right, uint64_t length);
ARBOR_ASM_EXTERN const uint8_t *memory_find_byte(const void *buffer, uint64_t value, uint64_t length);
ARBOR_ASM_EXTERN void *memory_move(void *destination, const void *source, uint64_t length);
ARBOR_ASM_EXTERN void *memory_secure_clear(void *destination, uint64_t length);
ARBOR_ASM_EXTERN void *memory_set(void *destination, uint64_t value, uint64_t length);
ARBOR_ASM_EXTERN void *memory_zero(void *destination, uint64_t length);

/* Percent codec. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 percent_decode(const void *source, uint64_t source_length, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN arbor_asm_result_u64 percent_encode(const void *source, uint64_t source_length, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN arbor_asm_result_u64 percent_encoded_length(const void *source, uint64_t source_length);

/* Range algebra. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 range_contains_point(uint64_t base, uint64_t length, uint64_t point);
ARBOR_ASM_EXTERN arbor_asm_result_u64 range_contains_span(uint64_t base, uint64_t length, uint64_t child_base, uint64_t child_length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 range_end_checked(uint64_t base, uint64_t length);
ARBOR_ASM_EXTERN arbor_asm_result_u64 range_overlaps(uint64_t base_a, uint64_t length_a, uint64_t base_b, uint64_t length_b);
ARBOR_ASM_EXTERN arbor_asm_result_u64 range_remaining(uint64_t base, uint64_t length, uint64_t position);

/* Request target. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 request_target_from_request(const arbor_asm_http_request *request, arbor_asm_request_target *out);
ARBOR_ASM_EXTERN arbor_asm_result_u64 request_target_split(const void *target, uint64_t length, arbor_asm_request_target *out);

/* Pattern routing. */
ARBOR_ASM_EXTERN int64_t route_pattern_dispatch(const arbor_asm_route *routes, uint64_t count, const arbor_asm_http_request *request, void *context, arbor_asm_route_param *params_out, uint64_t params_capacity);
ARBOR_ASM_EXTERN arbor_asm_match_result route_pattern_match(const void *pattern, uint64_t pattern_length, const void *path, uint64_t path_length, arbor_asm_route_param *params_out, uint64_t params_capacity);

/* Checked arithmetic. */
ARBOR_ASM_EXTERN arbor_asm_result_u64 s64_to_i32_checked(int64_t value);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_add_checked(uint64_t left, uint64_t right);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_align_down_checked(uint64_t value, uint64_t alignment);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_align_up_checked(uint64_t value, uint64_t alignment);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_ceil_div_checked(uint64_t value, uint64_t divisor);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_floor_div_checked(uint64_t value, uint64_t divisor);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_mul_checked(uint64_t left, uint64_t right);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_sub_checked(uint64_t left, uint64_t right);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_to_i32_nonnegative_checked(uint64_t value);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_to_u32_checked(uint64_t value);

/* Server runtime. */
ARBOR_ASM_EXTERN arbor_asm_result_i64 server_accept_connection(int64_t listener_fd, int64_t epoll_fd, arbor_asm_connection *connection, arbor_asm_buffer *input_buffer, arbor_asm_buffer *output_buffer, arbor_asm_arena *arena);
ARBOR_ASM_EXTERN arbor_asm_result_u64 server_close_connection(int64_t epoll_fd, arbor_asm_connection *connection);
ARBOR_ASM_EXTERN int64_t server_create_epoll(int64_t listener_fd);
ARBOR_ASM_EXTERN arbor_asm_result_u64 server_handle_http_once(arbor_asm_connection *connection, arbor_asm_http_request *request, const arbor_asm_route *routes, uint64_t route_count, void *context, int64_t epoll_fd);
ARBOR_ASM_EXTERN int64_t server_open_listener(const void *sockaddr, uint64_t sockaddr_length, int64_t backlog);

/* Formatting. */
ARBOR_ASM_EXTERN uint64_t u64_decimal_length(uint64_t value);
ARBOR_ASM_EXTERN uint64_t u64_hex_length(uint64_t value);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_format_decimal(uint64_t value, void *destination, uint64_t capacity);
ARBOR_ASM_EXTERN arbor_asm_result_u64 u64_format_hex(uint64_t value, void *destination, uint64_t capacity);

#undef ARBOR_ASM_EXTERN

#ifdef __cplusplus
}
#endif

#endif
