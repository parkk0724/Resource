#pragma once
#include "ResCache.h"
#include <cctype>

class Resource
{
public:
	std::string m_name;
	Resource(const std::string & name)
	{
		m_name = name;
		std::transform(m_name.begin(), m_name.end(), m_name.begin(), (int(*)(int))std::tolower);
	}
};

//리소스 사용 방법의 예
/*
	Resource resource ("Brick.bmp");
	shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
	int size = texture->GetSize();
	char *brickBitmap = (char*) texture->Buffer();
*/


class IResourceFile
{
public:
	virtual bool VOpen() = 0;
	virtual int VGetRawResourceSize(const Resource &r) = 0;
	virtual int VGetRawResource(const Resource &r, char *buffer) = 0;
	virtual int VGetNumResources() const = 0;
	virtual std::string VGetResourceName(int num) const = 0;
	virtual bool VIsUsingDevelopmentDirectories(void) const = 0;
	virtual ~IResourceFile() { }
};

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

class IResourceLoader
{
public:
	virtual std::string VGetPattern() = 0;
	virtual bool VUseRawFile() = 0;
	virtual bool VDiscardRawBufferAfterLoad() = 0;
	virtual bool VAddNullZero() { return false; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) = 0;
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle) = 0;
};

class DefalultResourceLoader : public IResourceLoader
{
public:
	virtual bool VUseRawFile() { return true; }
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) { return rawSize; }
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, shared_ptr<ResHandle> handle) { return true; }
	virtual std::string VGetPattern() { return "*"; }
};
