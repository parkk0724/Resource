#include "ZipFile.h"

//#include <zlib.h> << zlib�� �ʿ���.

ZipFile::ZipFile()
{
	entries_ = 0; file_ = NULL; dirData_ = NULL;
}

ZipFile::~ZipFile()
{
	End(); fclose(file_);
}

bool ZipFile::Init(const std::wstring & resFileName)
{
	End(); // �����

	_wfopen_s(&file_, resFileName.c_str(), L"rb"); // _wfopen_s -> �����ڵ�� �� ������ �ҷ��� �� ( _wfopen_s(����������, �����̸�, ���); )
	// rb -> r + b >> r(�б���), b(binary[�������])

	if (!file_)
		return false;

	// Assuming no extra comment at the end, read the whole end record. - ���� �߰� �ǰ��� ���ٰ� �����ϸ� ��ü ����� �����ʽÿ�.
	TZipDirHeader dh;

	fseek(file_, -(int)sizeof(dh), SEEK_END); // fseek -> ����� ���Ͽ��� ���� �������� ��ġ�� ����. ( fseek(����������, �̵���ũ��, ������); )
	long dhOffset = ftell(file_); // ftell -> ������������ ���� ��ġ�� ��ȯ ( ftell(����������); )
	// >> fseek �� ftell�ϸ� ������ ũ�⸦ ���Ҽ� �ִ�!?

	memset(&dh, 0, sizeof(dh));  // 0���� �ʱ�ȭ
	fread(&dh, sizeof(dh), 1, file_); // fread -> ��Ʈ������ �����͸� �о� ���ۿ� ����. ( fread(����, ����1���� ũ��, ���� ����, ����������); )

	// Check - �˻�
	if (dh.sig != TZipDirHeader::Signature)
		return false;

	// Go to the beginning of the directory. - ���丮�� �������� �̵��Ͻʽÿ�.
	fseek(file_, dhOffset - dh.dirSize, SEEK_SET);

	// Allocate the data buffer, and read the whole thing. - ������ ���۸� �Ҵ��ϰ� ��ü ������ �н��ϴ�.
	dirData_ = new char[dh.dirSize + dh.nDirEntries * sizeof(*dir_)];
	if (!dirData_)
		return false;
	memset(dirData_, 0, dh.dirSize + dh.nDirEntries * sizeof(*dir_));
	fread(dirData_, dh.dirSize, 1, file_);

	// Now process each entry. - ���� �� �׸��� ó���Ͻʽÿ�.
	char *pfh = dirData_;
	dir_ = (const TZipDirFileHeader **)(dirData_ + dh.dirSize);

	bool success = true;

	for (int i = 0; i < dh.nDirEntries && success; i++)
	{
		TZipDirFileHeader &fh = *(TZipDirFileHeader*)pfh;

		// Store the address of nth file for quicker access. - ���� �׼����� ���� n ��° ������ �ּҸ� �����Ͻʽÿ�.
		dir_[i] = &fh;

		// Check the directory entry integrity. - ���丮 �׸� ���Ἲ�� Ȯ���Ͻʽÿ�.
		if (fh.sig != TZipDirFileHeader::Signature)
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
			zipContentsMap_[spath] = i;

			// Skip name, extra and comment fields. - �̸�, �߰� �� ���� �ʵ带 �ǳ� �ݴϴ�.
			pfh += fh.fnameLen + fh.xtraLen + fh.cmntLen;
		}
	}
	if (!success)
	{
		SAFE_DELETE_ARRAY(dirData_);
	}
	else
	{
		entries_ = dh.nDirEntries;
	}

	return success;
}

void ZipFile::End()
{
	zipContentsMap_.clear();
	SAFE_DELETE_ARRAY(dirData_);
	// SAFE_DELETE_ARRAY -> ���� �޸� ���� ��ũ�� (�����Ͱ� NULL�� ����� crash�� �������� �����Ϸ��� �����Ͱ� NULL���� �˻�, ���� �� NULL�� �ʱ�ȭ)
	entries_ = 0;
}

int ZipFile::GetNumFiles() const
{
	return entries_;
}

std::string ZipFile::GetFilename(int i) const
{
	std::string fileName = "";
	if (i >= 0 && i < entries_)
	{
		char pszDest[_MAX_PATH];
		memcpy(pszDest, dir_[i]->GetName(), dir_[i]->fnameLen);
		pszDest[dir_[i]->fnameLen] = '\0';
		fileName = pszDest;
	}
	return fileName;
}

int ZipFile::GetFileLen(std::optional<int> i) const
{
	if (i < 0 || i >= entries_)
		return -1;
	else
		return dir_[i.value()]->ucSize;
}

bool ZipFile::ReadFile(int i, void * pBuf)
{
	if (pBuf == NULL || i < 0 || i >= entries_)
		return false;

	// Quick'n dirty read, the whole file at once.
	// Ungood if the ZIP has huge files inside

	// Go to the actual file and read the local header.
	fseek(file_, dir_[i]->hdrOffset, SEEK_SET);
	TZipLocalHeader h;

	memset(&h, 0, sizeof(h));
	fread(&h, sizeof(h), 1, file_);
	if (h.sig != TZipLocalHeader::Signature)
		return false;

	// Skip extra fields
	fseek(file_, h.fnameLen + h.xtraLen, SEEK_CUR);

	if (h.compression == Z_NO_COMPRESSION)
	{
		// Simply read in raw stored data.
		fread(pBuf, h.cSize, 1, file_);
		return true;
	}
	else if (h.compression != Z_DEFLATED)
		return false;

	// Alloc compressed data buffer and read the whole stream
	char *pcData = new char[h.cSize];
	if (!pcData)
		return false;

	memset(pcData, 0, h.cSize);
	fread(pcData, h.cSize, 1, file_);

	bool ret = true;

	// Setup the inflate stream.
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)pcData;
	stream.avail_in = (uInt)h.cSize;
	stream.next_out = (Bytef*)pBuf;
	stream.avail_out = h.ucSize;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	// Perform inflation. wbits < 0 indicates no zlib header inside the data.
	err = inflateInit2(&stream, -MAX_WBITS);
	if (err == Z_OK)
	{
		err = inflate(&stream, Z_FINISH);
		inflateEnd(&stream);
		if (err == Z_STREAM_END)
			err = Z_OK;
		inflateEnd(&stream);
	}
	if (err != Z_OK)
		ret = false;

	delete[] pcData;
	return ret; return false;
}

bool ZipFile::ReadLargeFile(int i, void * pBuf, void(*progressCallback)(int, bool &))
{
	if (pBuf == NULL || i < 0 || i >= entries_)
		return false;

	// Quick'n dirty read, the whole file at once.
	// Ungood if the ZIP has huge files inside

	// Go to the actual file and read the local header.
	fseek(file_, dir_[i]->hdrOffset, SEEK_SET);
	TZipLocalHeader h;

	memset(&h, 0, sizeof(h));
	fread(&h, sizeof(h), 1, file_);
	if (h.sig != TZipLocalHeader::Signature)
		return false;

	// Skip extra fields
	fseek(file_, h.fnameLen + h.xtraLen, SEEK_CUR);

	if (h.compression == Z_NO_COMPRESSION)
	{
		// Simply read in raw stored data.
		fread(pBuf, h.cSize, 1, file_);
		return true;
	}
	else if (h.compression != Z_DEFLATED)
		return false;

	// Alloc compressed data buffer and read the whole stream
	char *pcData = new char[h.cSize];
	if (!pcData)
		return false;

	memset(pcData, 0, h.cSize);
	fread(pcData, h.cSize, 1, file_);

	bool ret = true;

	// Setup the inflate stream.
	z_stream stream;
	int err;

	stream.next_in = (Bytef*)pcData;
	stream.avail_in = (uInt)h.cSize;
	stream.next_out = (Bytef*)pBuf;
	stream.avail_out = (128 * 1024); //  read 128k at a time h.ucSize;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	// Perform inflation. wbits < 0 indicates no zlib header inside the data.
	err = inflateInit2(&stream, -MAX_WBITS);
	if (err == Z_OK)
	{
		uInt count = 0;
		bool cancel = false;
		while (stream.total_in < (uInt)h.cSize && !cancel)
		{
			err = inflate(&stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END)
			{
				err = Z_OK;
				break;
			}
			else if (err != Z_OK)
			{
				assert(0 && "Something happened.");
				break;
			}

			stream.avail_out = (128 * 1024);
			stream.next_out += stream.total_out;

			progressCallback(count * 100 / h.cSize, cancel);
		}
		inflateEnd(&stream);
	}
	if (err != Z_OK)
		ret = false;

	delete[] pcData;
	return ret;
}

std::optional<int> ZipFile::Find(const std::string & path) const
{
	std::string lowerCase = path;
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), (int(*)(int)) std::tolower); // tolower -> �ҹ��� ��ȯ
	// transform -> ���� �� ���ҵ� ������ ����  ���ڷ� ������ �Լ��� ������ ��, �� ����� d_first���� �� ���.
	// ( transform( ���ҵ��� ����Ű�� ����, �� ��° ���ڵ��� ����, ����� ������ ����, ���, ���ҵ��� ������ ���� �Լ�); )
	ZipContentsMap::const_iterator i = zipContentsMap_.find(lowerCase);  // find -> �ش� �ϴ� �׸��� �����ϴ��� �Ǵ�. 
	if (i == zipContentsMap_.end()) // �����ϴ��� Ȯ�� 
		return -1; // �������� ����

	return i->second; // i�� �ι�° ���� value ����;
}

char * ZipFile::TZipDirFileHeader::GetName() const
{
	return (char*)(this + 1);
}

char * ZipFile::TZipDirFileHeader::GetExtra() const
{
	return GetName() + fnameLen;
}

char * ZipFile::TZipDirFileHeader::GetComment() const
{
	return GetExtra() + xtraLen;
}
