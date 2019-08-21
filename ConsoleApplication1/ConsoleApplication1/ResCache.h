#pragma once
#include "Resource.h"

class IResourceExtraData
{
public:
	virtual std::string VToString() = 0;
};

class ResCache
{
protected:
	ResHandleList m_lru;	// LRU (least recently used) list
	ResHandleMap m_resources;	// STL map for fast resource lookup
	ResourceLoaders m_resourceLoaders;

	IResourceFile *m_file;	// Object that implements IResourceFile

	unsigned int m_cacheSize;	//total memory size
	unsigned int m_allocated;	//total memory allocated

	std::shared_ptr<ResHandle> Find(Resource * r);
	const void *Update(std::shared_ptr<ResHandle> handle);
	std::shared_ptr<ResHandle> Load(Resource * r);
	void Free(std::shared_ptr<ResHandle>gonner);

	bool MakeRoom(unsigned int size);
	char * Allocate(unsigned int size);
	void FreeOneResource();
	void MemoryHasBeenFreed(unsigned int size);

public:
	ResCache(const unsigned int sizeInMb, IResourceFile * resFile);
	~ResCache();
	bool Init();
	void RegisterLoader(std::shared_ptr<IResourceLoader> loader);

	std::shared_ptr<ResHandle> GetHandle(Resource * r);
	int Preload(const std::string pattern, void(*progressCallback)(int, bool&));
	void Flush(void);
};