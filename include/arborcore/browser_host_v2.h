#ifndef ARBORCORE_BROWSER_HOST_V2_H
#define ARBORCORE_BROWSER_HOST_V2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_BROWSER_HOST_V2_VERSION UINT32_C(0x00020000)
#define ARBOR_BROWSER_HOST_V2_CSS_BUFFER_BYTES UINT32_C(40)
#define ARBOR_BROWSER_HOST_V2_FAILURE_BUFFER_BYTES UINT32_C(512)

typedef enum arbor_browser_host_v2_status {
    ARBOR_BROWSER_HOST_V2_OK = 0,
    ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT = 1,
    ARBOR_BROWSER_HOST_V2_BUFFER_TOO_SMALL = 2,
    ARBOR_BROWSER_HOST_V2_OVERFLOW = 3
} arbor_browser_host_v2_status;

typedef enum arbor_browser_host_v2_size_mode {
    ARBOR_BROWSER_HOST_V2_SIZE_DEVICE_PIXEL_BOX = 1,
    ARBOR_BROWSER_HOST_V2_SIZE_CONTENT_DPR = 2
} arbor_browser_host_v2_size_mode;

typedef enum arbor_browser_host_v2_gpu_failure_class {
    ARBOR_BROWSER_HOST_V2_GPU_FAILURE_NONE = 0,
    ARBOR_BROWSER_HOST_V2_GPU_FAILURE_PLATFORM_UNAVAILABLE = 1,
    ARBOR_BROWSER_HOST_V2_GPU_FAILURE_DEVICE_LOSS_OR_ALLOCATION = 2,
    ARBOR_BROWSER_HOST_V2_GPU_FAILURE_PROGRAMMING = 3
} arbor_browser_host_v2_gpu_failure_class;

typedef enum arbor_browser_host_v2_present_mode {
    ARBOR_BROWSER_HOST_V2_PRESENT_FALLBACK = 0,
    ARBOR_BROWSER_HOST_V2_PRESENT_WEBGPU = 1
} arbor_browser_host_v2_present_mode;

typedef struct arbor_browser_host_v2_size {
    uint32_t width;
    uint32_t height;
    uint32_t mode;
    uint32_t reserved;
} arbor_browser_host_v2_size;

typedef struct arbor_browser_host_v2_layout {
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    uint32_t bytes_per_pixel;
    uint64_t byte_length;
} arbor_browser_host_v2_layout;

typedef struct arbor_browser_host_v2_state {
    uint32_t present_mode;
    uint32_t failure_class;
    uint32_t generation;
    uint32_t reserved;
} arbor_browser_host_v2_state;

uint32_t arbor_browser_host_v2_version(void);

arbor_browser_host_v2_status arbor_browser_host_v2_format_q32_css(
    int64_t raw_value,
    char *out,
    uint32_t capacity,
    uint32_t *written);

arbor_browser_host_v2_status arbor_browser_host_v2_resolve_device_size(
    uint32_t device_box_valid,
    uint32_t device_width,
    uint32_t device_height,
    double css_width,
    double css_height,
    double device_pixel_ratio,
    arbor_browser_host_v2_size *out);

arbor_browser_host_v2_status arbor_browser_host_v2_validate_rgba8(
    uint32_t width,
    uint32_t height,
    uint32_t stride_bytes,
    arbor_browser_host_v2_layout *out);

arbor_browser_host_v2_status arbor_browser_host_v2_validate_rgba16(
    uint32_t width,
    uint32_t height,
    uint32_t stride_bytes,
    arbor_browser_host_v2_layout *out);

uint32_t arbor_browser_host_v2_classify_gpu_failure(
    const char *message,
    uint32_t message_length);

void arbor_browser_host_v2_state_init(arbor_browser_host_v2_state *state);
void arbor_browser_host_v2_state_webgpu_ready(arbor_browser_host_v2_state *state);
void arbor_browser_host_v2_state_failure(
    arbor_browser_host_v2_state *state,
    uint32_t failure_class);
void arbor_browser_host_v2_state_destroy(arbor_browser_host_v2_state *state);

void arbor_browser_host_v2_prepare_gpu_tables(void);
uint32_t arbor_browser_host_v2_bucket12_ptr(void);
uint32_t arbor_browser_host_v2_bucket12_bytes(void);
uint32_t arbor_browser_host_v2_forward_lut_ptr(void);
uint32_t arbor_browser_host_v2_forward_lut_bytes(void);
uint32_t arbor_browser_host_v2_size_width(void);
uint32_t arbor_browser_host_v2_size_height(void);
uint32_t arbor_browser_host_v2_resolved_size_mode(void);
uint32_t arbor_browser_host_v2_layout_byte_length(void);
uint32_t arbor_browser_host_v2_rgba8_output_bytes(void);
uint32_t arbor_browser_host_v2_dispatch_x(void);
uint32_t arbor_browser_host_v2_dispatch_y(void);
uint32_t arbor_browser_host_v2_state_present_mode(void);
uint32_t arbor_browser_host_v2_state_failure_class(void);
uint32_t arbor_browser_host_v2_state_generation(void);
uint32_t arbor_browser_host_v2_css_scratch_ptr(void);
uint32_t arbor_browser_host_v2_written_scratch_ptr(void);
uint32_t arbor_browser_host_v2_written_value(void);
uint32_t arbor_browser_host_v2_css_scratch_bytes(void);
uint32_t arbor_browser_host_v2_failure_scratch_ptr(void);
uint32_t arbor_browser_host_v2_failure_scratch_bytes(void);
uint32_t arbor_browser_host_v2_size_scratch_ptr(void);
uint32_t arbor_browser_host_v2_layout_scratch_ptr(void);
uint32_t arbor_browser_host_v2_state_scratch_ptr(void);

#ifdef __cplusplus
}
#endif

#endif
