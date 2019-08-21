#pragma once

#include <optional>
#include <string>
#include <map>

//This maps a path to zip content id - zip 알림 ID에 대한 경로를 매핑
typedef std::map<std::string, int> ZipContentsMap;
class ZipFile
{
public:
	ZipFile() { m_nEntries = 0; m_pFile = NULL; m_pDirData = NULL; };
	virtual ~ZipFile() { End(); fclose(m_pFile); }

	bool Init(const std::wstring&resFileName);
	void End();

	int GetNumFiles()const { return m_nEntries; }
	std::string GetFilename(int i) const;
	int GetFileLen(int i)const;
	bool ReadFile(int i, void *pBuf);

	//Added to show multi-threaded decompression - 멀티버퍼 압축해제
	bool ReadLargeFile(int i, void *pBuf, void(*progressCallback)(int, bool &));
	std::optional<int> Find(const std::string &path) const;
	
	ZipContentsMap m_ZipContentsMap;

private:
	struct TZipDirHeader;
	struct TZipDirFileHeader;
	struct TZipLocaHeader;

	FILE * m_pFile;		// Zip file - 압축파일
	char *m_pDirData;	// Raw data buffer. - 윈시 데이터 버퍼
	int m_nEntries;		// Number of entries. - 참여자 수

	// Pointers to the dir entries in pDirData. - pDirData의 디렉토리 항목
	const TZipDirFileHeader **m_papDir;
};

// --------------------------------------------------------------------------------
// Basic types. - 기본 유형
// --------------------------------------------------------------------------------
typedef unsigned long dword;
typedef unsigned short word;
typedef unsigned char byte;

// --------------------------------------------------------------------------------
// ZIP file structures. Note these have to be packed. // ZIP 파일 구조. 이것들이 포장됨.
// --------------------------------------------------------------------------------

#pragma pack(1)
// --------------------------------------------------------------------------------
struct ZipFile::TZipLocaHeader
{
	enum
	{
		SIGNATURE = 0x04034b50
	};
	dword sig;
	word version;
	word flag;
	word compression;	// COMP_xxxx
	word modTime;
	word modDate;
	dword crc32;
	dword cSize;
	dword ucSize;
	word fnameLen;	//Filename string follows header. - 파일 이름
	word xtraLen;	//Extra field follows filename. - 추가 필드는 파일 이름
};

struct ZipFile::TZipDirHeader
{
	enum { SIGNATURE = 0x06054b50};
	dword sig;
	word nDisk;
	word nStartDisk;
	word nDirEntries;
	word totalDirEntries;
	dword dirSize;
	dword dirOffset;
	word cmntLen;
};

struct ZipFile::TZipDirFileHeader
{
	enum { SIGNATURE = 0x02014b50};
	dword sig;
	word verMade;
	word verNeeded;
	word flag;
	word compression;	// COMP_xxxx
	word modTime;
	word modDate;
	dword crc32;
	dword cSize;	// Compressed size - 압축 크기
	dword ucSize;	// Uncompressed size - 압추갛지 않은 크기
	word fnameLen;	// Filename string follows header. - 파일 이름
	word xtraLen;	// Extra field follows filename. - 추가 필드는 파일 이름
	word cmntLen;	// Comment field follows extra field. - 주석 필드
	word diskStart;
	word intAttr;
	dword extAttr;
	dword hdrOffset;

	char *GetName() const { return (char*)(this + 1); }
	char *GetExtra() const { return GetName() + fnameLen; }
	char *GetComment() const { return GetExtra() + xtraLen; }

};
// --------------------------------------------------------------------------------
#pragma pack()
