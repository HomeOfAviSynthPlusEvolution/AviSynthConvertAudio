# AviSynth — Convert Audio

**English** | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

AviSynth — Convert Audio is AviSynthMinus's independent audio sample-format conversion module. It can be built and used without AviSynth, or integrated into the AviSynth core through static linking. It converts between integer PCM and floating-point audio, using Google Highway for cross-platform SIMD acceleration while retaining ordinary C implementations as a fallback.

The project can be included in AviSynth as a Git submodule and also supports standalone testing. Its public interface uses C types and functions, while its implementation uses C++, with no dependency on the AviSynth SDK, AvsCore, or AvsSimd.

## Why separate audio conversion?

Audio sample conversion is a computational capability that can be used and validated independently. Separating it from the frameserver core allows conversion algorithms, SIMD implementations, correctness tests, and benchmarks to be maintained in a focused project, while reducing dependencies on host internals.

AviSynth — Convert Audio evolves alongside AviSynthMinus. The host handles clip access, script entry points, metadata, and audio requests; the conversion library handles conversion from the input format to the output format. Whether the algorithm converts directly or uses intermediate steps is handled within the library, so the host does not need to adjust its calls to follow implementation details.

## Supported formats

The project supports all 20 distinct-format conversion paths between these five sample formats:

| Format | Representation |
|---|---|
| U8 | Unsigned 8-bit PCM, with silence at 128. |
| S16 | Signed 16-bit PCM. |
| S24 | Signed 24-bit PCM, packed into three little-endian bytes. |
| S32 | Signed 32-bit PCM. |
| F32 | 32-bit floating-point audio. |

Conversions preserve existing AviSynth numerical semantics, including truncation, saturation, NaN-to-silence handling, and the S24 byte layout. Sample counts refer to individual channel values; for example, 1,024 stereo audio frames contain 2,048 sample values.

This project performs sample-format conversion, not sample-rate conversion, channel mixing, or video conversion. Those capabilities are maintained in their respective modules.

## SIMD and CPU restrictions

The project uses Highway to generate conversion implementations for different CPU targets and provides a separate ordinary C fallback. Standalone users can automatically select an implementation available on the local machine or explicitly choose ordinary C. During host integration, the host can query the targets actually provided by the library and select a conversion function within the permitted set.

In AviSynth, AvsSimd interprets the environment's CPU restrictions. The final selection is determined by the intersection of host-permitted targets, hardware-supported targets, and targets actually provided by the conversion library. `SetMaxCPU("none")` always selects ordinary C; when no common SIMD target is available, selection also falls back to ordinary C.

Target selection is per consumer instance and does not change Highway's global target restrictions. The integration interface directly uses Highway target bit values. Targets missing or unrecognized across versions are excluded from selection, and explicitly requesting an unsupported target returns failure.

## Building and integration

The project uses CMake and requires a compiler with C++17 support. The general-purpose interface uses C types and functions and exposes no C++ classes, STL containers, or Highway vector types. C++ exceptions do not cross the interface boundary.

The primary integration method is to pin this project as a Git submodule in the host repository and build it together with the host using CMake. The conversion library and its dependencies are linked as static-library targets, with CMake propagating dependencies. The host does not need to compile or enumerate kernel source files directly.

Standalone builds use a pinned Highway version, with 1.4.0 as the baseline. When built with a host, the library reuses a compatible host-provided `hwy` target to avoid building the dependency twice. AvsSimd remains in the AviSynth repository; the conversion library depends directly on Highway.

The interface may evolve alongside AviSynthMinus. A C interface simplifies integration, but does not promise that prebuilt libraries from different versions are interchangeable. Use matching headers and libraries when upgrading.

The CMake target is `AviSynth::ConvertAudio`, and the static library is named `AudioConvert`.

## Testing and performance

Kernel correctness tests and benchmarks can run independently without loading AviSynth. Tests focus on conversion results across formats, numerical boundaries, special floating-point inputs, irregular sample counts, and buffer boundaries. Integration tests in the AviSynth repository validate public filter entry points, audio request behavior, and video passthrough.

As of 2026-09-06, the suite contains **596 tests** after parameter expansion, including a C interface smoke test. All passed locally on Windows x64 in the MSVC, clang-cl, and ordinary C configurations.

The following selected native results convert **65,536 sample values (64K)** per call. Times are in **μs per call**. The baseline is specifically **AviSynth+ 3.7.5**, not later upstream versions.

| Conversion | AviSynth+ 3.7.5 native | Convert Audio native | Speedup |
|---|---:|---:|---:|
| S16 → F32 | 3.173 | 3.178 | 1.00× |
| F32 → S16 | 4.561 | 4.494 | 1.01× |
| S24 → F32 | 6.853 | 5.923 | 1.16× |
| F32 → S24 | 9.311 | 5.425 | 1.72× |
| S32 → F32 | 4.498 | 4.496 | 1.00× |
| F32 → S32 | 4.393 | 4.495 | 0.98× |
| U8 → S16 | 2.102 | 1.617 | 1.30× |
| U8 → S32 | 4.281 | 2.696 | 1.59× |
| S32 → U8 | 3.193 | 2.692 | 1.19× |

Measured on 2026-09-06 using an AMD Ryzen 9 7940H, Windows x64, and clang-cl 22.1.3. Both implementations were built with Release /O2 and no LTO, and ran on the same fixed logical processor. The original kernels came from AviSynth+ 3.7.5 (`6c7c2661`); the module was at `16259ac5`. Each used its available native implementation: SSE2, SSSE3, or AVX2 depending on the original route, and automatically selected Highway `AVX3_ZEN4` for this module.

Each timed batch ran 50,000 conversions, with seven rounds and the median time reported for each implementation. Each batch had 100 warmup calls, and the timing order rotated between the original implementation, this module, and the ordinary C baseline. All 20 routes passed byte-for-byte output comparisons on the timed inputs. Original S24↔F32 timings include both stages through S32; buffers and intermediate storage were allocated in advance and reused.

Speedup is the original time divided by the module time; values above 1 mean this module is faster. Timings cover conversion kernels only, excluding allocation, function selection, and host reads. This is a snapshot from one seven-round measurement; differences of a few percent should not be treated as established gains or regressions.

Performance comparisons should use the same compiler, build options, input data, and CPU restrictions, and clearly state sample counts and the scope of timing. Measure kernel time and complete filter time separately to avoid mixing host reads, buffer management, and other overhead into algorithm comparisons. Wider SIMD targets do not guarantee better performance on every path; implementation decisions should be based on measurements.

## Development and contributions

The project is led by its maintainer, who is responsible for technical direction, decisions about changes, and final releases. Bug reports, suggestions, code contributions, and technical discussions are welcome. For changes to conversion semantics, public interfaces, or substantial architectural changes, we recommend discussing the goals and approach in an issue first.

This project uses AI-assisted development, including implementation, test writing, and code review. The maintainer provides guidance, evaluates changes, and remains responsible for what is accepted and released. Contributions should clearly explain the problem, the implementation approach, and how the change was validated. Please also disclose how AI was involved when contributing with AI assistance.

When reporting a problem, include the project version or commit, operating system and processor architecture, compiler and build options, source and destination formats, and minimal input that reproduces the issue. Performance reports should also include the CPU model, target restrictions, sample count, and measurement method.

## Acknowledgments and license

AviSynth — Convert Audio builds on the work of AviSynth, AviSynth+, AviSynthMinus, and their contributors, and uses Google Highway for cross-platform SIMD capabilities. Thanks to the authors of the original implementations and to the developers and users who test, report issues, and contribute improvements.

Thanks to [SB.SB](https://sb.sb) for sponsoring the LLM subscription used in this project's development.

The project uses the GNU General Public License, version 2 or later, retaining the original wording and scope of the inherited AviSynth linking exception. See [LICENSE](LICENSE) for the full terms. Imported source files retain their original copyright and license notices; third-party components such as Highway follow their own licensing terms.
