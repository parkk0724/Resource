#include "ZipFile.h"

//#include <zlib.h> << zlib가 필요함.

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
	End(); // 비워줌

	_wfopen_s(&file_, resFileName.c_str(), L"rb"); // _wfopen_s -> 유니코드로 된 파일을 불러올 때 ( _wfopen_s(파일포인터, 파일이름, 모드); )
	// rb -> r + b >> r(읽기모드), b(binary[이진모드])

	if (!file_)
		return false;

	// Assuming no extra comment at the end, read the whole end record. - 끝에 추가 의견이 없다고 가정하면 전체 기록을 읽으십시오.
	TZipDirHeader dh;

	fseek(file_, -(int)sizeof(dh), SEEK_END); // fseek -> 개방된 파일에서 파일 포인터의 위치를 설정. ( fseek(파일포인터, 이동할크기, 기준점); )
	long dhOffset = ftell(file_); // ftell -> 파일포인터의 현재 위치를 반환 ( ftell(파일포인터); )
	// >> fseek 후 ftell하면 파일의 크기를 구할수 있다!?

	memset(&dh, 0, sizeof(dh));  // 0으로 초기화
	fread(&dh, sizeof(dh), 1, file_); // fread -> 스트림에서 데이터를 읽어 버퍼에 저장. ( fread(버퍼, 원소1개의 크기, 원소 개수, 파일포인터); )

	// Check - 검사
	if (dh.sig != TZipDirHeader::Signature)
		return false;

	// Go to the beginning of the directory. - 디렉토리의 시작으로 이동하십시오.
	fseek(file_, dhOffset - dh.dirSize, SEEK_SET);

	// Allocate the data buffer, and read the whole thing. - 데이터 버퍼를 할당하고 전체 내용을 읽습니다.
	dirData_ = new char[dh.dirSize + dh.nDirEntries * sizeof(*dir_)];
	if (!dirData_)
		return false;
	memset(dirData_, 0, dh.dirSize + dh.nDirEntries * sizeof(*dir_));
	fread(dirData_, dh.dirSize, 1, file_);

	// Now process each entry. - 이제 각 항목을 처리하십시오.
	char *pfh = dirData_;
	dir_ = (const TZipDirFileHeader **)(dirData_ + dh.dirSize);

	bool success = true;

	for (int i = 0; i < dh.nDirEntries && success; i++)
	{
		TZipDirFileHeader &fh = *(TZipDirFileHeader*)pfh;

		// Store the address of nth file for quicker access. - 빠른 액세스를 위해 n 번째 파일의 주소를 저장하십시오.
		dir_[i] = &fh;

		// Check the directory entry integrity. - 디렉토리 항목 무결성을 확인하십시오.
		if (fh.sig != TZipDirFileHeader::Signature)
			success = false;
		else
		{
			pfh += sizeof(fh);

			// Convert UNIX slashes to DOS backlashes. - UNIX 슬래시를 DOS 백래시로 변환하십시오.
			for (int j = 0; j < fh.fnameLen; j++)
				if (pfh[j] == '/') // << UNIX 슬래시
					pfh[j] = '\\'; // << DOS 백래시

			char fileName[_MAX_PATH];
			memcpy(fileName, pfh, fh.fnameLen);
			fileName[fh.fnameLen] = 0;
			_strlwr_s(fileName, _MAX_PATH);
			std::string spath = fileName;
			zipContentsMap_[spath] = i;

			// Skip name, extra and comment fields. - 이름, 추가 및 설명 필드를 건너 뜁니다.
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
	// SAFE_DELETE_ARRAY -> 동적 메모리 해제 메크로 (포인터가 NULL인 경우의 crash를 막기위해 해제하려는 포인터가 NULL인지 검사, 해제 후 NULL로 초기화)
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
	std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(), (int(*)(int)) std::tolower); // tolower -> 소문자 변환
	// transform -> 범위 내 원소들 각각에 대해  인자로 전달한 함수를 실행한 후, 그 결과를 d_first부터 쭉 기록.
	// ( transform( 원소들을 가리키는 범위, 두 번째 인자들의 시작, 결과를 저장할 범위, 방식, 원소들을 전달할 단항 함수); )
	ZipContentsMap::const_iterator i = zipContentsMap_.find(lowerCase);  // find -> 해당 하는 항목이 존재하는지 판단. 
	if (i == zipContentsMap_.end()) // 존재하는지 확인 
		return -1; // 존재하지 않음

	return i->second; // i의 두번째 원소 value 리턴;
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
