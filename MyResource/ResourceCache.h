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
	ResourceHandle(std::string r, char * buffer, unsigned int size, ResourceCache * pResourceCache);
	virtual ~ResourceHandle();
	unsigned int GetSize() const;
	char *GetBuffer() const;
	std::shared_ptr<IResourceExtraData>GetExtra();
	void SetExtra(std::shared_ptr<IResourceExtraData> extra);

//¸â¹ö º¯¼ö
protected:
	std::string resource_;
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
	std::shared_ptr<ResourceHandle> GetHandle(std::string r);
	void Flush(void);
protected:
	bool MakeRoom(unsigned int size);
	char *Allocate(unsigned int size);
	void Free(std::shared_ptr<ResourceHandle> gonner);
	std::shared_ptr<ResourceHandle> Load(std::string r);
	std::shared_ptr<ResourceHandle> Find(std::string r);
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