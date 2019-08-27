#pragma once
#include "Resource.h"
#include <list>
#include <assert.h>
#include <string>
#define SAFE_DELETE_ARRAY(x) if(x) delete[] x; x = NULL;


class ResourceCache;

class ResourceHandle
{
// ¼±¾ð
private:
	friend class ResourceCache;

//¸â¹ö ÇÔ¼ö
public:
	ResourceHandle(Resource & Resource, char * buffer, unsigned int size, ResourceCache * pResourceCache);
	virtual ~ResourceHandle();
	unsigned int GetSize() const;
	char *GetBuffer() const;
	char *WritableBuffer();
	std::shared_ptr<IResourceExtraData>GetExtra();
	void SetExtra(std::shared_ptr<IResourceExtraData> extra);

//¸â¹ö º¯¼ö
protected:
	Resource resource_;
	char * buffer_;
	unsigned int size_;
	std::shared_ptr<IResourceExtraData> extra_;
	ResourceCache *resourceCache_;
};

typedef std::list<std::shared_ptr <ResourceHandle>>ResourceHandleList;
typedef std::map<std::string, std::shared_ptr<ResourceHandle>>ResourceHandleMap;
typedef std::list<std::shared_ptr<IResourceLoader>>ResourceLoaders;

class ResourceCache
{
//¼±¾ð
private:
	friend class ResourceHandle;

//¸â¹ö ÇÔ¼ö
public:
	ResourceCache(const unsigned int sizeInMb, IResourceFile *file);
	virtual ~ResourceCache();
	bool Init();
	void RegisterLoader(std::shared_ptr<IResourceLoader> loader);
	std::shared_ptr<ResourceHandle> GetHandle(Resource * r);
	int Preload(const std::string pattern, void(*progResourcesCallback)(int, bool &));
	std::vector<std::string> Match(const std::string pattern);
	void Flush(void);
	bool IsUsingDevelopmentDirectories(void) const;
protected:
	bool MakeRoom(unsigned int size);
	char *Allocate(unsigned int size);
	void Free(std::shared_ptr<ResourceHandle> gonner);
	std::shared_ptr<ResourceHandle> Load(Resource * r);
	std::shared_ptr<ResourceHandle> Find(Resource * r);
	void Update(std::shared_ptr<ResourceHandle> handle);
	void FreeOneResource();
	void MemoryHasBeenFreed(unsigned int size);

//¸â¹ö º¯¼ö
private:
	ResourceHandleList lru_;							// lru list
	ResourceHandleMap resources_;
	ResourceLoaders resourceLoaders_;
	IResourceFile * file_;
	unsigned int cacheSize_;			// total memory size
	unsigned int allocated_;			// total memory allocated
};