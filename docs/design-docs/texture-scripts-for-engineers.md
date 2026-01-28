**Texture Scripts for Engineers: **



The texture script consists of a single text file that contains a single function script whose purpose is to take in input from user defined variables and create a single transformation matrix of which to generate texture coordinates. Scripts are case and white space insensitive and have a commenting style of # denoting a comment and extending to the end of a line. There are two primary parts to a script, a header and a body. The header is used to designate what the script expects as input, and what it produces as output. The body of the script consists of instructions to generate the actual matrix.



**The Header: **



The header has three fields that must be specified in the order of UserParams, Input, and Output. UserParams is a comma-delimited list of the names of the user parameters that the script can use and have accessible through game code. The current limit of this is six although it may be increased in the future. The input specifies the type of vector that the matrix will be multiplied by in order to obtain the output values. These can be one of the following:



| Input | Description |
| --- | --- |
| UV | The UV coordinates of the vertex for that channel. Anything beyond what is provided is 0, so for 2 texture coordinates, the 3 rd and 4 th components being multiplied are 0 |
| WSPos | The position of the vertex in world space. The 4 th parameter is 1 |
| WSNormal | The normal of the vertex in world space. Note that this is only applicable for vertices that specify a normal (currently only environment mapped polygons) |
| WSReflection | The vector from the camera to the vertex reflected over the normal. The same constraints that apply to the normal apply to this. |
| CSPos | These parameters are the same as above, but are provided in camera space instead of world space |
| CSNormal |  |
| CSReflection |  |



The output specifies how the product of the matrix and input should be used. It can be one of the following values:



| Output | Description |
| --- | --- |
| TexCoord2 | UV texture coordinates |
| TexCoord3 | UVW texture coordinates |
| TexCoord3Proj | U/W V/W texture coordinates |
| TexCoord4Proj | U/X V/X W/X texture coordinates |



**The Body: **



The body of the script must be enclosed within the words BeginScript and EndScript. Inside of this block is a list of semicolon-delimited assignments. The operations supported by the language are listed below:



| Operator | Meaning |
| --- | --- |
| + | Addition |
| - | Subtraction or negation |
| * | Multiplication |
| / | Division |
| % | Bind/Modulo (maps left to 0..right) |
| < | Min (returns smaller of two parameters) |
| > | Max (returns larger of two parameters) |
| = | Assign |



In addition it also supports the use of functions. Currently the only functions supported are sin, cos, and tan.



**Parameters: **



Inside of the script body the script has access to a number of variables. There are the user variables that were defined, and can be used by the exact same name. In addition, there are the matrix variables that represent to components of the matrix that is being filled out. These can be accessed by the variables MatRC where R is the row, and C the column. Both of these fields range from 0 to 3. For example, to access the upper left element in the matrix, Mat00 can be used. To access the upper right element, Mat03 can be used. In addition, there are two special parameters that scripts have access to. These are Time, which represents the total number of seconds the script has been running, and Elapsed, which represents the number of seconds since the last update.



**Hints and Tips: **



The fewer number of operations done, the faster the script will execute. Therefore it is best to try and reduce terms as much as possible by doing things such as performing math on constants by hand, and caching temporary results. These results can be cached in unused matrix elements and later replaced. Most of the complexity of the scripts comes from the underlying matrix math, so it is best to have a book with a good explanation of linear algebra nearby to operate on matrices.



**Sample Script: **



Below is a sample script heavily commented to help clarify the above explanation.



#########################################

# Warble

#

# This script causes the UV coordinates to

# warble around as if underwater based upon

# the user parameters.

##########################################



#Our parameters:

# XFreq, YFreq How many cycles per second for X and Y

# XScale, YScale, The amount to offset for each X and Y

UserParams XFreq, YFreq, XScale, YScale;



#We want to take the original UV coordinates in as parameters for our script

Input UV;



#We are going to output 2 texture coordinates, a U and a V

Output TexCoord2;



#Here is our actual script block

BeginScript

#Now setup the appropriate matrix components to warble

Mat01 = cos((Time * XScale) % (2 * 3.14159)) * XScale;

Mat10 = sin((Time * YScale) % (2 * 3.14159)) * YScale;

EndScript
