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
		RegisterLoader(std::shared_ptr<IResourceLoader>(new IResourceLoader())); //�⺻ ���ҽ� �δ��� �����Ͽ� ������� resourceLoaders_�� �߰�.
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
	std::shared_ptr<ResourceHandle> handle(Find(r)); // �ڵ鿡 ���� ���ҽ��� �ִ��� Ȯ�� �ϰ� ������ �ִ°����� �ڵ��� ä������ ������ ���� ������� null�ڵ��� ä������.
	if (handle == NULL) // ã�� ���ҽ��� ������
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
	ResourceHandleMap::iterator i = resources_.find(r); // �������� �ִ��� ã�´�.
	if (i == resources_.end()) // ������ ã�ƺôµ� ����.
		return std::shared_ptr<ResourceHandle>();

	return i->second; //����
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

// Flush - ĳ���� ��� �ڵ��� ����
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

	// return null if there's no possible way to allocate the memory - �޸𸮸� �Ҵ� �� ���ִ� ����� ������ null�� ��ȯ�մϴ�.
	while (size > (cacheSize_ - allocated_))
	{
		// The cache is empty, and there's still not enough room. - ĳ�ð� ��� �ְ� ������ ����� ������ �����ϴ�.
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