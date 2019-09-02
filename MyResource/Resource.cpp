#include "ResourceCache.h"
#include "ZipFile.h"
#include "Resource.h"

ResourceZipFile::~ResourceZipFile()
{
	SAFE_DELETE(zipFile_);
}

bool ResourceZipFile::VOpen()
{
	zipFile_ = new ZipFile;
	if (zipFile_)
	{
		return zipFile_->Init(resourceFileName_.c_str());
	}
	return false;
}

int ResourceZipFile::VGetRawResourceSize(const std::string r)
{
	std::optional<int> ResourceNum = zipFile_->Find(r.c_str());
	if (!ResourceNum.has_value())
		return -1;

	return zipFile_->GetFileLen(ResourceNum);
}

int ResourceZipFile::VGetRawResource(const std::string r, char * buffer)
{
	int size = 0;
	std::optional<int> ResourceNum = zipFile_->Find(r.c_str());
	if (ResourceNum.has_value())
	{
		size = zipFile_->GetFileLen(*ResourceNum);
		zipFile_->ReadFile(*ResourceNum, buffer);
	}
	return size;
}

int ResourceZipFile::VGetNumResources() const
{
	return (zipFile_ == NULL) ? 0 : zipFile_->GetNumFiles();
}

std::string ResourceZipFile::VGetResourceName(int num) const
{
	std::string ResourceName = "";
	if (zipFile_ != NULL && num >= 0 && num < zipFile_->GetNumFiles())
	{
		ResourceName = zipFile_->GetFilename(num);
	}
	return ResourceName;
}