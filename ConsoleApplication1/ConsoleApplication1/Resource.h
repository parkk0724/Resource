#pragma once
#include <vector>
#include <algorithm>
#include <windows.h>
#include <memory.h>
#include <cctype>
#include <fstream>
#include <map>

#define SAFE_DELETE(x) if(x) delete x; x=NULL;
typedef std::map<std::string, int> ZipContentsMap;

class ResHandle;
class ZipFile;

class IResourceExtraData
{
public:
	virtual std::string VToString() = 0;
};

class Resource
{s
public:
	std::string m_name;
	Resource(const std::string & name);
};

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

class IResourceLoader
{
public:
	virtual std::string VGetPattern() = 0;
	virtual bool VUseRawFile() = 0;
	virtual bool VDiscardRawBufferAfterLoad() = 0;
	virtual bool VAddNullZero() { return false; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) = 0;
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, std::shared_ptr<ResHandle> handle) = 0;
};

class DefalultResourceLoader : public IResourceLoader
{
public:
	virtual bool VUseRawFile() { return true; }
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) { return rawSize; }
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, std::shared_ptr<ResHandle> handle) { return true; }
	virtual std::string VGetPattern() { return "*"; }
};



class ResourceZipFile : public IResourceFile
{
	ZipFile* m_pZipFile;
	std::wstring m_resFileName;

public:
	ResourceZipFile(const std::wstring resFileName) { m_pZipFile = NULL; m_resFileName = resFileName; }
	virtual ~ResourceZipFile();

	virtual bool VOpen();
	virtual int VGetRawResourceSize(const Resource &r);
	virtual int VGetRawResource(const Resource &r, char *buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;
	virtual bool VIsUsingDevelopmentDirectories(void) const { return false; }
};

class DevelopmentResourceZipFile : public ResourceZipFile
{
public:
	enum Mode
	{
		Development,	// this mode checks the original asset directory for changes - helps during development
		Editor			// this mode only checks the original asset directory - the ZIP file is left unopened.
	};

	Mode m_mode;
	std::wstring m_AssetsDir;
	std::vector<WIN32_FIND_DATA> m_AssetFileInfo;
	ZipContentsMap m_DirectoryContentsMap;

	DevelopmentResourceZipFile(const std::wstring resFileName, const Mode mode);

	virtual bool VOpen();
	virtual int VGetRawResourceSize(const Resource &r);
	virtual int VGetRawResource(const Resource &r, char *buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;
	virtual bool VIsUsingDevelopmentDirectories(void) const { return true; }

	int Find(const std::string &path);

protected:
	void ReadAssetsDirectory(std::wstring fileSpec);
};

//리소스 사용 방법의 예
/*
	Resource resource ("Brick.bmp");
	shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
	int size = texture->GetSize();
	char *brickBitmap = (char*) texture->Buffer();
*/