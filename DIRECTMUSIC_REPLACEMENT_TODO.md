# DirectMusic Replacement TODO

Track what is needed to replace the legacy DirectMusic path with the ltjs implementation and expose it to game code.

## Core integration
- Confirm initialization order and lifetime: `Init()`, `Term()`, `InitLevel()`, `TermLevel()` are called from the client as expected.

## API parity gaps (ltjs_dmusic_manager)
- Decide/implement handling for transitions and intensity changes if they need finer timing than the current placeholder behavior.
- Decide/implement beat/measure/grid/marker scheduling for secondary/motif enact types (currently aligned to segment boundaries).
- Decide how to handle motifs that are tied to styles (current implementation treats motif names as segment file paths).

## Data and file handling
- Validate the expected search paths for segments/motifs (ensure the data dir is honored consistently with SMusicMgr).
- Confirm segment loading supports the formats the tools emit today (e.g., RIFF/WAV/OGG) and document supported formats.

## Capability reporting and docs
- Document which DirectMusic features are supported vs. emulated vs. unsupported.
- Add SDK documentation for `ILTDirectMusicMgr` describing limitations and differences from classic DirectMusic.

## Tests / validation
- Add a small playback/transition test harness or scripted map to verify:
  - Basic play/stop/pause/resume.
  - Intensity changes.
  - Motif overlay/secondary segments.
  - Looping and stop boundaries.
