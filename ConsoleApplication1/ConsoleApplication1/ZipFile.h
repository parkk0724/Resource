#pragma once

#include <optional>
#include <string>
#include <map>

//This maps a path to zip content id - zip �˸� ID�� ���� ��θ� ����
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

	//Added to show multi-threaded decompression - ��Ƽ���� ��������
	bool ReadLargeFile(int i, void *pBuf, void(*progressCallback)(int, bool &));
	std::optional<int> Find(const std::string &path) const;
	
	ZipContentsMap m_ZipContentsMap;

private:
	struct TZipDirHeader;
	struct TZipDirFileHeader;
	struct TZipLocaHeader;

	FILE * m_pFile;		// Zip file - ��������
	char *m_pDirData;	// Raw data buffer. - ���� ������ ����
	int m_nEntries;		// Number of entries. - ������ ��

	// Pointers to the dir entries in pDirData. - pDirData�� ���丮 �׸�
	const TZipDirFileHeader **m_papDir;
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
	word fnameLen;	//Filename string follows header. - ���� �̸�
	word xtraLen;	//Extra field follows filename. - �߰� �ʵ�� ���� �̸�
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
	dword cSize;	// Compressed size - ���� ũ��
	dword ucSize;	// Uncompressed size - ���߰��� ���� ũ��
	word fnameLen;	// Filename string follows header. - ���� �̸�
	word xtraLen;	// Extra field follows filename. - �߰� �ʵ�� ���� �̸�
	word cmntLen;	// Comment field follows extra field. - �ּ� �ʵ�
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
