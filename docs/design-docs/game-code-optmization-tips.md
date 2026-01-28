**Optimization Recommendations for LithTech Powered Games **



I have organized our optimization recommendations into three sections:



Recommended

Suggestions

Advanced



You’ll notice that there’s no single recommendation that is going to double the frame-rate. Instead, this is a situation where everything “adds up”. If we can do 20 things that all increase the frame-rate by ½ frame, we can go from 20fps to 30fps (for example).



Historically, we’ve found that spending time on the game code generates the most gains—as well as provides the most options for things to optimize and tune. Modifying game code is also usually the safest tactic in terms of minimizing the potential for new bugs. Hence, you’ll find that most of the items below involve changes and tweaks to game code areas and systems.



We won’t pretend that we have a thorough understanding of every one of your game systems. So, if any of the stuff below is already implemented…then that’s great start.





**Recommended **



Use NuMega’s “TrueTime” profiler (http://www.compuware.com/products/numega/dps/vc/tt_vc.htm). If you do not have this software, I highly recommend that you purchase a copy. It is very easy to use and will very quickly show you where to spend time optimizing.



Add a "Final Release" build target and define "_FINAL" (in the preprocessor) for cshell.dll and object.lto VC projects. Use #ifndef _FINAL to compile-out **everything **that is not needed for the shipping version. We'll use "Final Release" build targets for future profiling since they will run the fastest and be the closest to shipping code. This will also unclog the output from the profiler with stuff we already know is taking CPU time but is not really needed (debug code, console-output, sprintfs, etc.).



Make sure *all* debug and console-output code is compiled out in Final build targets (all those sprintf's really do add up).



Compile the engine with “NO_PROFILE” defined in the preprocessor settings for the project (I would define this in the new “Final Release” build target for the engine).



Remove (or comment-out) all engine profile counters (perfcountermanager.cpp) and/or calls to them. There are still a few in there that are not covered by NO_PROFILE (I believe so that some of the more useful console-vars continue to work), these could be surrounded by using NO_PROFILE or _FINAL #ifdefs.



Provide a method (single console-var?) to go directly to a level and skip 100% of the UI. This will make profiling much more accurate and much easier because the UI won’t clog up the results or slow down the startup process. It will also allow the game to be easily launched for an over-night profile session since no user interaction will be required (and the game can take a **long **time to start up when doing a full profile).



Try to reduce anything that generates IntersectSegment() type of calls, as this is an expensive call. You can do this by staggering object updates, reducing updates, caching values, etc.



Use "ShowClassTicks 1" to monitor the CPU time spent on the AI and other game objects. If possible, you should not have every AI update once per frame. Instead, have each AI only update once per second (or half-second, etc.) and stagger when they do their updates (so they don't all update at the same time). This is something the Monolith game teams have always had to do because AI can be so expensive (yet, each individual AI doesn’t necessarily need to update their decisions and states 30 times per second).



Add a way to toggle the updating of all AI on/off. This can be useful to determine the specific fps hit of AI…and also to eliminate AI when trying to profile other game systems.



Use "ShowFileAccess 1" to view what files/resources are getting loaded at run-time. Try to pre-load as much as possible. You can also use this to verify that resources aren’t getting constantly loaded and unloaded.



Add a way to toggle the in-game HUD on/off to see if some video cards are choking on the 2D blits. We have seen this before in other games (some ATI cards, for example, have really slow 2D blitting performance). One solution is to provide the option of a low-detail HUD (we did this in Bots, for example) and then mention in the readme that this is a good option for owners of specific video cards (and list the cards that QA determines can benefit from this option).



Get the detail options working in the menus. Make the game code aware of the detail settings and update accordingly (create fewer particles, play fewer sounds, etc.) You’ll need this to ship with anyway, and this will let QA give better feedback regarding how the game performs on low-end systems.



Provide the user with a chance to set the "Performance Setting" (low, medium, high) the very first time they run the game (we suggest defaulting to Medium). See the PerformanceDialog.pcx file attachment for an example. This is something Monolith has done in all of their games and can help the user get a good first impression in terms of performance (and also makes them aware of the different options, and the trade-offs in terms of speed vs. quality).



Enable "Inline Any Suitable" in the compiler's optimization settings for Final Release build targets (you may need to experiment with this one). And/or, manually change, small, frequently called functions to be inline. You may have a bunch of one-liner member accessor functions that could really benefit from being inline (as they are showing up in the profile reports).



Verify that the internal radiuses of models are being set properly. If any are way too large, this can cause many models to be considered visible and cause performance slowdowns due to clipping. The AVP2 team ran into this problem from some changes they made for some of their cut-scenes.





**Suggestions **



Use “ShowClassTicks 1” to monitor the CPU time spent on updating the Player. See if it’s possible to optimize anything in the player’s Update() functions…any easy optimizations could be helpful here.



Use all upper-case (or lower) and use strcmp() instead of stricmp() where possible.



Reduce memory usage if/when possible--texture memory, sounds, models, data structures, etc. Don't hurt the look of the game, but if there are excess (or unused) resources getting loaded, try to remove them. Any memory reduction can help a lot--especially on low-end machines. This is mainly what the AVP2 team did to help with their performance on low-end machines.





**Advanced **



Dynamically adjust some game systems and/or effects based on run-time feedback. If you detect that the frame-rate drops below X (for more than Y milliseconds), automatically scale back some of the game systems (particles, sounds, AI-updates?) until the frame-rate is above X (make X the bare-minimum since this would be a last resort type of situation). We did this in Bots with good success, although we made it an option that the user could turn off.
