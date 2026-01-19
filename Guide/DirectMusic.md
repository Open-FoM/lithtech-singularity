# DirectMusic Replacement (ltjs)

This note documents data/search paths and audio formats for the ltjs DirectMusic replacement.

## Data and Search Paths
- `ILTDirectMusicMgr::InitLevel(working_directory, control_file, ...)` sets the base working directory.
- Control file resolution:
  - If `control_file` has no path separator, it is resolved as `working_directory/control_file`.
  - If `control_file` already includes a path, it is used as-is.
- Segment (and motif) names in the control file are resolved relative to `working_directory`.
- Wave references inside a segment are resolved relative to the segment file's directory.

## Supported Audio Formats
- Segment files must be DirectMusic segment (RIFF `DMSG`) files.
- Embedded wave data is decoded via `ltjs::AudioDecoder` and supports:
  - PCM WAV
  - Microsoft IMA ADPCM WAV
  - MP3-in-WAV (WAVE format tag 0x0055)
- OGG/Vorbis is not supported by the decoder.
