@group(0) @binding(0) var source_rgba16: texture_2d<u32>;
@group(0) @binding(1) var<storage, read> bucket12: array<u32>;
@group(0) @binding(2) var<storage, read> srgb8_to_linear16: array<u32>;

fn unpremultiply16(value: u32, alpha: u32) -> u32 {
    if (alpha == 0u || value == 0u) { return 0u; }
    if (value >= alpha) { return 65535u; }
    if (alpha == 65535u) { return value; }
    let numerator = value * 65535u;
    var quotient = numerator / alpha;
    let remainder = numerator % alpha;
    let twice = remainder * 2u;
    if (twice > alpha || (twice == alpha && (quotient & 1u) != 0u)) {
        quotient = quotient + 1u;
    }
    return min(quotient, 65535u);
}

fn linear16_to_srgb8(value: u32) -> u32 {
    var code = bucket12[value >> 4u];
    if (code < 255u) {
        let lower = srgb8_to_linear16[code];
        let upper = srgb8_to_linear16[code + 1u];
        let twice_value = value * 2u;
        let midpoint_sum = lower + upper;
        if (twice_value > midpoint_sum ||
            (twice_value == midpoint_sum && (code & 1u) != 0u)) {
            code = code + 1u;
        }
    }
    return code;
}

fn alpha16_to_alpha8(alpha: u32) -> u32 {
    var quotient = alpha / 257u;
    let remainder = alpha % 257u;
    let twice = remainder * 2u;
    if (twice > 257u || (twice == 257u && (quotient & 1u) != 0u)) {
        quotient = quotient + 1u;
    }
    return min(quotient, 255u);
}

fn exact_rgba16_to_rgba8(color: vec4<u32>) -> vec4<u32> {
    let alpha = color.a;
    if (alpha == 0u) {
        return vec4<u32>(0u, 0u, 0u, 0u);
    }
    if (alpha == 65535u) {
        return vec4<u32>(
            linear16_to_srgb8(color.r),
            linear16_to_srgb8(color.g),
            linear16_to_srgb8(color.b),
            255u);
    }
    return vec4<u32>(
        linear16_to_srgb8(unpremultiply16(color.r, alpha)),
        linear16_to_srgb8(unpremultiply16(color.g, alpha)),
        linear16_to_srgb8(unpremultiply16(color.b, alpha)),
        alpha16_to_alpha8(alpha));
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

@vertex
fn vertex_main(@builtin(vertex_index) index: u32) -> VertexOutput {
    var positions = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
        vec2<f32>(-1.0,  3.0)
    );
    var output: VertexOutput;
    output.position = vec4<f32>(positions[index], 0.0, 1.0);
    return output;
}

@fragment
fn fragment_main(input: VertexOutput) -> @location(0) vec4<f32> {
    let pixel = vec2<i32>(i32(input.position.x), i32(input.position.y));
    let exact = exact_rgba16_to_rgba8(textureLoad(source_rgba16, pixel, 0));
    let a = f32(exact.a) / 255.0;
    let rgb = vec3<f32>(f32(exact.r), f32(exact.g), f32(exact.b)) / 255.0;
    return vec4<f32>(rgb * a, a);
}

@group(0) @binding(3) var<storage, read_write> output_rgba8: array<u32>;

@compute @workgroup_size(8, 8, 1)
fn compute_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let dimensions = textureDimensions(source_rgba16);
    if (gid.x >= dimensions.x || gid.y >= dimensions.y) { return; }
    let exact = exact_rgba16_to_rgba8(textureLoad(
        source_rgba16, vec2<i32>(i32(gid.x), i32(gid.y)), 0));
    let packed = exact.r | (exact.g << 8u) | (exact.b << 16u) | (exact.a << 24u);
    output_rgba8[gid.y * dimensions.x + gid.x] = packed;
}
