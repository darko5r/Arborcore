# Arborcore VIEW0

## Status

VIEW0 is under incremental construction above frozen HTTP1. C1 establishes its
generic bounded output transaction and is source-review closed at C1 R1. C2
adds ordinary HTML Data/text-context measurement/escaping and is source-review
closed at C2 R1. C3 qualifies the typed native C compiled-view pattern and is
source-review closed at C3 R1. C4 qualifies a real hand-written NASM consumer
of the same VIEW C boundary. T1 adds minimal prepared templates. M1 now admits
user-facing UTF-8 HTML through the existing MVC0 presenter and HTTP1 metadata
path; full document conformance remains a separate V1 development-time layer.

V1 is now review-complete through V1N4 for its frozen 104-rule development-time
admission matrix. The NC1 native corpus freezes 832 case IDs and reproduces 1,759
records per pass across two exact historical-version replays. This is a bounded
native qualification, not a claim of complete HTML or ECMAScript conformance.
D1 remains blocked on the post-V1N4 manuals, runnable-examples, and documentation-
consistency gate.

## Architectural role

VIEW is the presentation boundary; it is not synonymous with a template engine.
A producer (for example native C, Assembly through the C ABI, a prepared
template, or later C/WASM) produces a representation through VIEW. HTML/CSS
remain first-class standard browser presentation, while the precision surface
remains parallel and optional.

VIEW0 does not query databases and does not know which persistence adapter an
application uses. Database results must reach VIEW through application/domain
boundaries. MariaDB is planned as Arborcore's first concrete database adapter
after HELLO0, not as a framework requirement.

## C1: bounded output transaction

C1 composes existing qualified low-level mechanisms rather than reimplementing
them in C:

- `u64_add_checked` provides overflow-checked measurement;
- `arena_mark`, `arena_alloc`, and `arena_rewind` provide request-lifetime
  allocation and rollback;
- `buffer_init` and `buffer_append` provide bounded writes and snapshot alias
  semantics.

A caller zero-initializes `arbor_view_output`, measures the exact byte length,
begins an output transaction, appends exactly that many bytes, and commits to
an `arbor_span`. The returned span borrows request-arena bytes.

The active transaction owns the arena allocation frontier until commit or
abort. Other allocations from the same arena during that interval are
prohibited because abort must be able to rewind only VIEW's allocation.

Once active transaction metadata and the arena frontier validate, detected
append failures rewind automatically. Commit likewise rewinds under-fill and
invalid publication outputs after that validation. If metadata/frontier
validation itself fails, VIEW fails closed without guessing at a rewind target;
callers must treat that as transaction corruption and must not assume rollback
occurred. Higher-level renderers must call explicit abort when their own
processing fails between successful C1 appends.

The public transaction structure is caller-owned storage but framework-private
state by contract. Callers zero-initialize it before first use and must not
inspect or mutate its fields while a transaction is active. The arena, output
metadata, publication output, and non-empty append sources must also be valid
live objects/spans with the required read/write access for the duration of each
call. Integer range checks establish representability; they cannot make a
dangling or inaccessible C pointer safe. Shared transaction metadata or a
shared arena therefore requires external synchronization; VIEW0 C1 provides no
internal locking.

`arbor_view_output_append()` is representation-agnostic raw output. In an HTML
producer it is appropriate for trusted static markup or bytes already known to
be safe for their exact HTML context. It must not be treated as a generic
untrusted-dynamic-HTML API.

## C2: ordinary HTML text-context escaping

C2 adds two operations:

- `arbor_view_html_text_measure()` computes the exact escaped byte length and
  updates a measurement only if the complete calculation succeeds;
- `arbor_view_html_text_append()` emits dynamic text into an active C1 output
  transaction.

The normative parser basis is the WHATWG HTML syntax for normal element text
and the tokenizer Data state. In that context a literal `<` can start markup
and `&` can start a character reference. C2 serializes:

- `&` as `&amp;`;
- `<` as `&lt;`;
- `>` as `&gt;`.

Escaping `>` is an Arborcore conservative canonical-output choice; ordinary
HTML Data-state safety does not require it. Double quote (`"`) and apostrophe
(`'`) remain literal because C2 is deliberately not an attribute-value
escaper. Existing entity-looking input such as `&copy;` is dynamic text, not
trusted markup, and therefore becomes `&amp;copy;` so that the displayed text is
preserved rather than reinterpreted as a character reference.

C2 explicitly does not admit interpolation into HTML attributes, URLs, CSS,
JavaScript, raw-text/RCDATA, comments, or XML. Those contexts have different
parsing and escaping rules and require separate future contracts. There is no
`escape_everything()` API.

Measurement starts from the input byte length and uses checked additions only
for bytes that expand. The worst-case output is five bytes per input byte
(all ampersands). Appending copies maximal ordinary-byte runs through the
qualified C1 bounded append and emits fixed character-reference bytes only at
escape points. The writable `arbor_view_measure` must not overlap the borrowed
text span: measurement is logically read-only with respect to source bytes and
rejects that alias rather than allowing the result write to mutate its input.

Measurement is not a snapshot. A higher-level two-pass renderer that measures a
dynamic value and later appends it must keep that value byte-stable across the
pair (and externally synchronize any shared producer). A mutation that changes
escaped length is caught by C1 capacity/exact-commit failure, but a mutation with
the same escaped length need not be detectable; append-time bytes would win.
Therefore source mutation between the paired operations is prohibited for a
coherent render.

A dynamic text source may live in an earlier non-overlapping portion of the
same request arena, which is useful for request/model data rendered into a body
allocated later in that arena. The source must not overlap the active output
body, VIEW transaction metadata, or arena metadata because C2 is a transforming
multi-write operation and cannot promise C1's single-append snapshot semantics
for such aliases. Detected source-policy failures after active output validation
rewind the C1 transaction.

C2 is byte-oriented and preserves every non-escape byte exactly in the
serialized byte stream. That is not a claim that arbitrary byte sequences
round-trip unchanged through the HTML parser or DOM: HTML parsing has separate
input-stream, control-character, tree-construction, and encoding rules. C2 does
not validate Unicode or UTF-8. The HTML integration boundary must establish
valid UTF-8 before user-facing HTML is qualified; C2 deliberately keeps that
representation-validity obligation separate from parser-context escaping.

C2 still contains no template parser, router, controller, HTTP serializer, heap
allocator, mutable registry, internal lock, database, or R runtime integration.

## C3: native C compiled views

C3 proves that a native compiled view is an ordinary application-defined,
typed C function that composes the C1 bounded-output transaction and C2 HTML
text-context escaper directly. C3 deliberately adds no generic renderer
callback, no `void *` model ABI, no runtime view registry, and no new production
VIEW symbol. A view can therefore keep its model type explicit, for example a
`hello_model`, and expose the function signature that best fits that
application module.

A compiled HTML view follows one synchronous two-pass operation:

1. measure trusted static literal byte counts with `arbor_view_measure_add()`;
2. measure each dynamic ordinary-text value with
   `arbor_view_html_text_measure()`;
3. reserve exactly the measured request-arena body with
   `arbor_view_output_begin()`;
4. append trusted static HTML/CSS-reference markup through the raw C1 append;
5. append untrusted ordinary element text only through C2;
6. publish only through `arbor_view_output_commit()`.

The model and all borrowed spans must remain live and byte-stable for that
synchronous measure/render operation. C3 does not snapshot a model. A compiled
view must also externally synchronize any shared mutable producer state.

Because the model layout is application-defined, the compiled view owns its
cross-role alias policy. Before beginning an output transaction, the canonical
C3 pattern rejects a model container that overlaps mutable arena metadata or a
future body reservation that can actually fit, and rejects a writable result
object that overlaps the model container or any borrowed model span. This
prevents rendering writes or final result publication from mutating the
caller-owned model. Model bytes or dynamic source bytes stored in an earlier,
non-overlapping region of the same request arena remain valid. If the measured
body cannot fit at all, no future reservation exists and ordinary C1 capacity
failure remains authoritative rather than being replaced by a speculative
alias error.

If application-specific view logic fails after a successful begin/append but
before another C1/C2 operation performs rollback, the compiled view must call
`arbor_view_output_abort()` explicitly before returning the mechanism failure.

Trusted literals are source-code/application-owned representation bytes whose
exact context the compiled view author controls. C3 does not make raw append a
safe route for untrusted markup. Dynamic HTML attribute, URL, CSS, JavaScript,
raw-text/RCDATA, comment, and XML interpolation remain outside the admitted
surface. A fixed external stylesheet reference can be emitted as trusted HTML
markup; this does not create a dynamic CSS interpolation mechanism.

C3 returns framework/mechanism failures through `arbor_status`. It introduces
no business-outcome encoding and does not involve MVC or HTTP yet. Rendered
body bytes remain request-arena owned and are published as a borrowed
`arbor_span`. The compiled-view function must not publish a body on failure.

C3 qualification uses only valid ASCII/UTF-8 HTML examples. General UTF-8
validation is still not performed by C1/C2/C3; the later user-facing MVC/HTTP
integration gate must establish the representation-validity policy before
claiming arbitrary dynamic HTML input is admitted.

The absence of a new C3 production API is intentional: direct C view support
is demonstrated by composition rather than by another abstraction layer. T1
will add prepared templates as a separate view mechanism over the same C1/C2
foundation, while C4 will qualify a real NASM caller of the existing VIEW C
API under the frozen SysV AMD64 ABI.



## C4: real NASM consumer of the VIEW C API

C4 proves that native Assembly is a first-class VIEW producer without creating
a second production view interface. The C4 renderer is test-only application-
style NASM. Production `include/arborcore/view.h` and `src/c/view.c` remain
byte-exact to C2 R1, the public VIEW function count remains seven, and frozen
Assembly ABI v1 is not reopened.

The NASM consumer performs the same two-pass composition as C3:

1. call `arbor_view_measure_add()` for trusted `<p>` markup;
2. call `arbor_view_html_text_measure()` for borrowed dynamic text;
3. add the trusted `</p>` suffix;
4. reject destructive source/result aliases before reserving a body;
5. call `arbor_view_output_begin()` for exact request-arena storage;
6. call `arbor_view_output_append()` for trusted literals;
7. call `arbor_view_html_text_append()` for C2 Data/text escaping;
8. call `arbor_view_output_commit()` to publish only the complete body.

A separate NASM helper begins and aborts a transaction so all seven existing
VIEW functions are consumed by a real NASM object. C4 also composes directly
with the already-frozen `range_end_checked`/`range_overlaps` Assembly helpers
for C3-equivalent pre-reservation alias analysis; those calls do not create a
new Assembly ABI.

### SysV ABI obligations

On the qualified Linux x86-64 target, the by-value `arbor_span` consists of two
INTEGER-class eightbytes, so the renderer observes `text.data` and `text.length`
in the next two integer argument registers after the arena pointer. The
16-byte `arbor_status` similarly consists of two INTEGER-class eightbytes, so
NASM observes/forwards its code and native status in the two integer return
registers. C4 checks both fields from C, rather than assuming that testing only
one register proves the aggregate ABI.

The NASM renderer uses callee-saved registers for values that must survive C
calls and restores them before returning. Its local frame also self-checks that
its call sites have the required 16-byte stack alignment. A second NASM caller
seeds all six SysV callee-saved general-purpose registers and verifies that the
renderer preserves them.

### Alias and lifetime policy

C4 does not weaken C3's source-stability rules. The borrowed dynamic text must
remain stable from measurement through append, and no snapshot is introduced.
An earlier, non-overlapping source in the same request arena is allowed. A
source that overlaps the body reservation that can actually be made is rejected
before any trusted literal can overwrite it. If the body cannot fit, the alias
precheck does not speculate about a reservation; C1 remains authoritative for
`ENOSPC` and other arena errors.

The C4 test renderer also rejects result metadata overlapping borrowed text or
mutable arena metadata. These are application-style recipe checks above the
seven-function VIEW core, not new generic production APIs.

### Scope deliberately not admitted by C4

C4 does not introduce a production Assembly renderer registry, a new Assembly
ABI v1 symbol, templates, dynamic attributes/URLs/CSS/JavaScript/XML, UTF-8
validation, MVC/HTTP transport integration, database access, R integration, or
browser-side WASM behavior. User-facing HTML admission still waits for the M1
representation/integration gate.


## T1: minimal prepared HTML templates

T1 adds a conventional template path without redefining VIEW as a template
engine. Native C compiled views and real NASM -> C VIEW composition remain
first-class alternatives.

The T1 source grammar recognizes named substitutions written as `{{name}}` or
`{{ name }}`. Names are case-sensitive ASCII identifiers matching
`[A-Za-z_][A-Za-z0-9_]*`; optional placeholder padding is limited to ASCII
space/tab. A field-name table supplied at preparation time defines the allowed
names and their stable runtime value-slot ordering. Duplicate field names are
rejected and an unknown placeholder is a preparation error.

T1's source scanner is deliberately a conservative security classifier, not a
complete HTML validator. A substitution is admitted only in the scanner's
ordinary HTML Data/text region. Substitutions are rejected inside tags and
attribute values, comments, and the raw/RCDATA-like contents of `script`,
`style`, `title`, `textarea`, `xmp`, `iframe`, `noembed`, `noframes`,
`noscript`, and `plaintext`. T1 rejects `svg` and `math` source elements rather
than pretending its HTML-text-only substitution policy covers foreign-content
parsing. Dynamic attributes, URLs, CSS, JavaScript, comments, raw/RCDATA and
XML remain future context-specific admissions.

The scanner also rejects a placeholder while a preceding trusted literal could
still be forming an HTML character reference, such as `&{{name}};` or
`&amp{{name}};`. `&amp;{{name}}` is admissible because the trusted character
reference is complete before the dynamic text begins. This prevents trusted
prefix bytes and dynamic value bytes from composing into a parser token that
would bypass C2's per-value escaping.

Literal `{{` has no dedicated T1 source escape. For visible brace text, a
template author can use trusted HTML character references or choose a compiled
C/Assembly view; a richer source escape may be admitted later. T1 is
intentionally minimal rather than accepting ambiguous grammar early.

### Preparation and ownership

Template source and field-name bytes are borrowed only for the synchronous
measurement/preparation call. Successful preparation copies all trusted literal
bytes into caller-owned persistent storage and resolves every placeholder name
to a numeric slot. The source buffer and field-name table may therefore be
released, reused, or reset after preparation succeeds.

The persistent storage consists of:

- a caller-owned typed `arbor_view_html_template_part` array;
- a caller-owned byte buffer containing copied trusted literal bytes;
- a caller-owned `arbor_view_html_template` descriptor.

`arbor_view_html_template_measure()` returns exact part and literal-byte
requirements. `arbor_view_html_template_prepare()` validates the full source and
field schema before writing, checks caller capacity/alias rules, then emits the
canonical immutable prepared representation. Expected grammar, binding,
capacity, and alias failures leave persistent output unpublished and backing
storage unchanged.

The prepared representation is application-lifetime immutable by contract.
Its literal parts point only into the caller's persistent literal copy, never
back into template source. HTML-text parts contain resolved numeric slots.
Runtime rendering validates the prepared canonical shape and guard fields so
accidental mutation fails closed.

### Runtime rendering

`arbor_view_html_template_render()` performs no source parsing and no field-name
lookup. It receives an exact value array whose order is the field declaration
order resolved during preparation. It measures the prepared parts through C1
and C2, reserves exactly one request-arena body, appends copied trusted literals
through C1 and dynamic values through the C2 HTML Data/text escaper, and
publishes only through C1 exact commit.

All value spans are borrowed and must remain live and byte-stable for that
synchronous measure/render operation. An earlier non-overlapping value in the
same request arena is allowed. A fitting future body must not overlap the
prepared template, its persistent backing, the value array, or any value span.
If the body cannot fit at all, C1 capacity failure remains authoritative rather
than being replaced by a speculative future-body alias error.

A writable result span must not alias the prepared representation or dynamic
value inputs. T1 adds no heap allocation, global template registry, refcount,
internal lock, database, R runtime, route/controller pipeline, or HTTP
serializer.

### Data structure and performance choice

Named-field resolution uses a deterministic linear scan of the caller's field
array during preparation only. The prepared part stores the numeric slot, so
request-time rendering is linear in prepared part count and performs zero string
lookups and zero parsing. T1 intentionally does not add a hash table: the
startup-only lookup is simple, bounded by the template field count, preserves
declaration order, and avoids memory/code complexity before HELLO0 provides
evidence that a more complex index is useful.

This choice follows the supplied C/data-structure references only as general
guidance: arrays and linear searching are ordinary tools whose tradeoffs should
be compared against trees/hashing. Arborcore-specific policy is stricter:
admit a more complex lookup structure only if realistic template/schema
measurement demonstrates a bottleneck without weakening determinism or
resource bounds.

### Encoding and later integration

T1 remains byte-oriented and does not claim general UTF-8 validation or
user-facing HTTP HTML admission. The later M1 MVC/HTTP integration must
establish valid UTF-8 representation policy before templates are admitted as
user-facing HTML. Static HTML/CSS remains first-class; an external stylesheet
or browser-host script can be referenced by trusted literal markup without
introducing dynamic CSS/JavaScript interpolation.

T1 does not load files itself. Applications may obtain template source through
their startup/configuration mechanism and then prepare it into persistent
caller-owned storage. D1 will provide the canonical manuals and runnable file-
based examples after the mechanism and integration boundaries are qualified.

## M1: MVC0 presenter + HTTP1 UTF-8 HTML integration

M1 connects the already-qualified VIEW mechanisms to the existing presentation
pipeline without inventing another presenter framework. An application-defined
MVC0 presenter renders through one of the admitted VIEW mechanisms, validates
the resulting byte sequence as UTF-8, constructs a temporary AF1 response plan,
adds `Content-Type: text/html; charset=utf-8` through the HTTP1 request-local
response-field sidecar, and only then publishes the response plan.

The sequence is deliberately ordered:

1. render a complete request-arena body through T1, the C3 compiled-view
   pattern, or the C4 NASM-to-C VIEW path;
2. call `arbor_view_utf8_validate()` on the complete body;
3. construct the AF1 `arbor_response_plan` in local presenter storage;
4. append the single HTML `Content-Type` field through HTTP1;
5. copy the already-valid plan to the presenter output.

Consequently, invalid UTF-8 is detected before the HTTP response-field sidecar
is changed. Failure to build the response plan likewise occurs before field
publication. If the HTTP1 field append fails, no response plan is published.
There is no later fallible framework operation in this canonical one-field
presenter sequence before the final plain C structure assignment.

`arbor_view_utf8_validate()` is a byte-preserving validation primitive matching
the well-formed UTF-8 scalar-value boundary used by the WHATWG Encoding
Standard's fatal decoding behavior. It accepts ASCII and valid 2-, 3-, and
4-byte encodings; rejects overlong forms, surrogate encodings, stray or
truncated continuation bytes, and values above U+10FFFF; and treats an encoded
U+FEFF BOM as valid UTF-8 without stripping it. It performs no normalization and
is not an HTML validator.

M1's real-socket qualification drives the same MVC0 -> HTTP1 -> HTTP0 -> rich
Assembly transport path for three application-defined presenters: a prepared
T1 template, a typed native C compiled view, and the real C4 NASM consumer.
Each produces identical escaped HTML from valid non-ASCII model bytes and is
serialized with exactly one `Content-Type: text/html; charset=utf-8` field.
An invalid UTF-8 model rendered by the template path is rejected with `EILSEQ`
before response bytes are serialized. A zero-capacity HTTP1 response-field
sidecar likewise returns `ENOSPC` at the `Content-Type` append and publishes no
response bytes, proving metadata failure does not leak a response plan.

This is the first VIEW0 increment allowed to call its HTML output user-facing,
but the claim is intentionally bounded. M1 establishes representation encoding,
context-safe dynamic text, response metadata, body lifetime, and real transport.
It does not yet claim that an arbitrary complete document satisfies every WHATWG
HTML authoring/content-model rule. V1 supplies that separate development-time
conformance layer before D1 manuals/examples are admitted.

M1 adds no database, R runtime, browser authority change, hidden heap, global
registry, new MVC pipeline, new HTTP serializer, or new Assembly ABI. The
presenter remains application-defined so HELLO0 can reveal whether a convenience
wrapper is actually justified by real framework ergonomics.


## V1: development-time whole-document HTML conformance

V1 keeps whole-document standards conformance separate from both T1's
substitution-context security scanner and M1's UTF-8 representation validator.
The distinction is deliberate: a template can place dynamic text in a safe HTML
Data context while the complete document is still nonconforming because of
content-model, document-structure, attribute, obsolete-feature, or other authoring
rules. Conversely, a conformance checker does not replace context-correct escaping
of untrusted dynamic values.

V1 is development/build/qualification tooling only. It adds no `arbor_view_*`
function, request-path work, production dependency, MVC/HTTP adapter, hidden heap,
database dependency, or R runtime. The C1-C4/T1/M1 production boundary remains
byte-exact.

The initial oracle is the upstream Nu HTML Checker `vnu.jar`. Arborcore pins the
accepted checker bytes by SHA-256 rather than trusting the upstream moving
`latest` release URL as a stable identity:

```text
c70279e260e5f4f9e95b3890ef6c9548f90ebdaec2a75219ec41694ab6241e34
```

This V1 oracle revision corresponds to the upstream `latest` release at
validator/validator commit `01d1e57683dd6e995c95d60173a06e58c6cb5699`, whose `vnu.jar` asset was published at
`2026-08-15T02:23:54Z`. The release tag remains intentionally moving; the digest above
is therefore the Arborcore qualification identity.

The jar remains ignored historical qualification evidence and is never committed.
A later R3 attempt to make that Java oracle reconstructible from a clean clone did
not establish the intended versioned-package reconstruction path: it fell back to
the moving GitHub release. That failed reconstruction attempt occurred after the
R3 tooling text had been written, so the earlier clean-clone claim is superseded
rather than promoted. The user then explicitly chose to remove Java from the
future Arborcore toolchain and pivot V1 to a native C checker.

The accepted Nu result is therefore historical, nonauthoritative differential
evidence only. `vnu.jar`, Java, npm, and the moving GitHub checker URL are not
required by the active V1N0 gate. Existing ignored jar bytes need not be deleted;
they may be retained for forensic comparison while the native replacement is
qualified.

The qualification generator renders three complete UTF-8 HTML documents through
the already-admitted mechanisms: a T1 prepared template, the C3 native-C compiled
view pattern, and the C4 real NASM-to-C VIEW path. Each document includes the HTML
document doctype, `lang`, UTF-8 metadata, title, ordinary body structure, escaped
dynamic text, and a trusted external stylesheet reference. V1 runs the pinned
checker with language detection disabled and treats both checker errors and
warnings as blocking for VIEW0-qualified artifacts. Warnings are a stricter
Arborcore admission policy; they are not redefined as HTML conformance errors.

Two intentionally nonconforming control documents must be rejected by the same
checker, proving that the oracle is actually active rather than merely returning
success. Generated artifacts are written beneath `build/view0-v1/` and are not
source authority.

The original plan to defer a native checker until after HELLO0 was superseded by an
explicit pre-HELLO0 architecture decision. Lexbor v3.0.0 is admitted as the private
C99 tokenizer/tree-builder/DOM foundation, while Arborcore will own the
machine-checkable authoring-rule layer in C. Lexbor is development tooling only;
its types do not enter public Arborcore headers and it is never linked into the
production request path. A custom Arborcore HTML parser is not justified by the
current evidence.

For the M1 HTML presenter recipe, `Content-Type` remains an exclusive
representation-presenter responsibility. A middleware/controller that has already
added `Content-Type` violates the qualified composition precondition; V1 does not
reopen HTTP1's generic ordered duplicate-field semantics. D1 manuals and runnable
examples must state this rule and must pass the V1 conformance gate before they are
marked QUALIFIED.

### V1 oracle reconstruction stability

The Java-oracle reconstruction experiment is historical and no longer an active
qualification dependency. V1N0 instead pins the Lexbor source by tag, commit, Git
tree, deterministic `git archive`, and a canonical source manifest built from
C-locale-sorted relative paths. A clean source cache must reproduce all of those
identities before compilation.

### V1 lower-gate extension awareness

V1 advances the current top-level VIEW contract revision while preserving the full M1
subcontract.  Therefore the M1 regression contract verifier checks the retained M1
markers and a public-function floor rather than requiring the historical top-level
`ARBORCORE_VIEW_CORE_VERSION=0.1-VIEW0-M1` string.  Requiring that historical
current-version marker under V1 is a verifier-only false failure, not an M1 semantic
regression.


## V1N0: native C/Lexbor parse-conformance foundation

V1N0 is the first construction phase of the native replacement. It is deliberately
not the complete HTML authoring validator. Its only job is to establish a precise
private C boundary around Lexbor v3.0.0, retain M1 UTF-8 as the byte-encoding
authority, translate tokenizer and tree-builder parse errors into deterministic
Arborcore diagnostics, enforce resource bounds, and prove that no Java/JAR process
is needed by the active gate.

The exact Lexbor identity is tag `v3.0.0`, commit
`2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe`, tree
`70da8da84cabdc4f02d47378602c41090b2b610c`, canonical relative-path source
manifest `a38edb39fe84f7fff90ff6206e6114aa3edab3c75ff363abaa11ee200d23e20d`
over 1055 tracked files, and deterministic Git archive
`b738cffc343868268d59109be5a1378dc854bfc06ddd5564954060398d3016e6`.
Lexbor source and build trees live only beneath ignored `build/view0-v1/native/`;
there is no Git submodule and no system-package authority. Lexbor headers are
included through `-isystem`, preserving Arborcore's strict `-Werror`, conversion,
shadow, prototype, and format warnings for Arborcore-owned C without pretending
third-party inline code follows the same warning policy.

The conformance adapter explicitly reverses Lexbor's full-parser duplicate-attribute
optimization with `lxb_html_tokenizer_keep_duplicate_set(..., false)` and enables
input-stream validation with `lxb_html_tokenizer_input_validation_set(..., true)`.
Preconstruction evidence proved the expected duplicate-attribute,
control-character-in-input-stream, unexpected-question-mark, and doctype-in-body
error paths. Tokenizer error positions and tree-error begin/end ranges were also
proved to point into the original borrowed input. The input must therefore remain
live and immutable until those pointers have been converted to integer offsets and
line/column diagnostics; no Lexbor pointer survives that call.

V1N0 diagnostics are private development-tool structures (352 bytes on qualified x86-64). Each record carries a
stable numeric rule ID, copied symbolic name, severity, byte offset, source length,
one-based line, one-based byte column, origin, external parser error ID, deterministic
message, and discovery sequence. Diagnostics are sorted by byte offset, rule ID,
severity, then discovery sequence. Caller-owned diagnostic storage is bounded to
4096 records and input is bounded to 1 MiB for this first foundation. Qualification
runs the checker under a 256 MiB address-space guard and a 10-second wall-clock
guard. These values are foundation limits, not performance targets; widening them
requires a controlled evidence-backed revision.

Exit status `0` means only that the document is parse-clean for the implemented
foundation. Exit `1` means a UTF-8/tokenizer/tree parse violation, `2` means usage
or input-contract failure, and `3` means resource/mechanism/internal failure. V1N0
implements zero authoring-rule groups, so it must never label its result as complete
HTML conformance. The subsequent sequence is V1N1 groups 02-06, V1N2 groups 07-11,
V1N3 groups 12-16, and V1N4 full rule-matrix/negative-corpus/differential review
before any complete-conformance claim or D1 admission.

Lexbor parse errors that use its explicit tokenizer EOF sentinel are normalized to the one-past-end byte offset `input.length`; any other out-of-input parser pointer fails as a mechanism error rather than being published as a misleading source location.

V1N0 treats a zero-length span (including `{{NULL, 0}}`) as parse-clean for the
foundation; the later document-structure rule group is responsible for authoring
requirements such as the required HTML doctype. Lexbor human error text is copied
for messages, while the private symbolic diagnostic name deterministically replaces
ASCII spaces with hyphens beneath an origin prefix such as
`html.parse.tokenizer.` or `html.parse.tree.`. All Lexbor errors are counted and
source/text-preflighted before any caller diagnostic is published, preserving
failure atomicity on bounded-capacity and mechanism errors.


### V1N0 R1: fixture-derived diagnostic-location verification

The initial V1N0 authoritative gate reached the native CLI controls after the
foundation implementation, internal tests, canonical template/native-C/NASM
parse-clean checks, and lower-layer regression had passed. It then stopped on a
verifier-only false failure: `parse-error-tokenizer.html` stores the Lexbor
unexpected-question-mark error at byte offset 114, which is line 4 column 8 under
the contracted one-based byte-column convention. The verifier had incorrectly
hard-coded column 7, which is the preceding `<` byte. No production VIEW source,
Lexbor adapter, or native checker behavior was implicated.

R1 does not merely replace 7 with 8. The native foundation verifier derives the
expected tokenizer location from the exact fixture's `?` byte and derives the
tree-control location from the `d` in the second `doctype` keyword. Exact fixture
bytes remain part of the cumulative candidate identity, so this provides an
independent source-text oracle for CLI location reporting and avoids duplicating
manual column arithmetic in the gate. The accepted expectations are tokenizer
`byte=114 length=1 line=4 column=8` and tree `byte=110 length=7 line=4 column=9`.
V1N0 R1 changes verification/traceability only; the private native C checker,
Lexbor adapter, production VIEW API, and all browser artifacts remain unchanged.


### V1N0 R2: source-review boundary hardening

V1N0 R1 passed its native foundation gate, then a separate source review held V1N1
on three development-tool boundary findings. R2 closes those findings without
changing the production VIEW API, the Lexbor adapter semantics, the parser choice,
or the zero-authoring-rule-group scope.

First, an exact Lexbor source worktree now means more than the accepted commit,
tree, tracked-file manifest, and Git archive. Ordinary untracked files were already
rejected in R1; R2 additionally asks Git to surface ignored files and rejects any
modified, untracked, or ignored source-tree file outside Git metadata. This policy
applies to both an explicit local-source override and the persistent ignored cache.
The qualification suite proves the ignored-extra case in an isolated temporary
override rather than contaminating the live cache.

Second, the authoritative V1N0 gate revalidates the exact Lexbor source and removes
only V1N0-derived release/sanitizer build directories, native objects, checker/test
executables, and sanitizer executable before qualification. The source cache,
historical Java evidence, canonical browser documents, and unrelated Arborcore build
evidence are preserved. Missing derived targets then force Make/CMake to reconstruct
the release and sanitized Lexbor libraries and Arborcore native checker from the
verified source during that gate run. Incremental builds remain available outside
the authoritative gate.

Third, the command-line checker admits regular files only. It snapshots the initial
bounded size, reads exactly those bytes, performs a one-byte EOF probe, and repeats
`fstat()` to require the same regular-file type and size before publishing the
snapshot. A short read or extra byte is an input-contract failure rather than an
empty/truncated success. This does not claim to lock files against hostile concurrent
same-size rewriting; callers are responsible for supplying a stable regular file
during the short snapshot operation.

TSV output also has an explicit machine-boundary rule: FILE path bytes containing
TAB, LF, or CR are rejected before the file is opened and before any TSV record is
published. This keeps the first TSV field and one-record-per-line grammar
unambiguous without inventing an escaping dialect in V1N0. Human output retains the
ordinary filesystem path behavior.

Lexbor remains the exact v3.0.0 development parser. Its pinned source manifest
already covers the upstream `LICENSE` and `NOTICE`; Arborcore records Lexbor as an
Apache-2.0 third-party development dependency and must retain appropriate dependency
notice traceability before distribution. The upstream license files themselves are
not vendored into Arborcore merely because the ignored source cache exists.

R2 still implements zero HTML authoring-rule groups and still makes no complete
HTML-conformance claim. V1N1 remains held until the complete R2 gate passes and a
read-only post-R2 source review closes all three findings.

## V1N1 C0: private document-facts construction foundation

The corrected V1N1 F1-R2 rule/spec/test freeze is accepted before this
construction boundary. It contains 36 active G02-G06 rules. G02 has six active
rules; numeric IDs `0x0000000030020004` and `0x0000000030020005` are permanently
retired/reserved after the F1 semantic repair and are not reused. The corrected
matrix identity is
`2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94`,
and the exact pinned Optional-tags source slice has SHA-256
`8f6299138f9229e56c84f4d64f66f4ae63bf7317871f05759ce61e4d7428a93b`.

C0 does **not** implement those six G02 authoring rules. Its problem is narrower:
V1N0 deliberately destroys the Lexbor document before returning from the private
adapter, while the corrected G02 matrix requires a mixture of original-source
facts and repaired-DOM facts. For example, duplicate `body` start tags can be
merged by the HTML tree builder, while doctype syntax must be evaluated from the
author's original doctype name/identifier bytes rather than from a serialized
repaired tree. Moving the G02 rules into `lexbor_adapter.c` would make Lexbor the
authoring-rule authority and would couple Arborcore's rule layer to third-party
parser types. C0 therefore inserts a value-data facts boundary instead.

During one synchronous parse, the adapter wraps Lexbor's existing token-completion
callback. The wrapper records only the relevant original-source evidence and then
forwards every token to the original Lexbor tree callback with its original
context. This preserves ordinary tree construction. For the first doctype it
records bounded offsets and lengths for the `DOCTYPE` keyword, doctype name,
external keyword, public identifier, and system identifier. For `title`, `base`,
and `body` it records source start-tag counts and the second-start-tag offset.
After parsing, while the Lexbor document is still alive, the adapter records the
corresponding document/root/head/body/title/base DOM cardinalities. Raw source-tag
aggregates are evidence only; they are not themselves conformance decisions.

The facts structure contains only `uint64_t` value fields and is 184 bytes on the
qualified x86-64 build. It contains no pointer and no Lexbor type. Source slices
are represented by offsets and lengths into the still-borrowed immutable input;
`UINT64_MAX` is the unavailable-offset sentinel. The callback context itself is a
stack-owned object whose lifetime is exactly the synchronous `lxb_html_parse()`
call. It borrows the input, the downstream callback/context, and caller-local facts
storage. Nothing from that context, and no Lexbor object pointer, survives the
adapter return.

This ownership choice is intentional. It avoids hidden heap ownership in Arborcore,
prevents a callback context from becoming a dangling pointer, and keeps destruction
simple: Lexbor continues to own and destroy its parser/document objects, while
Arborcore owns only local value data. It also keeps the boundary ABI-neutral with
respect to production VIEW: C0 adds zero public `arbor_view_*` functions and does
not reopen Assembly ABI v1.

Aliasing is explicit at this private boundary as well: borrowed input, diagnostic
storage, parse-count output, and facts output must be mutually disjoint. The adapter
validates those ranges before it creates the parser, so an accidental overlapping
future internal call fails with `EINVAL` rather than overwriting borrowed source or
published diagnostic storage.

Publication remains failure-atomic. The adapter accumulates source facts, DOM
facts, parse counts, and diagnostic preflight results in local storage. It publishes
parse counts and document facts only after all bounded-capacity and parser-pointer
checks succeed. Existing diagnostic publication still occurs only after the full
error count and text/source preflight, so no new partial-result state is admitted.
A facts-capacity concept is unnecessary because the facts object has fixed size;
input remains bounded to the V1N0 1 MiB limit and diagnostics remain bounded to
4096 records.

The security consequence is primarily reduction of authority and lifetime risk:
Lexbor-specific pointers remain confined to one private adapter translation unit,
source-derived locations are validated against the borrowed input before becoming
integer offsets, and raw text/comment bytes such as the characters `<body>` do not
become structural evidence unless Lexbor actually tokenizes them as start tags.
The cost is a constant-size local facts object, a constant amount of work for the
small set of relevant tokens, and bounded direct-child DOM counting. C0 adds no
request-path cost because the entire V1 checker remains development/qualification
tooling.

C0 also makes the retained V1N0 regression verifiers extension-aware. They still
require every historical V1N0 path and contract marker, but no longer require the
historical V1N0 top-level version string or an exact 71-path ceiling after an
admitted V1N1 extension. This is a verifier compatibility change, not a relaxation
of V1N0 parser semantics.

After C0 qualification, G02 construction may consume this facts substrate, but it
must still implement each frozen rule in the Arborcore-owned rule layer. C0 itself
reports zero implemented authoring-rule groups and zero implemented G02 rules.
G03-G06 remain untouched.


## V1N1 G02 R1: required document DOCTYPE

G02 construction begins with exactly one frozen semantic rule: numeric ID
`0x0000000030020001`, symbolic name `ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED`.
The corrected F1-R2 matrix says a standalone conforming HTML document requires a
DOCTYPE preamble. Arborcore evaluates this as a document-level authoring rule: a
Lexbor-created document with no document doctype node receives one ERROR diagnostic
anchored deterministically at byte 0 with source length 0. The authoring rule is
separate from parser acceptance; Lexbor accepting a document without a doctype does
not make that authored document conforming.

C0 already proved that parser diagnostics and document facts can be collected without
leaking Lexbor objects. R1 adds a private **measurement pass** before publication.
This is a correctness choice, not a parser optimization: the measurement pass learns
parser-error cardinality and the fixed-size C0 facts without writing caller diagnostic
storage. Arborcore can then add the one possible DOCTYPE-required diagnostic to that
count and reject insufficient capacity before a second parse publishes anything.
Thus ENOSPC preserves the pre-existing whole-check failure-atomicity contract even
when parser errors and Arborcore authoring diagnostics share one caller-owned array.
The cost is a second development-time parse for non-empty UTF-8 input while this
architecture is in force; no request-path or production VIEW cost is introduced.
Future optimization requires measurement evidence and may not weaken publication
atomicity.

The existing `PARSE_CLEAN` result bit keeps its literal parser meaning rather than
being redefined as conformance-clean. The exact F1 negative fixture that starts with an
`html` start tag and omits the doctype produces Lexbor's existing initial-mode tree
error **and** the stable G02 authoring diagnostic; Arborcore preserves both. The frozen
V1N0 empty-input foundation special case remains parser-clean, but G02 R1 now reports
the missing-doctype authoring violation for that empty document. V1N0 regression
checks are therefore extension-aware: they continue to test tokenizer/tree cleanliness
without requiring every higher-stage authoring checker invocation to have zero total
diagnostics.

The C0 facts adapter itself remains rule-semantic free. `lexbor_adapter.c` measures and
collects parser/fact evidence; the stable G02 numeric ID, symbolic name, severity,
message, and authoring decision remain in Arborcore-owned `native.c`. No generic
callback registry, `void *` model, hidden Arborcore heap, Java/JAR path, public
`arbor_view_*` function, database dependency, or R dependency is introduced. R1
implements **1 of 6** active G02 diagnostics and does not implement G03-G06.


## V1N1 G02 R2: DOCTYPE authoring syntax

G02 R2 adds exactly the second active G02 diagnostic, stable numeric ID
`0x0000000030020002` and symbolic name `ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX`.
R1 remains authoritative for the separate requirement that a standalone document
have a doctype at all. R2 runs only when the C0 source evidence contains an actual
DOCTYPE token; a malformed declaration that never becomes a DOCTYPE token remains
visible through the existing tokenizer/tree diagnostics and, when no document
doctype exists, R1's required-doctype diagnostic.

The frozen WHATWG source defines the author syntax directly. The short form is
`<!DOCTYPE html>` case-insensitively for `DOCTYPE` and `html`, with one or more
ASCII-whitespace bytes between them and zero or more ASCII-whitespace bytes before
`>`. The admitted generator-compatibility form additionally contains one or more
ASCII-whitespace bytes, `SYSTEM` as an ASCII-case-insensitive match, one or more
ASCII-whitespace bytes, and the exact literal `about:legacy-compat` enclosed in a
matching single-quote or double-quote pair. The quoted literal itself is
case-sensitive. For this rule ASCII whitespace is exactly HT, LF, FF, CR, and SPACE.
`PUBLIC` identifiers and arbitrary system identifiers are therefore not admitted
G02 R2 authoring forms even when Lexbor can tokenize them.

R2 deliberately does not place this semantic grammar inside `lexbor_adapter.c`.
The C0 adapter remains an evidence boundary and carries no stable G02 rule IDs.
`native.c` begins from C0's first-doctype source anchor and scans only the borrowed
immutable input bytes necessary for this small frozen authoring microsyntax. This is
not a second HTML parser: token recognition, tree construction, raw-text/comment
state, and parser diagnostics remain Lexbor's job. The native scanner merely decides
whether an already-recognized DOCTYPE matches the six source components frozen by
G02 R2.

The diagnostic is anchored to the first nonconforming doctype component rather than
blindly to document byte zero. For the frozen negative fixture `<!DOCTYPE svg>`, the
anchor is the three-byte `svg` name. For a `PUBLIC` form the anchor is `PUBLIC`; for
an incorrect legacy system identifier it is the identifier contents. Line/column
assignment remains the shared deterministic byte-based mechanism after all parser and
authoring diagnostics have been sorted.

Parser diagnostics are preserved rather than de-duplicated. For example Lexbor emits
its existing initial-mode bad-doctype tree error for `<!DOCTYPE svg>`, while Arborcore
also emits the stable G02 R2 authoring diagnostic. The two diagnostics answer
different questions: the parser error describes tree-construction processing and the
Arborcore diagnostic identifies the frozen author requirement.

The two-pass capacity model introduced by R1 remains unchanged. The measurement pass
obtains parser cardinality and C0 facts without publishing diagnostics; Arborcore then
adds the R1 and R2 authoring counts and fails with `ENOSPC` before the collection pass
if the caller's bounded array cannot hold the entire result. Thus adding R2 cannot
partially publish parser diagnostics while dropping the authoring diagnostic.

The legacy `SYSTEM "about:legacy-compat"` form is **syntax-valid** in R2. Its separate
WHATWG `should not be used` obligation belongs to G02 R3
`DOCTYPE_LEGACY_DISCOURAGED` and is intentionally not emitted yet. This separation
prevents an ERROR syntax rule from swallowing the later WARNING policy.

R2 adds no public `arbor_view_*` function, no production request-path work, no hidden
Arborcore heap allocation, no Java/JAR dependency, and no database or R dependency.
The production VIEW symbol count remains 11. With R2, Arborcore implements **2 of 6**
active G02 diagnostics; G03-G06 remain entirely unimplemented.


## V1N1 G02 R3: legacy DOCTYPE compatibility warning

G02 R3 adds exactly the third active G02 diagnostic, stable numeric ID
`0x0000000030020003` and symbolic name
`ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED`. The frozen WHATWG source permits
the legacy `SYSTEM "about:legacy-compat"` syntax for generators that cannot emit
the short doctype, but separately says that this legacy string **should not be
used** unless that generator limitation exists. Arborcore represents this author
SHOULD as a WARNING, exactly as frozen in F1-R2; R3 does not convert it into an
ERROR.

R2 remains the syntax authority. R3 fires only when the R2 doctype syntax evaluator
has accepted the legacy-compat form. An invalid `SYSTEM` value, `PUBLIC` form, wrong
quote pairing, or other R2 syntax violation receives the R2 ERROR and no R3 warning.
This prevents one malformed doctype from receiving both a syntax error and a policy
warning for a form it did not actually satisfy.

The warning is anchored to the exact 19-byte `about:legacy-compat` contents in the
borrowed immutable input. The C0 source facts already carry the system-identifier
offset/length, so the Lexbor adapter needs no new fact field, no new allocation and
no G02 semantic knowledge. The Arborcore rule layer confirms the exact source slice
only after R2 has declared the whole doctype syntax valid, then publishes the stable
R3 ID, WARNING severity and Arborcore-authoring origin.

R3 retains the two-pass failure-atomicity model from R1/R2. The measurement pass
counts parser diagnostics and collects C0 facts without caller-visible diagnostic
publication. Arborcore includes the possible warning in the required bounded
capacity before the collection pass. Therefore a warning that would overflow the
caller's diagnostic array yields `ENOSPC` with the result and diagnostic storage
unchanged rather than silently dropping the warning.

The existing `PARSE_CLEAN` bit remains parser-specific: a valid legacy doctype with
only the R3 warning is still parser-clean. R3 also refines the development CLI's
hard-failure policy so a **warning-only** result exits 0, while any ERROR diagnostic
still exits 1. The warning is still printed in human/TSV output and remains counted
in `diagnostic_count`; API consumers can inspect diagnostic severity directly.
This is the concrete consequence of the frozen requirement that R3 not turn the
WHATWG SHOULD into a hard conformance error.

R3 adds no public `arbor_view_*` function, no production request-path work, no
hidden Arborcore heap allocation, no Java/JAR dependency, and no database or R
dependency. The production VIEW symbol count remains 11. With R3, Arborcore
implements **3 of 6** active G02 diagnostics. The next active G02 rule is
`0x0000000030020006` `HEAD_TITLE_CARDINALITY`; retired IDs `...0004` and `...0005`
remain permanently reserved, and G03-G06 remain unimplemented.


## V1N1 G02 R4: standalone head/title cardinality

G02 R4 adds the fourth active G02 diagnostic, stable numeric ID
`0x0000000030020006` and symbolic name
`ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY`. The frozen `head` element source
distinguishes two modes: iframe `srcdoc` documents or documents whose title
information is supplied by a higher-level protocol may contain zero `title`
elements, while ordinary documents require exactly one. V1N1 qualification is
explicitly the `STANDALONE_DOCUMENT` mode, so R4 requires exactly one direct
`title` child in the parsed HTML `head`. Alternate modes remain deferred until a
future contract exposes them explicitly; R4 never guesses a mode from document
bytes.

The rule consumes C0 facts without extending their 184-byte layout. The decisive
fact is `dom_head_title_child_count`, because legal omission of literal `head`
tags and HTML parser insertion must not make source-token presence the rule
authority. An omitted `head` tag with a parsed `title` in the implied head is
therefore conforming for R4. Conversely, a `title` authored in `body` does not
satisfy R4 merely because a `title` token exists somewhere in the source; G03
will later own the separate context/content-model violation.

A missing title is an absence condition and is anchored deterministically at
document byte 0 with source length 0. If the final head contains more than one
`title`, C0's retained source evidence anchors the diagnostic to the five-byte
name of the second authored `title` start tag. The rule fails closed with a
mechanism error if a duplicate final-head title cannot be reconciled with that
source anchor rather than inventing a misleading location. Lexbor still contains
no stable G02 rule ID or policy.

R4 retains parser diagnostics, deterministic sorting/line-column assignment and
the two-pass bounded publication model introduced by R1. Parser cleanliness is
not redefined: a parser-clean document with no title or two head titles is
`parse_clean=yes` while still carrying the R4 ERROR. The empty-input foundation
special case remains parser-clean for historical V1N0 compatibility, but now
accumulates both the already-admitted R1 missing-doctype ERROR and the R4
missing-title ERROR. Capacity for both is checked before either diagnostic or the
result is published.

Specific G02 title-cardinality reporting owns this obligation before the future
generic G03 content-model evaluator, as frozen in F1-R2, so later G03 work must
not double-report the same missing/duplicate-title condition. R4 adds no public
`arbor_view_*` function, no production request-path work, no hidden Arborcore
heap allocation, no Java/JAR dependency, and no database or R dependency. The
production VIEW symbol count remains 11. With R4, Arborcore implements **4 of 6**
active G02 diagnostics. The next active G02 rule is `0x0000000030020007`
`HEAD_BASE_CARDINALITY`; retired IDs `...0004` and `...0005` remain permanently
reserved, and G03-G06 remain unimplemented.

## V1N1 G02 R5: head/base maximum cardinality

G02 R5 adds exactly the fifth active G02 diagnostic, stable numeric ID
`0x0000000030020007` and symbolic name
`ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY`. The frozen `head` content model permits
**no more than one** `base` element. Unlike R4 title cardinality, R5 is not an
exactly-one rule: zero `base` elements and one `base` element are both conforming.
Only a parsed document head containing more than one direct `base` child receives
the R5 ERROR.

R5 consumes the C0 facts that already exist: `dom_head_base_child_count`,
`source_base_start_tag_count`, and `source_second_base_start_tag_offset`. The C0
facts layout therefore remains 23 `uint64_t` fields / 184 bytes on x86-64. No Lexbor
type or object crosses the private adapter boundary and the adapter gains no G02
semantic knowledge.

The decision uses final DOM head-child cardinality as frozen by F1-R2. When the
count exceeds one, the diagnostic is anchored to the second authored `base`
start-tag name. The anchor length is four bytes. This preserves a deterministic
source location while keeping parser/tree behavior distinct from Arborcore's
authoring rule. A `base` used outside its admitted element context is **not** an R5
cardinality decision; later G03 element-context/content-model rules own that
separate obligation.

R5 retains the two-pass whole-check failure-atomicity contract. Parser diagnostics
and R1-R5 authoring diagnostics are counted before publication; insufficient caller
capacity yields `ENOSPC` without partial result or diagnostic writes. Parser
diagnostics remain preserved, and `PARSE_CLEAN` continues to mean zero tokenizer/tree
errors rather than zero conformance diagnostics.

The specific G02 base-cardinality diagnostic takes precedence over future generic G03
content-model reporting for the same duplicate-base obligation, preventing duplicate
diagnostics. R5 adds no public `arbor_view_*` function, no production request-path
work, no direct Arborcore heap allocation, no Java/JAR dependency, and no database or
R dependency. The production VIEW symbol count remains 11. With R5, Arborcore
implements **5 of 6** active G02 diagnostics; G03-G06 remain unimplemented.



## V1N1 G02 R6: logical body singleton and parser-repair provenance

G02 R6 adds the sixth and final active G02 diagnostic, stable numeric ID
`0x0000000030020008` and symbolic name `ARBOR_VIEW_V1_G02_BODY_SINGLETON`.
The pinned `body` source states that conforming documents have only one `body`
element, while the corrected F1 matrix requires one logical body and explicitly
forbids treating legal omission of the literal `body` start tag as an error.
R6 therefore does not require exactly one authored `<body>` token.

The rule consumes the unchanged 184-byte C0 facts boundary. A normal document
with zero authored body start tags is conforming when text/html parsing implies
exactly one HTML `body`. A document whose final HTML root has zero body children
is diagnosed at document byte 0/length 0. Multiple authored body start tags are
diagnosed even when Lexbor repairs them to the same single DOM body: the stable
authoring diagnostic is anchored to the four-byte name of the second authored
`body` start tag. This is why C0 preserved both
`source_body_start_tag_count` / `source_second_body_start_tag_offset` and
`dom_html_body_element_count`. Raw-text and comment text such as the characters
`<body>` are not structural body tokens and do not trigger R6.

R6 preserves Lexbor tokenizer/tree diagnostics rather than replacing them. The
frozen duplicate-body fixture produces parser-repair diagnostics and the stable
R6 authoring diagnostic together; `PARSE_CLEAN` continues to mean only that the
tokenizer/tree builders reported zero parse errors. The document-level phrase
"body as the second element in html" is not duplicated here: element context and
`html` content-model sequencing remain owned by G03 as established by the
corrected F1 taxonomy.

The rule layer still performs a measurement pass before publishing any caller-
visible diagnostics, so parser errors plus all six G02 diagnostics must fit the
caller-owned bounded array before any result bytes are committed. R6 adds no
heap allocation, public VIEW API, Lexbor type exposure, runtime request-path
work, Java/VNU dependency, database dependency, or R dependency. The production
VIEW symbol count remains 11 and the C0 facts layout remains 184 bytes.

With R6, Arborcore implements **6 of 6** active G02 rules. Retired/reserved IDs
`0x0000000030020004` and `0x0000000030020005` remain unused. This completes G02
construction only; G03-G06 remain unimplemented and G03 construction is not
authorized until the complete G02 group receives its own review/freeze boundary.

## V1N1 G03 C0: parser-repair provenance and neutral semantic observation substrate

G03 C0 adds **no G03 conformance rule**. It prepares the structural mechanism needed by the frozen seven-rule G03 group while retaining the accepted G02 GF1 group unchanged. The production VIEW API remains 11 functions and Lexbor remains a private development-time dependency.

The existing 184-byte G02 document-facts object remains unchanged. G03 needs arbitrary element parent/ancestor relationships, attributes, direct-child order, text/whitespace classification and source provenance after HTML tree repair, so C0 adds a parallel synchronous observation surface rather than turning the G02 facts object into a copied DOM.

For the exact pinned static Lexbor v3.0.0 development build, source provenance uses the qualified GNU-compatible linker `--wrap=lxb_html_interface_create` mechanism. The tokenizer callback exposes the currently authored start tag only while Lexbor synchronously creates its node; the wrapper delegates to the real Lexbor constructor and stores the borrowed source-name pointer in `node.user` for that authored node. Parser-synthesized nodes such as omitted `html`/`head`/`body`, implied `tbody`, and repair clones remain explicitly unanchored. No Lexbor source file is modified, no mutable global/TLS provenance registry is introduced, and observation fails closed if the link wrapper is absent.

The adapter traverses the live DOM iteratively through existing parent/child/sibling links. It exposes stable Arborcore standard-element IDs derived from the frozen F1-R2 113-token ordinal, namespace class, authored-source offset or synthetic marker, parent/grandparent IDs, fixed ancestor-presence bits, attributes, and a deterministic direct-child stream of Element/Text nodes. Direct Text observations classify only the HTML space bytes used for inter-element whitespace. No second tree is copied and no recursive C DOM walk is used.

`template` contents are intentionally not traversed. The pinned template source states that the template contents are a separate `DocumentFragment` rather than children of `template` and that those template contents have no conformance requirements. Treating them as ordinary G03 descendants would therefore manufacture false diagnostics.

All observation spans are borrowed for the callback duration only. The input remains caller-owned; Lexbor owns its document/nodes; adapter parse outputs remain failure-atomic. Consumer-owned observer context is explicitly not transactional and may have changed before a callback reports failure. Direct Arborcore heap allocation remains zero.

C0 implements zero G03 rule IDs. `ELEMENT_CONTEXT` (`0x0000000030030001`) remains forbidden until this substrate passes its own scope/contract/native/adversarial/analyzer/sanitizer gate.

## V1N1 G03 C0-L1: true DFS traversal lifetime retrofit

G03 C0-L1 extends the accepted neutral observation substrate without adding any
G03 conformance rule. The private semantic observer gains optional
`traversal_enter` and `traversal_leave` callbacks. `traversal_enter` is emitted at
the true iterative DFS entry of an element before the already-qualified
`element_begin` callback. On normal traversal, `traversal_leave` is emitted only
after every descendant element has been traversed. The existing
`element_begin -> attribute/direct-child -> element_complete` semantics are not
redefined; in particular, `element_complete` remains completion of the existing
per-element observation stream rather than DOM-subtree exit.

Enter and leave receive the same Lexbor-independent value observation vocabulary
as the existing element callbacks. The adapter removes the current element from
its fixed ancestor counters before constructing the leave observation, so
`ancestor_bits` excludes the current element and matches the corresponding enter
observation. All spans remain borrowed for callback duration only. No Lexbor
object escapes, no shadow DOM is created, and the traversal remains the same
iterative parent/child/sibling walk rather than recursive C traversal.

Any callback mechanism failure aborts synchronously. The parse counts, C0 facts
and observation counts remain unpublished/failure-atomic; the consumer-owned
observer context remains nontransactional. Once an earlier callback fails, later
leave callbacks are explicitly not guaranteed, so consumers must treat their
working state as scratch until the whole observation call succeeds.

The accepted resource correction qualifies the existing 4096-nested-`div`
stress case at maximum neutral DOM observation depth 4097 (`html` depth 0,
`body` depth 1, deepest nested `div` depth 4097). C0-L1 proves the lifecycle
mechanism delivers and balances that case. The **4097-inclusive R1 admission is
owned by the future G03 R1 evaluator**, not enforced as a generic C0 adapter
limit. This retrofit therefore does not add HTML rule semantics.

C0-L1 intentionally does not implement accessible-name computation. The
accepted F1-R4/M1-R3 sequence requires a separate bounded cross-node
accessible-name support review and construction before `ELEMENT_CONTEXT` can
be implemented completely. G03 rule IDs therefore remain zero, the 184-byte G02
document-facts layout remains unchanged, and the production VIEW API remains 11
functions.


## V1N1 G03 R1A: partial structural ELEMENT_CONTEXT evaluator

R1A activates the frozen `0x0000000030030001` `ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT` diagnostic over the accepted G03 C0-L1 neutral observation/lifecycle substrate. It is deliberately **partial**: structurally definite context violations are diagnosed, but the `main` ancestor branch that depends on a potentially named `form` is explicitly non-rejecting until a later accessibility/rendered-state phase can supply exact AccName inputs.

The frozen 62-alternative context catalog is tracked by `tests/data/view0_v1n1_g03_r1a_context_coverage.tsv`: 58 alternatives are implemented for stable DOM relations, two title/base alternatives retain the already-frozen G02-specific ownership, one `main` alternative is partial under the AN-P0 tri-state policy, and the compound-subdocument `html` alternative is not reachable in the currently admitted standalone HTML-document mode. Parser-repair-erased source relations remain future G03-R5 territory.

R1A uses the exact seven pinned body-ok link keywords, the accepted 4097-inclusive observation depth, true DFS enter/leave state for ordered ancestry and transparent context inheritance, and caller-stack/fixed workspace only. It adds no production VIEW API, no runtime request-path dependency, no Java, database, R, CSS/rendered-tree, shadow-DOM, or full accessibility engine.

`main` under a `form` with no `aria-label`, no `aria-labelledby`, and no `title` is statically definitely unnamed for this stage. A `form` carrying any of those potential naming attributes sets the explicit deferred result flag and is not rejected solely on that branch. The G13 custom-element overlap remains a non-rejecting ASCII-hyphen superset, not a claim of exact custom-element validity.

G03 R2-R7 remain unimplemented. G03 group freeze and any complete HTML-conformance claim remain forbidden while R1 is partial.

### V1N0 LX1: derived Lexbor ruby parser compatibility

The exact upstream Lexbor authority remains **v3.0.0** at commit
`2ae88a1c6b5261830eff73ee12bb3cdf805f3cfe` with canonical source manifest
`a38edb39fe84f7fff90ff6206e6114aa3edab3c75ff363abaa11ee200d23e20d`.
Arborcore does not modify that ignored canonical source cache.

R2A-P3 isolated a parser false positive in the `rp`/`rt` in-body handler. The
pinned WHATWG source slice (`47a21ff61645cdcb0146f1f474ecef3af2dabbc90343ccaeb7eaaee374ace086`)
permits the current node to be either `rtc` or `ruby`; exact Lexbor v3.0.0 used
`not rtc OR not ruby`, which reports an error for both allowed nodes. P3
reproduced the false positive and demonstrated that changing that one Boolean
operator to `AND` makes a valid ruby control parse-clean while preserving the
invalid `rt`-outside-ruby parse error.

The compatibility is applied only to an **ignored derived build source**
reconstructed from the exact canonical Git object on every Lexbor build. The
canonical cache stays byte-identical and remains the source authority. The one
changed derived file is pinned at
`142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8`,
and the complete 1,055-file derived source manifest is
`e5e126ad79684b69f42a81a356c268d7cce978d0a0b3948214550683007a15e5`.
This remains development-time tooling only: no production VIEW API, runtime
request path, Lexbor public-type boundary, or production linkage changes.

## V1N1 G03 R2A: partial residual CONTENT_MODEL evaluator

R2A activates the frozen `0x0000000030030002`
`ARBOR_VIEW_V1_G03_CONTENT_MODEL` diagnostic over the accepted R1A/LX1
foundation. It is deliberately **partial**. The accepted F1-R6 support plan and
R2A-P2 predicate ledger separate deterministic immediate-parent content-model
residuals from obligations already owned by R1, R3, R4, G04, G06, and from six
branches whose static authority is not yet sufficient.

R2 remains a parent-model diagnostic. A supported failure is anchored to the
authored parent element whose content model fails, and at most one R2 diagnostic
is emitted for a failing parent even when several residual predicates fail.
R1 remains child-context scoped. If an authored child relation is already
invalid under R1, R2A does not emit a second diagnostic for that same relation.
The accepted P2 construction ledger is retained byte-for-byte as
`tests/data/view0_v1n1_g03_r2a_predicate_coverage.tsv`.

The admitted structural residuals include the `dl` alternative and nested group
requirements, the individually-valid first+last double-`figcaption` case,
`datalist` alternative consistency, required `summary`, `selectedcontent`
cardinality for the admitted customizable-select button, non-whitespace `title`,
list/hgroup/picture structural residuals, select/optgroup residuals, exact ruby
base/annotation repetition, and immediate-parent residuals for special R1
ancestor-admitted relations. Source misuse erased or reparented by HTML tree
construction is not fabricated from the repaired DOM; those cases remain on the
accepted parser/R5 boundary.

Ruby R2A qualification consumes the V1N0 LX1 derived Lexbor compatibility build.
The canonical Lexbor v3.0.0 cache remains immutable. LX1 changes only the
derived build copy's qualified `rp`/`rt` Boolean from `OR` to `AND`, matching the
pinned WHATWG current-node rule and preserving the invalid `rt` control.

Six R2 branches are explicitly non-rejecting at this stage:

- conformant stylesheet semantics for `style`;
- script type/language/content semantics;
- `noscript` content selection without an explicit checker scripting mode;
- the `select` first-button decision when an authored `size` value requires the
  shared G06 non-negative-integer scanner;
- the platform-dependent `multiple` + display-size-1 select rendering branch;
- category membership of unclassified foreign/custom direct children.

`Nothing` is delegated to G03 R4, transparent-model derivation to G04,
descendant-wide exclusions to G03 R3, and `time` date/time lexical validity to
G06. R2A does not preimplement those later rule identities.

R2A uses the existing neutral C0/C0-L1 attribute/direct-child/DFS observations;
no adapter retrofit, shadow DOM, second parser, or Lexbor pointer escape is
introduced. Its fixed x86-64 frame is 160 bytes and its complete evaluator
workspace is 655736 bytes, caller-stack/fixed and below the existing 1 MiB
admission ceiling. Direct Arborcore heap allocation remains zero. The production
VIEW API remains 11 functions and R2A remains development-time tooling only.

R1 and R2 are both still incomplete. G03 R3-R7 are not implemented, G03 group
freeze remains forbidden, and no complete HTML-conformance claim is made.

## V1N1 G03 R3A: partial `DESCENDANT_EXCLUSIONS` evaluator

R3A adds the third G03 authoring rule as development-time native C tooling only. It reuses the accepted C0/C0-L1 iterative DOM lifecycle, R1A element-context evaluator, R2A content-model boundary, and V1N0 LX1 derived Lexbor compatibility build; it adds no production `arbor_view_*` function and does not reopen MVC0/HTTP0/HTTP1.

The accepted F1-R7 support plan covers 20 standard-element definitions: 13 predicates are implemented statically, six parents have definite partial subsets (`option`, `legend`, `button`, `a`, `label`, `canvas`), and `noscript` remains mode-deferred. Five branch families stay explicitly non-rejecting until their owning mechanisms exist: authored `input[type]` interactive classification, exact labeled-control resolution, canvas authored input-state exceptions, canvas `select[size]` interpretation, and checker scripting mode. Parser-repair-erased source relationships remain G03-R5-owned.

R3A keeps descendant restrictions as bounded active predicates during the existing iterative DFS. A descendant is evaluated only after its authored attributes are observable; parent restrictions activate afterward and are removed on lifecycle leave. No DOM copy, recursion, hidden registry, or Arborcore heap allocation is introduced. The fixed R3A context is 262472 bytes on x86-64 (64-byte frame, 4098 frames), with a 32768-byte R1 source-offset scratch array. R1 collection and the R3 traversal are deliberately separate stack phases: the R3 traversal helper is marked noinline so the compiler cannot co-reside the R3 context with R1A's 786872-byte evaluator frame. The gate checks compiler stack-usage evidence against 900000-byte R1-phase and 400000-byte R3-phase bounds; the admitted ceiling remains 1 MiB.

R1 retains higher diagnostic ownership for a standard element whose own context is already invalid. To avoid duplicating R1 logic, R3A uses a private R1A source-offset collection mode that executes the exact existing R1A evaluator and returns only R1-owned source offsets. This is a private construction mechanism: it adds no public VIEW API and no new R1 semantics. R2A already delegates descendant-wide restrictions to R3, so no duplicate R2 evaluator is introduced.

R3A emits at most one `ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS` diagnostic for each authored forbidden descendant, anchored to that descendant's authored start-tag name byte range even if multiple active ancestors forbid it. Measurement precedes publication so aggregate diagnostic-capacity failure remains atomic and deterministic.

R3 is still partial after R3A. R1 and R2 are also still partial, R4-R7 remain unimplemented, G03 is not frozen, and no complete HTML-conformance claim is made.

## V1N1 G03 R4A: partial `NOTHING_MODEL` evaluator

R4A implements only the F1-R8-admitted stable Nothing-model subset: `iframe`, `option` when both `label` and `value` are authored, and `colgroup` when `span` is authored. Thirteen text/html void subjects and `template` are vacuous on the accepted DOM observation surface. `selectedcontent` remains explicitly non-rejecting because post-connection selected-option cloning destroys the distinction between authored-empty and authored-content source.

R4A reuses the private R1A source-offset collection mode to suppress element-child diagnostics already owned by R1. Parser-repair/source-only misuse of void subjects remains owned by G03 R5. The production VIEW API remains unchanged at 11 functions, there is no hidden heap allocation, and complete HTML conformance is not claimed.


## V1N1 G03 C0-SR1: neutral single-parser source/repair context

C0-SR1 is private development-time substrate for later source-repair-aware rules. It does not implement G03 R5 and does not change the active top contract from `0.1-VIEW0-V1N1-G03-R4A`. The existing pinned Lexbor parse is observed at two points for each source standard start tag: immediately before the existing downstream tree-builder callback and, when the exact same source token reaches element insertion, immediately before the real insertion call.

The 104-byte record contains stable Arborcore element and insertion-mode IDs, source offsets, open-elements depths, `insertion_seen`, and a neutral foster-parenting bit. Synthetic implied context is represented with the existing `ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE`. Foster parenting and implied-parent insertion are observations, not validity decisions. Raw Lexbor pointers, function pointers, and types never cross the private adapter boundary.

Correlation uses the exact current source-token pointer plus its source name range. Synthetic parser tokens cannot publish source insertion facts. The adapter stores only one parse-local current record and delivers it synchronously to optional consumer-owned observer state; it adds no result array, Arborcore heap allocation, or runtime global registry. Observer failure aborts the observation call before parse counts, document facts, or observation counts are published.

C0-SR1 uses the existing single Lexbor parser pass and static-link wrappers for source provenance and insertion observation. It does not mutate pinned Lexbor source, introduce a second HTML parser, grow the production VIEW API, or claim complete HTML conformance.


## V1N1 G03 R5A: partial `EXPLICIT_HTML_ELEMENT_ALLOWANCE` evaluator

R5A activates only the single source/repair class admitted by the accepted R5-P3 inventory and refined by the accepted F1-R10 support-plan correction: `FOSTER_PARENTED_SOURCE_STANDARD_START_TAG`. It consumes the already-qualified C0-SR1 record from the same pinned Lexbor parse and emits `ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE` only when a source-backed standard HTML start tag is observed at insertion with the foster-parenting flag set **and** the insertion-current standard element is one of `table`, `tbody`, `tfoot`, `thead`, or `tr`. The foster flag alone is parser state and is not proof that adjusted foster insertion actually applied. In particular, an HTML child of an SVG `foreignObject` integration point remains a non-R5 applicability control even when observed while the parser's foster state is enabled. No second parser, reconstructed lexical nesting model, or Lexbor source mutation is introduced.

The diagnostic is anchored to the authored source start-tag name range carried by C0-SR1. Before publication, R5A evaluates the accepted R1A-R4A rules in private source-offset collection mode and suppresses R5A when an earlier R1-R4 authoring `ERROR` already owns the same source start-tag anchor. The prior-owner set is bounded by the existing 4096-diagnostic ceiling and stored as a fixed 4096-entry `uint64_t` array; membership uses a deterministic bounded linear scan rather than adding hash/tree state. The production VIEW API remains 11 functions and no Arborcore heap allocation or mutable runtime registry is added.

The remaining P3 classes are deliberately not guessed into R5A. Non-foster reprocessing, no-insertion repair, other non-foster parser repair, and table-special non-foster repair remain non-rejecting deferrals. Valid implied-parent and stable-context classes are controls, and custom elements remain G13-owned. R5 therefore remains incomplete after R5A; R6-R7 are still unimplemented, G03 is not frozen, and no complete HTML-conformance claim is made.

## V1N1 G03 R6A: `SCALAR_VALUE_TEXT` retained-owner integration

R6A constructs `0x0000000030030006` `ARBOR_VIEW_V1_G03_SCALAR_VALUE_TEXT` without adding a second semantic evaluator or a dedicated R6 diagnostic. Accepted F1-R11 corrects the frozen rule to the complete pinned HTML requirement: text nodes and attribute values consist of scalar values excluding noncharacters and controls other than ASCII whitespace. The imported Infra terms are reproducibly pinned by R6-P1; this is an Arborcore reproducibility pin, not a claim that the pinned HTML revision historically locked that Infra commit.

Ownership remains with already-qualified mechanisms. M1 rejects malformed/non-scalar UTF-8 byte sequences before parsing. The pinned tokenizer/native diagnostic stream owns out-of-range or surrogate numeric references, literal/reference noncharacters, and literal/reference forbidden controls. TAB, LF, FF, CR, and SPACE remain admitted. R6A publishes no `document/scalar-value` diagnostic and no new result flag; its construction consists of contract, verifier, and regression-test retention around those existing owners.

The R6A gate freezes the accepted R5A semantic/runtime bytes and proves the F1-R11 seven-obligation ownership ledger and six-class fixture plan. No `g03_r6a.c` evaluator exists, no C0/C0-SR1 retrofit is introduced, no Lexbor source is changed, and the production VIEW API remains 11 functions. R1A-R5A remain retained at their existing partial boundaries. R6 is complete through retained-owner integration, while R7 remains unimplemented, G03 remains unfrozen, and no complete HTML-conformance claim is made.


## V1N1 G03 R7A: partial `PALPABLE_PHRASING_NONEMPTY` warning evaluator

R7A constructs `0x0000000030030007` `ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY` as an authoring `WARNING` over the accepted F1-R13 planning boundary. The advisory applies only when the authored subject is known to have an active content model that allows flow or phrasing content. The evaluator then inspects the subject's direct DOM children only: non-inter-element-whitespace text satisfies the advisory, as does a known palpable element without `hidden`; descendant-only content does not satisfy it. The warning is anchored at the authored subject source start.

The evaluator reuses the existing C0/C0-SR1 Lexbor observation surface and does not create a second HTML parser or change the production VIEW API. It uses a bounded depth-indexed frame stack and waits until element leave so conditional palpable children can be classified from local parsed-DOM state. `audio` requires `controls`; `input` must not be in the Hidden state; `dl` requires a name-value group; `menu`, `ol`, and `ul` require an `li` child; MathML `math` and SVG `svg` are palpable foreign children. A direct autonomous-custom-element child remains unresolved and blocks a warning while publishing the G13 deferral flag.

Subject applicability is frozen to the accepted 65-standard-element inventory: 62 stable flow/phrasing subjects plus active branches for `div`, `time`, and `option`. A `div` child of `dl` is known not to be an R7 subject; transparent `div` applicability under `option`/`optgroup`/`select` remains G04-owned and non-warning. `time` is an R7 subject only with `datetime`. `option` is an R7 subject only in its no-`label`, non-`datalist` branch. Parser-implied/unanchored subjects are non-warning. Autonomous-custom-element subject applicability remains G13-owned and non-warning.

R7A is therefore intentionally partial while G04 transparent-model resolution and G13 autonomous-custom-element category admission remain deferred. Those unresolved branches are non-warning to avoid false positives. The accepted F1-R13 fixture authority contains 18 isolated fixture classes; R7A also qualifies conditional palpability, deterministic two-pass publication, capacity failure atomicity, UTF-8 precedence, retained lower-rule behavior, bounded stack usage, analyzer results, and ASan/UBSan. G03 is not group-frozen by R7A construction alone, and complete HTML conformance is not claimed.

### R7A SR2: global mechanism-failure atomicity correction

Independent post-construction source review found that the original global coordinator could publish Lexbor/G02 diagnostics before a later fallible G03 pass, violating the private checker contract that caller diagnostics and `result_out` remain unchanged on mechanism failure. SR2 repairs publication order without changing R1-R7 semantics. G03 R1-R5 and R7 collect only compact 8-byte source anchors during the fallible second phase, bounded to 4096 anchors (32768 bytes). After every G03 evaluation and consistency check succeeds, an exact Lexbor pass compares parse counts and C0 document facts against the first measurement before publishing parser diagnostics. That exact parse publication is the last fallible operation. G02/G03 materialization, ordering, line/column assignment, and final `result_out` commit then execute without an error-return path.

SR2 retains the 1 MiB input, 4096-diagnostic, 400000-byte R7 evaluator, and 900000-byte phased compiled-stack bounds; it does not add heap allocation, a second parser, Lexbor source changes, runtime registries, or production VIEW functions. Qualification additionally injects late R7-anchor and final-Lexbor failures and requires byte-identical caller outputs, checks exact direct-diagnostic versus anchor-materialization equivalence for R1-R5/R7, strengthens G04/G13 deferred-mask controls to exact masks, proves true R7/non-R7 coexistence, and asserts the fixed R7 CLI source anchor at byte offset 38, length 1, line 1, column 39. R7 remains partial at the same G04/G13 boundaries; G03 remains unfrozen and complete HTML conformance is still not claimed.

## V1N1 G04 R1A: partial transparent parent-model resolution

G04 R1A begins the transparent-content group without reopening the frozen G02/G03 semantics.
The authoritative rule is `ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL`: the transparent part of an
element's content model inherits the containing part of its parent model, and nested transparent
elements resolve that relationship iteratively. It is therefore incorrect to assign every
transparent element one universal category such as `phrasing` or `flow`.

The evaluator reuses the private C0-SR1 source-repair observation emitted by the same pinned Lexbor
tree builder. This matters for invalid authored markup such as the frozen negative fixture
`<p><a><div>...`: the final DOM is repaired and no longer preserves the authored parent relation,
while the start-tag observation still records the current authored insertion context before repair.
No second parser is introduced.

The admitted R1A boundary resolves standard transparent elements whose containing branch can be
determined from the already frozen element/category and source-context substrate. It supports
iterative nesting, distinguishes flow and phrasing parent parts, handles the transparent tail of the
select-family `div` branch where the branch is source-provable, and leaves `source`/`track` prefix
validity for `video`/`audio` to the existing content-model owner. If a G03 R1-R5 ERROR already owns
the same authored start-tag anchor, G04 suppresses the duplicate rather than producing a cascade.

R1A deliberately remains partial. It publishes explicit no-warning deferrals for scripting-dependent
`noscript`, the `option`/`datalist`-dependent transparent-`div` branch, and autonomous custom
elements owned by G13. Non-whitespace source text in the select-family transparent branch also
remains a static R1A residual because the existing source-repair record is start-tag based and does
not yet bind authored text-token provenance. Parentless transparent fallback is not part of R1A; it
is the separately frozen G04 R2 fragment-mode rule.

The coordinator extends the existing SR2 two-pass mechanism: G04 is measured, then compact source
anchors are collected before the exact Lexbor publication pass. The exact Lexbor publication remains
the last fallible operation; final G02/G03/G04 diagnostic materialization, ordering, line/column
assignment, and result publication remain no-fail. The production VIEW API remains 11 functions,
and this development-time checker state still makes no complete HTML-conformance claim.

Prior-owner suppression is also bounded and stack-phased: G04 reuses 8-byte G03 source anchors in a
32768-byte workspace, and its transparent-resolution mirror uses 24-byte frames. The deepest
conservative compiled-stack composition remains admitted by the existing 900000-byte phased bound;
R1A does not widen the frozen threshold.

## V1N1 G04 R1B: option branch and select-source text closure

R1B extends the reviewed R1A `ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL` evaluator without
starting G04 R2. It binds two R1A residuals to source evidence carried by the same pinned Lexbor
parse. No second parser, Lexbor source modification, production VIEW function, heap registry, or
stack-bound widening is introduced.

For an authored `option` start tag, the private adapter now exposes callback-lifetime borrowed
attribute observations. R1B uses the authored presence of `label` and `value`, together with the
bounded authored open-element mirror for `datalist` ancestry, to select the exact frozen `option`
content-model branch. A no-`label`, non-`datalist` option admits zero or more `div` elements or
phrasing content; other option branches do not admit a `div` and remain owned by the existing G03
parent/content-model diagnostics rather than producing a duplicate G04 error. The R1A option
runtime deferral flag therefore remains numerically reserved but is no longer published on
successful R1B paths.

For transparent `div` elements in the `select`/`optgroup` tails, the adapter also exposes the
pinned tokenizer's decoded text token plus the pre-repair current-element/source context. R1B
admits inter-element whitespace and rejects non-whitespace authored text, including decoded
character references such as `&amp;`, while admitting a decoded whitespace reference such as
`&#32;`. The error remains anchored to the owning transparent element start tag, so source repair
does not change diagnostic ownership.

Both new private observer channels are failure-atomic: a callback mechanism failure aborts the
single Lexbor observation pass without publishing parse counts, document facts, observation
counts, diagnostics, or result metadata. Borrowed Lexbor attribute/text spans are used only during
the callback and are never retained.

Two R1 dependencies remain intentionally unresolved. `noscript` transparency genuinely depends on
the parser's scripting mode, so the checker must gain an explicit reviewed scripting-mode input
before that branch can be evaluated. Autonomous custom elements have a transparent frozen element
definition, but source hyphen syntax alone does not prove that an authored element is an autonomous
custom element; that identity remains with the G13 custom-element owner. R1B therefore remains a
partial R1 construction, G04 R2 is still untouched, G04 is not group-frozen, and complete HTML
conformance is not claimed.

R1B qualification records compiler path/version/target and compile flags alongside stack-usage
observations. Source path/manifest/tree identities remain deterministic; raw compiler-generated
`-fstack-usage` values are treated as environment evidence and are qualified against the unchanged
900000-byte phased bound rather than being used as a cross-toolchain archive identity.

## V1N1 G04 R1C: explicit scripting-disabled `noscript` closure

R1C closes the final standard-element residual of `ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL`
without starting G04 R2. The pinned `noscript` definition makes its content model transparent outside
`head` only when scripting is disabled. The existing VIEW0 development checker had already been
parsing with Lexbor's disabled-scripting default, but R1A/R1B deliberately refused to treat an
implicit library default as normative checker authority. R1C makes that mode explicit in the single
pinned Lexbor parser and verifies the readback before parsing.

The current V1N1 checker mode is therefore frozen to **scripting disabled**. R1C does not expose an
enabled-scripting switch: changing parser mode would alter HTML parsing as well as several G03
branches, so such an expansion requires a separate cross-evaluator review rather than a local G04
flag. This is an explicit checker admission policy, not a claim that the HTML Standard has only one
scripting state.

Under the frozen mode, an authored `noscript` outside `head` is transparent. In ordinary flow and
phrasing contexts it inherits the containing parent part exactly like the already-qualified
transparent elements. In `select` and `optgroup` it inherits the respective special tail model. In a
no-`label`, non-`datalist` `option`, `noscript` enters through the phrasing-content alternative and
therefore inherits the phrasing part rather than the `div` alternative. A `noscript` in `head` is not
a G04 R1 transparent subject; its separate link/style/meta model remains with the existing content-
model owner. Prior G03 ERROR ownership at the same authored child anchor continues to suppress a
duplicate G04 diagnostic.

The historical G04 `noscript` deferred-result bit remains numerically reserved for evidence
compatibility but is no longer published by successful R1C paths. The sole remaining R1 dependency
is autonomous-custom-element identity. The pinned custom-element source establishes that an
autonomous custom element is transparent, but it also establishes that autonomous identity depends
on a custom element definition; a hyphenated source name alone is not sufficient. That dependency
therefore remains explicitly G13-owned. R1C completes standard-element G04 R1 semantics for the
frozen scripting-disabled checker mode, but it does not claim full R1 closure across G13, does not
start R2, and does not claim complete HTML conformance.

### V1N1 G04 R2 — explicit fragment-model parentless fallback

The second frozen G04 rule is evaluated only when fragment-model checking is explicitly requested.
The development CLI exposes this as `--fragment-model`; normal document checking remains unchanged.
The same pinned Lexbor parser is used in HTML `body` fragment context with scripting disabled. The
synthetic fragment wrapper is parser machinery and is **not** treated as an authored parent.

For a top-level authored standard element whose applicable content model is transparent, the
transparent part accepts **flow content**, exactly as required by the frozen WHATWG source. Nested
transparent elements inherit that flow model iteratively. Source-repair provenance is retained, so
an invalid child can be diagnosed even when the fragment tree builder refuses to insert it.

The fragment-model checker is intentionally not a second whole-document conformance path: G02/G03
whole-document rules are not run in this mode. Autonomous custom-element identity remains owned by
G13; hyphenated unknown elements publish the G13 deferral without an R2 authoring warning. This
keeps G04's standard-element surface complete while preserving the later custom-element owner and
continues to make no complete-HTML-conformance claim.


### V1N1 G05 C0 applicability foundation

G05 C0 is a private development-checker foundation only. It implements **zero G05 rule IDs**. It freezes authority-generated catalogs for the four G05 rules (106 global applicability forms, 261 element-specific references, 43 conditional predicates, and 18 additional `body` Window-event names) and extends the existing single-Lexbor authored-attribute observer with exact source offsets/lengths for attribute names. Attribute value semantics remain with G06/later owners as frozen; ARIA family name/value conformance remains a later referenced-ARIA owner.


### V1N1 G05 R1A — global attribute applicability

G05 R1A admits the frozen global-attribute catalog on standard HTML elements. Exact global names, generic event-handler content attributes, nonempty `data-` and `aria-` placement families, and the HTML `xmlns` allowance are admitted for placement only; value/name semantics owned by later groups remain outside R1. Names known to the frozen element-specific or body Window-event catalogs are handed to G05 R2/R3/R4 rather than rejected by R1. An otherwise unknown attribute name on a frozen standard HTML element produces `ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY` at the authored attribute-name source range. Nonstandard/custom-element ownership remains outside this standard-element R1 boundary.


### V1N1 G05 R2A — element-specific attribute applicability

G05 R2A implements the frozen static element/attribute pair rule from the 261-record C0 catalog. An authored attribute whose name is present in the frozen element-specific catalog is admitted by R2 only when that exact standard-element-id/name pair exists. The rule is independent of parser acceptance and anchors any error at the authored attribute-name source range.

R2 deliberately preserves later ownership. Global names are already admitted by R1. Unknown names remain R1-owned and do not receive a duplicate R2 diagnostic. The 18 additional `body` Window-event names remain R4-owned. A pair that is statically listed is admitted by R2 even when one of the 43 frozen conditional clauses later restricts that attribute; those state, cross-attribute and presence predicates remain exclusively R3-owned. The five custom-element references excluded from the standard 261-record catalog remain G13-owned.

R2 uses the same single pinned Lexbor authored-attribute observer introduced by C0, performs no raw-HTML rescan, adds no production VIEW function, and retains the existing two-pass failure-atomic publication and 900000-byte phased stack bound. R3/R4 remain unimplemented and complete HTML conformance is not claimed.

### V1N1 G05 C0-SR1 input-state authority correction

Before G05 R3 construction, independent review of the frozen 43-clause conditional table found four derived input-state ownership errors in the C0 nearest-heading map. C0-SR1 preserves all 43 normative clause identities and all R1/R2 semantics, but replaces the fragile single-state string with an exact private input-state bitmask. The shared Text/Search clause applies to both states; the Date, Local Date and Time (`datetime-local`), and Range clauses are bound to their exact pinned WHATWG state headings. The corrected surface contains 24 input-related clauses, 25 state-to-clause applicability rows, and all 22 input type states. R3 remains unimplemented at this boundary.

### V1N1 G05 R3A — conditional attribute applicability

G05 R3A implements exactly the 43 frozen conditional-applicability clauses over the corrected C0-SR1 authority. The rule evaluates applicability predicates only: same-element attribute presence/absence, ASCII-case-insensitive `rel` token predicates, the 22-state `input` type model (with missing/invalid `type` defaulting to Text), the `img[ismap]` ancestor-`a[href]` requirement, the `optgroup` direct-child `legend` alternative, and the remaining required/forbidden-attribute relationships from the accepted clause table. No additional conditional prose outside those 43 frozen rows is silently added.

R3 reuses the existing single pinned Lexbor observation pass. Authored attribute-name anchors come from the C0 source-attribute channel; ancestor/child predicates use the DOM traversal observations only where the frozen clause requires them. Present offending attributes are anchored at their authored attribute-name range. A missing required attribute is anchored at the owning authored start tag because no attribute source span exists. R1-owned unknown names and R2-owned wrong-element pairs receive no R3 duplicate; the 18 additional `body` Window-event names remain R4-owned, and nonstandard/custom-element identity remains later-owned.

Value-language conformance is deliberately not widened into R3. URL, CSS, JavaScript, media-query, numeric, preload-destination, and other value grammars remain with their frozen later owners; R3 examines a value only when that value is itself part of the accepted applicability predicate (for example `rel` keywords, `as=image`, `wrap=hard`, or input state selection). The production VIEW API remains 11 functions, no second HTML parser or direct Arborcore heap allocation is introduced, and the existing two-pass failure-atomic publication and 900000-byte phased stack bound remain in force. R4 is not implemented and complete HTML conformance is not claimed.

### V1N1 G05 R4A — body Window-event attribute applicability

G05 R4A implements the frozen 18-name `body` Window-reflecting event-handler applicability rule. The 18 names are admitted for placement on the authored `body` element only. An occurrence on any other authored owner is an R4 ERROR anchored at the authored attribute-name source range. R1/R2/R3 deliberately hand these names forward so R4 is the sole G05 placement owner.

R4 validates placement only. Event-handler JavaScript FunctionBody/value semantics remain explicitly deferred to G16. The evaluator reuses the existing single pinned Lexbor authored-attribute observation pass, adds no production VIEW API, performs no direct Arborcore heap allocation, and retains the existing two-pass failure-atomic publication and 900000-byte phased stack admission. G05 group freeze remains pending independent group review.

### V1N1 G06 C0 bounded microsyntax foundation

G06 C0 is a private development-checker foundation and publishes **zero G06
diagnostics**. It binds all 17 frozen G06 identities to bounded, locale-independent
native validators and freezes the accepted WA0-R1 author-facing consumer surface as
79 unique policies, including the two source-proven `time` supplements for yearless
dates and time-zone offsets. R15 remains implemented as a pure validator with zero
author-facing consumers; no diagnostic is invented for it.

The `link[as]` policy is copied from the independently accepted G06-A2 cross-standard
closure. G05 R3 retains presence and placement ownership. G06 R2 will validate only
present applicable values: the six preload destinations, nine modulepreload
destinations, the `style`/`script` intersection when both relationships occur, and
no R2 diagnostic when neither relationship applies.

All scanners operate directly on bounded spans without locale state, mutable global
registries, a second parser, or Arborcore heap allocation. Integer syntax is checked
before bounded conversion so syntax failure has stable precedence over overflow.
Calendar validation retains the exact proleptic-Gregorian rules even for extended
years by using the 400-year cycle. Unique token sets use a fixed 4096-entry local
hash workspace and return capacity separately from an authored duplicate, avoiding
quadratic scans and false diagnostics. This foundation does not expand the production
VIEW API, does not yet connect consumers to authoring diagnostics, and makes no
complete-HTML-conformance claim.

### V1N1 G06 R1-R17 author-facing microsyntax consumers

The G06 R17A wave connects the 17 frozen C0 validators to exactly the 79 accepted
author-facing consumer policies derived from A0, A1, WA0-R1, F1-R0, and the
accepted A2 cross-standard closure. It does not promote general microsyntax
definitions, parsing-algorithm references, user-agent behavior, examples, or
later-group semantics into diagnostics. R1-R14 and R16-R17 publish deterministic
ERROR diagnostics at authored attribute-name ranges (or at the authored `time`
element when its child text is the consumer). R15 retains its bounded validator
but has zero accepted author-facing consumers and therefore publishes no R15
diagnostic.

G05 remains the sole owner of attribute presence and placement. A G06 validator
runs only after its accepted element/state/relationship predicate is satisfied;
otherwise the occurrence is handed back to the earlier applicability owner and
no value-language duplicate is produced. The accepted A2 `link[as]` destination
sets remain exact for `preload`, `modulepreload`, and their `style`/`script`
intersection. Consumer-specific restrictions are not generalized: for example,
file `accept` tokens reject ASCII-case-insensitive duplicates, while email and
metadata comma-list consumers retain their own restrictions.

The signed-integer authoring rule is lexical and unbounded, as required by the
pinned definition. C0 continues to report conversion range separately for callers
that need a machine integer, but R3 does not misclassify that conversion limit as
an authoring-syntax error. Consumer-specific numerical bounds, such as `col@span`
being at most 1000, remain explicit consuming restrictions.

The `time` element consumes either its `datetime` value or, when that attribute is
absent, its concatenated direct text. The pinned definition also admits a valid
year-only string, which has no V1N1 G06 rule identity; R17A therefore admits that
branch without inventing a diagnostic. An invalid union publishes exactly one
deterministic diagnostic selected by lexical shape, with the first normative union
branch (R6 month) as the ambiguous fallback. A `time` element with element
descendants remains G03-owned and receives no G06 union diagnostic.

The wave retains the single pinned Lexbor parser and the existing measure/collect/
materialize publication transaction. Source attributes, unique token sets, and
`time` text use fixed workspaces. Workspace exhaustion is a mechanism failure and
leaves caller diagnostics and result metadata unchanged; it is never recast as an
authoring violation. No production VIEW function, direct Arborcore heap allocation,
mutable runtime registry, locale dependency, second parser, or stack-threshold
widening is introduced. G06 group freeze remains pending independent review, and
complete HTML conformance is not claimed.

### V1N1 G06 R17A-SR1 verifier reproducibility correction

Independent review of the live Arch R17A qualification found two GNU `grep`
warnings caused by unnecessary backslashes before literal double quotes in the
diagnostic-message-table regular expression. R17A-SR1 removes only those
nonportable escapes. The clean-room review also proved that the native verifier
must reset the shared `native.o` changed by R17A, in addition to the G06-specific
objects and test programs, so ignored build timestamps can never select an older
coordinator object. SR1 binds both corrections to the accepted R17A evidence
archive. No G06 rule identity, consumer policy, diagnostic behavior, ownership
boundary, production API, or HTML-conformance claim changes. G06 group freeze
remains pending independent review of the corrected candidate.

### V1N1 RC1 dependency reconciliation

RC1 applies the independently frozen RC0 21-row disposition matrix without
widening the V1N1 rule authority: seven dependencies are resolved using mechanisms
already accepted in G04-G06, thirteen later/external boundaries remain explicit,
and the `time` lexical branch remains already owned by G06. The shared G05 C0-SR1
input-state classifier now supplies G03 interactive and canvas decisions; the G06
R4 nonnegative-integer parser supplies `select` display-size derivation while G06
retains sole ownership of invalid size-value diagnostics. The checker remains in
its frozen scripting-disabled mode for `noscript`.

The generic G03 `noscript`, input-state, canvas-state, select-size, and transparent
G04 deferral publications are retired. The narrower rendering-dependent
`select[multiple]` display-size-one boundary remains, as do style, script,
accessibility, parser-repair, G13 custom-element, and G16 event-handler-value
boundaries. G03 R7 asks the G04 containing-model mechanism whether a transparent
`div` actually derives a flow-or-phrasing subject; tag proximity is not used.

RC1 adds no production VIEW function, parser, heap allocation, locale dependency,
or mutable runtime registry. Diagnostic ordering, UTF-8 precedence, prior-owner
suppression, failure atomicity, and the 900000-byte phased stack admission remain
required. RC1 is a V1N1 integration reconciliation, not a complete HTML-conformance
claim.

### V1N2 C0 authority and resource foundation

V1N2 C0 binds the independently frozen G07-G11 matrix and its cross-standard
authority closure into a zero-diagnostic implementation foundation. It records
all 38 rule identities at the exact group boundary (G07 5, G08 12, G09 6,
G10 13, G11 2), the 36/1/1 static-document, HTML-integration, and deterministic-
subset admission partition, and all 12 external-authority dispositions. Eight
authority rows carry exact selected commit and source-byte identities; deferred
WebVTT, SVG/MathML language, ECMAScript RegExp, and accessibility semantics remain
explicit and non-rejecting.

The foundation defines one caller-bounded 4096-entry anchor arena shared by the
later G07-G11 evaluators. C0 validates only immutable metadata and resource
contracts. It adds no G07-G11 diagnostic rule to the public checker, allocates no
heap storage, introduces no locale dependency or mutable runtime registry, and
uses the existing single pinned Lexbor parser. The production VIEW API remains at
11 functions and the retained phased stack admission remains 900000 bytes.

Group evaluators are constructed in later checkpoints in the frozen order G07,
G08, G09, G10, and G11. Consequently C0 makes no complete HTML-conformance claim.
The C0 gate resets its metadata object, the shared native coordinator object, and
their dependent C0/RC1 test programs before qualification, so ignored build
timestamps cannot select a pre-C0 executable.

### V1N2 G07 static link semantics

G07 implements the five frozen link-rule identities over the C0 authority and
resource foundation. The admitted surface is static-document authoring only:
hyperlink `href` and `target` relationships; the relationship between `download`
and a hyperlink-bearing `href`; nonempty HTTP(S) URL tokens in `ping`; the pinned
HTML relation-to-element applicability matrix for `link`, `a`, `area`, and `form`;
and the frozen per-relation companion, exclusion, and legacy-synonym constraints.

G03 remains the content-model owner, G05 remains the attribute-placement owner,
and G06 remains the token/value-microsyntax owner. G07 therefore does not repeat
duplicate-token, wrong-element-attribute, or generic URL-list mechanics already
owned by those groups. Relation names outside the pinned HTML table remain
non-rejecting because the runtime extension registry was not frozen as V1N2
authority. Likewise navigation, origin checks, fetching, ping transmission,
download filename selection, CSP enforcement, and other user-agent algorithms are
outside this static checker checkpoint.

The evaluator uses the C0 shared anchor representation and the existing single
pinned Lexbor observation path. Measurement precedes exact anchor collection; the
last parser-dependent materialization occurs before any caller publication. A
capacity or parser failure leaves result and diagnostic buffers unchanged. No
production VIEW function, direct Arborcore heap allocation, mutable runtime
registry, locale dependency, second parser, or stack-threshold widening is added.
G08-G11 remain unimplemented, and complete HTML conformance is not claimed.

### V1N2 G08 static embedded-content semantics

G08 implements the twelve frozen embedded-content identities over the accepted
G07 group boundary. Eleven identities publish bounded static-document diagnostics
covering picture/source ordering, image resource and alternative-text conditions,
iframe/embed/object declarations, media and track declarations, media-source
cross-resource conditions, image-map references, and paired image dimensions.
R11 records HTML foreign-content integration and keeps full SVG and MathML
language conformance outside this checker boundary.

G03 retains content-model ownership, G05 retains attribute applicability, and G06
retains shared value-microsyntax ownership. WebVTT resource-body validation,
runtime fetching and media algorithms, intrinsic-layout calculations, and human-
language adequacy judgments remain explicit non-rejecting exclusions. The static
map, track, figure, picture, and media relationships are evaluated only from the
pinned HTML observation surface.

G08 reuses the C0 bounded anchor representation and the existing Lexbor parse.
Measurement precedes collection and exact materialization; any anchor, workspace,
or final parser failure preserves caller output atomically. No production VIEW
function, heap allocation, mutable global registry, locale dependency, second
parser, or stack-bound widening is introduced. G09-G11 remain unimplemented and
complete HTML conformance is not claimed.

### V1N2 G09 static table semantics

G09 implements the six frozen static-document table identities over the accepted
G08 group boundary. The evaluator covers the bounded table slot model, caption
cardinality, column and column-group spans, row-group intervals, applicable
header scope, and explicit or implicit cell/header association. G03 and G05
retain content-model and attribute-applicability ownership, while G06 retains
integer-microsyntax diagnostics; G09 suppresses duplicates at those boundaries.

The table model is a sparse rectangle/interval representation whose storage is
linear in parsed table nodes and header tokens. It never allocates a width by
height product grid. One invocation-local Lexbor mraw arena owns transient model
records, checked arithmetic protects all coordinates, and arena or collection
failure leaves caller diagnostics and results unchanged. No direct Arborcore
heap call, build-surface change, public VIEW function, locale dependency, mutable
runtime registry, second parser, or stack-threshold widening is introduced.

ARIA and HTML-AAM accessibility naming, CSS table layout, and runtime table
presentation remain explicit non-rejecting exclusions. G10-G11 remain
unimplemented and complete HTML conformance is not claimed.

### V1N2 G10 static form semantics

G10 implements the thirteen frozen form identities over the accepted G09 group
freeze: twelve static-document rules plus the deterministic constraint-validation
subset. The checker binds form owners and ID references, the complete 22-state
input partition, control-family author relationships, static validation predicates,
and submission declarations without executing submission or runtime UI algorithms.

The implementation retains G03/G04 content and placement, G05 applicability, and
G06 microsyntax as prior owners. URL, Fetch, MIME, and Encoding are consumed only at
their frozen static byte boundaries. ECMAScript RegExp semantics remain explicitly
deferred to G16; ARIA/HTML-AAM, runtime form mutability and UI, and form-submission
execution remain excluded. One invocation-local Lexbor mraw arena stores relations
in O(forms + controls + labels + options + tokens); a 22-state product table and
direct Arborcore heap allocation are forbidden. The 4096-diagnostic cap, 900000-byte
phased stack bound, 11-function production View API, and global failure atomicity are
retained. G11 remains unimplemented, and complete HTML conformance is not claimed.

### V1N2 G11 static interactive-element semantics

G11 closes the two frozen interactive-element identities over the independently
frozen G10 boundary. R1 enforces the author-visible `details` name-group rules:
nonempty group names, at most one authored `open` member per exact case-sensitive
document group, and no same-group `details` descendant. The evaluator preserves
source anchors for the responsible `name` or `open` attribute.

R2 completes the static `dialog` census without inventing runtime state. G05 remains
the sole owner of the `dialog[tabindex]` prohibition and G06 remains the sole owner
of `open` and `closedby` microsyntax. The global `popover` attribute is allowed on
`dialog`; authored `open` plus `popover` does not prove simultaneous runtime showing
states and therefore is non-rejecting. Focus selection, toggle events, close watchers,
inertness, activation, top-layer behavior, and light-dismiss execution remain explicit
runtime exclusions.

G11 reuses the single C0 4096-entry staging arena shared cumulatively by G07-G11 and
uses one invocation-local Lexbor mraw arena for its relations and source anchors.
Measurement precedes collection and materialization, and injected arena or collection
failure preserves caller output. No production VIEW function, direct Arborcore heap
allocation, locale dependency, mutable runtime registry, second parser, or stack-bound
widening is introduced. This freezes V1N2 G07-G11; it is not a complete HTML-
conformance claim.

### V1N3 C1 static scripting, custom-element, microdata, interaction, and handler semantics

V1N3 C1 adds the thirty frozen G12-G16 identities over the published V1N2
boundary: eight scripting-element rules, six custom-element rules, six microdata
rules, eight user-interaction rules, and two static web-application integration
rules. The implementation is tooling-private; the eleven production View API
functions remain unchanged. Callers that need V1N3 configuration use the native-C
configured-checker ABI with an explicit scripting mode and an immutable bounded
custom-element definition table.

The three admitted ECMAScript consumers use an Arborcore-owned, purpose-built,
parse-only native-C frontend. It recognizes the frozen constructor subset,
FunctionBody early-error boundary, and Pattern UnicodeSets-mode boundary. It does
not execute script, generate bytecode, instantiate modules, call host bindings, or
perform regular-expression matching. Unicode identifier and property handling is
pinned to Unicode 17.0.0. The frozen Test262 selection is qualification authority
for these parse-only operations; it is not a claim of complete ECMAScript support.

G12-G16 share one invocation-local Lexbor mraw arena and bounded source anchors.
Each group is measured and then collected, and the two evaluations must match
before materialization. The 4096-diagnostic cap, 1048576-byte HTML and constructor
source bounds, deterministic ordering, locale independence, and caller-output
failure atomicity are retained. Direct Arborcore heap allocation and mutable
runtime registries remain absent. User-agent execution, fetching, navigation,
custom-element reactions, event dispatch, accessibility-tree computation, and
complete HTML conformance remain explicitly outside this checkpoint.

### V1N4 full-matrix and NC1 review completion

V1N4 closes review of the frozen V1 authoring matrix without adding new production
VIEW functions or new runtime request-path behavior. The admitted matrix contains
104 identities: 36 from V1N1, 38 from V1N2, and 30 from V1N3. One hundred identities
publish ordinary rule diagnostics. Four retain specialized non-generic ownership:
G03 R6 stays owned by M1/tokenizer evidence, G06 R15 remains a direct bounded
date-or-global-date-time validator with no accepted author-facing consumer, G08 R11
is HTML foreign-content integration only, and G11 R2 suppresses prior-owner/runtime
dialog duplicates.

NC1 preserves eight obligation case IDs for every frozen identity, for 832 cases in
total. All 832 case/source bindings are present with zero gaps. The native corpus
contains 1,759 result records per pass and was replayed twice against the exact
historical V1N1, V1N2, and V1N3 implementation boundaries. All 3,518 comparisons
matched the expected bytes and the two passes were byte-deterministic. Diagnostic
ownership is therefore qualified for the bounded 100 generic plus four specialized
matrix, and the NC1 native corpus is frozen.

V1 conformance remains development/build tooling. There is no tracked VIEW0 V1
browser or WASM conformance target, so native/WASM equivalence is not applicable to
this tooling surface; the global native/WASM equivalence requirement remains in force
for browser surfaces where such a boundary exists. The frozen browser authority is
not reopened, and historical Nu/v.Nu evidence remains nonauthoritative.

V1N4 deliberately does not promote this bounded matrix into a complete standards
claim. Complete ECMAScript conformance remains unclaimed because the V1N3 frontend
is parse-only and limited to its frozen constructor, FunctionBody, and Pattern
UnicodeSets-mode operations. Complete HTML conformance also remains unclaimed:
retained deferred/excluded boundaries include full SVG/MathML language conformance,
WebVTT resource bodies, fetching/navigation/media execution, custom-element
reactions, event dispatch, accessibility computation, runtime form/dialog behavior,
and other explicitly recorded residuals.

With those limits preserved, the V1N4 review-completion decision is **yes** for the
bounded 104-rule native qualification. D1 is not admitted by this publication.
The next required gate is the already-frozen post-V1N4 manuals, runnable-examples,
and documentation-consistency review, including the carried content-type singleton
and C4 Assembly-consumer stability obligations.
