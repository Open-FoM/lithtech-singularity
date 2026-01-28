| ### Content Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Appendix B: Localization Guidelines

You can use Jupiter to write a game that is easily localized for distribution in foreign countries. Here are some guidelines that will make it easier for you to localize your game.

- Move every text string that is displayed in the game into the CRES.DLL.
- All bitmap fonts in the game need a way to be turned off so that system TrueType fonts can be used instead.
- All the font names and sizes should be selectable in CRES.DLL so the localization company can adjust them.
- All single key responses need to be selectable in CRES.DLL (for example, in a Yes/No dialog, yes is *oui *in French so the user needs to press the O key).
- All string manipulation must be done with the tchar (or mbcs) routines. Information about this is available in Visual C/C++ help. This allows the program to use multibyte character sets.
- All projects should have the **_MBCS **flag set in the compiler directives.
- There should be a way to disable gore completely from the game in CRES.DLL so that the option to set the gore level is not even available to the user. Some countries will not allow gore.
- Anyplace where you use word wrapping, you must provide a way to deal with languages (such as Japanese) that do not have spaces between words. For these languages some type of hard wrapping where the line can be wrapped in the middle of a word must be used. This should be an option in CRES.DLL. Note that the rules for hard wrapping are not completely simple because certain characters should never be allowed to start or be in the first two characters of a line.
- If you are not localizing huge quantities of graphics and sounds, you can probably put all of your localized resources and a localized version of your CRES.DLL in a separate language .REZ file. It should be loaded last so its resources take priority. Usually you can then make patches and a new version of your software and still use the same localized .REZ file.
- The default keyboard setup for a localized keyboard will probably be different so it will need to be re-configured in the AUTOEXEC.CFG file for each localized version. Sometimes the ## keyboard codes must be used for this to work properly on foreign keyboards.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Appendix\Apnd-B\Localize.md)2006, All Rights Reserved.
