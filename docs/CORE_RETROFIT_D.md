# Assembly Core Retrofit D — HTTP, Target, Routing and Response

Retrofit D is constructed as one recoverable fast-forward package with six internal gates.

## D0 — qualification methodology

Historical absolute performance may decide a candidate only when the unchanged D reference commit can still reproduce the accepted historical environment. If that reference fails the historical envelope, the session is reported as `ENVIRONMENT_INADMISSIBLE` and qualification switches to balanced paired reference-versus-candidate rounds. Historical profiles remain read-only.

D reference commit:

```text
24e014b791b46c4b7c8dffd2dee14dcb8eb4354a
```

## D1 — request-target hardening

`request_target_split` validates the complete raw target before publishing path/query spans. The first `?` is the separator; later `?` bytes are query data. Raw `#` and non-visible ASCII are rejected in either component.

## D2 — routing contracts

Parameterized route output spans are borrowed aliases. Parameter names alias immutable catalog/pattern storage; values alias the active request-target storage. Parameter records are valid only for a successful match. Catalog order is authoritative and defines exact-versus-parameter precedence and duplicate first-match behavior.

## D3 — response aliasing

Response serialization preserves the entry-time body byte sequence even when the body aliases output storage. If metadata would overwrite the source before body publication, the body is staged with overlap-safe movement into its final destination before metadata is written. Expected failure paths publish no backing-storage changes.

## D4 — prepared exact-route index experiment

A non-production open-addressed prepared index is qualified against `router_find_exact`. It preserves insertion/first-valid duplicate order and is benchmarked for first/last/miss lookups. The production ordered linear router remains authoritative unless a separate source-level promotion is justified by the experiment evidence.

## D5 — Core Retrofit D Gate

The final gate requires:

- warning-clean full reconstruction and `make check`;
- D1/D2/D3 property/contract tests;
- D4 equivalence and performance experiment;
- ABI and dependency closure;
- GNU-stack checks;
- production code-size evidence;
- historical strict performance when the reference environment is admissible;
- paired reference qualification otherwise.

Scatter/gather output, immediate socket writes, EPOLLOUT policy and server work budgets remain frozen for Retrofit E.
