#include "ResourceCache.h"
#include "ZipFile.h"
#include "Resource.h"

std::string ws2s(const std::wstring& s)
{
	int slength = (int)s.length() + 1;
	int len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0) - 1;
	std::string r(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}


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

int ResourceZipFile::VGetRawResourceSize(const Resource & r)
{
	std::optional<int> ResourceNum = zipFile_->Find(r.m_name.c_str());
	if (!ResourceNum.has_value())
		return -1;

	return zipFile_->GetFileLen(ResourceNum);
}

int ResourceZipFile::VGetRawResource(const Resource & r, char * buffer)
{
	int size = 0;
	std::optional<int> ResourceNum = zipFile_->Find(r.m_name.c_str());
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

DevelopmentResourceZipFile::DevelopmentResourceZipFile(const std::wstring ResourceFileName, const Mode mode) : ResourceZipFile(ResourceFileName)
{
	mode_ = mode;

	TCHAR dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir);

	assetsDir_ = dir;
	int lastSlash = (int)assetsDir_.find_last_of(L"\\");
	assetsDir_ = assetsDir_.substr(0, lastSlash);
	assetsDir_ += L"\\Assets\\";
}

bool DevelopmentResourceZipFile::VOpen()
{
	if (mode_ != Editor)
	{
		ResourceZipFile::VOpen();
	}

	// open the asset directory and read in the non-hidden contents
	if (mode_ == Editor)
	{
		ReadAssetsDirectory(L"*");
	}
	else
	{
		// FUTURE WORK - iterate through the ZipFile contents and go grab WIN32_FIND_DATA 
		//   elements for each one. Then it would be possible to compare the dates/times of the
		//   asset in the Zip file with the source asset.
		assert(0 && "Not implemented yet");
	}

	return true;
}

int DevelopmentResourceZipFile::VGetRawResourceSize(const Resource & r)
{
	int size = 0;

	if (mode_ == Editor)
	{
		int num = Find(r.m_name.c_str());
		if (num == -1)
			return -1;

		return assetFileInfo_[num].nFileSizeLow;
	}

	return ResourceZipFile::VGetRawResourceSize(r);
}

int DevelopmentResourceZipFile::VGetRawResource(const Resource & r, char * buffer)
{
	if (mode_ == Editor)
	{
		int num = Find(r.m_name.c_str());
		if (num == -1)
			return -1;

		std::string fullFileSpec = ws2s(assetsDir_) + r.m_name.c_str();
		FILE *f = fopen(fullFileSpec.c_str(), "rb");
		size_t bytes = fread(buffer, 1, assetFileInfo_[num].nFileSizeLow, f);
		fclose(f);
		return (int)bytes;
	}

	return ResourceZipFile::VGetRawResource(r, buffer);
}

int DevelopmentResourceZipFile::VGetNumResources() const
{
	return (mode_ == Editor) ? (int)assetFileInfo_.size() : (int)ResourceZipFile::VGetNumResources();
}

std::string DevelopmentResourceZipFile::VGetResourceName(int num) const
{
	if (mode_ == Editor)
	{
		std::wstring wideName = assetFileInfo_[num].cFileName;
		return ws2s(wideName);
	}

	return ResourceZipFile::VGetResourceName(num);
}

int DevelopmentResourceZipFile::Find(const std::string & name)
{
	std::string lowerCase = name;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), (int(*)(int)) std::tolower);
	ZipContentsMap::const_iterator i = directoryContentsMap_.find(lowerCase);
	if (i == directoryContentsMap_.end())
		return -1;

	return i->second;
}

void DevelopmentResourceZipFile::ReadAssetsDirectory(std::wstring fileSpec)
{
	HANDLE fileHandle;
	WIN32_FIND_DATA findData;

	// get first file
	std::wstring pathSpec = assetsDir_ + fileSpec;
	fileHandle = FindFirstFile(pathSpec.c_str(), &findData);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		// loop on all remeaining entries in dir
		while (FindNextFile(fileHandle, &findData))
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				continue;

			std::wstring fileName = findData.cFileName;
			if (findData.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fileName != L".." && fileName != L".")
				{
					fileName = fileSpec.substr(0, fileSpec.length() - 1) + fileName + L"\\*";
					ReadAssetsDirectory(fileName);
				}
			}
			else
			{
				fileName = fileSpec.substr(0, fileSpec.length() - 1) + fileName;
				std::wstring lower = fileName;
				std::transform(lower.begin(), lower.end(), lower.begin(), (int(*)(int)) std::tolower);
				wcscpy_s(&findData.cFileName[0], MAX_PATH, lower.c_str());
				directoryContentsMap_[ws2s(lower)] = (int)assetFileInfo_.size();
				assetFileInfo_.push_back(findData);
			}
		}
	}

	FindClose(fileHandle);
}

Resource::Resource(const std::string & name)
{
	m_name = name; //파일명
	//std::transform(m_name.begin(), m_name.end(), m_name.begin(), (int(*)(int)) std::tolower); // 파일명 소문자로 변환
}