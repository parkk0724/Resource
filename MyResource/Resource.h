#pragma once
#include <vector>
#include <algorithm>
#include <windows.h>
#include <memory.h>
#include <cctype>
#include <fstream>
#include <map>
#include <string>

#define SAFE_DELETE(x) if(x) delete x; x=NULL;
typedef std::map<std::string, int> ZipContentsMap;

class ResourceHandle;
class ZipFile;

class IResourceExtraData
{
	// ¸É¹ö ÇÔ¼ö
public:
	virtual std::string VToString() = 0;
};

class IResourceFile
{
	// ¸â¹ö ÇÔ¼ö
public:
	virtual bool VOpen() = 0;
	virtual int VGetRawResourceSize(const std::string r) = 0;
	virtual int VGetRawResource(const std::string r, char *buffer) = 0;
	virtual int VGetNumResources() const = 0;
	virtual std::string VGetResourceName(int num) const = 0;
	virtual ~IResourceFile() { }
};

class IResourceLoader
{
	// ¸â¹ö ÇÔ¼ö
public:
	virtual bool VUseRawFile() { return true; }
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual bool VAddNullZero() { return false; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) { return rawSize; }
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, std::shared_ptr<ResourceHandle> handle) { return true; }
};

class ResourceZipFile : public IResourceFile
{
	// ¸â¹ö ÇÔ¼ö
public:
	ResourceZipFile(const std::wstring ResourceFileName) { zipFile_ = NULL; resourceFileName_ = ResourceFileName; }
	virtual ~ResourceZipFile();
	virtual bool VOpen();
	virtual int VGetRawResourceSize(const std::string r);
	virtual int VGetRawResource(const std::string r, char *buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;

	// ¸â¹ö º¯¼ö
private:
	ZipFile* zipFile_;
	std::wstring resourceFileName_;
};