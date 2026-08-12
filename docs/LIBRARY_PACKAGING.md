# Arborcore Assembly Library Packaging v1.0.0

This phase turns the already-frozen Arborcore Assembly ABI v1.0 into formal,
reproducible library products. It does not change production Assembly code or
widen the ABI surface.

## Products

The canonical static product is:

- `libarborcore.a`

The canonical shared product is:

- real file: `libarborcore.so.1.0.0`
- SONAME: `libarborcore.so.1`
- ABI version node: `ARBORCORE_1.0`
- runtime symlink: `libarborcore.so.1 -> libarborcore.so.1.0.0`
- development/link symlink: `libarborcore.so -> libarborcore.so.1`

`start.o` is process-entry code and is never a library member.

## Default installation layout

With the default `PREFIX=/usr/local`:

- libraries install under `/usr/local/lib`
- frozen ABI metadata installs under `/usr/local/share/arborcore/abi`

`DESTDIR`, `PREFIX`, `LIBDIR`, and `DATADIR` are supported so packaging and
staged-install systems do not need privileged writes.

The default installed relative file set is recorded in
`packaging/arborcore-library-files.list`.

## Reproducibility

`libarborcore.a` uses deterministic GNU `ar` mode (`rcsD`). The release gate
builds the library products independently in two clean source snapshots and
requires byte-for-byte identical SHA-256 identities. It also requires the
resulting hashes to match the artifact identities qualified at the Assembly
ABI v1 freeze.

This is a deterministic-build contract for the frozen source and qualified
build toolchain. A future toolchain that intentionally changes bytes must be
reviewed rather than silently redefining the frozen artifact identity.

## Installation ownership

`make install-libraries` installs only the files listed by the library
packaging policy. `make uninstall-libraries` removes only those Arborcore-owned
files and symlinks; it does not recursively remove shared system directories.
The install qualification is performed under an isolated `DESTDIR` and proves
both static and shared consumers before uninstalling the staged files.

## ABI relationship

The tracked `abi/arborcore-1.freeze` record identifies the actual Git freeze
commit and the frozen source/ABI/library hashes. Library packaging must match
that record exactly. Changing one of the 94 public symbols, a frozen layout,
calling convention, ownership/lifetime contract, or error contract is not a
packaging change and requires an explicit ABI compatibility decision.
