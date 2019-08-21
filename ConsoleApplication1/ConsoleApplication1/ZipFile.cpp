#include "pch.h"
#include "ZipFile.h"
#include <cctype>

//#include <zlib.h> << zlib�� �ʿ���.

bool ZipFile::Init(const std::wstring & resFileName)
{
	End();

	_wfopen_s(&m_pFile, resFileName.c_str(), _T("rb")); // _wfopen_s -> �����ڵ�� �� ������ �ҷ��� �� ( _wfopen_s(����������, �����̸�, ���); )
	// rb -> r + b >> r(�б���), b(binary[�������])

	if (!m_pFile)
		return false;

	// Assuming no extra comment at the end, read the whole end record. - ���� �߰� �ǰ��� ���ٰ� �����ϸ� ��ü ����� �����ʽÿ�.
	TZipDirHeader dh;

	fseek(m_pFile, -(int)sizeof(dh), SEEK_END); // fseek -> ����� ���Ͽ��� ���� �������� ��ġ�� ����. ( fseek(����������, �̵���ũ��, ������); )
	long dhOffset = ftell(m_pFile); // ftell -> ������������ ���� ��ġ�� ��ȯ ( ftell(����������); )
	// >> fseek �� ftell�ϸ� ������ ũ�⸦ ���Ҽ� �ִ�!?

	memset(&dh, 0, sizeof(dh));  // 0���� �ʱ�ȭ
	fread(&dh, sizeof(dh), 1, m_pFile); // fread -> ��Ʈ������ �����͸� �о� ���ۿ� ����. ( fread(����, ����1���� ũ��, ���� ����, ����������); )

	// Check - �˻�
	if (dh.sig != TZipDirHeader::SIGNATURE)
		return false;

	// Go to the beginning of the directory. - ���丮�� �������� �̵��Ͻʽÿ�.
	fseek(m_pFile, dhOffset - dh.dirSize, SEEK_SET);

	// Allocate the data buffer, and read the whole thing. - ������ ���۸� �Ҵ��ϰ� ��ü ������ �н��ϴ�.
	m_pDirData = GCC_NEW char[dh.dirSize + dh.nDirEntries * sizeof(*m_papDir)];
	if (!m_pDirData)
		return false;
	memset(m_pDirData, 0, dh.dirSize + dh.nDirEntries * sizeof(*m_papDir));
	fread(m_pDirData, dh.dirSize, 1, m_pFile);

	// Now process each entry. - ���� �� �׸��� ó���Ͻʽÿ�.
	char *pfh = m_pDirData;
	m_papDir = (const TZipDirFileHeader **)(m_pDirData + dh.dirSize);

	bool success = true;

	for (int i = 0; i < dh.nDirEntries && success; i++)
	{
		TZipDirFileHeader &fh = *(TZipDirFileHeader*)pfh;

		// Store the address of nth file for quicker access. - ���� �׼����� ���� n ��° ������ �ּҸ� �����Ͻʽÿ�.
		m_papDir[i] = &fh;

		// Check the directory entry integrity. - ���丮 �׸� ���Ἲ�� Ȯ���Ͻʽÿ�.
		if (fh.sig != TZipDirFileHeader::SIGNATURE)
			success = false;
		else
		{
			pfh += sizeof(fh);

			// Convert UNIX slashes to DOS backlashes. - UNIX �����ø� DOS �鷡�÷� ��ȯ�Ͻʽÿ�.
			for (int j = 0; j < fh.fnameLen; j++)
				if (pfh[j] == '/') // << UNIX ������
					pfh[j] = '\\'; // << DOS �鷡��

			char fileName[_MAX_PATH];
			memcpy(fileName, pfh, fh.fnameLen);
			fileName[fh.fnameLen] = 0;
			_strlwr_s(fileName, _MAX_PATH);
			std::string spath = fileName;
			m_ZipContentsMap[spath] = i;

			// Skip name, extra and comment fields. - �̸�, �߰� �� ���� �ʵ带 �ǳ� �ݴϴ�.
			pfh += fh.fnameLen + fh.xtraLen + fh.cmntLen;
		}
	}
	if (!success)
	{
		SAFE_DELETE_ARRAY(m_pDirData);
	}
	else
	{
		m_nEntries = dh.nDirEntries;
	}

	return success;
}

void ZipFile::End()
{
	m_ZipContentsMap.clear();
	SAFE_DELETE_ARRAY(m_pDirData); 
	// SAFE_DELETE_ARRAY -> ���� �޸� ���� ��ũ�� (�����Ͱ� NULL�� ����� crash�� �������� �����Ϸ��� �����Ͱ� NULL���� �˻�, ���� �� NULL�� �ʱ�ȭ)
	m_nEntries = 0;
}

std::string ZipFile::GetFilename(int i) const
{
	std::string fileName = "";
	if (i >= 0 && i < m_nEntries)
	{
		char pszDest[_MAX_PATH];
		memcpy(pszDest, m_papDir[i]->GetName(), m_papDir[i]->fnameLen);
		pszDest[m_papDir[i]->fnameLen] = '\0';
		fileName = pszDest;
	}
	return fileName;
}

int ZipFile::GetFileLen(int i) const
{
	if (i < 0 || i >= m_nEntries)
		return -1;
	else
		return m_papDir[i]->ucSize;
}

//bool ZipFile::ReadFile(int i, void * pBuf)
//{
//	if (pBuf == NULL || i < 0 || i >= m_nEntries)
//		return false;
//
//	// Quick'n dirty read, the whole file at once.
//	// Ungood if the ZIP has huge files inside
//
//	// Go to the actual file and read the local header.
//	fseek(m_pFile, m_papDir[i]->hdrOffset, SEEK_SET);
//	TZipLocalHeader h;
//
//	memset(&h, 0, sizeof(h));
//	fread(&h, sizeof(h), 1, m_pFile);
//	if (h.sig != TZipLocalHeader::SIGNATURE)
//		return false;
//
//	// Skip extra fields
//	fseek(m_pFile, h.fnameLen + h.xtraLen, SEEK_CUR);
//
//	if (h.compression == Z_NO_COMPRESSION)
//	{
//		// Simply read in raw stored data.
//		fread(pBuf, h.cSize, 1, m_pFile);
//		return true;
//	}
//	else if (h.compression != Z_DEFLATED)
//		return false;
//
//	// Alloc compressed data buffer and read the whole stream
//	char *pcData = GCC_NEW char[h.cSize];
//	if (!pcData)
//		return false;
//
//	memset(pcData, 0, h.cSize);
//	fread(pcData, h.cSize, 1, m_pFile);
//
//	bool ret = true;
//
//	// Setup the inflate stream.
//	z_stream stream;
//	int err;
//
//	stream.next_in = (Bytef*)pcData;
//	stream.avail_in = (uInt)h.cSize;
//	stream.next_out = (Bytef*)pBuf;
//	stream.avail_out = h.ucSize;
//	stream.zalloc = (alloc_func)0;
//	stream.zfree = (free_func)0;
//
//	// Perform inflation. wbits < 0 indicates no zlib header inside the data.
//	err = inflateInit2(&stream, -MAX_WBITS);
//	if (err == Z_OK)
//	{
//		err = inflate(&stream, Z_FINISH);
//		inflateEnd(&stream);
//		if (err == Z_STREAM_END)
//			err = Z_OK;
//		inflateEnd(&stream);
//	}
//	if (err != Z_OK)
//		ret = false;
//
//	delete[] pcData;
//	return ret; return false;
//}

//bool ZipFile::ReadLargeFile(int i, void * pBuf, void(*progressCallback)(int, bool &))
//{
//	if (pBuf == NULL || i < 0 || i >= m_nEntries)
//		return false;
//
//	// Quick'n dirty read, the whole file at once.
//	// Ungood if the ZIP has huge files inside
//
//	// Go to the actual file and read the local header.
//	fseek(m_pFile, m_papDir[i]->hdrOffset, SEEK_SET);
//	TZipLocalHeader h;
//
//	memset(&h, 0, sizeof(h));
//	fread(&h, sizeof(h), 1, m_pFile);
//	if (h.sig != TZipLocalHeader::SIGNATURE)
//		return false;
//
//	// Skip extra fields
//	fseek(m_pFile, h.fnameLen + h.xtraLen, SEEK_CUR);
//
//	if (h.compression == Z_NO_COMPRESSION)
//	{
//		// Simply read in raw stored data.
//		fread(pBuf, h.cSize, 1, m_pFile);
//		return true;
//	}
//	else if (h.compression != Z_DEFLATED)
//		return false;
//
//	// Alloc compressed data buffer and read the whole stream
//	char *pcData = GCC_NEW char[h.cSize];
//	if (!pcData)
//		return false;
//
//	memset(pcData, 0, h.cSize);
//	fread(pcData, h.cSize, 1, m_pFile);
//
//	bool ret = true;
//
//	// Setup the inflate stream.
//	z_stream stream;
//	int err;
//
//	stream.next_in = (Bytef*)pcData;
//	stream.avail_in = (uInt)h.cSize;
//	stream.next_out = (Bytef*)pBuf;
//	stream.avail_out = (128 * 1024); //  read 128k at a time h.ucSize;
//	stream.zalloc = (alloc_func)0;
//	stream.zfree = (free_func)0;
//
//	// Perform inflation. wbits < 0 indicates no zlib header inside the data.
//	err = inflateInit2(&stream, -MAX_WBITS);
//	if (err == Z_OK)
//	{
//		uInt count = 0;
//		bool cancel = false;
//		while (stream.total_in < (uInt)h.cSize && !cancel)
//		{
//			err = inflate(&stream, Z_SYNC_FLUSH);
//			if (err == Z_STREAM_END)
//			{
//				err = Z_OK;
//				break;
//			}
//			else if (err != Z_OK)
//			{
//				GCC_ASSERT(0 && "Something happened.");
//				break;
//			}
//
//			stream.avail_out = (128 * 1024);
//			stream.next_out += stream.total_out;
//
//			progressCallback(count * 100 / h.cSize, cancel);
//		}
//		inflateEnd(&stream);
//	}
//	if (err != Z_OK)
//		ret = false;
//
//	delete[] pcData;
//	return ret;
//}

std::optional<int> ZipFile::Find(const std::string & path) const
{
	std::string lowerCase = path;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), (int(*)(int)) std::tolower);
	ZipContentsMap::const_iterator i = m_ZipContentsMap.find(lowerCase);
	if (i == m_ZipContentsMap.end())
		return std::nullopt;

	return i->second;
}
