# Font Resolution (SDL_ttf backend)

On non-Windows builds, texture string glyphs are generated via SDL_ttf.
Font lookup is intentionally simple and does **not** scan system fonts.

Resolution order:
1) If `CFontInfo::m_szTypeface` looks like a file path (contains `/` or `\\`, or ends in `.ttf`, `.otf`, `.ttc`), that file is loaded directly.
2) Otherwise, if the font was registered via `RegisterCustomFontFile`, SDL_ttf is used to read the font family name and that family is matched (case-insensitive).
3) Fallback: `OpenSans-Regular.ttf` (copied next to the executable for mac builds).

Notes:
- `RegisterCustomFontFile` will extract the font to a temp file if it cannot be accessed directly on disk.
- Extracted temp files currently use `/tmp` and are deleted when the font is unregistered.
- System font discovery (CoreText / fontconfig) is not used yet.
