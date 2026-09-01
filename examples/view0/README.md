# Arborcore VIEW0 D1 runnable examples

This directory is the canonical post-V1N4 runnable example surface for VIEW0.
It demonstrates the three already-qualified VIEW producer patterns without adding
a new production abstraction:

- T1 prepared template loaded from `page.html` at startup;
- C3-style typed native C compiled view;
- C4-style hand-written NASM consumer of the existing VIEW C API.

Build and run all three examples from the repository root:

```sh
make view0-d1-example-documents
```

The command writes three byte-identical HTML documents:

```text
build/view0-d1/documents/template.html
build/view0-d1/documents/native-c.html
build/view0-d1/documents/nasm.html
```

D1 qualification runs those generated documents through the current native V1N4
bounded 104-rule checker and requires zero diagnostics, zero tokenizer/tree errors,
and a parse-clean result. This is a bounded Arborcore qualification; it is not a
claim of complete HTML or ECMAScript conformance.

## Template lifetime

`page.html` is read during application-style startup. The example measures and
prepares it into caller-owned persistent storage before rendering. T1 preparation
copies trusted literal bytes and resolves the named `{{text}}` placeholder to a
numeric slot. The file buffer itself is not a request-time dependency after a
successful preparation.

Dynamic values are admitted only in ordinary HTML Data/text context and are escaped
through `arbor_view_html_text_*`. D1 does not generalize this mechanism to dynamic
attributes, URLs, CSS, JavaScript, raw text, comments, or XML.

## HTTP publication rule

The runnable renderer writes body bytes only. In the canonical M1 MVC/HTTP recipe,
the presenter must:

1. render the complete body;
2. validate it with `arbor_view_utf8_validate()`;
3. prepare the AF1 response plan;
4. append exactly one `Content-Type: text/html; charset=utf-8` field through HTTP1;
5. publish the already-valid response plan.

A controller or middleware that has already added `Content-Type` violates this
qualified composition precondition. D1 does not change HTTP1's general ordered
response-field semantics or silently de-duplicate fields.

## Assembly stability

`nasm_view.asm` is an application-style C4 consumer. It calls the existing VIEW C
functions and frozen Assembly range helpers under the System V AMD64 ABI. It adds no
new production Assembly symbol and does not reopen Assembly ABI v1. The existing C4
ABI qualification remains the authoritative stability check and is rerun by the D1
gate.
