#include "ResourceCache.h"

bool WildcardMatch(const char *pat, const char *str) {
	int i, star;

new_segment:

	star = 0;
	if (*pat == '*') {
		star = 1;
		do { pat++; } while (*pat == '*'); /* enddo */
	} /* endif */

test_match:

	for (i = 0; pat[i] && (pat[i] != '*'); i++) {
		//if (mapCaseTable[str[i]] != mapCaseTable[pat[i]]) {
		if (str[i] != pat[i]) {
			if (!str[i]) return 0;
			if ((pat[i] == '?') && (str[i] != '.')) continue;
			if (!star) return 0;
			str++;
			goto test_match;
		}
	}
	if (pat[i] == '*') {
		str += i;
		pat += i;
		goto new_segment;
	}
	if (!str[i]) return 1;
	if (i && pat[i - 1] == '*') return 1;
	if (!star) return 0;
	str++;
	goto test_match;
}

ResourceHandle::ResourceHandle(Resource & resource, char * buffer, unsigned int size, ResourceCache * pResourceCache)
	: resource_(resource)
{
	buffer_ = buffer;
	size_ = size;
	extra_ = NULL;
	resourceCache_ = pResourceCache;
}

ResourceHandle::~ResourceHandle() 
{
	SAFE_DELETE_ARRAY(buffer_);
	resourceCache_->MemoryHasBeenFreed(size_);
}

unsigned int ResourceHandle::GetSize() const
{
	return size_;
}

char * ResourceHandle::GetBuffer() const
{
	return buffer_;
}

char * ResourceHandle::WritableBuffer()
{
	return buffer_;
}

std::shared_ptr<IResourceExtraData> ResourceHandle::GetExtra()
{
	return extra_;
}

void ResourceHandle::SetExtra(std::shared_ptr<IResourceExtraData> extra)
{
	extra_ = extra;
}

ResourceCache::ResourceCache(const unsigned int sizeInMb, IResourceFile *ResourceFile)
{
	cacheSize_ = sizeInMb * 1024 * 1024;	// total memory size
	allocated_ = 0;	// total memory allocated
	file_ = ResourceFile;
}

ResourceCache::~ResourceCache()
{
	while (!lru_.empty())
	{
		FreeOneResource();
	}
	SAFE_DELETE(file_);
}

bool ResourceCache::Init()
{
	bool retValue = false;
	if (file_->VOpen())
	{
		RegisterLoader(std::shared_ptr<IResourceLoader>(new DefalultResourceLoader())); //기본 리소스 로더를 생성하여 멤버변수 resourceLoaders_에 추가.
		retValue = true;
	}
	return retValue;
}

void ResourceCache::RegisterLoader(std::shared_ptr<IResourceLoader> loader)
{
	resourceLoaders_.push_front(loader);
}

std::shared_ptr<ResourceHandle> ResourceCache::GetHandle(Resource * r)
{
	std::shared_ptr<ResourceHandle> handle(Find(r)); // 핸들에 같은 리소스가 있는지 확인 하고 있으면 있는것으로 핸들이 채워지고 없으면 새로 만들어진 null핸들이 채워진다.
	if (handle == NULL) // 찾는 리소스가 없으면
	{
		handle = Load(r);
		assert(handle);
	}
	else
	{
		Update(handle);
	}
	return handle;
}

std::shared_ptr<ResourceHandle> ResourceCache::Load(Resource * r)
{
	// Create a new Resource and add it to the lru list and map

	std::shared_ptr<IResourceLoader> loader;
	std::shared_ptr<ResourceHandle> handle;

	for (ResourceLoaders::iterator it = resourceLoaders_.begin(); it != resourceLoaders_.end(); ++it)
	{
		std::shared_ptr<IResourceLoader> testLoader = *it;

		if (WildcardMatch(testLoader->VGetPattern().c_str(), r->m_name.c_str())) // c_str() -> 반환형이 char* string의 첫번째 문자 주소값을 반환
		{
			loader = testLoader;
			break;
		}
	}

	if (!loader)
	{
		assert(loader && (L"Default Resource loader not found!"));
		return handle;		// Resource not loaded!
	}

	int rawSize = file_->VGetRawResourceSize(*r);
	if (rawSize < 0)
	{
		assert(rawSize > 0 && "Resource size returned -1 - Resource not found");
		return std::shared_ptr<ResourceHandle>();
	}

	int allocSize = rawSize + ((loader->VAddNullZero()) ? (1) : (0)); // ???
	char *rawBuffer = loader->VUseRawFile() ? Allocate(allocSize) : new char[allocSize]; // ???
	memset(rawBuffer, 0, allocSize);

	if (rawBuffer == NULL || file_->VGetRawResource(*r, rawBuffer) == 0)
	{
		// Resource cache out of memory
		return std::shared_ptr<ResourceHandle>();
	}

	char *buffer = NULL;
	unsigned int size = 0;

	if (loader->VUseRawFile())
	{
		buffer = rawBuffer;
		handle = std::shared_ptr<ResourceHandle>(new ResourceHandle(*r, buffer, rawSize, this));
	}
	else
	{
		size = loader->VGetLoadedResourceSize(rawBuffer, rawSize);
		buffer = Allocate(size);
		if (rawBuffer == NULL || buffer == NULL)
		{
			// Resource cache out of memory
			return std::shared_ptr<ResourceHandle>();
		}
		handle = std::shared_ptr<ResourceHandle>(new ResourceHandle(*r, buffer, size, this));
		bool success = loader->VLoadResource(rawBuffer, rawSize, handle);

		// [mrmike] - This was added after the chapter went to copy edit. It is used for those
		//            Resourceoruces that are converted to a useable format upon load, such as a compResourcesed
		//            file. If the raw buffer from the Resource file isn't needed, it shouldn't take up
		//            any additional memory, so we release it.
		//
		if (loader->VDiscardRawBufferAfterLoad())
		{
			SAFE_DELETE_ARRAY(rawBuffer);
		}

		if (!success)
		{
			// Resource cache out of memory
			return std::shared_ptr<ResourceHandle>();
		}
	}

	if (handle)
	{
		lru_.push_front(handle);
		resources_[r->m_name] = handle;
	}

	assert(loader && (L"Default Resource loader not found!"));
	return handle;		// ResourceCache is out of memory!
}

std::shared_ptr<ResourceHandle> ResourceCache::Find(Resource * r)
{
	ResourceHandleMap::iterator i = resources_.find(r->m_name); // 같은것이 있는지 찾는다.
	if (i == resources_.end()) // 끝까지 찾아봤는데 없음.
		return std::shared_ptr<ResourceHandle>();

	return i->second; //있음
}

//
// ResourceCache::Update									- Chapter 8, page 228
//
void ResourceCache::Update(std::shared_ptr<ResourceHandle> handle)
{
	lru_.remove(handle);
	lru_.push_front(handle);
}

char *ResourceCache::Allocate(unsigned int size)
{
	if (!MakeRoom(size))
		return NULL;

	char *mem = new char[size];
	if (mem)
		allocated_ += size;

	return mem;
}

void ResourceCache::FreeOneResource()
{
	ResourceHandleList::iterator gonner = lru_.end();
	gonner--;

	std::shared_ptr<ResourceHandle> handle = *gonner;

	lru_.pop_back();
	resources_.erase(handle->resource_.m_name);
}

//
// ResourceCache::Flush									- not described in the book
//
//    Frees every handle in the cache - this would be good to call if you are loading a new
//    level, or if you wanted to force a refResourceh of all the data in the cache - which might be 
//    good in a development environment.
//
void ResourceCache::Flush()
{
	while (!lru_.empty())
	{
		std::shared_ptr<ResourceHandle> handle = *(lru_.begin());
		Free(handle);
		lru_.pop_front();
	}
}

bool ResourceCache::IsUsingDevelopmentDirectories(void) const
{
	assert(file_); return file_->VIsUsingDevelopmentDirectories();
}

bool ResourceCache::MakeRoom(unsigned int size)
{
	if (size > cacheSize_)
	{
		return false;
	}

	// return null if there's no possible way to allocate the memory
	while (size > (cacheSize_ - allocated_))
	{
		// The cache is empty, and there's still not enough room.
		if (lru_.empty())
			return false;

		FreeOneResource();
	}

	return true;
}

void ResourceCache::Free(std::shared_ptr<ResourceHandle> gonner)
{
	lru_.remove(gonner);
	resources_.erase(gonner->resource_.m_name);
	// Note - the Resource might still be in use by something,
	// so the cache can't actually count the memory freed until the
	// ResourceHandle pointing to it is destroyed.

	//m_allocated -= gonner->m_Resource.m_size;
	//delete gonner;
}

void ResourceCache::MemoryHasBeenFreed(unsigned int size)
{
	allocated_ -= size;
}

std::vector<std::string> ResourceCache::Match(const std::string pattern)
{
	std::vector<std::string> matchingNames;
	if (file_ == NULL)
		return matchingNames;

	int numFiles = file_->VGetNumResources();
	for (int i = 0; i < numFiles; ++i)
	{
		std::string name = file_->VGetResourceName(i);
		std::transform(name.begin(), name.end(), name.begin(), (int(*)(int)) std::tolower);
		if (WildcardMatch(pattern.c_str(), name.c_str()))
		{
			matchingNames.push_back(name);
		}
	}
	return matchingNames;
}

int ResourceCache::Preload(const std::string pattern, void(*progResourcesCallback)(int, bool &))
{
	if (file_ == NULL)
		return 0;

	int numFiles = file_->VGetNumResources();
	int loaded = 0;
	bool cancel = false;
	for (int i = 0; i < numFiles; ++i)
	{
		Resource Resource(file_->VGetResourceName(i));

		if (WildcardMatch(pattern.c_str(), Resource.m_name.c_str()))
		{
			// >> g_pApp Game에 필요한 정의를 한 클래스 같음.
			/*std::shared_ptr<ResourceHandle> handle =
				g_pApp->m_ResourceCache->GetHandle(&Resource);
			++loaded;*/
			// << 
		}

		if (progResourcesCallback != NULL)
		{
			progResourcesCallback(i * 100 / numFiles, cancel);
		}
	}
	return loaded;
}