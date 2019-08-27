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
	// �ɹ� �Լ�
public:
	virtual std::string VToString() = 0;
};

class Resource
{
	// ��� �Լ�
public:
	Resource(const std::string & name);
	// ��� ����
public:
	std::string m_name;
};

class IResourceFile
{
	// ��� �Լ�
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
	// ��� �Լ�
public:
	virtual std::string VGetPattern() = 0;
	virtual bool VUseRawFile() = 0;
	virtual bool VDiscardRawBufferAfterLoad() = 0;
	virtual bool VAddNullZero() { return false; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) = 0;
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, std::shared_ptr<ResourceHandle> handle) = 0;
};

class DefalultResourceLoader : public IResourceLoader
{
	// ��� �Լ�
public:
	virtual bool VUseRawFile() { return true; }
	virtual bool VDiscardRawBufferAfterLoad() { return true; }
	virtual unsigned int VGetLoadedResourceSize(char *rawBuffer, unsigned int rawSize) { return rawSize; }
	virtual bool VLoadResource(char *rawBuffer, unsigned int rawSize, std::shared_ptr<ResourceHandle> handle) { return true; }
	virtual std::string VGetPattern() { return "*"; }
};



class ResourceZipFile : public IResourceFile
{
	// ��� �Լ�
public:
	ResourceZipFile(const std::wstring ResourceFileName) { zipFile_ = NULL; resourceFileName_ = ResourceFileName; }
	virtual ~ResourceZipFile();
	virtual bool VOpen();
	virtual int VGetRawResourceSize(const Resource &r);
	virtual int VGetRawResource(const Resource &r, char *buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;
	virtual bool VIsUsingDevelopmentDirectories(void) const { return false; }

	// ��� ����
private:
	ZipFile* zipFile_;
	std::wstring resourceFileName_;
};

class DevelopmentResourceZipFile : public ResourceZipFile
{
	//����
public:
	enum Mode
	{
		Development,	// this mode checks the original asset directory for changes - helps during development
		Editor			// this mode only checks the original asset directory - the ZIP file is left unopened.
	};

	// ��� �Լ�
public:
	DevelopmentResourceZipFile(const std::wstring ResourceFileName, const Mode mode);
	virtual bool VOpen();
	virtual int VGetRawResourceSize(const Resource &r);
	virtual int VGetRawResource(const Resource &r, char *buffer);
	virtual int VGetNumResources() const;
	virtual std::string VGetResourceName(int num) const;
	virtual bool VIsUsingDevelopmentDirectories(void) const { return true; }
	int Find(const std::string &path);
protected:
	void ReadAssetsDirectory(std::wstring fileSpec);

	// ��� ����
public:
	Mode mode_;
	std::wstring assetsDir_;
	std::vector<WIN32_FIND_DATA> assetFileInfo_;
	ZipContentsMap directoryContentsMap_;
};

//���ҽ� ��� ����� ��
/*
	Resource Resource ("Brick.bmp");
	shared_ptr<ResourceHandle> texture = g_pApp->m_ResourceCache->GetHandle(&Resource);
	int size = texture->GetSize();
	char *brickBitmap = (char*) texture->Buffer();
*/