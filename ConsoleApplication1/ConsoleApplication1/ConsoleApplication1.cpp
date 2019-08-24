// ConsoleApplication1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include "ZipFile.h"
#include "Resource.h"
#include "ResCache.h"

int main()
{
	//Resource 사용 예
	// m_ResCache, g_pApp = GameCode 멤버변수
	ResourceZipFile zipFile("Assets.zip");
	ResCache resCache(50, zipFile);
	if (m_ResCache.Init())
	{
		Resource resource("art\\brict.bmp");
		std::shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
		int size = texture->GetSize();
		char *brickBitmap = (char*)texture->Buffer();
	}
		//do something cool with brickBitmap!
}

// >> zipfile 사용 예
char* ZIPFile() {
	char *buffer = NULL;
	ZipFile zipFile;
	if (zipFile.Init(L"resFileName"))
	{
		std::optional<int> index = zipFile.Find("path");
		if (index.has_value()) // 값이 있는지 확인
		{
			int size = zipFile.GetFileLen(*index); // 압축하지 않은 file의 크기를 읽어옴. 파일을 찾지 못하면 -1 리턴
			buffer = new char[size]; // 사이즈만큼 버퍼 메모리 할당.
			if (buffer) // 
			{
				zipFile.ReadFile(*index, buffer); // zlib사용해서 버퍼에 파일을 읽어옴.
			}
		}
	}
	return buffer;
}

//Caching Resources into DirectX et al.
/*
	Resource resource(m_params.m_Texture);
	std::shared_ptr<ResHandle> texture = g_pApp->m_ResCache->GetHandle(&resource);
	if( FAILED(
		D3DXCreateTextureFromFileInMemory(
			DXUTGetD3D9Device(),
			texture->Buffer(),
			texture->Size(),
			&m_pTexture)))
	{
		return E_FAIL;
	}
*/


// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
