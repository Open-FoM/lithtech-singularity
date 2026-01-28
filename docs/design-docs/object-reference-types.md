**Object Reference Types **(a.k.a. Inter-object Linking Issues)



Traditionally, object handles were cleared to null on object deletion by creating an inter-object link. This method is clumsy because of the fact that it requires extra function calls to establish the link, and a broken link handler, which has to figure out which member variable to clear as a result. This is both inconvenient and error prone.



The new method is to use an LTObjRef in the place of an HOBJECT variable. This variable will behave almost exactly like an HOBJECT for all intents and purposes, including saving and loading. The difference is that the LTObjRef variable will automatically clear itself to NULL when the object gets deleted. The size of an LTObjRef is currently 16 bytes, and assignments are more expensive than with an HOBJECT. (They are still O(1), but make API function calls internally.) One important word of caution is that assignment of an LTObjRef must happen through the assignment operator, and **never **through a memory copy. (For example, a memset to 0 on a record containing an LTObjRef is a bad idea, as it will clear the vtable pointer.)



If something further than clearing to NULL is required, use an LTObjRef_Notify to emulate the previous inter-object linking behavior. The object which will receive the notification is determined by calling the SetNotifyObj() member function. Note that this function takes an ILTBaseClass* parameter, and not an HOBJECT. If other behavior is preferred, create a new class derived from LTObjRef and override the OnObjDelete() member function. Make sure to call LTObjRef::OnObjDelete during the handling of this function in order to clear out the object handle.
