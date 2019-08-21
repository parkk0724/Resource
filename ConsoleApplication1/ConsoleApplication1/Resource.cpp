#include "pch.h"
#include "Resource.h"
#include "String.h"

#define SAFE_DELETE_ARRAY(x) if(x) delete []x; x = NULL;
#define GCC_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define SAFE_DELETE(p) {if(p) {delete(p); (p) = NULL;}}

ResHandle::ResHandle(
	Resource & resource, char * buffer, unsigned int size, ResCache * pResCache) : m_resource(resource)
{
	m_buffer = buffer;
	m_size = size;
	m_extra = NULL;
	m_pResCache = pResCache;
}

ResHandle::~ResHandle() {
	SAFE_DELETE_ARRAY(m_buffer);
	m_pResCache->MemoryHasBeenFreed(m_size);
}