#include "pch.h"
#include "Resource.h"
#include <cassert>

#define SAFE_DELETE_ARRAY(x) if(x) delete []x; x = NULL;
#define GCC_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define SAFE_DELETE(p) {if(p) {delete(p); (p) = NULL;}}

ResHandle::ResHandle(
	Resource & resource, char * buffer, unsigned int size, ResCache * pResCache)
	: m_resource(resource)
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


ResCache::ResCache(const unsigned int sizeInMb, IResourceFile *resFile)
{
	m_cacheSize = sizeInMb * 1024 * 1024;	// total memory size
	m_allocated = 0;	// total memory allocated
	m_file = resFile;
}

ResCache::~ResCache()
{
	while (!m_lru.empty())
	{
		FreeOneResource();
	}
	SAFE_DELETE(m_file);
}

bool ResCache::Init()
{
	bool retValue = false;
	if (m_file->VOpen())
	{
		RegisterLoader(std::shared_ptr<IResourceLoader>(GCC_NEW DefalultResourceLoader()));
		retValue = true;
	}
	return retValue;
}

void ResCache::RegisterLoader(std::shared_ptr<IResourceLoader> loader)
{
}

std::shared_ptr<ResHandle> ResCache::GetHandle(Resource * r)
{
	std::shared_ptr<ResHandle> handle(Find(r));
	if (handle == NULL)
		handle = Load(r);
	else
		Update(handle);

	return handle;
}

std::shared_ptr<ResHandle> ResCache::Load(Resource * r)
{
	std::shared_ptr<IResourceLoader> loader;
	std::shared_ptr<ResHandle> handle;

	for (ResourceLoaders::iterator it = m_resourceLoaders.begin();
		it != m_resourceLoaders.end(); ++it)
	{
		std::shared_ptr<IResourceLoader> testLoader = *it;
		if (WildcardMatch(testLoader->VGetPattern().c_str(), r->m_name.c_str()))
		{
			loader = testLoader;
			break;
		}
	}
	if (!loader)
	{
		assert(loader & _T("Default resource loader not found!"));
		return handle;	//Resource not loaded!
	}

	unsigned int rawSize = m_file->VGetRawResourceSize(*r);
	char *rawBuffer = loader->VUseRawFile() ? Allocate(rawSize) : GCC_NEW char[rawSize];

	if (rawBuffer == NULL)
	{
		// resource cache out of memory
		return std::shared_ptr<ResHandle>();
	}
	m_file->VGetRawResource(*r, rawBuffer);
	char * buffer = NULL;
	unsigned int size = 0;

	if (loader->VUseRawFile())
	{
		buffer = rawBuffer;
		handle = std::shared_ptr<ResHandle>(
			GCC_NEW ResHandle(*r, buffer, rawSize, this));
	}
	else
	{
		size = loader->VGetLoadedResourceSize(rawBuffer, rawSize);
		buffer = Allocate(size);
		if (rawBuffer == NULL || buffer == NULL)
		{
			//resource cache out of memory
			return std::shared_ptr<ResHandle>();
		}
		handle = std::shared_ptr<ResHandle>(
			GCC_NEW ResHandle(*r, buffer, size, this));
		bool success = loader->VLoadResource(rawBuffer, rawSize, handle);
		SAFE_DELETE_ARRAY(rawBuffer);

		if (!success)
		{
			//resource cache out of memory
			return std::shared_ptr<ResHandle>();
		}
	}

	if (handle)
	{
		m_lru.push_front(handle);
		m_resources[r->m_name] = handle;
	}

	assert(loader && _T("Default resource loader not found!"));
	return handle;	//ResCache is out of memory!
}

void ResCache::Free(std::shared_ptr<ResHandle> gonner)
{
}

char *ResCache::Allocate(unsigned int size)
{
	if (!MakeRoom(size))
		return NULL;

	char *mem = GCC_NEW char[size];
	if (mem)
		m_allocated += size;

	return mem;
}

bool ResCache::MakeRoom(unsigned int size)
{
	if (size > m_cacheSize)
	{
		return false;
	}
	// return null if there's no possible way to allocate the memory
	while (size > (m_cacheSize - m_allocated))
	{
		//The cache is empty, and there's still not enough room.
		if (m_lru.empty())
			return false;

		FreeOneResource();
	}

	return true;
}

void ResCache::FreeOneResource()
{
	ResHandleList::iterator gonner = m_lru.end();
	gonner--;

	std::shared_ptr<ResHandle> handle = *gonner;

	m_lru.pop_back();
	m_resources.erase(handle->m_resource.m_name);
}

void ResCache::MemoryHasBeenFreed(unsigned int size)
{
}


//»ç¿ë ¿¹
/*
	ResourceZipFile zipFile("Assets.zip");
	ResCache resCache(50, zipFile);
	if(m_ResCache.Init())
	{
		Resource resource("art\\brict.bmp");
		std::shared_ptr<ResHandle> texture =  g_pApp->m_ResCache->GetHandle(&resource);
		int size = texture->GetSize();
		char *brickBitmap = (char*) texture->Buffer();
		//do something cool with brickBitmap!

		
*/

//Caching Resources into DirectX et al.
/*
	Resource resource(m_params.m_Texture);
	std::shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
	if( FAILED(
		D3DXCreateTextureFromFileInMemory(
			DXUTGetD3D9Device(),
			texture->Buffer(),
			texture->Size(),
			&m_pTexture)))
	{
		return E_FAIL;
	}
*/

int ResCache::Preload(const std::string pattern, void(*progressCallback)(int, bool &))
{
	if (m_file == NULL)
		return 0;

	int numFiles = m_file->VGetNumResources();
	int loaded = 0;
	bool cancel = false;
	for (int i = 0; i < numFiles; ++i)
	{
		Resource resource(m_file->VGetResourceName(i));

		if (WildcardMatch(pattern.c_str(), resource.m_name.c_str()))
		{
			std::shared_ptr<ResHandle> handle =
				g_pApp->m_ResCache->GetHandle(&resource);
			++loaded;
		}

		if (progressCallback != NULL)
		{
			progressCallback(i * 100 / numFiles, cancel);
		}
	}
	return loaded;
}

void ResCache::Flush(void)
{
}
