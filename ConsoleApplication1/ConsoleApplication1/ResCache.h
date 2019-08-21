#pragma once
#include "Resource.h"
#include "Logger.h"

class IResourceExtraData
{
public:
	virtual std::string VToString() = 0;
};

class ResCache
{
	friend class ResHandle;

	ResHandleList m_lru;							// lru list
	ResHandleMap m_resources;
	ResourceLoaders m_resourceLoaders;

	IResourceFile *m_file;

	unsigned int			m_cacheSize;			// total memory size
	unsigned int			m_allocated;			// total memory allocated

protected:

	bool MakeRoom(unsigned int size);
	char *Allocate(unsigned int size);
	void Free(shared_ptr<ResHandle> gonner);

	shared_ptr<ResHandle> Load(Resource * r);
	shared_ptr<ResHandle> Find(Resource * r);
	void Update(shared_ptr<ResHandle> handle);

	void FreeOneResource();
	void MemoryHasBeenFreed(unsigned int size);

public:
	ResCache(const unsigned int sizeInMb, IResourceFile *file);
	virtual ~ResCache();

	bool Init();

	void RegisterLoader(shared_ptr<IResourceLoader> loader);

	shared_ptr<ResHandle> GetHandle(Resource * r);

	int Preload(const std::string pattern, void(*progressCallback)(int, bool &));
	std::vector<std::string> Match(const std::string pattern);

	void Flush(void);

	bool IsUsingDevelopmentDirectories(void) const { GCC_ASSERT(m_file); return m_file->VIsUsingDevelopmentDirectories(); }

};