# Arborcore CONFIG0-R0

CONFIG0 is Arborcore's deterministic, typed configuration boundary. It accepts
only caller-supplied schemas and byte spans, resolves four explicit source
classes, and publishes an immutable result into caller-owned storage. The core
does not read files, inspect the environment or arguments, allocate memory,
log, use locale state, or consult clocks or the network.

## Public API

Include `<arborcore/config.h>` and link `build/libarborcore_config0.a`.

- `arbor_config_measure()` validates a schema and sources and reports exact
  storage requirements without changing its output on failure.
- `arbor_config_prepare()` validates again and publishes values, provenance,
  copied UTF-8 data, and a result only after every check succeeds.
- `arbor_config_validate()` checks an already prepared result against a schema.

The public model has five kinds: Bool, U64, I64, UTF-8, and closed Enum.
Descriptors give four independent exact names for the semantic, file,
environment, and command-line namespaces. Matching is byte-exact and
case-sensitive.

All output is caller-owned. Writable result, value, provenance, scratch,
persistent, requirements, and diagnostic ranges must be disjoint from one
another and from all nonempty inputs. Read-only inputs may alias each other.
CONFIG0 performs no heap allocation or ownership transfer.

## Sources and precedence

The fixed precedence is:

```text
default < file < environment < command line
```

Each semantic key may occur once in each class. A duplicate in one class is an
error. Every lower source is still validated; a malformed lower assignment is
not hidden by a valid higher override, and an invalid higher override never
falls back.

CONFIG0 receives an optional file document span, an explicit vector of
environment-entry spans, and an explicit vector of command-line-entry spans.
Applications own file reading, environment selection, and argument handling.

## Exact grammar

File records use LF or CRLF and may omit the terminal newline. Only a
zero-length record is blank. Only a hash in column zero begins a comment.
Assignments split at the first equals byte; later equals bytes belong to the
value. No byte is trimmed.

Environment entries are `NAME=VALUE`. Command-line entries are
`--NAME=VALUE`. NUL, embedded line endings, missing delimiters, empty names,
unknown names, and duplicates fail. Quotes, escapes, continuation,
interpolation, includes, sections, arrays, nesting, heredocs, and shell syntax
have no special meaning.

Bool accepts only `true` or `false`. U64 is ASCII decimal. I64 is ASCII decimal
with an optional minus. Leading zeroes and minus zero are accepted; plus signs,
whitespace, suffixes, other bases, partial conversion, and overflow fail.
UTF-8 must be shortest-form scalar text without NUL and is never normalized.
Enum tokens match one exact declared choice.

## Diagnostics

Failures can publish a structured `arbor_config_diagnostic` containing only a
numeric code, source, record, byte offset, and key index. It never contains or
prints raw configuration bytes. A successful call leaves the diagnostic
destination untouched.

File provenance uses the physical zero-based record number and an absolute
document offset. Environment and command-line provenance use the vector index
and entry-relative offset. Default provenance uses the descriptor index and
offset zero.

## HELLO0 and ECHO0

Both examples use this schema:

| Key | Environment | Command line | Kind | Rule |
| --- | --- | --- | --- | --- |
| `template` | `ARBORCORE_TEMPLATE` | `--template=` | UTF-8 | required, 1..4095 bytes |
| `bind_ipv4` | `ARBORCORE_BIND_IPV4` | `--bind-ipv4=` | UTF-8 | default `127.0.0.1`, max 15 |
| `port` | `ARBORCORE_PORT` | `--port=` | U64 | required, 0..65535 |
| `backlog` | `ARBORCORE_BACKLOG` | `--backlog=` | I64 | default 16, 1..INT_MAX |
| `event_wait_ms` | `ARBORCORE_EVENT_WAIT_MS` | `--event-wait-ms=` | I64 | default 250, 0..INT_MAX |
| `drain_timeout_ms` | `ARBORCORE_DRAIN_TIMEOUT_MS` | `--drain-timeout-ms=` | U64 | default 2000 |

The historical form remains valid and ignores CONFIG0 environment aliases:

```sh
./build/hello0/hello0 examples/hello0/page.html 8080
./build/echo0/echo0 examples/echo0/page.html 8080
```

Named command-line mode:

```sh
./build/hello0/hello0 \
  --template=examples/hello0/page.html --port=8080
```

Explicit file mode:

```text
template=examples/echo0/page.html
bind_ipv4=127.0.0.1
port=8080
backlog=16
event_wait_ms=250
drain_timeout_ms=2000
```

```sh
./build/echo0/echo0 --config=server.conf
```

Zero or one `--config=PATH` is accepted. Bare positionals cannot be mixed with
named mode. Configuration and numeric IPv4 validation finish before template
acquisition, signal installation, HOST1 preparation, listener creation, or
epoll creation.

## Evidence

Run focused CONFIG0 evidence:

```sh
make config0-focused-gate
```

Run CONFIG0 plus the frozen HOST1/LIFE0/HELLO0/ECHO0 dependency gates:

```sh
make config0-gate
```

The suite includes strict compiler and analyzer checks, native and sanitizer
tests, a 200-case fixed-seed differential resolver, application live modes and
signals, deterministic archive reproduction, and a threshold-free resolution
diagnostic.
