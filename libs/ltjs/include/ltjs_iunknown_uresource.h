#ifndef LTJS_IUNKNOWN_URESOURCE_INCLUDED
#define LTJS_IUNKNOWN_URESOURCE_INCLUDED


#include <memory>

#ifdef _WIN32
#include <windows.h>
#else
struct IUnknown;
#endif


namespace ltjs
{


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

struct IUnknownUDeleter
{
	void operator()(
		::IUnknown* iunknown);
}; // IUnknownUDeleter

#ifndef _WIN32
inline void IUnknownUDeleter::operator()(
	::IUnknown*)
{
}
#endif

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

template<
	typename TResource
>
using IUnknownUResource = std::unique_ptr<TResource, IUnknownUDeleter>;

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


} // ltjs


#endif // LTJS_IUNKNOWN_URESOURCE_INCLUDED
