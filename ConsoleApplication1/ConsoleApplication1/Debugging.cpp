#include "pch.h"
//========================================================================
// Game Coding Complete Referance - Debugging Chapter
//========================================================================
#include <unknwn.h>
int Refs(IUnknown *pUnk)
{
	pUnk->AddRef();
	return pUnk->Release();
}
