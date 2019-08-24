#pragma once
#include <list>
#include <map>
#include <memory>
#include <assert.h>
#include <cctype>
#include <vector>
#include <string>
#include <algorithm>
#define SAFE_DELETE_ARRAY(x) if(x) delete[] x; x = NULL;


class Resource;
class IResourceExtraData;
class IResourceLoader;
class IResourceFile;
class ResCache;

class ResHandle
{
	friend class ResCache;

protected:
	Resource m_resource;
	char * m_buffer;
	unsigned int m_size;
	std::shared_ptr<IResourceExtraData> m_extra;
	ResCache *m_pResCache;

public:
	ResHandle(Resource & resource, char * buffer, unsigned int size, ResCache * pResCache);
	virtual ~ResHandle();

	unsigned int Size() const { return m_size; }
	char *buffer() const { return m_buffer; }
	char *WritableBuffer() { return m_buffer; }
	std::shared_ptr<IResourceExtraData>GetExtra() { return m_extra; }
	void SetExtra(std::shared_ptr<IResourceExtraData> extra) { m_extra = extra; }
};

typedef std::list<std::shared_ptr <ResHandle>>ResHandleList;
typedef std::map<std::string, std::shared_ptr<ResHandle>>ResHandleMap;
typedef std::list<std::shared_ptr<IResourceLoader>>ResourceLoaders;

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
	void Free(std::shared_ptr<ResHandle> gonner);

	std::shared_ptr<ResHandle> Load(Resource * r);
	std::shared_ptr<ResHandle> Find(Resource * r);
	void Update(std::shared_ptr<ResHandle> handle);

	void FreeOneResource();
	void MemoryHasBeenFreed(unsigned int size);

public:
	ResCache(const unsigned int sizeInMb, IResourceFile *file);
	virtual ~ResCache();

	bool Init();

	void RegisterLoader(std::shared_ptr<IResourceLoader> loader);

	std::shared_ptr<ResHandle> GetHandle(Resource * r);

	int Preload(const std::string pattern, void(*progressCallback)(int, bool &));
	std::vector<std::string> Match(const std::string pattern);

	void Flush(void);

	bool IsUsingDevelopmentDirectories(void) const;
};