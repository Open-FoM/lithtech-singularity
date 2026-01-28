Important Jupiter engineering notes for 6/03/02



The ILTMessage interface has been removed and replaced with ILTMessage_Read and ILTMessage_Write. To create a new message for writing, call ILTCommon::CreateMessage(). All ILTMessage::Read*FL member functions and stream operators whose functionality was duplicated in other member functions were removed. The list of the remaining original member functions and their new counterparts is as follows:

ILTMessage::ReadByte -> ILTMessage_Read::Readuint8

ILTMessage::ReadWord -> ILTMessage_Read::Readuint16

ILTMessage::ReadDWord -> ILTMessage_Read::Readuint32

ILTMessage::ReadFloat -> ILTMessage_Read::Readfloat

ILTMessage::ReadHString -> ILTMessage_Read::ReadHString

ILTMessage::ReadObject -> ILTMessage_Read::ReadObject

ILTMessage::ReadVector -> ILTMessage_Read::ReadLTVector

ILTMessage::ReadCompVector -> ILTMessage_Read::ReadCompLTVector

ILTMessage::ReadCompPos -> ILTMessage_Read::ReadCompPos

ILTMessage::ReadRotation -> ILTMessage_Read::ReadLTRotation

ILTMessage::ReadCompRotation -> ILTMessage_Read::ReadCompLTRotation

ILTMessage::ReadMessage -> ILTMessage_Read::ReadMessage

ILTMessage::ReadStringFL -> ILTMessage_Read::ReadString

ILTMessage::ReadHStringAsString -> ILTMessage_Read::ReadHStringAsString

ILTMessage::ReadRawFL -> ILTMessage_Read::ReadData

Note : This function now takes the data size as a number of bits instead of bytes.

ILTMessage::WriteByte -> ILTMessage_Write::Writeuint8

ILTMessage::WriteWord -> ILTMessage_Write::Writeuint16

ILTMessage::WriteDWord -> ILTMessage_Write::Writeuint32

ILTMessage::WriteFloat -> ILTMessage_Write::Writefloat

ILTMessage::WriteString -> ILTMessage_Write::WriteString

ILTMessage::WriteVector -> ILTMessage_Write::WriteLTVector

ILTMessage::WriteCompVector -> ILTMessage_Write::WriteCompLTVector

ILTMessage::WriteCompPos -> ILTMessage_Write::WriteCompPos

ILTMessage::WriteRotation -> ILTMessage_Write::WriteLTRotation

ILTMessage::WriteCompRotation -> ILTMessage_Write::WriteCompLTRotation

ILTMessage::WriteRaw -> ILTMessage_Write::WriteData

Note : This function now takes the data size as a number of bits instead of bytes.

ILTMessage::WriteMessage -> ILTMessage_Write::WriteMessage

ILTMessage::WriteHString -> ILTMessage_Write::WriteHString

ILTMessage::WriteHStringFormatted -> ILTMessage_Write::WriteHStringFormatted

ILTMessage::WriteHStringArgList -> ILTMessage_Write::WriteHStringArgList

ILTMessage::WriteStringAsHString -> ILTMessage_Write::WriteStringAsHString

ILTMessage::WriteObject -> ILTMessage_Write::WriteObject

ILTMessage::Seek -> ILTMessage_Read::SeekTo

Note : This function now takes the position as a number of bits instead of bytes.

ILTMessage::Tell -> ILTMessage_Read::Tell

Note : This function now returns the position as a number of bits instead of bytes.

ILTMessage::Length -> ILTMessage_Read/ILTMessage_Write::Size

Note : This function now returns the message size as a number of bits instead of bytes.

The message interfaces are now derived from ILTRefCount, which is a reference counting interface. Use the CLTMsgRef_Read and CLTMsgRef_Write types to automatically increment and decrement the reference count of the message objects. Note that engine functions will properly increment and decrement the reference count, which allows passing a 0-reference count object into an interface member function without leaking memory. (This is useful for dealing with ILTMessage_Write::Read).

The following new message manipulation functions were added to ILTMessage_Write:

Reset – Clear the message. (i.e. forget what’s been written.)

Read – Create an ILTMessage_Read object which contains the data that has been written to the ILTMessage_Write object. The message will be reset by this function.

WriteBits – Write a number of bits from a 32-bit integer to the message.

WriteBits64 – Write a number of bits from a 64-bit integer to the message.

WriteYRotation – Write a rotation about the Y axis as an 8-bit integer.

Writebool – Write a boolean value. (1-bit)

Writeuint64 – Write a 64-bit unsigned integer.

Writeint* - Write a signed integer.

Writedouble – Write a 64-bit floating-point value.

WriteType – Write the binary representation of the provided object to the message. (This is a templatized data member function.)

The following new message manipulation functions were added to ILTMessage_Read:

Clone – Create a duplicate of this ILTMessage_Read object.

SubMsg – Create a duplicate message which contains a sub-portion of the data.

Seek – Offset the read position by a specified number of bits.

TellEnd – Returns the number of bits remaining between the current read position and the end of the message.

EOM – Returns true if the read position is at the end of the message. Attempts to read beyond this point will return 0’s.

ReadBits – Read up to 32 bits from the message.

ReadBits64 – Read up to 64 bits from the message.

ReadYRotation – Read a rotation about the Y axis from the message.

Readbool – Read a Boolean value. (1-bit)

Readuint64 – Read a 64-bit unsigned integer

Readint* - Read a signed integer

Readdouble – Read a 64-bit floating-point value.

ReadType – Read the binary representation of an object of the specified type from the object. (This is a templatized data member function.)

Peek* - These member functions will read from the message without incrementing the read position.

Parameters to most interface member functions are now const-correct.

Changed all interfaces to use bool instead of LTBOOL.

Removed message writing member functions from ILTCSBase. (WriteTo*)

Moved the message ID’s in ILTBaseClass::ObjectMessageFn, IClientShell::OnMessage, IServerShell::OnMessage and IServerShell::OnObjectMessage into the actual message. This is to allow future modifications to the length of the message ID.
