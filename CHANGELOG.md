# Changelog

All notable changes to Cerberus are documented here. Versions follow
[Semantic Versioning](https://semver.org/).

## [2.1.0] — 2026-10-26

### Added

- **macOS Universal builds** — a single VST3 bundle and standalone `.app` for
  Apple Silicon (`arm64`) and Intel (`x86_64`), targeting macOS 11.0 or newer.
- macOS CI job on `macos-14` that builds the Universal binaries, ad-hoc signs
  both bundles, runs the DSP and VST3 host smoke tests, and packages
  `Cerberus-macOS-Universal-v2.1.0.zip` with a SHA-256 checksum.
- macOS install and Gatekeeper guidance in the README.

### Changed

- The release workflow now requires Windows x64, Linux x64, and macOS Universal
  to pass their test suites before publishing the GitHub Release.
- macOS packages are created with `ditto` so bundle metadata and executable
  permissions survive archiving.

## [2.0.0] — 2026-10-26

### Added

- Redesigned **Sleek Futuristic Studio Hardware** interface: brushed gunmetal
  chassis, flush glass overlay with a muted crimson LED edge-glow, low-profile
  knurled metal knobs, ultra-thin vertical glass sliders, and tactile module
  buttons with a thin crimson active border.
- Central **live waveform visualizer** driven by a pre-allocated, lock-free
  post-FX ring buffer; the audio thread performs no allocation or locking.
- Lightweight custom **standalone application** that shows the editor before
  opening an audio device, adds an **AUDIO** button for device/sample-rate/
  buffer selection, and falls back to the full selector when auto-open is
  skipped.
- Single-source release version (`Source/Version.h`, fed by
  `project(Cerberus VERSION ...)`) so the editor footer, standalone app, and
  screenshot tool always match the VST3 bundle metadata.
- `CHANGELOG.md` and a "What's new in 2.0" README section.

### Changed

- Complete interface refresh across the header pill readout, effect panels,
  meters, preset navigation, and footer.
- Linux standalone startup prefers a direct ALSA hardware device and skips
  auto-open when `/dev/snd` is unavailable, avoiding PipeWire/driver stalls in
  `snd_pcm_prepare()`.
- CI, packaging, and README release instructions now target `v2.0.0`.

### Fixed

- Launching the standalone app could appear to do nothing while a stalled
  audio driver blocked startup; the window now opens independently of the
  audio backend.

## [1.0.0] — 2026-10-23

- Initial public release: auto-wah → compressor → distortion VST3 and
  standalone builds, eight factory presets, constant-latency bypass, and
  Windows/Linux CI packages.

[2.1.0]: https://github.com/briansilberbauer-gamatam/cerberus/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/briansilberbauer-gamatam/cerberus/compare/v1.0.0...v2.0.0
[1.0.0]: https://github.com/briansilberbauer-gamatam/cerberus/releases/tag/v1.0.0
