| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Compressed Sounds

LithTech can play sounds in .WAV format. Compressed sounds, such as .MP3, can be played if they are embedded in a .WAV file. There are various third-party tools available to embed other file formats into the .WAV format. You will need to arrange for a license from Fraunhofer for use of the .mp3 codec (visit [http://mp3licensing.com/ ](http://mp3licensing.com/)).

Playing compressed sounds significantly impacts game performance. You should only use compressed sounds when CPU load is low.

The decompression works seamlessly, as long as the file is a .wav file. You can just make the normal **PlaySound **calls and the engine will decompress, if necessary. IMA-ADPCM compression is also supported.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\Compress.md)2006, All Rights Reserved.
