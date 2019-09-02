#include "ResourceCache.h"

ResourceHandle::ResourceHandle(std::string r, char * buffer, unsigned int size, ResourceCache * pResourceCache)
	: resource_(r)
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
		RegisterLoader(std::shared_ptr<IResourceLoader>(new IResourceLoader())); //기본 리소스 로더를 생성하여 멤버변수 resourceLoaders_에 추가.
		retValue = true;
	}
	return retValue;
}

void ResourceCache::RegisterLoader(std::shared_ptr<IResourceLoader> loader)
{
	resourceLoaders_.push_front(loader);
}

std::shared_ptr<ResourceHandle> ResourceCache::GetHandle(std::string r)
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

std::shared_ptr<ResourceHandle> ResourceCache::Load(std::string r)
{
	// Create a new Resource and add it to the lru list and map

	std::shared_ptr<IResourceLoader> loader;
	std::shared_ptr<ResourceHandle> handle;

	loader = *resourceLoaders_.begin();

	if (!loader)
	{
		assert(loader && (L"Default Resource loader not found!"));
		return handle;		// Resource not loaded!
	}

	int rawSize = file_->VGetRawResourceSize(r);
	if (rawSize < 0)
	{
		assert(rawSize > 0 && "Resource size returned -1 - Resource not found");
		return std::shared_ptr<ResourceHandle>();
	}

	int allocSize = rawSize;
	char *rawBuffer = Allocate(allocSize);
	memset(rawBuffer, 0, allocSize);

	if (rawBuffer == NULL || file_->VGetRawResource(r, rawBuffer) == 0)
	{
		// Resource cache out of memory
		return std::shared_ptr<ResourceHandle>();
	}

	char *buffer = NULL;
	unsigned int size = 0;

	buffer = rawBuffer;
	handle = std::shared_ptr<ResourceHandle>(new ResourceHandle(r, buffer, rawSize, this));

	if (handle)
	{
		lru_.push_front(handle);
		resources_[r] = handle;
	}

	assert(loader && (L"Default Resource loader not found!"));
	return handle;		// ResourceCache is out of memory!
}

std::shared_ptr<ResourceHandle> ResourceCache::Find(std::string r)
{
	ResourceHandleMap::iterator i = resources_.find(r); // 같은것이 있는지 찾는다.
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
	resources_.erase(handle->resource_);
}

// Flush - 캐시의 모든 핸들을 해제
void ResourceCache::Flush()
{
	while (!lru_.empty())
	{
		std::shared_ptr<ResourceHandle> handle = *(lru_.begin());
		Free(handle);
		lru_.pop_front();
	}
}

bool ResourceCache::MakeRoom(unsigned int size)
{
	if (size > cacheSize_)
	{
		return false;
	}

	// return null if there's no possible way to allocate the memory - 메모리를 할당 할 수있는 방법이 없으면 null을 반환합니다.
	while (size > (cacheSize_ - allocated_))
	{
		// The cache is empty, and there's still not enough room. - 캐시가 비어 있고 여전히 충분한 공간이 없습니다.
		if (lru_.empty())
			return false;

		FreeOneResource();
	}

	return true;
}

void ResourceCache::Free(std::shared_ptr<ResourceHandle> gonner)
{
	lru_.remove(gonner);
	resources_.erase(gonner->resource_);
}

void ResourceCache::MemoryHasBeenFreed(unsigned int size)
{
	allocated_ -= size;
}