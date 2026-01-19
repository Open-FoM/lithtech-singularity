# DirectMusic Replacement TODO

Track what is needed to replace the legacy DirectMusic path with the ltjs implementation and expose it to game code.

## Core integration
- Audit result: no engine-side call sites were found for `ILTDirectMusicMgr` init/term/level calls; game/client shell must invoke these explicitly.

## API parity gaps (ltjs_dmusic_manager)
- Decide/implement handling for transitions and intensity changes if they need finer timing than the current placeholder behavior.
- Marker scheduling: decide whether to implement marker parsing (currently mapped to segment end).

## Data and file handling
- Done: Documented search paths and supported wave formats in `Guide/DirectMusic.md`.

## Capability reporting and docs
- Document which DirectMusic features are supported vs. emulated vs. unsupported.
- Add SDK documentation for `ILTDirectMusicMgr` describing limitations and differences from classic DirectMusic.

## Tests / validation
- Add a small playback/transition test harness or scripted map to verify:
  - Basic play/stop/pause/resume.
  - Intensity changes.
  - Motif overlay/secondary segments.
  - Looping and stop boundaries.
