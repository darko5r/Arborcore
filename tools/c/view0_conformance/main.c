#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum output_format {
    OUTPUT_FORMAT_HUMAN = 0,
    OUTPUT_FORMAT_TSV = 1
} output_format;

static int tsv_path_is_safe(const char *path)
{
    if (path == NULL) {
        return 0;
    }

    for (const unsigned char *cursor = (const unsigned char *)path;
         *cursor != '\0';
         ++cursor) {
        if (*cursor == (unsigned char)'\t' ||
            *cursor == (unsigned char)'\n' ||
            *cursor == (unsigned char)'\r') {
            return 0;
        }
    }

    return 1;
}

static int read_document(const char *path, uint8_t **data_out, uint64_t *length_out)
{
    if (path == NULL || data_out == NULL || length_out == NULL) {
        return 2;
    }

    const int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        (void)fprintf(stderr, "error: cannot open %s: %s\n", path, strerror(errno));
        return 2;
    }

    struct stat st = {0};
    if (fstat(fd, &st) != 0) {
        (void)fprintf(stderr, "error: cannot stat %s: %s\n", path, strerror(errno));
        (void)close(fd);
        return 3;
    }
    if (!S_ISREG(st.st_mode)) {
        (void)fprintf(stderr, "error: input must be a regular file\n");
        (void)close(fd);
        return 2;
    }
    if (st.st_size < 0) {
        (void)fprintf(stderr, "error: negative file size: %s\n", path);
        (void)close(fd);
        return 3;
    }

    const uintmax_t file_size = (uintmax_t)st.st_size;
    if (file_size > (uintmax_t)ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES ||
        file_size > (uintmax_t)UINT64_MAX || file_size > (uintmax_t)SIZE_MAX) {
        (void)fprintf(
            stderr,
            "error: input exceeds V1N0 limit (%" PRIu64 " bytes): %s\n",
            ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES,
            path);
        (void)close(fd);
        return 2;
    }

    const uint64_t length = (uint64_t)file_size;
    const size_t allocation = length == 0u ? 1u : (size_t)length;
    uint8_t *data = (uint8_t *)malloc(allocation);
    if (data == NULL) {
        (void)fprintf(stderr, "error: allocation failed for %s\n", path);
        (void)close(fd);
        return 3;
    }

    uint64_t offset = 0u;
    while (offset < length) {
        const size_t remaining = (size_t)(length - offset);
        const ssize_t got = read(fd, data + offset, remaining);
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            (void)fprintf(stderr, "error: read failed for %s: %s\n", path, strerror(errno));
            free(data);
            (void)close(fd);
            return 3;
        }
        if (got == 0) {
            (void)fprintf(stderr, "error: input changed while reading\n");
            free(data);
            (void)close(fd);
            return 2;
        }
        offset += (uint64_t)got;
    }

    uint8_t eof_probe = 0u;
    for (;;) {
        const ssize_t got = read(fd, &eof_probe, 1u);
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            (void)fprintf(stderr, "error: EOF probe failed for %s: %s\n", path, strerror(errno));
            free(data);
            (void)close(fd);
            return 3;
        }
        if (got != 0) {
            (void)fprintf(stderr, "error: input changed while reading\n");
            free(data);
            (void)close(fd);
            return 2;
        }
        break;
    }

    struct stat st_after = {0};
    if (fstat(fd, &st_after) != 0) {
        (void)fprintf(stderr, "error: cannot restat %s: %s\n", path, strerror(errno));
        free(data);
        (void)close(fd);
        return 3;
    }
    if (!S_ISREG(st_after.st_mode) || st_after.st_size != st.st_size) {
        (void)fprintf(stderr, "error: input changed while reading\n");
        free(data);
        (void)close(fd);
        return 2;
    }

    if (close(fd) != 0) {
        (void)fprintf(stderr, "error: close failed for %s: %s\n", path, strerror(errno));
        free(data);
        return 3;
    }

    *data_out = data;
    *length_out = length;
    return 0;
}

static const char *severity_name(uint32_t severity)
{
    return severity == (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_WARNING
        ? "warning"
        : "error";
}

static const char *origin_name(uint32_t origin)
{
    switch ((arbor_view0_native_origin)origin) {
        case ARBOR_VIEW0_NATIVE_ORIGIN_UTF8:
            return "utf8";
        case ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER:
            return "lexbor-tokenizer";
        case ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TREE:
            return "lexbor-tree";
        case ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING:
            return "arborcore-authoring";
        default:
            return "unknown";
    }
}

static void print_human(
    const char *path,
    const arbor_view0_native_diagnostic *diagnostic)
{
    (void)printf(
        "%s:%" PRIu64 ":%" PRIu64 ": %s [0x%016" PRIx64 " %s]: %s\n",
        path,
        diagnostic->line,
        diagnostic->column,
        severity_name(diagnostic->severity),
        diagnostic->rule_id,
        diagnostic->symbolic_name,
        diagnostic->message);
}

static void print_tsv(
    const char *path,
    const arbor_view0_native_diagnostic *diagnostic)
{
    (void)printf(
        "%s\t%s\t0x%016" PRIx64 "\t%s\t%s\t%" PRIu64 "\t%" PRIu64
        "\t%" PRIu64 "\t%" PRIu64 "\t%u\t%s\n",
        path,
        severity_name(diagnostic->severity),
        diagnostic->rule_id,
        diagnostic->symbolic_name,
        origin_name(diagnostic->origin),
        diagnostic->byte_offset,
        diagnostic->source_length,
        diagnostic->line,
        diagnostic->column,
        diagnostic->external_id,
        diagnostic->message);
}

static int parse_arguments(
    int argc,
    char **argv,
    output_format *format_out,
    const char **path_out)
{
    if (format_out == NULL || path_out == NULL) {
        return 2;
    }

    *format_out = OUTPUT_FORMAT_HUMAN;
    *path_out = NULL;

    if (argc == 2) {
        *path_out = argv[1];
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--format=human") == 0) {
        *path_out = argv[2];
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--format=tsv") == 0) {
        *format_out = OUTPUT_FORMAT_TSV;
        *path_out = argv[2];
        return 0;
    }

    (void)fprintf(
        stderr,
        "usage: arborcore-view0-html-check [--format=human|--format=tsv] FILE\n");
    return 2;
}

int main(int argc, char **argv)
{
    output_format format = OUTPUT_FORMAT_HUMAN;
    const char *path = NULL;
    int rc = parse_arguments(argc, argv, &format, &path);
    if (rc != 0) {
        return rc;
    }

    if (format == OUTPUT_FORMAT_TSV && !tsv_path_is_safe(path)) {
        (void)fprintf(
            stderr,
            "error: --format=tsv requires a FILE path without tab, LF, or CR bytes\n");
        return 2;
    }

    uint8_t *data = NULL;
    uint64_t length = 0u;
    rc = read_document(path, &data, &length);
    if (rc != 0) {
        return rc;
    }

    const size_t diagnostic_count = (size_t)ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS;
    arbor_view0_native_diagnostic *diagnostics =
        (arbor_view0_native_diagnostic *)calloc(
            diagnostic_count,
            sizeof(*diagnostics));
    if (diagnostics == NULL) {
        (void)fprintf(stderr, "error: diagnostic allocation failed\n");
        free(data);
        return 3;
    }

    arbor_view0_native_result result = {0};
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){data, length},
        diagnostics,
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS,
        &result);

    if (status.native != 0) {
        (void)fprintf(
            stderr,
            "error: checker mechanism failure: native=%" PRId64 " code=%d\n",
            status.native,
            (int)status.code);
        free(diagnostics);
        free(data);
        return 3;
    }

    int has_error = 0;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (diagnostics[i].severity ==
            (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR) {
            has_error = 1;
        }
        if (format == OUTPUT_FORMAT_TSV) {
            print_tsv(path, diagnostics + i);
        } else {
            print_human(path, diagnostics + i);
        }
    }

    if (format == OUTPUT_FORMAT_TSV) {
        (void)printf(
            "SUMMARY\t%s\tdiagnostics=%" PRIu64 "\ttokenizer=%" PRIu64
            "\ttree=%" PRIu64 "\tparse_clean=%s\tcomplete_conformance=no"
            "\tg03_r1=partial\tmain_form_accessible_name_deferred=%s"
            "\tg03_r2=partial\tr2_style_deferred=%s\tr2_script_deferred=%s"
            "\tr2_noscript_deferred=%s\tr2_select_size_deferred=%s"
            "\tr2_select_platform_deferred=%s\tr2_unclassified_deferred=%s"
            "\tg03_r3=partial\tr3_input_type_deferred=%s"
            "\tr3_labeled_control_deferred=%s\tr3_canvas_input_deferred=%s"
            "\tr3_canvas_select_size_deferred=%s\tr3_noscript_deferred=%s"
            "\tg03_r4=partial\tr4_selectedcontent_provenance_deferred=%s"
            "\tg03_r5=partial\tg03_r7=partial\tr7_g04_transparent_deferred=%s"
            "\tr7_g13_custom_deferred=%s\n",
            path,
            result.diagnostic_count,
            result.tokenizer_error_count,
            result.tree_error_count,
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_STYLE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SCRIPT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_NOSCRIPT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_SIZE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_PLATFORM) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_UNCLASSIFIED) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_INPUT_STATE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_SELECT_SIZE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_NOSCRIPT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G04_TRANSPARENT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM) != 0u ? "yes" : "no");
    } else {
        (void)printf(
            "summary: diagnostics=%" PRIu64 " tokenizer=%" PRIu64
            " tree=%" PRIu64 " parse_clean=%s complete_conformance=no"
            " g03_r1=partial main_form_accessible_name_deferred=%s"
            " g03_r2=partial r2_style_deferred=%s r2_script_deferred=%s"
            " r2_noscript_deferred=%s r2_select_size_deferred=%s"
            " r2_select_platform_deferred=%s r2_unclassified_deferred=%s"
            " g03_r3=partial r3_input_type_deferred=%s"
            " r3_labeled_control_deferred=%s r3_canvas_input_deferred=%s"
            " r3_canvas_select_size_deferred=%s r3_noscript_deferred=%s"
            " g03_r4=partial r4_selectedcontent_provenance_deferred=%s"
            " g03_r5=partial g03_r7=partial r7_g04_transparent_deferred=%s"
            " r7_g13_custom_deferred=%s\n",
            result.diagnostic_count,
            result.tokenizer_error_count,
            result.tree_error_count,
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_STYLE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SCRIPT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_NOSCRIPT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_SIZE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_PLATFORM) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_UNCLASSIFIED) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_INPUT_STATE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_SELECT_SIZE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_NOSCRIPT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G04_TRANSPARENT) != 0u ? "yes" : "no",
            (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM) != 0u ? "yes" : "no");
    }

    const int exit_code = has_error != 0 ? 1 : 0;
    free(diagnostics);
    free(data);
    return exit_code;
}
