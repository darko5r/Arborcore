#include "arborcore/browser_host_v2.h"

#include "../../browser/linear16_srgb8_bucket12.h"
#include "../../renderer/srgb8_linear16_lut.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define ARBOR_CSS_DECIMAL_SCALE UINT64_C(1000000000)
#define ARBOR_Q32_DENOMINATOR UINT64_C(4294967296)

static char css_scratch[ARBOR_BROWSER_HOST_V2_CSS_BUFFER_BYTES];
static uint32_t written_scratch;
static uint32_t rgba8_output_bytes_scratch;
static uint32_t dispatch_x_scratch;
static uint32_t dispatch_y_scratch;
static char failure_scratch[ARBOR_BROWSER_HOST_V2_FAILURE_BUFFER_BYTES];
static arbor_browser_host_v2_size size_scratch;
static arbor_browser_host_v2_layout layout_scratch;
static arbor_browser_host_v2_state state_scratch;
static uint32_t bucket12_gpu[ARBOR_BROWSER_LINEAR16_BUCKET_COUNT];
static uint32_t forward_lut_gpu[256];

static uint64_t round_nearest_even_u64(uint64_t numerator, uint64_t denominator)
{
    uint64_t quotient = numerator / denominator;
    uint64_t remainder = numerator % denominator;
    uint64_t twice = remainder * UINT64_C(2);
    if (twice > denominator ||
        (twice == denominator && (quotient & UINT64_C(1)) != UINT64_C(0))) {
        quotient += UINT64_C(1);
    }
    return quotient;
}

static uint32_t decimal_digits_u64(uint64_t value)
{
    uint32_t digits = 1u;
    while (value >= UINT64_C(10)) {
        value /= UINT64_C(10);
        digits += 1u;
    }
    return digits;
}

static void write_u64_decimal(char *out, uint32_t digits, uint64_t value)
{
    uint32_t i = digits;
    while (i != 0u) {
        i -= 1u;
        out[i] = (char)('0' + (char)(value % UINT64_C(10)));
        value /= UINT64_C(10);
    }
}

static int ascii_equal(char a, char b)
{
    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    return a == b;
}

static int contains_ascii(const char *text, uint32_t text_len, const char *needle, uint32_t needle_len)
{
    uint32_t i;
    if (needle_len == 0u || text_len < needle_len) return 0;
    for (i = 0u; i <= text_len - needle_len; ++i) {
        uint32_t j;
        for (j = 0u; j < needle_len; ++j) {
            if (!ascii_equal(text[i + j], needle[j])) break;
        }
        if (j == needle_len) return 1;
    }
    return 0;
}

static arbor_browser_host_v2_status validate_layout(
    uint32_t width,
    uint32_t height,
    uint32_t stride_bytes,
    uint32_t bytes_per_pixel,
    arbor_browser_host_v2_layout *out)
{
    uint64_t tight_stride;
    uint64_t byte_length;
    if (out == (arbor_browser_host_v2_layout *)0 || width == 0u || height == 0u) {
        return ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT;
    }
    tight_stride = (uint64_t)width * (uint64_t)bytes_per_pixel;
    if (tight_stride > UINT32_MAX || stride_bytes != (uint32_t)tight_stride) {
        return ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT;
    }
    byte_length = tight_stride * (uint64_t)height;
    if (height != 0u && byte_length / (uint64_t)height != tight_stride) {
        return ARBOR_BROWSER_HOST_V2_OVERFLOW;
    }
    if (byte_length > UINT32_MAX) return ARBOR_BROWSER_HOST_V2_OVERFLOW;
    out->width = width;
    out->height = height;
    out->stride_bytes = stride_bytes;
    out->bytes_per_pixel = bytes_per_pixel;
    out->byte_length = byte_length;
    rgba8_output_bytes_scratch = width * height * 4u;
    dispatch_x_scratch = (width + 7u) / 8u;
    dispatch_y_scratch = (height + 7u) / 8u;
    return ARBOR_BROWSER_HOST_V2_OK;
}

uint32_t arbor_browser_host_v2_version(void)
{
    return ARBOR_BROWSER_HOST_V2_VERSION;
}

arbor_browser_host_v2_status arbor_browser_host_v2_format_q32_css(
    int64_t raw_value,
    char *out,
    uint32_t capacity,
    uint32_t *written)
{
    uint64_t magnitude;
    uint64_t integer_part;
    uint64_t fraction_raw;
    uint64_t fraction;
    uint32_t integer_digits;
    uint32_t negative;
    uint32_t required;
    uint32_t pos = 0u;
    uint32_t i;

    if (out == (char *)0 || written == (uint32_t *)0) {
        return ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT;
    }
    negative = raw_value < 0 ? 1u : 0u;
    if (negative != 0u) {
        magnitude = (uint64_t)(-(raw_value + INT64_C(1))) + UINT64_C(1);
    } else {
        magnitude = (uint64_t)raw_value;
    }
    integer_part = magnitude >> 32u;
    fraction_raw = magnitude & UINT64_C(0xffffffff);
    fraction = round_nearest_even_u64(
        fraction_raw * ARBOR_CSS_DECIMAL_SCALE,
        ARBOR_Q32_DENOMINATOR);
    if (fraction == ARBOR_CSS_DECIMAL_SCALE) {
        integer_part += UINT64_C(1);
        fraction = UINT64_C(0);
    }
    if (integer_part == UINT64_C(0) && fraction == UINT64_C(0)) negative = 0u;
    integer_digits = decimal_digits_u64(integer_part);
    required = negative + integer_digits + 1u + 9u + 2u;
    *written = required;
    if (capacity <= required) return ARBOR_BROWSER_HOST_V2_BUFFER_TOO_SMALL;
    if (negative != 0u) out[pos++] = '-';
    write_u64_decimal(out + pos, integer_digits, integer_part);
    pos += integer_digits;
    out[pos++] = '.';
    for (i = 0u; i < 9u; ++i) {
        uint64_t divisor = UINT64_C(100000000);
        uint32_t j;
        for (j = 0u; j < i; ++j) divisor /= UINT64_C(10);
        out[pos++] = (char)('0' + (char)((fraction / divisor) % UINT64_C(10)));
    }
    out[pos++] = 'p';
    out[pos++] = 'x';
    out[pos] = '\0';
    return ARBOR_BROWSER_HOST_V2_OK;
}

arbor_browser_host_v2_status arbor_browser_host_v2_resolve_device_size(
    uint32_t device_box_valid,
    uint32_t device_width,
    uint32_t device_height,
    double css_width,
    double css_height,
    double device_pixel_ratio,
    arbor_browser_host_v2_size *out)
{
    double scaled_width;
    double scaled_height;
    uint64_t width;
    uint64_t height;
    if (out == (arbor_browser_host_v2_size *)0) return ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT;
    if (device_box_valid != 0u && device_width != 0u && device_height != 0u) {
        out->width = device_width;
        out->height = device_height;
        out->mode = ARBOR_BROWSER_HOST_V2_SIZE_DEVICE_PIXEL_BOX;
        out->reserved = 0u;
        return ARBOR_BROWSER_HOST_V2_OK;
    }
    if (!(css_width > 0.0) || !(css_height > 0.0) || !(device_pixel_ratio > 0.0)) {
        return ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT;
    }
    scaled_width = css_width * device_pixel_ratio;
    scaled_height = css_height * device_pixel_ratio;
    if (!(scaled_width > 0.0) || !(scaled_height > 0.0) ||
        scaled_width > 4294967295.0 || scaled_height > 4294967295.0) {
        return ARBOR_BROWSER_HOST_V2_OVERFLOW;
    }
    width = (uint64_t)(scaled_width + 0.5);
    height = (uint64_t)(scaled_height + 0.5);
    if (width == 0u) width = 1u;
    if (height == 0u) height = 1u;
    out->width = (uint32_t)width;
    out->height = (uint32_t)height;
    out->mode = ARBOR_BROWSER_HOST_V2_SIZE_CONTENT_DPR;
    out->reserved = 0u;
    return ARBOR_BROWSER_HOST_V2_OK;
}

arbor_browser_host_v2_status arbor_browser_host_v2_validate_rgba8(
    uint32_t width,
    uint32_t height,
    uint32_t stride_bytes,
    arbor_browser_host_v2_layout *out)
{
    return validate_layout(width, height, stride_bytes, 4u, out);
}

arbor_browser_host_v2_status arbor_browser_host_v2_validate_rgba16(
    uint32_t width,
    uint32_t height,
    uint32_t stride_bytes,
    arbor_browser_host_v2_layout *out)
{
    return validate_layout(width, height, stride_bytes, 8u, out);
}

uint32_t arbor_browser_host_v2_classify_gpu_failure(const char *message, uint32_t message_length)
{
    static const char oom1[] = "vk_error_out_of_device_memory";
    static const char oom2[] = "gpuoutofmemoryerror";
    static const char lost[] = "device lost";
    static const char lost2[] = "device is lost";
    static const char lost3[] = "device] is lost";
    static const char unavailable[] = "requestadapter returned null";
    static const char unavailable2[] = "navigator.gpu unavailable";
    if (message == (const char *)0 && message_length != 0u) return ARBOR_BROWSER_HOST_V2_GPU_FAILURE_PROGRAMMING;
    if (message_length == 0u) return ARBOR_BROWSER_HOST_V2_GPU_FAILURE_NONE;
    if (contains_ascii(message, message_length, oom1, (uint32_t)(sizeof(oom1) - 1u)) ||
        contains_ascii(message, message_length, oom2, (uint32_t)(sizeof(oom2) - 1u)) ||
        contains_ascii(message, message_length, lost, (uint32_t)(sizeof(lost) - 1u)) ||
        contains_ascii(message, message_length, lost2, (uint32_t)(sizeof(lost2) - 1u)) ||
        contains_ascii(message, message_length, lost3, (uint32_t)(sizeof(lost3) - 1u))) {
        return ARBOR_BROWSER_HOST_V2_GPU_FAILURE_DEVICE_LOSS_OR_ALLOCATION;
    }
    if (contains_ascii(message, message_length, unavailable, (uint32_t)(sizeof(unavailable) - 1u)) ||
        contains_ascii(message, message_length, unavailable2, (uint32_t)(sizeof(unavailable2) - 1u))) {
        return ARBOR_BROWSER_HOST_V2_GPU_FAILURE_PLATFORM_UNAVAILABLE;
    }
    return ARBOR_BROWSER_HOST_V2_GPU_FAILURE_PROGRAMMING;
}

void arbor_browser_host_v2_state_init(arbor_browser_host_v2_state *state)
{
    if (state == (arbor_browser_host_v2_state *)0) return;
    state->present_mode = ARBOR_BROWSER_HOST_V2_PRESENT_FALLBACK;
    state->failure_class = ARBOR_BROWSER_HOST_V2_GPU_FAILURE_NONE;
    state->generation = 0u;
    state->reserved = 0u;
}

void arbor_browser_host_v2_state_webgpu_ready(arbor_browser_host_v2_state *state)
{
    if (state == (arbor_browser_host_v2_state *)0) return;
    state->present_mode = ARBOR_BROWSER_HOST_V2_PRESENT_WEBGPU;
    state->failure_class = ARBOR_BROWSER_HOST_V2_GPU_FAILURE_NONE;
    state->generation += 1u;
}

void arbor_browser_host_v2_state_failure(arbor_browser_host_v2_state *state, uint32_t failure_class)
{
    if (state == (arbor_browser_host_v2_state *)0) return;
    state->present_mode = ARBOR_BROWSER_HOST_V2_PRESENT_FALLBACK;
    state->failure_class = failure_class;
}

void arbor_browser_host_v2_state_destroy(arbor_browser_host_v2_state *state)
{
    if (state == (arbor_browser_host_v2_state *)0) return;
    state->present_mode = ARBOR_BROWSER_HOST_V2_PRESENT_FALLBACK;
    state->failure_class = ARBOR_BROWSER_HOST_V2_GPU_FAILURE_NONE;
    state->generation += 1u;
}

void arbor_browser_host_v2_prepare_gpu_tables(void)
{
    uint32_t i;
    for (i = 0u; i < ARBOR_BROWSER_LINEAR16_BUCKET_COUNT; ++i) {
        bucket12_gpu[i] = (uint32_t)arbor_browser_linear16_to_srgb8_bucket12[i];
    }
    for (i = 0u; i < 256u; ++i) {
        forward_lut_gpu[i] = (uint32_t)arbor_srgb8_to_linear16[i];
    }
}

uint32_t arbor_browser_host_v2_bucket12_ptr(void) { return (uint32_t)(uintptr_t)bucket12_gpu; }
uint32_t arbor_browser_host_v2_bucket12_bytes(void) { return (uint32_t)sizeof(bucket12_gpu); }
uint32_t arbor_browser_host_v2_forward_lut_ptr(void) { return (uint32_t)(uintptr_t)forward_lut_gpu; }
uint32_t arbor_browser_host_v2_forward_lut_bytes(void) { return (uint32_t)sizeof(forward_lut_gpu); }
uint32_t arbor_browser_host_v2_size_width(void) { return size_scratch.width; }
uint32_t arbor_browser_host_v2_size_height(void) { return size_scratch.height; }
uint32_t arbor_browser_host_v2_resolved_size_mode(void) { return size_scratch.mode; }
uint32_t arbor_browser_host_v2_layout_byte_length(void) { return (uint32_t)layout_scratch.byte_length; }
uint32_t arbor_browser_host_v2_rgba8_output_bytes(void) { return rgba8_output_bytes_scratch; }
uint32_t arbor_browser_host_v2_dispatch_x(void) { return dispatch_x_scratch; }
uint32_t arbor_browser_host_v2_dispatch_y(void) { return dispatch_y_scratch; }
uint32_t arbor_browser_host_v2_state_present_mode(void) { return state_scratch.present_mode; }
uint32_t arbor_browser_host_v2_state_failure_class(void) { return state_scratch.failure_class; }
uint32_t arbor_browser_host_v2_state_generation(void) { return state_scratch.generation; }
uint32_t arbor_browser_host_v2_css_scratch_ptr(void) { return (uint32_t)(uintptr_t)css_scratch; }
uint32_t arbor_browser_host_v2_written_scratch_ptr(void) { return (uint32_t)(uintptr_t)&written_scratch; }
uint32_t arbor_browser_host_v2_written_value(void) { return written_scratch; }
uint32_t arbor_browser_host_v2_css_scratch_bytes(void) { return (uint32_t)sizeof(css_scratch); }
uint32_t arbor_browser_host_v2_failure_scratch_ptr(void) { return (uint32_t)(uintptr_t)failure_scratch; }
uint32_t arbor_browser_host_v2_failure_scratch_bytes(void) { return (uint32_t)sizeof(failure_scratch); }
uint32_t arbor_browser_host_v2_size_scratch_ptr(void) { return (uint32_t)(uintptr_t)&size_scratch; }
uint32_t arbor_browser_host_v2_layout_scratch_ptr(void) { return (uint32_t)(uintptr_t)&layout_scratch; }
uint32_t arbor_browser_host_v2_state_scratch_ptr(void) { return (uint32_t)(uintptr_t)&state_scratch; }
