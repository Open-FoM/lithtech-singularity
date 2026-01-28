**COMMAND ARCHITECTURE **

******

***Basic “grammar”: ***

****

**(If you don’t follow this, don’t worry about it. It is more important for you to just understand how the commands below work) **

****

**Command ** **<command name> <arg 1> <arg 2> … <arg N> ***Number of arguments depends on the command *.

****

A command can be made up of multiple commands (seperated by semicolons):



**Command 1 <arg 1> … <arg N>;Command 2 <arg 1> … <arg N>;…Command N <arg 1> … <arg N> **



Also commands can be nested inside other commands if a command is expected as an argument:



**Command 1 <arg1 > <Command 2 <arg 1>…<arg N>> … <arg N> **

******

***Commands: ***

None of the commands, command ids, object names, or object messages are case sensitive! Msg = msg = MSG = mSg 



**ListCommands **



This is used internally to list out all of the available commands (i.e., if “commands” is specified in the console the “ListCommands” command is sent to the CommandMgr).



**Msg <object name> <object msg> **



The Msg command should be used to send one or more objects a message to be processed by the object.



| <object name> <object name>  <object name> <object name> | *Case-insensitive name of the object(s) * |
| --- | --- |
| <object msg> | *String that the object parses – i.e., syntax not known to command mgr * |



**Examples: **

| Msg prop0 destroy | *Sends the “destroy” message to the object named prop0 * |
| --- | --- |
| Msg prop0 fire | *Sends the “fire” message to the object named prop0 * |
| Msg prop0 (fire player) | *Sends the “fire player” message to the object named prop0 * |





**Delay <delay> <command> **



The Delay command is used to delay the sending of a command by some length of time.



| <delay> | *Time for the command to be delayed * |
| --- | --- |
| <command> | *See Command above *** |

****

**Example: **

| Delay 0.5 (msg prop0 destroy) | *Waits ½ a second then processes the “msg prop0 destroy” command * |
| --- | --- |

****

**DelayId <command id> <delay> <command> **



The DelayId command is exactly like the Delay command. However, a DelayedId command can also be stopped (without being processed) by sending the *Abort *command (See below).



| <command id> | *Unique string id for this command (16 characters max). * |
| --- | --- |
| <delay> | *Time for the command to be delayed * |
| <command> | *See Command above *** |

****

**Example: **

| DelayId d1 2.5 (msg prop0 destroy) | *Waits 2.5 seconds then processes the “msg prop0 destroy” command. This command is named “d1” so the command could be aborted by sending an “Abort d1” command (See Abort below) * |
| --- | --- |

****

**Rand <command1 percent> <command 1> <command 2> **



The *Rand *command is used to randomly send one of two commands based on the specified percentage.



| <command 1 percent> | *Percentage (between 0.0 and 1.0) that <command 1> will be processed * |
| --- | --- |
| <command 1> | *Command that will be processed <command 1 percent> of the time (See Command above). * |
| <command 2> | *Command that will be processed 1.0 - <command 1 percent> of the time (See Command above). *** |

****

**Example: **

| Rand 0.25 (msg prop0 fire) (msg prop2 fire) | *Randomly “msg prop0 fire” or “msg prop2 fire” is processed. There is a 25% chance “msg prop0 fire” will be processed and a 75% chance “msg prop2 fire” will be processed. * |
| --- | --- |

**

**Rand2 <command 1> <command 2> **

**Rand3 <command 1> … <command3> **

**Rand4 <command 1> … <command4> **

**Rand5 <command 1> … <command5> **

**Rand6 <command 1> … <command6> **

**Rand7 <command 1> … <command7> **

**Rand8 <command 1> … <command8> **



The *Rand2 – Rand8 *commands are used to randomly send one of 2 to 8 commands equally. These should be used in place of the *Rand *command if you want to randomly select 2 or more commands and have each command have the same chance of being processed.



**Example: **

| Rand3 (msg prop0 fire) (msg prop2 fire) (msg prop3 fire) | *There is an equal chance that any of the 3 commands will be processed. * |
| --- | --- |

**

**Loop <min delay> <max delay> <command> **



The *Loop *command is used to continuously repeat a specific command. The time between processing of the command is recalculated every time the command is processed based on the min and max delay arguments.

| <min delay> | *Minimum time between processing the command. * ** |
| --- | --- |
| <max delay> | *Maximum time between processing the command. * |
| <command> | *See Command above *** |

****

**Example: **

| Loop 0.25 2.0 (msg prop0 fire) | *Continuously process the “msg prop0 fire” randomly every ½ second to every 2 seconds. (the delay to the next time processed is recalculated each time the command is processed). * |
| --- | --- |

**

**

**LoopId <command id> <min delay> <max delay> <command> **



The *LoopId *command is exactly like the Loop command. However, the LoopId command can also be stopped by sending the *Abort *command (see below). If you don’t need to abort the command, use the Loop command specified above.

| <command id> | *Unique string id for this command (16 characters max). * |
| --- | --- |
| <min delay> | *Minimum time between processing the command. * ** |
| <max delay> | *Maximum time between processing the command. * |
| <command> | *See Command above *** |

****

**Example: **

| Loop l1 0.25 2.0 (msg prop0 fire) | *Continuously process the “msg prop0 fire” randomly every ½ second to every 2 seconds. (the delay to the next time processed is recalculated each time the command is processed). * This command is named “l1” so the command could be aborted by sending an “Abort l1” command (See Abort below) |
| --- | --- |

**

**Repeat <min times> <max times> <min delay> <max delay> <command> **



The *Repeat *command is used to repeat a specific command a certain number of times. The time between processing of the command is recalculated every time the command is processed based on the min and max delay arguments. The command will be repeated randomly between the min and max number of times requested.

| <min times> | *Minimum number of times to repeat * |
| --- | --- |
| <max times> | *Maximum number of times to repeat * |
| <min delay> | *Minimum time between processing the command * |
| <max delay> | *Maximum time between processing the command. * |
| <command> | *See Command above. * |

****

**Example: **

| Repeat 2 5 0.25 2.0 (msg prop0 fire) | *Process the “msg prop0 fire” randomly every ½ second to every 2 seconds between 2 and 5 times (The delay to the next time processed is recalculated each time the command is processed). * |
| --- | --- |

**

**RepeatId <command id> <min times> <max times> <min delay> <max delay> <command> **



The *RepeatId *command is exactly like the Repeat command. However, a RepeatId command can be stopped before it has been processed the requested number of times by sending the *Abort *command (see below). If you don’t need to abort the command, use the Repeat command described above.

| <command id> | *Unique string id for this command (16 characters max). * |
| --- | --- |
| <min times> | *Minimum number of times to repeat * |
| <max times> | *Maximum number of times to repeat * |
| <min delay> | *Minimum time between processing the command * |
| <max delay> | *Maximum time between processing the command. * |
| <command> | *See Command above. * |

****

**Example: **

| RepeatId r1 2 5 0.25 2.0 (msg prop0 fire) | *Process the “msg prop0 fire” randomly every ½ second to every 2 seconds between 2 and 5 times (The delay to the next time processed is recalculated each time the command is processed). * This command is named “r1” so the command could be aborted by sending an “Abort r1” command (See Abort below) |
| --- | --- |

**

**Abort <command id> **

****

The *Abort *command is used to stop a LoopId, RepeatId, or DelayId command (See *LoopId, RepeatId, *and *DelayId *above).



| <command id> | *Id of the command to abort. * |
| --- | --- |

**

**Example: **

| Abort r1 | Abort the command named “r1” |
| --- | --- |
|  |  |



**Int <variable name> <value> **

****

The *Int *command is used to declare an integer variable and assign an initial value. Once a variable is declared you can modify its value by using the commands *Set, Add *or *, Sub *(see below).



| <variable name> | *Unique string representing the name of the variable. (32 characters max). Use this name to reference the variable in other commands such as Set. NOTE: The first character of the variable name CAN NOT be a number. * |
| --- | --- |
| <value> | *The initial value given to the variable. Value must be an integer number. * |

**

**Example: **

| Int Var1 100 | Declare a variable named Var1 with an initial value of 100. |
| --- | --- |



**Set <variable name> <value> **

****

The *Set *command is used to assign a value to a variable. The variable must be declared before setting its value. (See *Int *above).



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are assigning the variable to. Value can be a number, 3, or another variable, Var2. Assigning a variable to another variable will simply make the variables equal to each other. * |

**

**Example: **

| Set Var1 0 | Assign the value 0 to the variable Var1 |
| --- | --- |
| Set Var1 Var2 | Assign the value of Var2 to the variable Var1. |



**Add <variable name> <value> **

****

The *Add *command is used to add a value to a variable. The variable must be declared before setting its value. (See *Int *above).



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are adding to the variable. Value can be a number, 3, or another variable, Var2. Adding a variable to another variable will simply increase the variable by the other variables value. * |

**

**Example: **

| Add Var1 5 | Increment the value of Var1 by 5. |
| --- | --- |
| Add Var1 Var2 | Increment the value of Var1 by the value of Var2. |



**Sub <variable name> <value> **

****

The *Sub *command is used to subtract a value from a variable. The variable must be declared before setting its value. (See *Int *above).



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are subtracting from the variable. Value can be a number, 3, or another variable, Var2. Subtracting a variable from another variable will simply decrease the variable by the other variables value. * |

**

**Example: **

| Sub Var1 5 | Decrement the value of Var1 by 5. |
| --- | --- |
| Sub Var1 Var2 | Decrement the value of Var1 by the value of Var2. |



**If <condition> THEN <commands> **

****

The *If *command is used to process a command or set of commands if the condition argument is evaluated to TRUE.



| <condition> | *The condition argument is an expression or set of expressions consisting of variable comparisons using Operators. *(See *Operators *below). |
| --- | --- |
| <commands> | *The command(s) that should be processed when the condition is met. If you want to process multiple commands they must be surrounded by parenthesis. * |

**

**Example: **

| If (Var1 equals 1) Then (msg Door1 Open) | If the variable Var1 has a value of 1 then send a message to object Door1 to open. |
| --- | --- |
| If ((Var1 == 1) AND (Var2 <= 5)) Then ((msg Door1 open) (msg Door2 Close)) | If the variable Var1 has a value of 1 and the variable Var2 has a value that is less than or equal to 5 then send a message to object Door1 to open and a message to object Door2 to close. |



**When <condition> THEN <commands> **

****

The *When *command is used to process a command or set of commands when the condition argument is evaluated to TRUE. Very similar to the *If *(see above) command however the *When *command does not need to be processed more than once to check its condition. As soon as the condition is true the command(s) will be processed.



| <condition> | *The condition argument is an expression or set of expressions consisting of variable comparisons using Operators. *(See *Operators *below). |
| --- | --- |
| <commands> | *The command(s) that should be processed when the condition is met. If you want to process multiple commands they must be surrounded by parenthesis. * |

**

**Example: **

| When (Var1 equals 1) Then (msg Door1 Open) | When the variable Var1 has a value of 1 then send a message to object Door1 to open. |
| --- | --- |
| When (((Var1 == 1) AND (Var2 <= 5)) OR (Var3 != 0)) Then ((msg Door1 open) (msg Door2 Close)) | When the variable Var1 has a value of 1 and the variable Var2 has a value that is less than or equal to 5 or the variable Var3 has a value that is not equal to 0 then send a message to object Door1 to open and a message to object Door2 to close. |
|  |  |





***Operators: ***



A couple of the above commands *(If, On *) have a condition argument where you can compare a variable to a number value or another variable. The condition argument can be broken down into a single expression or a series of expressions. An expression has 3 arguments, the first parameter followed by an operator followed by the second parameter. Depending on the operator, either of the parameters can be another expression. Every operator can be expressed by either a mathematical symbol or the English phrase equivalent.





**<Variable> Equals (==) <Value> ******

****

The *Equals **(==) *operator is a comparison operator used to determine if a variable is equal to a number value or another variable. An expression with the *Equals (==) *operator will evaluate to true when <Variable> has a value equal to <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 == 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Equals Var2 | Compare Var1 to the variable value Var2 |



**<Variable> Not_Equals (!=) <Value> ******

****

The *Not_Equals **(!=) *operator is a comparison operator used to determine if a variable is equal to a number value or another variable. An expression with the *Not_Equals (!=) *operator will evaluate to true when <Variable> has a value that is not equal to <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 != 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Not_Equals Var2 | Compare Var1 to the variable value Var2 |



**<Variable> Not_Equals (!=) <Value> ******

****

The *Not_Equals **(!=) *operator is a comparison operator used to determine if a variable is equal to a number value or another variable. An expression with the *Not_Equals (!=) *operator will evaluate to true when <Variable> has a value that is not equal to <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 != 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Not_Equals Var2 | Compare Var1 to the variable value Var2 |



**<Variable> Greater_Than (>) <Value> ******

****

The *Greater_Than **(>) *operator is a comparison operator used to determine if a variable is greater than a number value or another variable. An expression with the *Greater_Than (>) *operator will evaluate to true when <Variable> has a value that is greater than <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 > 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Greater_Than Var2 | Compare Var1 to the variable value Var2 |



**<Variable> Less_Than (<) <Value> ******

****

The *Less_Than **(<) *operator is a comparison operator used to determine if a variable is less than a number value or another variable. An expression with the *Less_Than (<) *operator will evaluate to true when <Variable> has a value that is less than <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 < 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Less_Than Var2 | Compare Var1 to the variable value Var2 |



**<Variable> Greater_Than_Or_Equal_To (>=) <Value> ******

****

The *Greater_Than_Or_Equal_To **(>=) *operator is a comparison operator used to determine if a variable is greater than or equal to a number value or another variable. An expression with the *Greater_Than_Or_Equal_To (>=) *operator will evaluate to true when <Variable> has a value that is greater than or equal to <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 >= 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Greater_Than_Or_Equal_To Var2 | Compare Var1 to the variable value Var2 |



**<Variable> Less_Than_Or_Equal_To (<=) <Value> ******

****

The *Less_Than_Or_Equal_To **(<=) *operator is a comparison operator used to determine if a variable is less than or equal to a number value or another variable. An expression with the *Less_Than_Or_Equal_To (<=) *operator will evaluate to true when <Variable> has a value that is less than or equal to <Value>.



| <variable name> | *Unique string representing the name of an existing variable. * |
| --- | --- |
| <value> | *The value you are comparing the variable to. Value can be a number, 3, or another variable, Var2. * |

**

**Example: **

| Var1 <= 5 | Compare Var1 to the number value 5 |
| --- | --- |
| Var1 Less_Than_Or_Equal_To Var2 | Compare Var1 to the variable value Var2 |



**<Expression1> And (&&) <Expression2> ******

****

The *And **(&&) *operator is a logical operator used to determine if both expressions evaluate to true. An expression with the *And (&&) *operator will evaluate to true when both <Expression1> and <Expression2> evaluate to true. Both expressions need to be enclosed in parenthesis.



| <Expression1> | *An expression that you wish to evaluate. * |
| --- | --- |
| <Expression2> | *An expression that you wish to evaluate. * |

**

**Example: **

| (Var1 == 2) And (Var2 <= Var1) | The entire expression will evaluate to true if variable Var1 is equal to number value 2 and variable Var2 is less than or equal to the variable Var1. |
| --- | --- |
| ((Var1 == 2) && (Var2 <= Var1)) && (Var3 >= Var1) | If all of the three expressions evaluate to true the whole expression is true. If any one of them evaluate to false the whole expression is false. |





**<Expression1> Or (||) <Expression2> ******

****

The *Or **(||) *operator is a logical operator used to determine if either expressions evaluate to true. An expression with the *Or (||) *operator will evaluate to true when either <Expression1> or <Expression2> evaluate to true. Both expressions need to be enclosed in parenthesis.



| <Expression1> | *An expression that you wish to evaluate. * |
| --- | --- |
| <Expression2> | *An expression that you wish to evaluate. * |

**

**Example: **

| (Var1 == 2) Or (Var2 <= Var1) | The entire expression will evaluate to true if variable Var1 is equal to number value 2 or variable Var2 is less than or equal to the variable Var1. |
| --- | --- |
| ((Var1 == 2) || (Var2 <= Var1)) || (Var3 >= Var1) | If either of the three expressions evaluate to true the whol expression is true. If they all evaluate to false the whole expression is false. |





***Use of Parenthesis: ***



Parenthesis must be used to group multi-term arguments together. This is most often necessary when a command is an argument to another command (i.e., “Repeat 2 5 0.25 2.0 **(msg prop0 fire) **”). In this example *msg prop0 fire *is a single argument to the command *Repeat *. ****In order for the *Repeat *command to correctly parse the string “ *msg prop0 fire *” as a single argument, the individual terms must be grouped together using parenthesis.
