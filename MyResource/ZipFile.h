#pragma once

#include <optional>
#include <algorithm>
#include <assert.h>
#include <zlib.h>
#include <cctype>
#include <string>
#include <optional>
#include <map>
#define SAFE_DELETE_ARRAY(x) if(x) delete[] x; x = NULL;

//This maps a path to zip content id - zip �˸� ID�� ���� ��θ� ����
typedef std::map<std::string, int> ZipContentsMap;

class ZipFile
{
	//����
private:
	struct TZipDirHeader;
	struct TZipDirFileHeader;
	struct TZipLocalHeader;
	//��� �Լ�
public:
	ZipFile();
	virtual ~ZipFile();
	bool Init(const std::wstring&resFileName);
	void End();
	int GetNumFiles()const;
	std::string GetFilename(int i) const;
	int GetFileLen(std::optional<int> i)const;
	bool ReadFile(int i, void *pBuf);
	//Added to show multi-threaded decompression - ��Ƽ���� ��������
	bool ReadLargeFile(int i, void *pBuf, void(*progressCallback)(int, bool &));
	std::optional<int> Find(const std::string &path) const;

	//��� ����
public:
	ZipContentsMap zipContentsMap_;
private:
	FILE * file_;		// Zip file - ��������
	char *dirData_;	// Raw data buffer. - ���� ������ ����
	int entries_;		// Number of entries. - ������ ��
	// Pointers to the dir entries in pDirData. - pDirData�� ���丮 �׸�
	const TZipDirFileHeader **dir_;
};

// --------------------------------------------------------------------------------
// Basic types. - �⺻ ����
// --------------------------------------------------------------------------------
typedef unsigned long dword;
typedef unsigned short word;
typedef unsigned char byte;

// --------------------------------------------------------------------------------
// ZIP file structures. Note these have to be packed. // ZIP ���� ����. �̰͵��� �����.
// --------------------------------------------------------------------------------

#pragma pack(1)
// --------------------------------------------------------------------------------
struct ZipFile::TZipLocalHeader
{
	enum
	{
		Signature = 0x04034b50
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
	word fnameLen;	//Filename string follows header. - ���� �̸�
	word xtraLen;	//Extra field follows filename. - �߰� �ʵ�� ���� �̸�
};

struct ZipFile::TZipDirHeader
{
	enum 
	{
		Signature = 0x06054b50
	};
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
	enum 
	{ 
		Signature = 0x02014b50
	};
	dword sig;
	word verMade;
	word verNeeded;
	word flag;
	word compression;	// COMP_xxxx
	word modTime;
	word modDate;
	dword crc32;
	dword cSize;	// Compressed size - ���� ũ��
	dword ucSize;	// Uncompressed size - �������� ���� ũ��
	word fnameLen;	// Filename string follows header. - ���� �̸�
	word xtraLen;	// Extra field follows filename. - �߰� �ʵ�� ���� �̸�
	word cmntLen;	// Comment field follows extra field. - �ּ� �ʵ�
	word diskStart;
	word intAttr;
	dword extAttr;
	dword hdrOffset;

	char *GetName() const;
	char *GetExtra() const;
	char *GetComment() const;
};
// --------------------------------------------------------------------------------
#pragma pack()