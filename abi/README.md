# Arborcore Assembly ABI v1

This directory defines the stable Assembly capability surface frozen after the
A–E runtime retrofit series.

- `arborcore-1.symbols` — stable externally supported symbols.
- `arborcore-1.internal-symbols` — implementation symbols intentionally not
  exported by the shared-object ABI. Static archives may still contain some
  of these ELF globals because cross-object resolution requires them; they are
  not compatibility promises.
- `arborcore-1.map` — ELF symbol-version/visibility policy for
  `libarborcore.so.1` readiness builds.
- `arborcore-1.layout` — frozen sizes, offsets and numeric state values that
  callers may need to construct/pass ABI objects.

ABI v1 target is Linux x86-64 using the System V AMD64 calling convention.
Functions preserve the standard callee-saved registers and return with DF
clear. Status-returning functions use the established negative-Linux-errno
convention documented by their modules.

Adding a new symbol in a later compatible ABI revision is allowed. Removing a
v1 symbol, changing its calling convention, changing a frozen layout/offset,
or changing documented ownership/lifetime/error semantics requires an explicit
ABI version decision.
