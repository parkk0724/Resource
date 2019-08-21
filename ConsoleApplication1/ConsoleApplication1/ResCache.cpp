#include "pch.h"
#include "ResCache.h"
#include "String.h"

typedef std::list<std::shared_ptr <ResHandle>>ResHandleList;
typedef std::map<std::string, std::shared_ptr<ResHandle>>ResHandleMap;
typedef std::list<std::shared_ptr<IResourceLoader>>ResourceLoaders;

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
	m_resourceLoaders.push_front(loader);
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
	// Create a new resource and add it to the lru list and map

	shared_ptr<IResourceLoader> loader;
	shared_ptr<ResHandle> handle;

	for (ResourceLoaders::iterator it = m_resourceLoaders.begin(); it != m_resourceLoaders.end(); ++it)
	{
		shared_ptr<IResourceLoader> testLoader = *it;

		if (WildcardMatch(testLoader->VGetPattern().c_str(), r->m_name.c_str()))
		{
			loader = testLoader;
			break;
		}
	}

	if (!loader)
	{
		GCC_ASSERT(loader && _T("Default resource loader not found!"));
		return handle;		// Resource not loaded!
	}

	int rawSize = m_file->VGetRawResourceSize(*r);
	if (rawSize < 0)
	{
		GCC_ASSERT(rawSize > 0 && "Resource size returned -1 - Resource not found");
		return shared_ptr<ResHandle>();
	}

	int allocSize = rawSize + ((loader->VAddNullZero()) ? (1) : (0));
	char *rawBuffer = loader->VUseRawFile() ? Allocate(allocSize) : GCC_NEW char[allocSize];
	memset(rawBuffer, 0, allocSize);

	if (rawBuffer == NULL || m_file->VGetRawResource(*r, rawBuffer) == 0)
	{
		// resource cache out of memory
		return shared_ptr<ResHandle>();
	}

	char *buffer = NULL;
	unsigned int size = 0;

	if (loader->VUseRawFile())
	{
		buffer = rawBuffer;
		handle = shared_ptr<ResHandle>(GCC_NEW ResHandle(*r, buffer, rawSize, this));
	}
	else
	{
		size = loader->VGetLoadedResourceSize(rawBuffer, rawSize);
		buffer = Allocate(size);
		if (rawBuffer == NULL || buffer == NULL)
		{
			// resource cache out of memory
			return shared_ptr<ResHandle>();
		}
		handle = shared_ptr<ResHandle>(GCC_NEW ResHandle(*r, buffer, size, this));
		bool success = loader->VLoadResource(rawBuffer, rawSize, handle);

		// [mrmike] - This was added after the chapter went to copy edit. It is used for those
		//            resoruces that are converted to a useable format upon load, such as a compressed
		//            file. If the raw buffer from the resource file isn't needed, it shouldn't take up
		//            any additional memory, so we release it.
		//
		if (loader->VDiscardRawBufferAfterLoad())
		{
			SAFE_DELETE_ARRAY(rawBuffer);
		}

		if (!success)
		{
			// resource cache out of memory
			return shared_ptr<ResHandle>();
		}
	}

	if (handle)
	{
		m_lru.push_front(handle);
		m_resources[r->m_name] = handle;
	}

	GCC_ASSERT(loader && _T("Default resource loader not found!"));
	return handle;		// ResCache is out of memory!
}

shared_ptr<ResHandle> ResCache::Find(Resource * r)
{
	ResHandleMap::iterator i = m_resources.find(r->m_name);
	if (i == m_resources.end())
		return shared_ptr<ResHandle>();

	return i->second;
}

//
// ResCache::Update									- Chapter 8, page 228
//
void ResCache::Update(shared_ptr<ResHandle> handle)
{
	m_lru.remove(handle);
	m_lru.push_front(handle);
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

void ResCache::FreeOneResource()
{
	ResHandleList::iterator gonner = m_lru.end();
	gonner--;

	std::shared_ptr<ResHandle> handle = *gonner;

	m_lru.pop_back();
	m_resources.erase(handle->m_resource.m_name);
}

//
// ResCache::Flush									- not described in the book
//
//    Frees every handle in the cache - this would be good to call if you are loading a new
//    level, or if you wanted to force a refresh of all the data in the cache - which might be 
//    good in a development environment.
//
void ResCache::Flush()
{
	while (!m_lru.empty())
	{
		shared_ptr<ResHandle> handle = *(m_lru.begin());
		Free(handle);
		m_lru.pop_front();
	}
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
		// The cache is empty, and there's still not enough room.
		if (m_lru.empty())
			return false;

		FreeOneResource();
	}

	return true;
}

void ResCache::Free(shared_ptr<ResHandle> gonner)
{
	m_lru.remove(gonner);
	m_resources.erase(gonner->m_resource.m_name);
	// Note - the resource might still be in use by something,
	// so the cache can't actually count the memory freed until the
	// ResHandle pointing to it is destroyed.

	//m_allocated -= gonner->m_resource.m_size;
	//delete gonner;
}

void ResCache::MemoryHasBeenFreed(unsigned int size)
{
	m_allocated -= size;
}

std::vector<std::string> ResCache::Match(const std::string pattern)
{
	std::vector<std::string> matchingNames;
	if (m_file == NULL)
		return matchingNames;

	int numFiles = m_file->VGetNumResources();
	for (int i = 0; i < numFiles; ++i)
	{
		std::string name = m_file->VGetResourceName(i);
		std::transform(name.begin(), name.end(), name.begin(), (int(*)(int)) std::tolower);
		if (WildcardMatch(pattern.c_str(), name.c_str()))
		{
			matchingNames.push_back(name);
		}
	}
	return matchingNames;
}

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
			// >> g_pApp Game에 필요한 정의를 한 클래스 같음.
			/*std::shared_ptr<ResHandle> handle =
				g_pApp->m_ResCache->GetHandle(&resource);
			++loaded;*/
			// << 
		}

		if (progressCallback != NULL)
		{
			progressCallback(i * 100 / numFiles, cancel);
		}
	}
	return loaded;
}
