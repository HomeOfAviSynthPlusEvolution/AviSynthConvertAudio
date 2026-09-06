# Vendored Google Highway

## Upstream

- Source: https://github.com/google/highway
- Release: 1.4.0, published 2026-04-23
- Commit: `2607d3b5b0113992fe84d3848859eae13b3b52c1`
- License: Apache-2.0 or BSD-3-Clause. The complete dual-license text is in
  `LICENSE`.

## Retained files

This is a source-only subset used by AviSynth+'s internal SIMD support:

- Highway core headers and runtime sources under `hwy/`;
- all architecture operation headers under `hwy/ops/`, including x86, Arm,
  RISC-V, LoongArch, PowerPC, WebAssembly, scalar, and emulation backends;
- `CMakeLists.txt` and `cmake/FindAtomics.cmake`; and
- the upstream `LICENSE`.

## Removed files

Upstream tests, test support, examples, contrib libraries, benchmarks, CI,
documentation, packaging metadata, alternative build-system files, Git
metadata, and release archives are intentionally not included. In particular,
there is no `hwy/tests/`, `hwy/examples/`, `hwy/contrib/`, `.github/`,
`g3doc/`, `docs/`, or `README.md` from upstream.

## Local CMake changes

The vendored `CMakeLists.txt` has four small changes so the removed test
sources are never referenced when tests are disabled and the runtime can be consumed as an object library:

1. `HWY_ENABLE_TOOLS` was added, defaulting to `OFF`, and gates the
   `hwy_list_targets` executable.
2. `HWY_TEST_SOURCES` is declared only when `HWY_ENABLE_TESTS` is enabled.
3. The `hwy_test` target is created only when `HWY_ENABLE_TESTS` is enabled.
4. `HWY_LIBRARY_TYPE` checks `IF(NOT DEFINED HWY_LIBRARY_TYPE)` so callers can configure it as `OBJECT`.

No Highway source or header implementation was changed. To update this copy,
replace it manually with a fixed upstream release, repeat the same pragmatic
crop, record any local CMake changes here, and run the AviSynth+ test suite.

## AviSynthConvertAudio integration

This copy was imported from AviSynthMinus commit `aa638ca4`. The source/header
implementations and the inherited CMake adjustments are unchanged. This project
selects `HWY_LIBRARY_TYPE=STATIC`; the historical OBJECT option remains available
in the vendored CMake file but is not used by this project's integration.
When the host already supplies a compatible `hwy` target, this copy is not built.
Run the standalone conversion tests after updates, and the AviSynth integration
tests when updating the host's pinned version.
