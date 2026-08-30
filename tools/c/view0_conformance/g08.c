#include "g08.h"

#include <lexbor/css/syntax/tokenizer.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum g08_attr_code {
    G08_ATTR_NONE = 0,
    G08_ATTR_ALT,
    G08_ATTR_ALLOW,
    G08_ATTR_ALLOWFULLSCREEN,
    G08_ATTR_AUTOPLAY,
    G08_ATTR_CONTROLS,
    G08_ATTR_COORDS,
    G08_ATTR_CROSSORIGIN,
    G08_ATTR_DATA,
    G08_ATTR_DECODING,
    G08_ATTR_DEFAULT,
    G08_ATTR_GENERATOR_ALT,
    G08_ATTR_HEIGHT,
    G08_ATTR_HREF,
    G08_ATTR_ID,
    G08_ATTR_ITEMPROP,
    G08_ATTR_KIND,
    G08_ATTR_LABEL,
    G08_ATTR_LOADING,
    G08_ATTR_LOOP,
    G08_ATTR_MEDIA,
    G08_ATTR_NAME,
    G08_ATTR_POSTER,
    G08_ATTR_PRELOAD,
    G08_ATTR_PLAYSINLINE,
    G08_ATTR_REFERRERPOLICY,
    G08_ATTR_SANDBOX,
    G08_ATTR_SHAPE,
    G08_ATTR_SIZES,
    G08_ATTR_SRC,
    G08_ATTR_SRCDOC,
    G08_ATTR_SRCSET,
    G08_ATTR_SRCLANG,
    G08_ATTR_TITLE,
    G08_ATTR_TYPE,
    G08_ATTR_USEMAP,
    G08_ATTR_WIDTH,
    G08_ATTR_MUTED,
    G08_ATTR__COUNT
} g08_attr_code;

typedef struct g08_value {
    arbor_span span;
    bool present;
} g08_value;

typedef struct g08_source_attr {
    uint64_t owner_source_offset;
    uint32_t source_offset;
    uint32_t source_length;
    uint16_t code;
    uint16_t reserved;
} g08_source_attr;

typedef struct g08_current {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t parent_standard_element_id;
    uint64_t depth;
    g08_value values[G08_ATTR__COUNT];
} g08_current;

typedef struct g08_frame {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t depth;
    bool picture_seen_img;
    bool picture_img_allows_auto_sizes;
    bool media_has_src;
    uint64_t picture_source_begin;
    bool figure_has_non_iew_caption;
    uint32_t figure_img_count;
    uint32_t figure_other_flow_count;
    uint32_t default_subtitle_caption_count;
    uint32_t default_description_count;
    uint32_t default_chapters_count;
} g08_frame;

typedef struct g08_track_summary {
    uint64_t parent_source_offset;
    arbor_span language;
    arbor_span label;
    uint8_t kind_state;
    bool language_present;
    bool label_present;
} g08_track_summary;

typedef struct g08_map_summary {
    uint64_t source_offset;
    arbor_span name;
} g08_map_summary;

typedef struct g08_usemap_summary {
    uint64_t owner_source_offset;
    uint64_t owner_source_length;
    arbor_span name;
    bool malformed;
} g08_usemap_summary;

typedef struct g08_picture_source_summary {
    uint64_t owner_source_offset;
    uint64_t owner_source_length;
    bool has_following_srcset;
    bool qualified_by_media_or_type;
    bool width_descriptor_without_sizes;
} g08_picture_source_summary;

typedef struct g08_area_summary {
    uint64_t map_source_offset;
    uint64_t owner_source_offset;
    uint64_t owner_source_length;
    arbor_span href;
    arbor_span alt;
    bool href_present;
    bool alt_present;
} g08_area_summary;

typedef struct g08_deferred_alt {
    uint64_t image_source_offset;
    uint64_t image_source_length;
    uint64_t figure_source_offset;
} g08_deferred_alt;

typedef struct g08_context {
    arbor_view0_native_v1n2_g08_anchor *anchors;
    uint64_t anchor_capacity;
    bool collect;
    arbor_view0_native_v1n2_g08_evaluation evaluation;
    g08_source_attr source_attrs[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_SOURCE_ATTRIBUTES];
    uint64_t source_attr_count;
    g08_frame frames[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_DEPTH];
    uint64_t frame_count;
    g08_track_summary tracks[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS];
    uint64_t track_count;
    g08_map_summary maps[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_MAPS];
    uint64_t map_count;
    g08_usemap_summary usemaps[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_USEMAPS];
    uint64_t usemap_count;
    g08_picture_source_summary picture_sources[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS];
    uint64_t picture_source_count;
    g08_area_summary areas[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS];
    uint64_t area_count;
    g08_deferred_alt deferred_alt[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_DEFERRED_ALT];
    uint64_t deferred_alt_count;
    g08_current current;
} g08_context;

static arbor_status evaluate_usemaps(g08_context *context);
static arbor_status evaluate_area_alternatives(g08_context *context);

static arbor_status err_status(int value) { return arbor_status_from_native(-(int64_t)value); }
static arbor_status ok_status(void) { return arbor_status_from_native(0); }

static uint8_t ascii_lower(uint8_t b) {
    return b >= (uint8_t)'A' && b <= (uint8_t)'Z'
        ? (uint8_t)(b + ((uint8_t)'a' - (uint8_t)'A')) : b;
}

static bool ascii_space(uint8_t b) {
    return b == UINT8_C(0x09) || b == UINT8_C(0x0a) || b == UINT8_C(0x0c) ||
           b == UINT8_C(0x0d) || b == UINT8_C(0x20);
}

static bool ascii_alnum(uint8_t b) {
    const uint8_t lower = ascii_lower(b);
    return (b >= (uint8_t)'0' && b <= (uint8_t)'9') ||
           (lower >= (uint8_t)'a' && lower <= (uint8_t)'z');
}

static bool ascii_hex(uint8_t b) {
    const uint8_t lower = ascii_lower(b);
    return (b >= (uint8_t)'0' && b <= (uint8_t)'9') ||
           (lower >= (uint8_t)'a' && lower <= (uint8_t)'f');
}

static bool span_eq_ci(arbor_span span, const char *literal) {
    const size_t n = strlen(literal);
    if (span.data == NULL || span.length != (uint64_t)n) return false;
    for (size_t i = 0u; i < n; ++i)
        if (ascii_lower(span.data[i]) != ascii_lower((uint8_t)literal[i])) return false;
    return true;
}

static bool span_eq_exact(arbor_span left, arbor_span right) {
    return left.length == right.length &&
        (left.length == 0u || (left.data != NULL && right.data != NULL &&
         memcmp(left.data, right.data, (size_t)left.length) == 0));
}

static bool span_eq_span_ci(arbor_span left, arbor_span right) {
    if (left.length != right.length ||
        (left.length != 0u && (left.data == NULL || right.data == NULL))) return false;
    for (uint64_t i = 0u; i < left.length; ++i)
        if (ascii_lower(left.data[i]) != ascii_lower(right.data[i])) return false;
    return true;
}

static bool contains_ascii_space(arbor_span value) {
    for (uint64_t i = 0u; i < value.length; ++i)
        if (ascii_space(value.data[i])) return true;
    return false;
}

static arbor_span trim(arbor_span span) {
    if (span.data == NULL) return (arbor_span){NULL, 0u};
    uint64_t begin = 0u, end = span.length;
    while (begin < end && ascii_space(span.data[begin])) ++begin;
    while (end > begin && ascii_space(span.data[end - 1u])) --end;
    return (arbor_span){span.data + begin, end - begin};
}

static bool valid_url(arbor_span raw) {
    const arbor_span value = trim(raw);
    if (value.data == NULL || value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t b = value.data[i];
        if (b <= UINT8_C(0x20) || b == UINT8_C(0x7f)) return false;
        if (b == (uint8_t)'%' &&
            (i + 2u >= value.length || !ascii_hex(value.data[i + 1u]) ||
             !ascii_hex(value.data[i + 2u]))) return false;
    }
    if (value.length >= 7u &&
        (span_eq_ci((arbor_span){value.data, 7u}, "http://") ||
         (value.length >= 8u && span_eq_ci((arbor_span){value.data, 8u}, "https://")))) {
        const uint64_t start = ascii_lower(value.data[4u]) == (uint8_t)'s' ? 8u : 7u;
        if (value.length == start || value.data[start] == (uint8_t)'/' ||
            value.data[start] == (uint8_t)'?' || value.data[start] == (uint8_t)'#') return false;
    }
    return true;
}

static bool mime_token_byte(uint8_t b) {
    return ascii_alnum(b) || b == (uint8_t)'!' || b == (uint8_t)'#' ||
           b == (uint8_t)'$' || b == (uint8_t)'&' || b == (uint8_t)'^' ||
           b == (uint8_t)'_' || b == (uint8_t)'-' || b == (uint8_t)'.' ||
           b == (uint8_t)'+';
}

static bool valid_mime(arbor_span raw) {
    const arbor_span value = trim(raw);
    uint64_t i = 0u;
    if (value.length == 0u) return false;
    while (i < value.length && mime_token_byte(value.data[i])) ++i;
    if (i == 0u || i >= value.length || value.data[i++] != (uint8_t)'/') return false;
    const uint64_t subtype = i;
    while (i < value.length && mime_token_byte(value.data[i])) ++i;
    if (i == subtype) return false;
    if (i == value.length) return true;
    return value.data[i] == (uint8_t)';';
}

static bool valid_target(arbor_span value) {
    if (value.data == NULL || value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i)
        if (value.data[i] == UINT8_C(0x09) || value.data[i] == UINT8_C(0x0a) ||
            value.data[i] == UINT8_C(0x0c) || value.data[i] == UINT8_C(0x0d)) return false;
    if (value.data[0] != (uint8_t)'_') return true;
    return span_eq_ci(value, "_blank") || span_eq_ci(value, "_self") ||
           span_eq_ci(value, "_parent") || span_eq_ci(value, "_top") ||
           span_eq_ci(value, "_unfencedtop");
}

static bool valid_language(arbor_span raw) {
    const arbor_span value = trim(raw);
    if (value.length == 0u || value.length > 63u || value.data[0] == (uint8_t)'-' ||
        value.data[value.length - 1u] == (uint8_t)'-') return false;
    bool segment = false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (value.data[i] == (uint8_t)'-') {
            if (!segment) return false;
            segment = false;
        } else if (ascii_alnum(value.data[i])) segment = true;
        else return false;
    }
    return segment;
}

static bool token_next(arbor_span value, uint64_t *cursor, arbor_span *token) {
    uint64_t i = *cursor;
    while (i < value.length && ascii_space(value.data[i])) ++i;
    if (i == value.length) { *cursor = i; return false; }
    const uint64_t start = i;
    while (i < value.length && !ascii_space(value.data[i])) ++i;
    *cursor = i;
    *token = (arbor_span){value.data + start, i - start};
    return true;
}

static bool token_has(arbor_span value, const char *literal) {
    uint64_t cursor = 0u;
    arbor_span token = {0};
    while (token_next(value, &cursor, &token)) if (span_eq_ci(token, literal)) return true;
    return false;
}

static bool parse_nonnegative(arbor_span raw, uint64_t *value_out);

static bool span_one_of_ci(arbor_span value, const char *const *keywords, size_t count) {
    value = trim(value);
    for (size_t i = 0u; i < count; ++i)
        if (span_eq_ci(value, keywords[i])) return true;
    return false;
}

static bool valid_boolean_value(arbor_span value, const char *attribute_name) {
    return value.length == 0u || span_eq_ci(value, attribute_name);
}

static bool valid_referrer_policy(arbor_span value) {
    static const char *const keywords[] = {
        "", "no-referrer", "no-referrer-when-downgrade", "same-origin", "origin",
        "strict-origin", "origin-when-cross-origin", "strict-origin-when-cross-origin",
        "unsafe-url"
    };
    return span_one_of_ci(value, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

static bool valid_loading(arbor_span value) {
    static const char *const keywords[] = {"eager", "lazy"};
    return span_one_of_ci(value, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

static bool valid_decoding(arbor_span value) {
    static const char *const keywords[] = {"sync", "async", "auto"};
    return span_one_of_ci(value, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

static bool valid_preload(arbor_span value) {
    static const char *const keywords[] = {"", "none", "metadata", "auto"};
    return span_one_of_ci(value, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

static bool valid_crossorigin(arbor_span value) {
    static const char *const keywords[] = {"", "anonymous", "use-credentials"};
    return span_one_of_ci(value, keywords, sizeof(keywords) / sizeof(keywords[0]));
}

static bool balanced_css_value(arbor_span raw, bool allow_top_level_comma) {
    const arbor_span value = trim(raw);
    uint32_t paren = 0u, bracket = 0u;
    uint8_t quote = 0u;
    bool escaped = false, token = false, comma_needs_token = false;
    if (value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t b = value.data[i];
        if (b == UINT8_C(0x00) || b == UINT8_C(0x7f)) return false;
        if (quote != 0u) {
            if (escaped) escaped = false;
            else if (b == (uint8_t)'\\') escaped = true;
            else if (b == quote) quote = 0u;
            else if (b == UINT8_C(0x0a) || b == UINT8_C(0x0c) || b == UINT8_C(0x0d)) return false;
            continue;
        }
        if (b == (uint8_t)'\'' || b == (uint8_t)'"') { quote = b; token = true; continue; }
        if (b == (uint8_t)'(') { if (paren == UINT32_MAX) return false; ++paren; token = true; continue; }
        if (b == (uint8_t)')') { if (paren == 0u) return false; --paren; continue; }
        if (b == (uint8_t)'[') { if (bracket == UINT32_MAX) return false; ++bracket; token = true; continue; }
        if (b == (uint8_t)']') { if (bracket == 0u) return false; --bracket; continue; }
        if (b == (uint8_t)'{' || b == (uint8_t)'}' || b == (uint8_t)';') return false;
        if (b == (uint8_t)',' && paren == 0u && bracket == 0u) {
            if (!allow_top_level_comma || !token || comma_needs_token) return false;
            comma_needs_token = true;
            token = false;
            continue;
        }
        if (!ascii_space(b)) { token = true; comma_needs_token = false; }
    }
    return quote == 0u && paren == 0u && bracket == 0u && token && !comma_needs_token;
}

#define G08_MQ_MAX_TOKENS UINT32_C(512)
#define G08_MQ_MAX_DEPTH UINT32_C(64)

typedef struct g08_mq_token {
    lxb_css_syntax_token_type_t type;
    arbor_span text;
    uint8_t text_bytes[16];
    lxb_codepoint_t delimiter;
} g08_mq_token;

typedef struct g08_mq_parser {
    const g08_mq_token *tokens;
    uint32_t end;
    uint32_t depth;
} g08_mq_parser;

static uint32_t mq_skip_ws(const g08_mq_parser *parser, uint32_t position) {
    while (position < parser->end &&
           parser->tokens[position].type == LXB_CSS_SYNTAX_TOKEN_WHITESPACE) ++position;
    return position;
}

static bool mq_ident_at(const g08_mq_parser *parser, uint32_t position, const char *keyword) {
    return position < parser->end &&
        parser->tokens[position].type == LXB_CSS_SYNTAX_TOKEN_IDENT &&
        span_eq_ci(parser->tokens[position].text, keyword);
}

static bool mq_reserved_media_type(arbor_span value) {
    return span_eq_ci(value, "only") || span_eq_ci(value, "not") ||
        span_eq_ci(value, "and") || span_eq_ci(value, "or") || span_eq_ci(value, "layer");
}

static bool mq_find_close(
    const g08_mq_parser *parser, uint32_t open, uint32_t *close_out) {
    lxb_css_syntax_token_type_t expected[G08_MQ_MAX_DEPTH];
    uint32_t depth = 0u;
    for (uint32_t i = open; i < parser->end; ++i) {
        const lxb_css_syntax_token_type_t type = parser->tokens[i].type;
        lxb_css_syntax_token_type_t close = LXB_CSS_SYNTAX_TOKEN_UNDEF;
        if (type == LXB_CSS_SYNTAX_TOKEN_FUNCTION ||
            type == LXB_CSS_SYNTAX_TOKEN_L_PARENTHESIS)
            close = LXB_CSS_SYNTAX_TOKEN_R_PARENTHESIS;
        else if (type == LXB_CSS_SYNTAX_TOKEN_LS_BRACKET)
            close = LXB_CSS_SYNTAX_TOKEN_RS_BRACKET;
        else if (type == LXB_CSS_SYNTAX_TOKEN_LC_BRACKET)
            close = LXB_CSS_SYNTAX_TOKEN_RC_BRACKET;
        if (close != LXB_CSS_SYNTAX_TOKEN_UNDEF) {
            if (depth >= G08_MQ_MAX_DEPTH) return false;
            expected[depth++] = close;
            continue;
        }
        if (type == LXB_CSS_SYNTAX_TOKEN_R_PARENTHESIS ||
            type == LXB_CSS_SYNTAX_TOKEN_RS_BRACKET ||
            type == LXB_CSS_SYNTAX_TOKEN_RC_BRACKET) {
            if (depth == 0u || expected[depth - 1u] != type) return false;
            --depth;
            if (depth == 0u) {
                *close_out = i;
                return true;
            }
        }
    }
    return false;
}

static bool mq_any_value(const g08_mq_parser *parser, uint32_t begin, uint32_t end) {
    for (uint32_t i = begin; i < end; ++i) {
        const lxb_css_syntax_token_type_t type = parser->tokens[i].type;
        if (type == LXB_CSS_SYNTAX_TOKEN_BAD_STRING ||
            type == LXB_CSS_SYNTAX_TOKEN_BAD_URL ||
            type == LXB_CSS_SYNTAX_TOKEN_SEMICOLON ||
            type == LXB_CSS_SYNTAX_TOKEN_CDO || type == LXB_CSS_SYNTAX_TOKEN_CDC)
            return false;
    }
    return true;
}

static bool mq_parse_condition(
    g08_mq_parser parser, uint32_t *position, bool allow_or);

static bool mq_parse_value(g08_mq_parser parser, uint32_t *position) {
    uint32_t p = mq_skip_ws(&parser, *position);
    if (p >= parser.end) return false;
    const lxb_css_syntax_token_type_t type = parser.tokens[p].type;
    if (type == LXB_CSS_SYNTAX_TOKEN_NUMBER ||
        type == LXB_CSS_SYNTAX_TOKEN_DIMENSION || type == LXB_CSS_SYNTAX_TOKEN_IDENT) {
        const uint32_t after_first = p + 1u;
        if (type == LXB_CSS_SYNTAX_TOKEN_NUMBER) {
            uint32_t slash = mq_skip_ws(&parser, after_first);
            if (slash < parser.end && parser.tokens[slash].type == LXB_CSS_SYNTAX_TOKEN_DELIM &&
                parser.tokens[slash].delimiter == (lxb_codepoint_t)'/') {
                uint32_t denominator = mq_skip_ws(&parser, slash + 1u);
                if (denominator < parser.end &&
                    parser.tokens[denominator].type == LXB_CSS_SYNTAX_TOKEN_NUMBER) {
                    *position = denominator + 1u;
                    return true;
                }
            }
        }
        *position = after_first;
        return true;
    }
    return false;
}

static bool mq_parse_comparison(
    g08_mq_parser parser, uint32_t *position, int *direction_out) {
    uint32_t p = mq_skip_ws(&parser, *position);
    if (p >= parser.end || parser.tokens[p].type != LXB_CSS_SYNTAX_TOKEN_DELIM)
        return false;
    const lxb_codepoint_t delimiter = parser.tokens[p].delimiter;
    if (delimiter == (lxb_codepoint_t)'=') {
        *direction_out = 0;
        *position = p + 1u;
        return true;
    }
    if (delimiter != (lxb_codepoint_t)'<' && delimiter != (lxb_codepoint_t)'>') return false;
    *direction_out = delimiter == (lxb_codepoint_t)'<' ? -1 : 1;
    ++p;
    if (p < parser.end && parser.tokens[p].type == LXB_CSS_SYNTAX_TOKEN_DELIM &&
        parser.tokens[p].delimiter == (lxb_codepoint_t)'=') ++p;
    *position = p;
    return true;
}

static bool mq_parse_feature(g08_mq_parser parser, uint32_t *position) {
    uint32_t p = mq_skip_ws(&parser, *position);
    const uint32_t begin = p;
    if (p < parser.end && parser.tokens[p].type == LXB_CSS_SYNTAX_TOKEN_IDENT) {
        ++p;
        uint32_t next = mq_skip_ws(&parser, p);
        if (next == parser.end) { *position = p; return true; }
        if (parser.tokens[next].type == LXB_CSS_SYNTAX_TOKEN_COLON) {
            p = next + 1u;
            if (!mq_parse_value(parser, &p) || mq_skip_ws(&parser, p) != parser.end) return false;
            *position = p;
            return true;
        }
        p = begin;
        uint32_t name = mq_skip_ws(&parser, p);
        p = name + 1u;
        int direction = 0;
        if (mq_parse_comparison(parser, &p, &direction)) {
            (void)direction;
            if (mq_parse_value(parser, &p) && mq_skip_ws(&parser, p) == parser.end) {
                *position = p;
                return true;
            }
        }
    }

    p = begin;
    if (!mq_parse_value(parser, &p)) return false;
    int first_direction = 0;
    if (!mq_parse_comparison(parser, &p, &first_direction)) return false;
    uint32_t name = mq_skip_ws(&parser, p);
    if (name >= parser.end || parser.tokens[name].type != LXB_CSS_SYNTAX_TOKEN_IDENT) return false;
    p = name + 1u;
    if (mq_skip_ws(&parser, p) == parser.end) {
        *position = p;
        return true;
    }
    if (first_direction == 0) return false;
    int second_direction = 0;
    if (!mq_parse_comparison(parser, &p, &second_direction) ||
        second_direction != first_direction || !mq_parse_value(parser, &p) ||
        mq_skip_ws(&parser, p) != parser.end) return false;
    *position = p;
    return true;
}

static bool mq_parse_in_parens(g08_mq_parser parser, uint32_t *position) {
    uint32_t p = mq_skip_ws(&parser, *position);
    if (p >= parser.end || parser.depth >= G08_MQ_MAX_DEPTH) return false;
    const lxb_css_syntax_token_type_t type = parser.tokens[p].type;
    if (type != LXB_CSS_SYNTAX_TOKEN_FUNCTION &&
        type != LXB_CSS_SYNTAX_TOKEN_L_PARENTHESIS) return false;
    uint32_t close = 0u;
    if (!mq_find_close(&parser, p, &close)) return false;
    if (type == LXB_CSS_SYNTAX_TOKEN_FUNCTION) {
        if (!mq_any_value(&parser, p + 1u, close)) return false;
        *position = close + 1u;
        return true;
    }

    g08_mq_parser inner = {
        .tokens = parser.tokens,
        .end = close,
        .depth = parser.depth + 1u
    };
    uint32_t inside = p + 1u;
    uint32_t candidate = inside;
    if (mq_parse_condition(inner, &candidate, true) &&
        mq_skip_ws(&inner, candidate) == inner.end) {
        *position = close + 1u;
        return true;
    }
    candidate = inside;
    if (mq_parse_feature(inner, &candidate) && mq_skip_ws(&inner, candidate) == inner.end) {
        *position = close + 1u;
        return true;
    }
    if (!mq_any_value(&inner, inside, close)) return false;
    *position = close + 1u;
    return true;
}

static bool mq_parse_condition(
    g08_mq_parser parser, uint32_t *position, bool allow_or) {
    uint32_t p = mq_skip_ws(&parser, *position);
    if (mq_ident_at(&parser, p, "not")) {
        ++p;
        if (!mq_parse_in_parens(parser, &p)) return false;
        *position = p;
        return true;
    }
    if (!mq_parse_in_parens(parser, &p)) return false;
    int connector = 0;
    for (;;) {
        uint32_t keyword = mq_skip_ws(&parser, p);
        int next_connector = 0;
        if (mq_ident_at(&parser, keyword, "and")) next_connector = 1;
        else if (allow_or && mq_ident_at(&parser, keyword, "or")) next_connector = 2;
        else break;
        if (connector != 0 && connector != next_connector) return false;
        connector = next_connector;
        p = keyword + 1u;
        if (!mq_parse_in_parens(parser, &p)) return false;
    }
    *position = p;
    return true;
}

static bool mq_parse_query(g08_mq_parser parser, uint32_t begin) {
    uint32_t p = begin;
    uint32_t condition = p;
    if (mq_parse_condition(parser, &condition, true) &&
        mq_skip_ws(&parser, condition) == parser.end) return true;

    p = mq_skip_ws(&parser, begin);
    if (mq_ident_at(&parser, p, "not") || mq_ident_at(&parser, p, "only"))
        p = mq_skip_ws(&parser, p + 1u);
    if (p >= parser.end || parser.tokens[p].type != LXB_CSS_SYNTAX_TOKEN_IDENT ||
        mq_reserved_media_type(parser.tokens[p].text)) return false;
    p = mq_skip_ws(&parser, p + 1u);
    if (p == parser.end) return true;
    if (!mq_ident_at(&parser, p, "and")) return false;
    p += 1u;
    if (!mq_parse_condition(parser, &p, false)) return false;
    return mq_skip_ws(&parser, p) == parser.end;
}

static arbor_status valid_media_query_list(arbor_span value, bool *valid_out) {
    g08_mq_token tokens[G08_MQ_MAX_TOKENS];
    uint32_t token_count = 0u;
    lxb_css_syntax_tokenizer_t *tokenizer = NULL;
    if (valid_out == NULL || (value.length != 0u && value.data == NULL)) return err_status(EINVAL);
    *valid_out = false;
    if (value.length == 0u) {
        *valid_out = true;
        return ok_status();
    }
    tokenizer = lxb_css_syntax_tokenizer_create();
    if (tokenizer == NULL) return err_status(ENOMEM);
    if (lxb_css_syntax_tokenizer_init(tokenizer) != LXB_STATUS_OK) {
        (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
        return err_status(ENOMEM);
    }
    lxb_css_syntax_tokenizer_buffer_set(tokenizer, value.data, (size_t)value.length);
    for (;;) {
        lxb_css_syntax_token_t *token = lxb_css_syntax_token_next(tokenizer);
        if (token == NULL) {
            (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
            return err_status(EIO);
        }
        if (token->type == LXB_CSS_SYNTAX_TOKEN__EOF) break;
        if (token_count >= G08_MQ_MAX_TOKENS) {
            (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
            return err_status(ENOSPC);
        }
        g08_mq_token *out = tokens + token_count++;
        *out = (g08_mq_token){.type = token->type};
        if (token->type == LXB_CSS_SYNTAX_TOKEN_IDENT &&
            token->types.ident.length <= sizeof(out->text_bytes)) {
            (void)memcpy(out->text_bytes, token->types.ident.data,
                token->types.ident.length);
            out->text = (arbor_span){out->text_bytes, token->types.ident.length};
        } else if (token->type == LXB_CSS_SYNTAX_TOKEN_DELIM)
            out->delimiter = token->types.delim.character;
    }

    g08_mq_parser full = {.tokens = tokens, .end = token_count, .depth = 0u};
    uint32_t begin = 0u;
    uint32_t nesting = 0u;
    for (uint32_t i = 0u; i <= token_count; ++i) {
        const bool at_end = i == token_count;
        if (!at_end) {
            const lxb_css_syntax_token_type_t type = tokens[i].type;
            if (type == LXB_CSS_SYNTAX_TOKEN_BAD_STRING ||
                type == LXB_CSS_SYNTAX_TOKEN_BAD_URL) {
                (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
                return ok_status();
            }
            if (type == LXB_CSS_SYNTAX_TOKEN_FUNCTION ||
                type == LXB_CSS_SYNTAX_TOKEN_L_PARENTHESIS ||
                type == LXB_CSS_SYNTAX_TOKEN_LS_BRACKET ||
                type == LXB_CSS_SYNTAX_TOKEN_LC_BRACKET) {
                if (nesting >= G08_MQ_MAX_DEPTH) {
                    (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
                    return err_status(ENOSPC);
                }
                ++nesting;
            } else if (type == LXB_CSS_SYNTAX_TOKEN_R_PARENTHESIS ||
                       type == LXB_CSS_SYNTAX_TOKEN_RS_BRACKET ||
                       type == LXB_CSS_SYNTAX_TOKEN_RC_BRACKET) {
                if (nesting == 0u) {
                    (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
                    return ok_status();
                }
                --nesting;
            }
        }
        const bool separator = !at_end && nesting == 0u &&
            tokens[i].type == LXB_CSS_SYNTAX_TOKEN_COMMA;
        if (!at_end && !separator) continue;
        g08_mq_parser entry = full;
        entry.end = i;
        const uint32_t nonspace = mq_skip_ws(&entry, begin);
        if (nonspace == entry.end) {
            if (!(at_end && begin == 0u)) {
                (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
                return ok_status();
            }
        } else if (!mq_parse_query(entry, begin)) {
            (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
            return ok_status();
        }
        begin = i + 1u;
    }
    if (nesting != 0u) {
        (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
        return ok_status();
    }
    *valid_out = true;
    (void)lxb_css_syntax_tokenizer_destroy(tokenizer);
    return ok_status();
}

typedef struct g08_srcset_result {
    bool has_width;
    bool has_density;
} g08_srcset_result;

static bool parse_positive_decimal(arbor_span value) {
    bool digit = false, dot = false, nonzero = false;
    if (value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t b = value.data[i];
        if (b >= (uint8_t)'0' && b <= (uint8_t)'9') {
            digit = true;
            if (b != (uint8_t)'0') nonzero = true;
        } else if (b == (uint8_t)'.' && !dot) dot = true;
        else return false;
    }
    return digit && nonzero;
}

static bool parse_density_ratio(arbor_span value, uint64_t *numerator_out, uint64_t *scale_out) {
    uint64_t numerator = 0u, scale = 1u;
    bool digit = false, decimal = false, fractional = false;
    if (value.length == 0u || numerator_out == NULL || scale_out == NULL) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t b = value.data[i];
        if (b >= (uint8_t)'0' && b <= (uint8_t)'9') {
            const uint64_t n = (uint64_t)(b - (uint8_t)'0');
            if (numerator > (UINT64_MAX - n) / 10u) return false;
            numerator = numerator * 10u + n;
            if (decimal) {
                if (scale > UINT64_MAX / 10u) return false;
                scale *= 10u;
                fractional = true;
            }
            digit = true;
        } else if (b == (uint8_t)'.' && !decimal) decimal = true;
        else return false;
    }
    if (!digit || numerator == 0u || (decimal && !fractional)) return false;
    while (scale > 1u && numerator % 10u == 0u) {
        numerator /= 10u;
        scale /= 10u;
    }
    *numerator_out = numerator;
    *scale_out = scale;
    return true;
}

static bool valid_srcset(arbor_span raw, g08_srcset_result *result_out) {
    arbor_span value = trim(raw);
    uint64_t i = 0u, candidates = 0u;
    uint64_t width_values[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS] = {0};
    uint64_t density_numerators[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS] = {0};
    uint64_t density_scales[ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS] = {0};
    uint64_t width_count = 0u, density_count = 0u;
    g08_srcset_result result = {0};
    if (value.length == 0u || result_out == NULL) return false;
    while (i < value.length) {
        while (i < value.length && ascii_space(value.data[i])) ++i;
        if (i == value.length || value.data[i] == (uint8_t)',') return false;
        const uint64_t url_begin = i;
        while (i < value.length && !ascii_space(value.data[i])) ++i;
        uint64_t url_end = i;
        uint64_t trailing_commas = 0u;
        while (url_end > url_begin && value.data[url_end - 1u] == (uint8_t)',') {
            --url_end; ++trailing_commas;
        }
        if (url_end == url_begin || !valid_url((arbor_span){value.data + url_begin, url_end - url_begin}))
            return false;
        bool width = false, density = false;
        uint64_t descriptor_number = 0u, density_scale = 1u;
        if (trailing_commas == 0u) {
            while (i < value.length && ascii_space(value.data[i])) ++i;
            if (i < value.length && value.data[i] != (uint8_t)',') {
                const uint64_t descriptor_begin = i;
                while (i < value.length && !ascii_space(value.data[i]) && value.data[i] != (uint8_t)',') ++i;
                const arbor_span descriptor = {value.data + descriptor_begin, i - descriptor_begin};
                if (descriptor.length < 2u) return false;
                const uint8_t suffix = ascii_lower(descriptor.data[descriptor.length - 1u]);
                const arbor_span number = {descriptor.data, descriptor.length - 1u};
                if (suffix == (uint8_t)'w') {
                    if (!parse_nonnegative(number, &descriptor_number) || descriptor_number == 0u)
                        return false;
                    width = true;
                } else if (suffix == (uint8_t)'x') {
                    if (!parse_positive_decimal(number) ||
                        !parse_density_ratio(number, &descriptor_number, &density_scale)) return false;
                    density = true;
                } else return false;
                while (i < value.length && ascii_space(value.data[i])) ++i;
                if (i < value.length && value.data[i] != (uint8_t)',') return false;
            } else { density = true; descriptor_number = 1u; }
        } else {
            if (trailing_commas != 1u) return false;
            density = true; descriptor_number = 1u;
        }
        if ((result.has_width && !width) || (result.has_density && !density) || (width && density))
            return false;
        result.has_width = result.has_width || width;
        result.has_density = result.has_density || density;
        if (width) {
            if (width_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS) return false;
            for (uint64_t j = 0u; j < width_count; ++j)
                if (width_values[j] == descriptor_number) return false;
            width_values[width_count++] = descriptor_number;
        } else {
            if (density_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS) return false;
            for (uint64_t j = 0u; j < density_count; ++j)
                if (density_numerators[j] == descriptor_number &&
                    density_scales[j] == density_scale) return false;
            density_numerators[density_count] = descriptor_number;
            density_scales[density_count++] = density_scale;
        }
        if (candidates == UINT64_MAX) return false;
        ++candidates;
        if (trailing_commas == 0u && i < value.length) {
            if (value.data[i] != (uint8_t)',') return false;
            ++i;
        }
    }
    *result_out = result;
    return candidates != 0u;
}

static bool css_length_marker(arbor_span value) {
    value = trim(value);
    if (value.length == 1u && value.data[0] == (uint8_t)'0') return true;
    static const char *const functions[] = {"calc(", "min(", "max(", "clamp("};
    for (size_t f = 0u; f < sizeof(functions) / sizeof(functions[0]); ++f) {
        const size_t n = strlen(functions[f]);
        if (value.length >= (uint64_t)n &&
            span_eq_ci((arbor_span){value.data, (uint64_t)n}, functions[f])) return true;
    }
    static const char *const units[] = {
        "px", "vw", "vh", "vmin", "vmax", "em", "rem", "ch", "ex", "cm", "mm", "q", "in", "pt", "pc"
    };
    for (size_t u = 0u; u < sizeof(units) / sizeof(units[0]); ++u) {
        const size_t n = strlen(units[u]);
        if (value.length > (uint64_t)n &&
            span_eq_ci((arbor_span){value.data + value.length - (uint64_t)n, (uint64_t)n}, units[u]))
            return true;
    }
    return false;
}

static bool valid_sizes(arbor_span raw, bool loading_lazy) {
    const arbor_span value = trim(raw);
    uint64_t begin = 0u;
    uint32_t paren = 0u;
    bool saw = false;
    if (!balanced_css_value(value, true)) return false;
    for (uint64_t i = 0u; i <= value.length; ++i) {
        const bool end = i == value.length;
        const uint8_t b = end ? (uint8_t)',' : value.data[i];
        if (!end && b == (uint8_t)'(') ++paren;
        else if (!end && b == (uint8_t)')') --paren;
        if ((end || (b == (uint8_t)',' && paren == 0u))) {
            arbor_span component = trim((arbor_span){value.data + begin, i - begin});
            if (component.length == 0u) return false;
            if (span_eq_ci(component, "auto")) {
                if (!loading_lazy) return false;
            } else {
                bool marker = css_length_marker(component);
                if (!marker) {
                    for (uint64_t j = 0u; j < component.length; ++j) {
                        if (ascii_space(component.data[j])) {
                            const arbor_span tail = trim((arbor_span){component.data + j + 1u,
                                component.length - j - 1u});
                            if (css_length_marker(tail)) marker = true;
                        }
                    }
                }
                if (!marker) return false;
            }
            saw = true;
            begin = i + 1u;
        }
    }
    return saw;
}

static bool allows_auto_sizes(arbor_span loading, bool loading_present, arbor_span sizes) {
    if (!loading_present || !span_eq_ci(trim(loading), "lazy") || sizes.data == NULL)
        return false;
    const bool prefix = span_eq_ci(sizes, "auto") || (sizes.length >= 5u &&
        span_eq_ci((arbor_span){sizes.data, 5u}, "auto,"));
    return prefix && valid_sizes(sizes, true);
}

static bool sandbox_token_allowed(arbor_span token) {
    static const char *const allowed[] = {
        "allow-downloads", "allow-forms", "allow-modals", "allow-orientation-lock",
        "allow-pointer-lock", "allow-popups", "allow-popups-to-escape-sandbox",
        "allow-presentation", "allow-same-origin", "allow-scripts",
        "allow-top-navigation", "allow-top-navigation-by-user-activation",
        "allow-top-navigation-to-custom-protocols"
    };
    return span_one_of_ci(token, allowed, sizeof(allowed) / sizeof(allowed[0]));
}

static bool valid_sandbox(arbor_span value) {
    arbor_span tokens[13] = {{0}};
    uint64_t cursor = 0u, count = 0u;
    arbor_span token = {0};
    while (token_next(value, &cursor, &token)) {
        if (!sandbox_token_allowed(token) || count >= 13u) return false;
        for (uint64_t i = 0u; i < count; ++i)
            if (span_eq_span_ci(tokens[i], token)) return false;
        tokens[count++] = token;
    }
    return true;
}

static bool valid_scheme_part(arbor_span value) {
    if (value.length == 0u || !((ascii_lower(value.data[0]) >= (uint8_t)'a') &&
        (ascii_lower(value.data[0]) <= (uint8_t)'z'))) return false;
    for (uint64_t i = 1u; i < value.length; ++i) {
        const uint8_t b = value.data[i];
        if (!ascii_alnum(b) && b != (uint8_t)'+' && b != (uint8_t)'-' && b != (uint8_t)'.')
            return false;
    }
    return true;
}

static bool valid_csp_host_part(arbor_span host) {
    uint64_t i = 0u;
    if (span_eq_ci(host, "*")) return true;
    if (host.length >= 2u && host.data[0] == (uint8_t)'*' && host.data[1] == (uint8_t)'.')
        i = 2u;
    if (i == host.length) return false;
    bool label = false;
    for (; i < host.length; ++i) {
        const uint8_t b = host.data[i];
        if (b == (uint8_t)'.') {
            if (!label) return false;
            label = false;
        } else if (ascii_alnum(b) || b == (uint8_t)'-') label = true;
        else return false;
    }
    return label || host.data[host.length - 1u] == (uint8_t)'.';
}

static bool valid_permissions_source_expression(arbor_span token) {
    if (token.length < 2u || token.data == NULL) return false;
    if (token.data[token.length - 1u] == (uint8_t)':')
        return valid_scheme_part((arbor_span){token.data, token.length - 1u});

    uint64_t cursor = 0u;
    for (uint64_t i = 0u; i + 2u < token.length; ++i) {
        if (token.data[i] == (uint8_t)':' && token.data[i + 1u] == (uint8_t)'/' &&
            token.data[i + 2u] == (uint8_t)'/') {
            if (!valid_scheme_part((arbor_span){token.data, i})) return false;
            cursor = i + 3u;
            break;
        }
    }
    const uint64_t host_begin = cursor;
    while (cursor < token.length && token.data[cursor] != (uint8_t)':' &&
           token.data[cursor] != (uint8_t)'/') ++cursor;
    if (!valid_csp_host_part((arbor_span){token.data + host_begin, cursor - host_begin}))
        return false;
    if (cursor < token.length && token.data[cursor] == (uint8_t)':') {
        ++cursor;
        const uint64_t port_begin = cursor;
        if (cursor < token.length && token.data[cursor] == (uint8_t)'*') ++cursor;
        else while (cursor < token.length && token.data[cursor] >= (uint8_t)'0' &&
                    token.data[cursor] <= (uint8_t)'9') ++cursor;
        if (cursor == port_begin || (cursor - port_begin == 1u &&
            token.data[port_begin] == (uint8_t)'*' && cursor < token.length &&
            token.data[cursor] != (uint8_t)'/')) return false;
    }
    if (cursor < token.length) {
        if (token.data[cursor] != (uint8_t)'/') return false;
        for (; cursor < token.length; ++cursor) {
            const uint8_t b = token.data[cursor];
            if (ascii_space(b) || b <= UINT8_C(0x20) || b == UINT8_C(0x7f) ||
                b == (uint8_t)';' || b == (uint8_t)',' || b == (uint8_t)'?' ||
                b == (uint8_t)'#' || b == (uint8_t)'\\' || b == (uint8_t)'"' ||
                b == (uint8_t)'[' || b == (uint8_t)']') return false;
            if (b == (uint8_t)'%') {
                if (cursor + 2u >= token.length ||
                    !((token.data[cursor + 1u] >= (uint8_t)'0' &&
                       token.data[cursor + 1u] <= (uint8_t)'9') ||
                      (ascii_lower(token.data[cursor + 1u]) >= (uint8_t)'a' &&
                       ascii_lower(token.data[cursor + 1u]) <= (uint8_t)'f')) ||
                    !((token.data[cursor + 2u] >= (uint8_t)'0' &&
                       token.data[cursor + 2u] <= (uint8_t)'9') ||
                      (ascii_lower(token.data[cursor + 2u]) >= (uint8_t)'a' &&
                       ascii_lower(token.data[cursor + 2u]) <= (uint8_t)'f')))
                    return false;
                cursor += 2u;
            }
        }
    }
    return cursor == token.length;
}

static bool valid_permissions_policy(arbor_span raw) {
    const arbor_span value = trim(raw);
    uint64_t begin = 0u;
    if (value.length == 0u) return true;
    for (uint64_t i = 0u; i <= value.length; ++i) {
        if (i != value.length && value.data[i] != (uint8_t)';') continue;
        const arbor_span declaration = trim((arbor_span){value.data + begin, i - begin});
        if (declaration.length == 0u) return false;
        uint64_t cursor = 0u;
        while (cursor < declaration.length && !ascii_space(declaration.data[cursor])) {
            const uint8_t b = declaration.data[cursor];
            if (!ascii_alnum(b) && b != (uint8_t)'-') return false;
            ++cursor;
        }
        if (cursor == 0u) return false;
        arbor_span token = {0};
        while (token_next(declaration, &cursor, &token)) {
            const bool keyword = span_eq_ci(token, "*") || span_eq_ci(token, "'self'") ||
                span_eq_ci(token, "'src'") || span_eq_ci(token, "'none'");
            if (!keyword && !valid_permissions_source_expression(token)) return false;
        }
        begin = i + 1u;
    }
    return true;
}

enum {
    G08_TRACK_SUBTITLES = 1,
    G08_TRACK_CAPTIONS = 2,
    G08_TRACK_DESCRIPTIONS = 3,
    G08_TRACK_CHAPTERS = 4,
    G08_TRACK_METADATA = 5
};

static uint8_t track_kind_state(arbor_span kind, bool present) {
    if (!present || span_eq_ci(trim(kind), "subtitles")) return G08_TRACK_SUBTITLES;
    kind = trim(kind);
    if (span_eq_ci(kind, "captions")) return G08_TRACK_CAPTIONS;
    if (span_eq_ci(kind, "descriptions")) return G08_TRACK_DESCRIPTIONS;
    if (span_eq_ci(kind, "chapters")) return G08_TRACK_CHAPTERS;
    return G08_TRACK_METADATA;
}

static g08_attr_code attr_code(arbor_span name) {
    static const struct { const char *name; g08_attr_code code; } attrs[] = {
        {"alt", G08_ATTR_ALT}, {"allow", G08_ATTR_ALLOW},
        {"allowfullscreen", G08_ATTR_ALLOWFULLSCREEN}, {"autoplay", G08_ATTR_AUTOPLAY},
        {"controls", G08_ATTR_CONTROLS}, {"coords", G08_ATTR_COORDS},
        {"crossorigin", G08_ATTR_CROSSORIGIN},
        {"data", G08_ATTR_DATA}, {"default", G08_ATTR_DEFAULT},
        {"decoding", G08_ATTR_DECODING},
        {"generator-unable-to-provide-required-alt", G08_ATTR_GENERATOR_ALT},
        {"height", G08_ATTR_HEIGHT}, {"href", G08_ATTR_HREF}, {"id", G08_ATTR_ID},
        {"itemprop", G08_ATTR_ITEMPROP}, {"kind", G08_ATTR_KIND},
        {"label", G08_ATTR_LABEL}, {"loading", G08_ATTR_LOADING}, {"loop", G08_ATTR_LOOP},
        {"media", G08_ATTR_MEDIA}, {"name", G08_ATTR_NAME},
        {"poster", G08_ATTR_POSTER}, {"preload", G08_ATTR_PRELOAD},
        {"playsinline", G08_ATTR_PLAYSINLINE}, {"referrerpolicy", G08_ATTR_REFERRERPOLICY},
        {"sandbox", G08_ATTR_SANDBOX}, {"shape", G08_ATTR_SHAPE},
        {"sizes", G08_ATTR_SIZES}, {"src", G08_ATTR_SRC},
        {"srcdoc", G08_ATTR_SRCDOC}, {"srcset", G08_ATTR_SRCSET},
        {"srclang", G08_ATTR_SRCLANG}, {"title", G08_ATTR_TITLE},
        {"type", G08_ATTR_TYPE}, {"usemap", G08_ATTR_USEMAP},
        {"width", G08_ATTR_WIDTH}, {"muted", G08_ATTR_MUTED}
    };
    for (size_t i = 0u; i < sizeof(attrs) / sizeof(attrs[0]); ++i)
        if (span_eq_ci(name, attrs[i].name)) return attrs[i].code;
    return G08_ATTR_NONE;
}

static const g08_source_attr *source_attr_for(
    const g08_context *context, uint64_t owner, g08_attr_code code) {
    for (uint64_t i = 0u; i < context->source_attr_count; ++i)
        if (context->source_attrs[i].owner_source_offset == owner &&
            context->source_attrs[i].code == (uint16_t)code) return context->source_attrs + i;
    return NULL;
}

static arbor_status emit_at(
    g08_context *context, uint16_t rule, uint64_t owner_offset,
    uint64_t owner_length, g08_attr_code code) {
    if (rule == 0u || rule > ARBOR_VIEW0_NATIVE_V1N2_G08_RULE_COUNT) return err_status(EIO);
    if (context->evaluation.diagnostic_count == UINT64_MAX ||
        context->evaluation.rule_violation_count[rule - 1u] == UINT64_MAX) return err_status(EOVERFLOW);
    if (context->collect) {
        if (context->evaluation.diagnostic_count >= context->anchor_capacity) return err_status(ENOSPC);
        uint64_t offset = owner_offset, length = owner_length;
        const g08_source_attr *attribute = source_attr_for(context, owner_offset, code);
        if (attribute != NULL) { offset = attribute->source_offset; length = attribute->source_length; }
        if (offset > UINT32_MAX || length > UINT32_MAX) return err_status(EIO);
        context->anchors[context->evaluation.diagnostic_count].shared =
            (arbor_view0_native_v1n2_anchor){
                .byte_offset = offset, .source_length = length,
                .discovery_sequence = context->evaluation.diagnostic_count,
                .subject_index = owner_offset, .group_ordinal = UINT16_C(2),
                .rule_ordinal = rule,
                .kind = attribute == NULL ? ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT
                                          : ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME
            };
    }
    context->evaluation.diagnostic_count += 1u;
    context->evaluation.rule_violation_count[rule - 1u] += 1u;
    return ok_status();
}

static arbor_status source_attribute(
    void *opaque, const arbor_view0_native_source_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    const g08_attr_code code = attr_code(observation->local_name);
    if (code == G08_ATTR_NONE) return ok_status();
    if (observation->owner_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset > UINT32_MAX || observation->source_length > UINT32_MAX)
        return err_status(EIO);
    if (source_attr_for(context, observation->owner_source_offset, code) != NULL) return ok_status();
    if (context->source_attr_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_SOURCE_ATTRIBUTES)
        return err_status(ENOSPC);
    context->source_attrs[context->source_attr_count++] = (g08_source_attr){
        .owner_source_offset = observation->owner_source_offset,
        .source_offset = (uint32_t)observation->source_offset,
        .source_length = (uint32_t)observation->source_length,
        .code = (uint16_t)code
    };
    return ok_status();
}

static arbor_status traversal_enter(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    if (observation->depth != context->frame_count ||
        context->frame_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_DEPTH) return err_status(ENOSPC);
    context->frames[context->frame_count++] = (g08_frame){
        .standard_element_id = observation->standard_element_id,
        .source_offset = observation->source_offset,
        .source_length = observation->source_length,
        .depth = observation->depth,
        .picture_source_begin = context->picture_source_count
    };
    return ok_status();
}

static arbor_status element_begin(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    (void)memset(&context->current, 0, sizeof(context->current));
    context->current.standard_element_id = observation->standard_element_id;
    context->current.namespace_id = observation->namespace_id;
    context->current.source_offset = observation->source_offset;
    context->current.source_length = observation->source_length;
    context->current.parent_standard_element_id = observation->parent_standard_element_id;
    context->current.depth = observation->depth;
    return ok_status();
}

static arbor_status attribute(
    void *opaque, const arbor_view0_native_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    const g08_attr_code code = attr_code(observation->local_name);
    if (code != G08_ATTR_NONE && !context->current.values[code].present)
        context->current.values[code] = (g08_value){observation->value, true};
    return ok_status();
}

static bool non_iew(arbor_span text) {
    for (uint64_t i = 0u; i < text.length; ++i) if (!ascii_space(text.data[i])) return true;
    return false;
}

static arbor_status direct_child(
    void *opaque, const arbor_view0_native_direct_child_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    g08_frame *frame = context->frames + context->frame_count - 1u;
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE) {
        if (observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT) {
            if (non_iew(observation->text)) frame->figure_other_flow_count += 1u;
        } else if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG) {
            frame->figure_img_count += 1u;
        } else if (observation->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION) {
            frame->figure_other_flow_count += 1u;
        }
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION &&
        context->frame_count >= 2u) {
        g08_frame *parent = context->frames + context->frame_count - 2u;
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE &&
            ((observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT && non_iew(observation->text)) ||
             observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT))
            parent->figure_has_non_iew_caption = true;
    }
    return ok_status();
}

static bool parse_nonnegative(arbor_span raw, uint64_t *value_out) {
    const arbor_span text = raw;
    uint64_t parsed = 0u;
    if (text.length == 0u) return false;
    for (uint64_t i = 0u; i < text.length; ++i) {
        if (text.data[i] < (uint8_t)'0' || text.data[i] > (uint8_t)'9') return false;
        const uint64_t digit = (uint64_t)(text.data[i] - (uint8_t)'0');
        if (parsed > (UINT64_MAX - digit) / 10u) return false;
        parsed = parsed * 10u + digit;
    }
    *value_out = parsed;
    return true;
}

static bool parse_coordinate_list(
    arbor_span raw, int64_t first[4], uint64_t *count_out) {
    uint64_t i = 0u, count = 0u;
    bool need_value = true;
    while (i < raw.length) {
        while (i < raw.length && ascii_space(raw.data[i])) ++i;
        if (i < raw.length && raw.data[i] == (uint8_t)',') {
            if (need_value) return false;
            need_value = true;
            ++i;
            continue;
        }
        if (i == raw.length) break;
        bool negative = false;
        if (raw.data[i] == (uint8_t)'+' || raw.data[i] == (uint8_t)'-') {
            negative = raw.data[i] == (uint8_t)'-';
            ++i;
        }
        if (i == raw.length || raw.data[i] < (uint8_t)'0' || raw.data[i] > (uint8_t)'9')
            return false;
        uint64_t magnitude = 0u;
        while (i < raw.length && raw.data[i] >= (uint8_t)'0' && raw.data[i] <= (uint8_t)'9') {
            const uint64_t digit = (uint64_t)(raw.data[i] - (uint8_t)'0');
            if (magnitude > (UINT64_C(9223372036854775807) - digit) / 10u) return false;
            magnitude = magnitude * 10u + digit;
            ++i;
        }
        if (count < 4u) first[count] = negative ? -(int64_t)magnitude : (int64_t)magnitude;
        if (count == UINT64_MAX) return false;
        ++count;
        need_value = false;
        if (i < raw.length && !ascii_space(raw.data[i]) && raw.data[i] != (uint8_t)',') return false;
        if (i < raw.length && ascii_space(raw.data[i])) {
            while (i < raw.length && ascii_space(raw.data[i])) ++i;
            if (i < raw.length && raw.data[i] != (uint8_t)',') need_value = true;
        }
    }
    if (need_value && count != 0u) return false;
    *count_out = count;
    return count != 0u;
}

enum {
    G08_SHAPE_RECT = 1,
    G08_SHAPE_CIRCLE = 2,
    G08_SHAPE_POLY = 3,
    G08_SHAPE_DEFAULT = 4
};

static uint8_t area_shape_state(arbor_span shape, bool present) {
    if (!present) return G08_SHAPE_RECT;
    shape = trim(shape);
    if (span_eq_ci(shape, "circle") || span_eq_ci(shape, "circ")) return G08_SHAPE_CIRCLE;
    if (span_eq_ci(shape, "poly") || span_eq_ci(shape, "polygon")) return G08_SHAPE_POLY;
    if (span_eq_ci(shape, "default")) return G08_SHAPE_DEFAULT;
    return G08_SHAPE_RECT;
}

static uint64_t nearest_map_offset(const g08_context *context) {
    for (uint64_t i = context->frame_count; i != 0u; --i)
        if (context->frames[i - 1u].standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_MAP)
            return context->frames[i - 1u].source_offset;
    return UINT64_MAX;
}

static arbor_status evaluate_current(g08_context *context) {
    const g08_current *c = &context->current;
    const g08_value *v = c->values;
    arbor_status status = ok_status();
    const bool embedded = c->standard_element_id >= ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE &&
        c->standard_element_id <= ARBOR_VIEW0_NATIVE_ELEMENT_AREA;
    if (embedded) {
        if (context->evaluation.embedded_element_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->evaluation.embedded_element_count += 1u;
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE &&
        c->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE) {
        if (context->evaluation.responsive_source_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->evaluation.responsive_source_count += 1u;
        g08_frame *picture = context->frame_count >= 2u ? context->frames + context->frame_count - 2u : NULL;
        if (picture != NULL && v[G08_ATTR_SRCSET].present) {
            for (uint64_t i = picture->picture_source_begin; i < context->picture_source_count; ++i)
                context->picture_sources[i].has_following_srcset = true;
        }
        g08_srcset_result srcset = {0};
        const bool valid_source_set = v[G08_ATTR_SRCSET].present &&
            valid_srcset(v[G08_ATTR_SRCSET].span, &srcset);
        const bool bad_sizes = v[G08_ATTR_SIZES].present &&
            (!srcset.has_width || !valid_sizes(v[G08_ATTR_SIZES].span, true));
        bool media_valid = true;
        if (v[G08_ATTR_MEDIA].present) {
            status = valid_media_query_list(v[G08_ATTR_MEDIA].span, &media_valid);
            if (status.native != 0) return status;
        }
        const bool bad = !valid_source_set || bad_sizes ||
            v[G08_ATTR_SRC].present || (picture != NULL && picture->picture_seen_img) ||
            (v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span)) ||
            !media_valid;
        if (bad) {
            g08_attr_code code = G08_ATTR_NONE;
            if (v[G08_ATTR_SRC].present) code = G08_ATTR_SRC;
            else if (v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span)) code = G08_ATTR_TYPE;
            else if (!media_valid) code = G08_ATTR_MEDIA;
            else if (!valid_source_set) code = G08_ATTR_SRCSET;
            else if (bad_sizes) code = G08_ATTR_SIZES;
            status = emit_at(context, UINT16_C(1), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (picture != NULL) {
            if (context->picture_source_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS)
                return err_status(ENOSPC);
            const arbor_span media = trim(v[G08_ATTR_MEDIA].span);
            context->picture_sources[context->picture_source_count++] =
                (g08_picture_source_summary){
                    .owner_source_offset = c->source_offset,
                    .owner_source_length = c->source_length,
                    .qualified_by_media_or_type = v[G08_ATTR_TYPE].present ||
                        (v[G08_ATTR_MEDIA].present && media.length != 0u &&
                         !span_eq_ci(media, "all")),
                    .width_descriptor_without_sizes = srcset.has_width &&
                        !v[G08_ATTR_SIZES].present
                };
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG) {
        if (context->frame_count >= 2u) {
            g08_frame *parent = context->frames + context->frame_count - 2u;
            if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE) {
                parent->picture_seen_img = true;
                parent->picture_img_allows_auto_sizes = allows_auto_sizes(
                    v[G08_ATTR_LOADING].span, v[G08_ATTR_LOADING].present,
                    v[G08_ATTR_SIZES].span);
            }
            if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE &&
                v[G08_ATTR_SRCSET].present) {
                for (uint64_t i = parent->picture_source_begin; i < context->picture_source_count; ++i)
                    context->picture_sources[i].has_following_srcset = true;
            }
        }
        g08_srcset_result srcset = {0};
        const bool valid_source_set = !v[G08_ATTR_SRCSET].present ||
            valid_srcset(v[G08_ATTR_SRCSET].span, &srcset);
        const bool loading_lazy = v[G08_ATTR_LOADING].present &&
            span_eq_ci(trim(v[G08_ATTR_LOADING].span), "lazy");
        const bool auto_sizes = allows_auto_sizes(v[G08_ATTR_LOADING].span,
            v[G08_ATTR_LOADING].present, v[G08_ATTR_SIZES].span);
        const bool invalid_sizes = v[G08_ATTR_SIZES].present &&
            (v[G08_ATTR_SRCSET].present
                ? (!srcset.has_width || !valid_sizes(v[G08_ATTR_SIZES].span, loading_lazy))
                : !auto_sizes);
        const bool invalid_img_keywords =
            (v[G08_ATTR_LOADING].present && !valid_loading(v[G08_ATTR_LOADING].span)) ||
            (v[G08_ATTR_DECODING].present && !valid_decoding(v[G08_ATTR_DECODING].span)) ||
            (v[G08_ATTR_REFERRERPOLICY].present &&
             !valid_referrer_policy(v[G08_ATTR_REFERRERPOLICY].span));
        if ((!v[G08_ATTR_SRC].present && !v[G08_ATTR_SRCSET].present) ||
            (v[G08_ATTR_SRC].present && !valid_url(v[G08_ATTR_SRC].span)) ||
            !valid_source_set || (srcset.has_width && !v[G08_ATTR_SIZES].present) ||
            invalid_sizes || invalid_img_keywords) {
            g08_attr_code code = G08_ATTR_NONE;
            if (v[G08_ATTR_SRC].present && !valid_url(v[G08_ATTR_SRC].span)) code = G08_ATTR_SRC;
            else if (!valid_source_set) code = G08_ATTR_SRCSET;
            else if ((srcset.has_width && !v[G08_ATTR_SIZES].present) || invalid_sizes) code = G08_ATTR_SIZES;
            else if (v[G08_ATTR_LOADING].present && !valid_loading(v[G08_ATTR_LOADING].span)) code = G08_ATTR_LOADING;
            else if (v[G08_ATTR_DECODING].present && !valid_decoding(v[G08_ATTR_DECODING].span)) code = G08_ATTR_DECODING;
            else if (v[G08_ATTR_REFERRERPOLICY].present) code = G08_ATTR_REFERRERPOLICY;
            status = emit_at(context, UINT16_C(2), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (!v[G08_ATTR_ALT].present &&
            !(v[G08_ATTR_TITLE].present && v[G08_ATTR_TITLE].span.length != 0u) &&
            !(v[G08_ATTR_GENERATOR_ALT].present && v[G08_ATTR_GENERATOR_ALT].span.length == 0u)) {
            if (c->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE) {
                if (context->deferred_alt_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_DEFERRED_ALT)
                    return err_status(ENOSPC);
                const g08_frame *parent = context->frame_count >= 2u
                    ? context->frames + context->frame_count - 2u : NULL;
                context->deferred_alt[context->deferred_alt_count++] = (g08_deferred_alt){
                    .image_source_offset = c->source_offset,
                    .image_source_length = c->source_length,
                    .figure_source_offset = parent == NULL ? UINT64_MAX : parent->source_offset
                };
            } else {
                status = emit_at(context, UINT16_C(3), c->source_offset, c->source_length, G08_ATTR_ALT);
                if (status.native != 0) return status;
            }
        }
        if (v[G08_ATTR_USEMAP].present) {
            if (context->usemap_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_USEMAPS) return err_status(ENOSPC);
            g08_usemap_summary *u = context->usemaps + context->usemap_count++;
            const arbor_span value = trim(v[G08_ATTR_USEMAP].span);
            u->owner_source_offset = c->source_offset;
            u->owner_source_length = c->source_length;
            u->malformed = value.data == NULL || value.length < 2u ||
                value.data[0] != (uint8_t)'#';
            if (!u->malformed)
                u->name = (arbor_span){value.data + 1u, value.length - 1u};
            context->evaluation.image_map_reference_count += 1u;
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME) {
        bool invalid = (v[G08_ATTR_SRC].present && !valid_url(v[G08_ATTR_SRC].span)) ||
            (v[G08_ATTR_ITEMPROP].present && !v[G08_ATTR_SRC].present);
        g08_attr_code code = invalid ? G08_ATTR_SRC : G08_ATTR_NONE;
        if (invalid && v[G08_ATTR_ITEMPROP].present && !v[G08_ATTR_SRC].present)
            code = G08_ATTR_ITEMPROP;
        if (!invalid && v[G08_ATTR_NAME].present && !valid_target(v[G08_ATTR_NAME].span)) {
            invalid = true; code = G08_ATTR_NAME;
        }
        if (!invalid && v[G08_ATTR_SANDBOX].present) {
            const arbor_span sandbox = v[G08_ATTR_SANDBOX].span;
            const bool top = token_has(sandbox, "allow-top-navigation");
            const bool activation = token_has(sandbox, "allow-top-navigation-by-user-activation");
            const bool protocol = token_has(sandbox, "allow-top-navigation-to-custom-protocols");
            const bool popups = token_has(sandbox, "allow-popups");
            if (!valid_sandbox(sandbox) || (top && activation) || (protocol && (top || popups))) {
                invalid = true; code = G08_ATTR_SANDBOX;
            }
        }
        if (!invalid && v[G08_ATTR_ALLOW].present &&
            !valid_permissions_policy(v[G08_ATTR_ALLOW].span)) {
            invalid = true; code = G08_ATTR_ALLOW;
        }
        if (!invalid && v[G08_ATTR_ALLOWFULLSCREEN].present &&
            !valid_boolean_value(v[G08_ATTR_ALLOWFULLSCREEN].span, "allowfullscreen")) {
            invalid = true; code = G08_ATTR_ALLOWFULLSCREEN;
        }
        if (!invalid && v[G08_ATTR_REFERRERPOLICY].present &&
            !valid_referrer_policy(v[G08_ATTR_REFERRERPOLICY].span)) {
            invalid = true; code = G08_ATTR_REFERRERPOLICY;
        }
        if (!invalid && v[G08_ATTR_LOADING].present && !valid_loading(v[G08_ATTR_LOADING].span)) {
            invalid = true; code = G08_ATTR_LOADING;
        }
        if (invalid) {
            status = emit_at(context, UINT16_C(4), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_EMBED) {
        bool invalid = (v[G08_ATTR_ITEMPROP].present && !v[G08_ATTR_SRC].present) ||
            (v[G08_ATTR_SRC].present && !valid_url(v[G08_ATTR_SRC].span)) ||
            (v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span));
        if (invalid) {
            const g08_attr_code code = v[G08_ATTR_SRC].present && !valid_url(v[G08_ATTR_SRC].span)
                ? G08_ATTR_SRC : (v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span)
                    ? G08_ATTR_TYPE : G08_ATTR_ITEMPROP);
            status = emit_at(context, UINT16_C(5), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT) {
        const bool invalid_data = !v[G08_ATTR_DATA].present || !valid_url(v[G08_ATTR_DATA].span);
        const bool invalid_type = v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span);
        const bool invalid_name = v[G08_ATTR_NAME].present && !valid_target(v[G08_ATTR_NAME].span);
        if (invalid_data || invalid_type || invalid_name) {
            const g08_attr_code code = invalid_data ? G08_ATTR_DATA
                : (invalid_type ? G08_ATTR_TYPE : G08_ATTR_NAME);
            status = emit_at(context, UINT16_C(6), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (v[G08_ATTR_USEMAP].present) {
            if (context->usemap_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_USEMAPS) return err_status(ENOSPC);
            g08_usemap_summary *u = context->usemaps + context->usemap_count++;
            const arbor_span value = trim(v[G08_ATTR_USEMAP].span);
            u->owner_source_offset = c->source_offset; u->owner_source_length = c->source_length;
            u->malformed = value.data == NULL || value.length < 2u ||
                value.data[0] != (uint8_t)'#';
            if (!u->malformed)
                u->name = (arbor_span){value.data + 1u, value.length - 1u};
            context->evaluation.image_map_reference_count += 1u;
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO ||
        c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO) {
        const bool invalid_src = v[G08_ATTR_SRC].present && !valid_url(v[G08_ATTR_SRC].span);
        const bool missing_itemprop_src = v[G08_ATTR_ITEMPROP].present && !v[G08_ATTR_SRC].present;
        const bool invalid_poster = c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO &&
            v[G08_ATTR_POSTER].present && !valid_url(v[G08_ATTR_POSTER].span);
        const bool invalid_preload = v[G08_ATTR_PRELOAD].present &&
            !valid_preload(v[G08_ATTR_PRELOAD].span);
        const bool invalid_loading = v[G08_ATTR_LOADING].present &&
            !valid_loading(v[G08_ATTR_LOADING].span);
        const bool invalid_crossorigin = v[G08_ATTR_CROSSORIGIN].present &&
            !valid_crossorigin(v[G08_ATTR_CROSSORIGIN].span);
        g08_attr_code invalid_boolean = G08_ATTR_NONE;
        if (v[G08_ATTR_AUTOPLAY].present &&
            !valid_boolean_value(v[G08_ATTR_AUTOPLAY].span, "autoplay")) invalid_boolean = G08_ATTR_AUTOPLAY;
        else if (v[G08_ATTR_LOOP].present &&
            !valid_boolean_value(v[G08_ATTR_LOOP].span, "loop")) invalid_boolean = G08_ATTR_LOOP;
        else if (v[G08_ATTR_MUTED].present &&
            !valid_boolean_value(v[G08_ATTR_MUTED].span, "muted")) invalid_boolean = G08_ATTR_MUTED;
        else if (v[G08_ATTR_CONTROLS].present &&
            !valid_boolean_value(v[G08_ATTR_CONTROLS].span, "controls")) invalid_boolean = G08_ATTR_CONTROLS;
        else if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO &&
            v[G08_ATTR_PLAYSINLINE].present &&
            !valid_boolean_value(v[G08_ATTR_PLAYSINLINE].span, "playsinline"))
            invalid_boolean = G08_ATTR_PLAYSINLINE;
        if (invalid_src || missing_itemprop_src || invalid_poster || invalid_preload ||
            invalid_loading || invalid_crossorigin || invalid_boolean != G08_ATTR_NONE) {
            const g08_attr_code code = invalid_src ? G08_ATTR_SRC
                : (invalid_poster ? G08_ATTR_POSTER : (missing_itemprop_src ? G08_ATTR_ITEMPROP
                : (invalid_preload ? G08_ATTR_PRELOAD : (invalid_loading ? G08_ATTR_LOADING
                : (invalid_crossorigin ? G08_ATTR_CROSSORIGIN : invalid_boolean)))));
            status = emit_at(context, UINT16_C(7), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (context->frame_count != 0u)
            context->frames[context->frame_count - 1u].media_has_src = v[G08_ATTR_SRC].present;
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE &&
        (c->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO ||
         c->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO)) {
        context->evaluation.media_source_count += 1u;
        bool media_valid = true;
        if (v[G08_ATTR_MEDIA].present) {
            status = valid_media_query_list(v[G08_ATTR_MEDIA].span, &media_valid);
            if (status.native != 0) return status;
        }
        bool invalid = !v[G08_ATTR_SRC].present || !valid_url(v[G08_ATTR_SRC].span) ||
            (v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span)) ||
            !media_valid;
        if (invalid) {
            const g08_attr_code code = !v[G08_ATTR_SRC].present || !valid_url(v[G08_ATTR_SRC].span)
                ? G08_ATTR_SRC : (v[G08_ATTR_TYPE].present && !valid_mime(v[G08_ATTR_TYPE].span)
                    ? G08_ATTR_TYPE : G08_ATTR_MEDIA);
            status = emit_at(context, UINT16_C(9), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (context->frame_count >= 2u && context->frames[context->frame_count - 2u].media_has_src) {
            status = emit_at(context, UINT16_C(7), c->source_offset, c->source_length, G08_ATTR_SRC);
            if (status.native != 0) return status;
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TRACK) {
        context->evaluation.text_track_count += 1u;
        const uint8_t kind_state = track_kind_state(
            v[G08_ATTR_KIND].span, v[G08_ATTR_KIND].present);
        const bool subtitles = kind_state == G08_TRACK_SUBTITLES;
        bool invalid = !v[G08_ATTR_SRC].present || !valid_url(v[G08_ATTR_SRC].span) ||
            (v[G08_ATTR_SRCLANG].present && !valid_language(v[G08_ATTR_SRCLANG].span)) ||
            (subtitles && !v[G08_ATTR_SRCLANG].present) ||
            (v[G08_ATTR_LABEL].present && v[G08_ATTR_LABEL].span.length == 0u);
        if (invalid) {
            const g08_attr_code code = !v[G08_ATTR_SRC].present || !valid_url(v[G08_ATTR_SRC].span)
                ? G08_ATTR_SRC : ((v[G08_ATTR_SRCLANG].present &&
                    !valid_language(v[G08_ATTR_SRCLANG].span)) ||
                    (subtitles && !v[G08_ATTR_SRCLANG].present)
                    ? G08_ATTR_SRCLANG : G08_ATTR_LABEL);
            status = emit_at(context, UINT16_C(8), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (context->track_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS) return err_status(ENOSPC);
        g08_track_summary *summary = context->tracks + context->track_count;
        const uint64_t parent = context->frame_count >= 2u
            ? context->frames[context->frame_count - 2u].source_offset : UINT64_MAX;
        summary->parent_source_offset = parent;
        summary->kind_state = kind_state;
        summary->language = trim(v[G08_ATTR_SRCLANG].span);
        summary->label = v[G08_ATTR_LABEL].span;
        summary->language_present = v[G08_ATTR_SRCLANG].present;
        summary->label_present = v[G08_ATTR_LABEL].present;
        for (uint64_t i = 0u; i < context->track_count; ++i) {
            const g08_track_summary *prior = context->tracks + i;
            const bool same_language = prior->language_present == summary->language_present &&
                (!summary->language_present || span_eq_span_ci(prior->language, summary->language));
            const bool same_label = prior->label_present == summary->label_present &&
                (!summary->label_present || span_eq_exact(prior->label, summary->label));
            if (prior->parent_source_offset == parent &&
                prior->kind_state == summary->kind_state && same_language && same_label) {
                status = emit_at(context, UINT16_C(8), c->source_offset, c->source_length, G08_ATTR_LABEL);
                if (status.native != 0) return status;
                break;
            }
        }
        context->track_count += 1u;
        if (v[G08_ATTR_DEFAULT].present && context->frame_count >= 2u) {
            g08_frame *media = context->frames + context->frame_count - 2u;
            uint32_t *counter = NULL;
            if (subtitles || kind_state == G08_TRACK_CAPTIONS)
                counter = &media->default_subtitle_caption_count;
            else if (kind_state == G08_TRACK_DESCRIPTIONS)
                counter = &media->default_description_count;
            else if (kind_state == G08_TRACK_CHAPTERS)
                counter = &media->default_chapters_count;
            if (counter != NULL && (*counter)++ != 0u) {
                status = emit_at(context, UINT16_C(8), c->source_offset, c->source_length, G08_ATTR_DEFAULT);
                if (status.native != 0) return status;
            }
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_MAP) {
        const arbor_span name = v[G08_ATTR_NAME].span;
        bool invalid = !v[G08_ATTR_NAME].present || name.length == 0u ||
            contains_ascii_space(name) ||
            (v[G08_ATTR_ID].present && !span_eq_exact(name, v[G08_ATTR_ID].span));
        for (uint64_t i = 0u; !invalid && i < context->map_count; ++i)
            if (span_eq_exact(context->maps[i].name, name)) invalid = true;
        if (invalid) {
            const g08_attr_code code = v[G08_ATTR_ID].present &&
                !span_eq_exact(name, v[G08_ATTR_ID].span) ? G08_ATTR_ID : G08_ATTR_NAME;
            status = emit_at(context, UINT16_C(10), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        } else {
            if (context->map_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_MAPS) return err_status(ENOSPC);
            context->maps[context->map_count].source_offset = c->source_offset;
            context->maps[context->map_count].name = name;
            context->map_count += 1u;
        }
    }

    if (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_AREA) {
        const bool invalid_alt = (v[G08_ATTR_HREF].present && !v[G08_ATTR_ALT].present) ||
            (!v[G08_ATTR_HREF].present && v[G08_ATTR_ALT].present);
        const uint8_t shape = area_shape_state(v[G08_ATTR_SHAPE].span, v[G08_ATTR_SHAPE].present);
        int64_t coordinates[4] = {0};
        uint64_t coordinate_count = 0u;
        const bool coordinates_valid = v[G08_ATTR_COORDS].present &&
            parse_coordinate_list(v[G08_ATTR_COORDS].span, coordinates, &coordinate_count);
        bool invalid_shape = false;
        if (shape == G08_SHAPE_DEFAULT) invalid_shape = v[G08_ATTR_COORDS].present;
        else if (!coordinates_valid) invalid_shape = true;
        else if (shape == G08_SHAPE_CIRCLE)
            invalid_shape = coordinate_count != 3u || coordinates[2] < 0;
        else if (shape == G08_SHAPE_POLY)
            invalid_shape = coordinate_count < 6u || (coordinate_count % 2u) != 0u;
        else
            invalid_shape = coordinate_count != 4u || coordinates[0] >= coordinates[2] ||
                coordinates[1] >= coordinates[3];
        const bool invalid_itemprop = v[G08_ATTR_ITEMPROP].present && !v[G08_ATTR_HREF].present;
        const bool invalid = invalid_alt || invalid_shape || invalid_itemprop;
        if (invalid) {
            const g08_attr_code code = invalid_alt ? G08_ATTR_ALT
                : (invalid_shape ? G08_ATTR_COORDS : G08_ATTR_ITEMPROP);
            status = emit_at(context, UINT16_C(10), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
        if (v[G08_ATTR_HREF].present && v[G08_ATTR_ALT].present) {
            if (context->area_count >= ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS)
                return err_status(ENOSPC);
            context->areas[context->area_count++] = (g08_area_summary){
                .map_source_offset = nearest_map_offset(context),
                .owner_source_offset = c->source_offset,
                .owner_source_length = c->source_length,
                .href = trim(v[G08_ATTR_HREF].span),
                .alt = v[G08_ATTR_ALT].span,
                .href_present = true,
                .alt_present = true
            };
        }
    }

    if (c->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_SVG ||
        c->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_MATHML) {
        context->evaluation.foreign_integration_count += 1u;
        context->evaluation.deferred_external_semantics_count += 1u;
    }

    const bool supports_dimensions =
        c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG ||
        c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME ||
        c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_EMBED ||
        c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT ||
        c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO ||
        (c->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE &&
         c->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE);
    if (supports_dimensions && (v[G08_ATTR_WIDTH].present || v[G08_ATTR_HEIGHT].present)) {
        uint64_t width = 0u, height = 0u;
        const bool width_valid = !v[G08_ATTR_WIDTH].present ||
            parse_nonnegative(v[G08_ATTR_WIDTH].span, &width);
        const bool height_valid = !v[G08_ATTR_HEIGHT].present ||
            parse_nonnegative(v[G08_ATTR_HEIGHT].span, &height);
        const bool invalid_pair = v[G08_ATTR_WIDTH].present && v[G08_ATTR_HEIGHT].present &&
            width_valid && height_valid && ((width == 0u) != (height == 0u));
        if (!width_valid || !height_valid || invalid_pair) {
            const g08_attr_code code = !width_valid ? G08_ATTR_WIDTH
                : (!height_valid ? G08_ATTR_HEIGHT : G08_ATTR_WIDTH);
            status = emit_at(context, UINT16_C(12), c->source_offset, c->source_length, code);
            if (status.native != 0) return status;
        }
    }
    return ok_status();
}

static arbor_status element_complete(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    if (observation->source_offset != context->current.source_offset ||
        observation->standard_element_id != context->current.standard_element_id) return err_status(EIO);
    return evaluate_current(context);
}

static arbor_status traversal_leave(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g08_context *context = (g08_context *)opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    const g08_frame *frame = context->frames + context->frame_count - 1u;
    if (frame->source_offset != observation->source_offset || frame->depth != observation->depth)
        return err_status(EIO);
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE) {
        const bool allowed = frame->figure_img_count == 1u && frame->figure_other_flow_count == 0u &&
            frame->figure_has_non_iew_caption;
        for (uint64_t i = 0u; i < context->deferred_alt_count; ++i) {
            const g08_deferred_alt *alt = context->deferred_alt + i;
            if (alt->figure_source_offset == frame->source_offset && !allowed) {
                arbor_status status = emit_at(context, UINT16_C(3), alt->image_source_offset,
                    alt->image_source_length, G08_ATTR_ALT);
                if (status.native != 0) return status;
            }
        }
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE) {
        for (uint64_t i = frame->picture_source_begin; i < context->picture_source_count; ++i) {
            const g08_picture_source_summary *source = context->picture_sources + i;
            if (source->width_descriptor_without_sizes &&
                !frame->picture_img_allows_auto_sizes) {
                arbor_status status = emit_at(context, UINT16_C(1),
                    source->owner_source_offset, source->owner_source_length,
                    G08_ATTR_SIZES);
                if (status.native != 0) return status;
            }
            if (source->has_following_srcset && !source->qualified_by_media_or_type) {
                arbor_status status = emit_at(context, UINT16_C(1), source->owner_source_offset,
                    source->owner_source_length, G08_ATTR_MEDIA);
                if (status.native != 0) return status;
            }
        }
        context->picture_source_count = frame->picture_source_begin;
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML && frame->depth == 0u) {
        arbor_status status = evaluate_usemaps(context);
        if (status.native != 0) return status;
        status = evaluate_area_alternatives(context);
        if (status.native != 0) return status;
    }
    context->frame_count -= 1u;
    return ok_status();
}

static arbor_status evaluate_usemaps(g08_context *context) {
    for (uint64_t i = 0u; i < context->usemap_count; ++i) {
        const g08_usemap_summary *u = context->usemaps + i;
        bool found = false;
        for (uint64_t j = 0u; !u->malformed && j < context->map_count; ++j)
            if (span_eq_exact(u->name, context->maps[j].name)) found = true;
        if (u->malformed || !found) {
            arbor_status status = emit_at(context, UINT16_C(10), u->owner_source_offset,
                u->owner_source_length, G08_ATTR_USEMAP);
            if (status.native != 0) return status;
        }
    }
    return ok_status();
}

static arbor_status evaluate_area_alternatives(g08_context *context) {
    for (uint64_t i = 0u; i < context->area_count; ++i) {
        const g08_area_summary *area = context->areas + i;
        if (!area->href_present || !area->alt_present || area->alt.length != 0u) continue;
        bool replacement = false;
        for (uint64_t j = 0u; !replacement && j < context->area_count; ++j) {
            const g08_area_summary *other = context->areas + j;
            replacement = i != j && other->map_source_offset == area->map_source_offset &&
                other->href_present && other->alt_present && other->alt.length != 0u &&
                span_eq_exact(other->href, area->href);
        }
        if (!replacement) {
            arbor_status status = emit_at(context, UINT16_C(10), area->owner_source_offset,
                area->owner_source_length, G08_ATTR_ALT);
            if (status.native != 0) return status;
        }
    }
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input, arbor_view0_native_v1n2_g08_anchor *anchors,
    uint64_t anchor_capacity, bool collect,
    arbor_view0_native_v1n2_g08_evaluation *evaluation_out) {
    if (evaluation_out == NULL || (collect && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    g08_context context = {.anchors = anchors, .anchor_capacity = anchor_capacity, .collect = collect};
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = element_begin,
        .attribute = attribute,
        .direct_child = direct_child,
        .element_complete = element_complete,
        .traversal_enter = traversal_enter,
        .traversal_leave = traversal_leave,
        .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &counts);
    if (status.native != 0) return status;
    if (context.frame_count != 0u) return err_status(EIO);
    if (collect && context.evaluation.diagnostic_count != anchor_capacity) return err_status(EIO);
    *evaluation_out = context.evaluation;
    return ok_status();
}

arbor_status arbor_view0_native_v1n2_g08_measure(
    arbor_span input, arbor_view0_native_v1n2_g08_evaluation *evaluation_out) {
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_v1n2_g08_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g08_anchor *anchors,
    uint64_t anchor_capacity, arbor_view0_native_v1n2_g08_evaluation *evaluation_out) {
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_v1n2_g08_materialize_anchor(
    const arbor_view0_native_v1n2_g08_anchor *anchor,
    uint64_t discovery_sequence, arbor_view0_native_diagnostic *diagnostic) {
    static const char *const messages[] = {
        "Responsive image source violates picture ordering or srcset companion requirements",
        "Image resource declaration violates a frozen src, srcset, or sizes relationship",
        "Image lacks a deterministic admitted alternative-text replacement",
        "Iframe declaration violates a frozen URL, target, or sandbox relationship",
        "Embed declaration violates a frozen resource or MIME relationship",
        "Object declaration violates a frozen data, MIME, or target relationship",
        "Media element declaration violates a frozen resource relationship",
        "Text track declaration violates a frozen source, language, label, or uniqueness relationship",
        "Media source declaration violates a frozen cross-resource relationship",
        "Image map declaration violates a frozen name, reference, alt, shape, or coordinate relationship",
        "Foreign embedded content exceeds the admitted HTML-integration boundary",
        "Paired dimension attributes violate the deterministic zero-dimension relationship"
    };
    if (anchor == NULL || diagnostic == NULL || anchor->shared.rule_ordinal == 0u ||
        anchor->shared.rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G08_RULE_COUNT) return;
    const arbor_view0_native_v1n2_rule_meta *meta = arbor_view0_native_v1n2_c0_rule_at(
        UINT64_C(5) + (uint64_t)anchor->shared.rule_ordinal - 1u);
    if (meta == NULL || meta->group != ARBOR_VIEW0_NATIVE_V1N2_GROUP_G08) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = meta->rule_id;
    diagnostic->byte_offset = anchor->shared.byte_offset;
    diagnostic->source_length = anchor->shared.source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, meta->rule_symbol, strlen(meta->rule_symbol) + 1u);
    (void)memcpy(diagnostic->message, messages[anchor->shared.rule_ordinal - 1u],
                 strlen(messages[anchor->shared.rule_ordinal - 1u]) + 1u);
}
