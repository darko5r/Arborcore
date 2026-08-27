#include "g05_c0.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(int ok, const char *message)
{
    if (!ok) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

typedef struct expected_input_clause {
    uint64_t conditional_index;
    uint64_t state_mask;
    const char *state_suffix;
    const char *clause_sha256;
} expected_input_clause;

int main(void)
{
    static const expected_input_clause expected[] = {
        {15u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN, "hidden", "be2cbb7a2af05cf8d1d07f81cc9c99dfe39288dd3d80468157c3d9b16fb773af"},
        {16u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN, "hidden", "6eb1b861067fae91fc6ac9b01c82b25bb4f942ef1741036fda11b1802a4a9911"},
        {17u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT | ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH, "text|search", "b54df7cf3e2d9f8dfef76a13024d40cab8e332a942ad2fb728b624a0de110b4a"},
        {18u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL, "tel", "b54df7cf3e2d9f8dfef76a13024d40cab8e332a942ad2fb728b624a0de110b4a"},
        {19u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL, "url", "b54df7cf3e2d9f8dfef76a13024d40cab8e332a942ad2fb728b624a0de110b4a"},
        {20u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL, "email", "ec9f8435fe782cf7a379facf3e08e27317f0a46b3fcf6ed9f179ec43285d6c5e"},
        {21u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD, "password", "4a5f590278d59dfd04416fd8cd60e8a7589cf14db96a35c9ff0615eff77b50ec"},
        {22u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE, "date", "e9ec50d24f16b0875ac95d18d8645bc1dcc4be5c1ef1d1132b408897a0553af8"},
        {23u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH, "month", "e9ec50d24f16b0875ac95d18d8645bc1dcc4be5c1ef1d1132b408897a0553af8"},
        {24u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK, "week", "518d859193d6de2dffe03aa2506420fcdfa1790e74a0ac7d1905f84f6b846166"},
        {25u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME, "time", "518d859193d6de2dffe03aa2506420fcdfa1790e74a0ac7d1905f84f6b846166"},
        {26u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL, "datetime-local", "e9ec50d24f16b0875ac95d18d8645bc1dcc4be5c1ef1d1132b408897a0553af8"},
        {27u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER, "number", "66e7bb047df0aebe031381bfbea2fea5c0c221ffe4b1dff62a9d2e8f8ee2071a"},
        {28u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE, "range", "9b4cdbbd272e085c6cead2b176ad771d45552971cb51fa7a363775b5604c5907"},
        {29u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR, "color", "391cf3d582489ca0e19f0d14913d7063e0224cf054d93d793660e77b0e266791"},
        {30u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX, "checkbox", "e5f74a1043fbbb592a5db8d3a68b390430d0e3f6a562cabf83d79e5b9383083d"},
        {31u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO, "radio", "e5f74a1043fbbb592a5db8d3a68b390430d0e3f6a562cabf83d79e5b9383083d"},
        {32u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE, "file", "59dd0c3ececd6a74ae6410e0e85a705ec04eedc72df7aaea8af976692eb16bad"},
        {33u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE, "file", "70104f8301ead83e949f251be8525a17b4171f1e54c35007314dcab0fff1366d"},
        {34u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT, "submit", "7d21262f6bd4086eced5d8e02635f1608da08f2679c73c0db6d9da36f9530517"},
        {35u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE, "image", "6c7e4dffa59289b9d13b9836e4f57cfd182da3d97d307288b77a290e4db1f611"},
        {36u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE, "image", "70104f8301ead83e949f251be8525a17b4171f1e54c35007314dcab0fff1366d"},
        {37u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET, "reset", "d82b2997a0f9f058c04396398211619e95caf28b1c60cb6c73d9cd4903ca9c33"},
        {38u, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON, "button", "d82b2997a0f9f058c04396398211619e95caf28b1c60cb6c73d9cd4903ca9c33"}
    };
    uint64_t union_mask = 0u;
    need(arbor_view0_native_g05_c0_conditional_count() == 43u, "conditional count retained");
    need(sizeof expected / sizeof expected[0] == 24u, "input clause count");
    for (size_t i = 0u; i < sizeof expected / sizeof expected[0]; ++i) {
        const arbor_view0_native_g05_c0_conditional_meta *meta =
            arbor_view0_native_g05_c0_conditional_at(expected[i].conditional_index);
        need(meta != NULL, "input conditional exists");
        need(meta->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, "input owner");
        need(meta->input_state_mask == expected[i].state_mask, "input state mask");
        need(strcmp(meta->input_state_suffix, expected[i].state_suffix) == 0, "input state suffix");
        need(strcmp(meta->clause_sha256, expected[i].clause_sha256) == 0, "input clause hash retained");
        union_mask |= meta->input_state_mask;
    }
    need(union_mask == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_ALL_MASK, "all 22 input states covered");
    puts("VIEW0_V1N1_G05_C0_SR1_INPUT_CLAUSE_ROWS=24");
    puts("VIEW0_V1N1_G05_C0_SR1_INPUT_STATE_CLAUSE_ROWS=25");
    puts("VIEW0_V1N1_G05_C0_SR1_DISTINCT_INPUT_STATES=22");
    puts("VIEW0_V1N1_G05_C0_SR1_TEXT_SEARCH_SHARED_CLAUSE=PASS");
    puts("VIEW0_V1N1_G05_C0_SR1_DATE_DATETIME_LOCAL_RANGE_CORRECTIONS=PASS");
    puts("PASS: VIEW0 V1N1 G05 C0-SR1 exact input-state applicability authority");
    return 0;
}
