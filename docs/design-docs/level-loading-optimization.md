**OPTIMIZING LEVEL LOADING VS. SEAMLESS LEVEL LOADING **

**(AND HOW IT APPLIES TO THE LITHTECH JUPITER SYSTEM) **





Introduction



Seamless Level Loading (SLL) is the process of allowing extremely large levels to be used in a game without having to have the entire level loaded into memory at the same time. Usually, the level is divided into sections and various sections can be loaded on the fly, as they are needed. Multiple sections can be in memory at the same time and sections that are no longer in use can be unloaded to free up memory and other resources. The process is performed “seamlessly” such that gaps in levels and pauses during loading are not noticeable.



Many developers would like to support seamless loading of large outdoor levels for their games. However, support for SLL of large levels is a difficult feature to implement and very few engines currently support it.



Although some developers are considering trying to add this feature into existing engines, the reality is that it can take 9-12 man months (or longer) to fully develop, test, and debug.



Another option that many developers are considering instead of SLL is Optimized Level Loading (OLL). OLL involves engine modifications and resource usage techniques that can drastically reduce the time needed to load a new level. OLL is often a more realistic solution for developers that do not have the time or resources to add SLL to their game.





Hurdles Involved in Supporting Seamless Level Loading



There are many hurdles to overcome in the development of SLL--for example:



SLL requires a tools system that allows artists to actually create, edit and store massive levels. Even 3D Studio Max and Maya can run into size limitations when working with extremely large levels.

SLL can quickly induce inaccuracy in numerical precision due to the size that the coordinate system must span.

SLL requires that more game objects be kept in memory and managed as to whether they are active, inactive or fully swapped out.

Resource management, frequent disk access and memory requirements can make it difficult to keep the game running smoothly, especially on lower-end systems.

Properly testing and debugging an SLL system can be extremely difficult and time consuming.





Due to the above reasons, an engine must be designed and developed with SLL from the start in order to properly and successfully support SLL.



Because Jupiter was not designed or developed to support SLL, almost every major Jupiter system would need to significantly modified or even re-written completely in order to support SLL. Also, the content creation tools would need to change significantly in order to handle the creation, processing and storage of the much larger level data sets.





Optimized Level Loading as an Alternative



Although many game developers would like to support Seamless Level Loading (SLL) in their game, it is not always practical or even possible given the technology they are using, their budget and/or their schedule. Adding an Optimized Level Loading (OLL) system may be a more viable alternative for many developers.



In an OLL system, the engine, file formats and resource usage are highly optimized for extremely fast and efficient loading and unloading of data. Engine modifications, careful level planning and resource sharing can potentially reduce level-loading times to only a few seconds.



Jupiter has many systems that can be better leveraged or modified to support OLL.





Techniques for Implementing Optimized Level Loading



There are various techniques that can be used to implement an Optimized Level Loading system for your game. Some of them require modifications to engine code, some require modifications to game code and others require changes to the way game resources are used and managed.



OLL techniques that can be used or added include:



*Sharing Game Resources *

A large part of loading a level often involves the loading of textures, models, and sounds for that specific level. By sharing more resources across all levels, less data will need to be loaded when switching levels.



*Keeping Levels Small *

By keeping levels small, there is generally less data to load. Because the level loading times will be very short, smaller levels should not have a negative impact on the gaming experience.



*Using a Background Loading Thread *

A background loading thread can use predictive algorithms to pre-load level data that will be needed soon. When the data is actually needed, it should already be loaded into memory.



*Optimizing Data Formats on Disk *

The format of all data on disk needs to match the format that the data needs to be in when it resides in memory. This means that any processing or conversion that the data needs to go through must happen when the data is created (usually by the tools). This will allow the data to be loaded directly into memory without any time consuming unpacking, processing, or conversions performed on the data.



*Storing all Resource Files within a Single Data File *

All files that get loaded should be stored within a single data file (like a Jupiter .rez file). This will eliminate the time needed by the file system to open and close multiple files when loading data. Instead, only one file is opened by the file system.



*Grouping and Duplicating Resource Data on Disk *

Related data needs to be grouped together on disk so that all of it can be loaded with a single disk read. This eliminates time spent by the file system performing disk seeks in order to read multiple resources from the data file. In some cases, this may mean that some resources need to be duplicated on the disk because they need to be in multiple places in order to further reduce seeking for all levels.



*Removing All Strings from File Formats *

All strings that are part of a level’s data should be converted into numerical ID values in order to reduce size as well as eliminate potential string compares which can take up a significant amount of time.



*Eliminating Dynamic Memory Allocation *

Eliminating dynamic memory allocation during level loading can significantly decrease loading times—simply because memory allocation (and de-allocation) takes time.



*Requiring More Memory for your Minimum Spec Machine *

By requiring more memory for your minimum spec machine, levels will generally load faster due to improved disk cache and virtual memory usage by the operating system.





All of the above OLL techniques can be added to the Jupiter engine code and/or used by a game developed with Jupiter. Note that not all of them need to be implemented in order to see reductions in level loading times. The more techniques that a developer implements, the faster their level loading times should be.





Conclusion



Although many developers would like to support Seamless Level Loading (SLL), the reality is that it is a very difficult feature to implement, test and support in a game. Content creation challenges, object handling, and memory management are tough problems that stand in the way of a successful game using SLL.



An alternative to SLL is to implement Optimized Level Loading (OLL). OLL still requires the loading of individual levels, but the loading times are significantly reduced—potentially down to only a few seconds, for example.



Because the LithTech Jupiter System was not designed to support SLL, the required changes to the engine code, tools code and file formats would require a significant amount of time—possibly as much as 12 man months in order to implement SLL. The use of OLL in a game developed with Jupiter is a much more realistic option in terms of completing a successful game on time and on budget.
