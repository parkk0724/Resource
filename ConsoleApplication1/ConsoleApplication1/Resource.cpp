#include "ResCache.h"
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
	SAFE_DELETE(m_pZipFile);
}

bool ResourceZipFile::VOpen()
{
	m_pZipFile = new ZipFile;
	if (m_pZipFile)
	{
		return m_pZipFile->Init(m_resFileName.c_str());
	}
	return false;
}

int ResourceZipFile::VGetRawResourceSize(const Resource & r)
{
	std::optional<int> resourceNum = m_pZipFile->Find(r.m_name.c_str());
	if (resourceNum.has_value())
		return -1;

	return m_pZipFile->GetFileLen(resourceNum);
}

int ResourceZipFile::VGetRawResource(const Resource & r, char * buffer)
{
	int size = 0;
	std::optional<int> resourceNum = m_pZipFile->Find(r.m_name.c_str());
	if (resourceNum.has_value())
	{
		size = m_pZipFile->GetFileLen(*resourceNum);
		m_pZipFile->ReadFile(*resourceNum, buffer);
	}
	return size;
}

int ResourceZipFile::VGetNumResources() const
{
	return (m_pZipFile == NULL) ? 0 : m_pZipFile->GetNumFiles();
}

std::string ResourceZipFile::VGetResourceName(int num) const
{
	std::string resName = "";
	if (m_pZipFile != NULL && num >= 0 && num < m_pZipFile->GetNumFiles())
	{
		resName = m_pZipFile->GetFilename(num);
	}
	return resName;
}

DevelopmentResourceZipFile::DevelopmentResourceZipFile(const std::wstring resFileName, const Mode mode) : ResourceZipFile(resFileName)
{
	m_mode = mode;

	TCHAR dir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, dir);

	m_AssetsDir = dir;
	int lastSlash = (int)m_AssetsDir.find_last_of(L"\\");
	m_AssetsDir = m_AssetsDir.substr(0, lastSlash);
	m_AssetsDir += L"\\Assets\\";
}

bool DevelopmentResourceZipFile::VOpen()
{
	if (m_mode != Editor)
	{
		ResourceZipFile::VOpen();
	}

	// open the asset directory and read in the non-hidden contents
	if (m_mode == Editor)
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

	if (m_mode == Editor)
	{
		int num = Find(r.m_name.c_str());
		if (num == -1)
			return -1;

		return m_AssetFileInfo[num].nFileSizeLow;
	}

	return ResourceZipFile::VGetRawResourceSize(r);
}

int DevelopmentResourceZipFile::VGetRawResource(const Resource & r, char * buffer)
{
	if (m_mode == Editor)
	{
		int num = Find(r.m_name.c_str());
		if (num == -1)
			return -1;

		std::string fullFileSpec = ws2s(m_AssetsDir) + r.m_name.c_str();
		FILE *f = fopen(fullFileSpec.c_str(), "rb");
		size_t bytes = fread(buffer, 1, m_AssetFileInfo[num].nFileSizeLow, f);
		fclose(f);
		return (int)bytes;
	}

	return ResourceZipFile::VGetRawResource(r, buffer);
}

int DevelopmentResourceZipFile::VGetNumResources() const
{
	return (m_mode == Editor) ? (int)m_AssetFileInfo.size() : (int)ResourceZipFile::VGetNumResources();
}

std::string DevelopmentResourceZipFile::VGetResourceName(int num) const
{
	if (m_mode == Editor)
	{
		std::wstring wideName = m_AssetFileInfo[num].cFileName;
		return ws2s(wideName);
	}

	return ResourceZipFile::VGetResourceName(num);
}

int DevelopmentResourceZipFile::Find(const std::string & name)
{
	std::string lowerCase = name;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), (int(*)(int)) std::tolower);
	ZipContentsMap::const_iterator i = m_DirectoryContentsMap.find(lowerCase);
	if (i == m_DirectoryContentsMap.end())
		return -1;

	return i->second;
}

void DevelopmentResourceZipFile::ReadAssetsDirectory(std::wstring fileSpec)
{
	HANDLE fileHandle;
	WIN32_FIND_DATA findData;

	// get first file
	std::wstring pathSpec = m_AssetsDir + fileSpec;
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
				m_DirectoryContentsMap[ws2s(lower)] = (int)m_AssetFileInfo.size();
				m_AssetFileInfo.push_back(findData);
			}
		}
	}

	FindClose(fileHandle);
}