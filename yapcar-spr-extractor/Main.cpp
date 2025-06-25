#include <cstdio>
#include <cstdint>
#include <clocale>
#include <shlwapi.h>
#include <strsafe.h>

#pragma comment(lib, "Shlwapi.lib")

/*
struct SPR
{
    u32 header;
    u32 unknown1; // can be zero, subtract 4 and >=
    u32 unknown2;
    u16 spriteCount;
    u16 unknown4;
    u8 unknown5[408];
};

struct SPRInfo
{
    u8 unknown[12];
};

struct Frame
{
    u16 width;
    u16 height;
    u16 index;
};

SPR sprTest @ 0x0;

SPRInfo sprInfo @ 0x1a8;

Frame frame @ 0x1b4;
*/

#pragma pack(push, 1)
struct SPRFileHeader
{
    uint32_t Code;
    uint32_t Unknown1;
    uint32_t Unknown2;
    uint16_t SpriteCount;
    uint16_t Unknown4;
    uint8_t Unknown5[408];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SpriteRect
{
    uint16_t Width;
    uint16_t Height;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SpriteInfo
{
    uint64_t Padding;
    uint16_t Width;
    uint16_t Height;
    uint16_t Index;
};
#pragma pack(pop)

void AnalyzeSPR(const wchar_t* pWszFilePath);

void AnalyzeSPR(const wchar_t* pWszFilePath)
{
	wchar_t* pFileName;
	wchar_t* pFileExt;
	FILE* pFile = nullptr;
	errno_t err;
	uint32_t bufSize;
	int8_t* paBuf = nullptr;
	uint32_t readByteCount;
	SPRFileHeader* pSPRFileHeader;
	SpriteRect* pSpriteRect;
	SpriteInfo* pSpriteInfo;
	int8_t* pOffset = 0;
	uint16_t spriteIndex;
	uint32_t areaByteCount;

	pFileName = PathFindFileNameW(pWszFilePath);
	pFileExt = PathFindExtensionW(pWszFilePath);

	wprintf(L"-- SPR FileName: \"%s\" --\n", pFileName);

	do
	{
		err = _wfopen_s(&pFile, pWszFilePath, L"rb");
		if (err != 0 || pFile == nullptr)
		{
			break;
		}

		fseek(pFile, 0, SEEK_END);
		bufSize = static_cast<uint32_t>(ftell(pFile));
		fseek(pFile, 0, SEEK_SET);

		paBuf = new int8_t[bufSize];
		readByteCount = static_cast<uint32_t>(fread_s(paBuf, bufSize, sizeof(int8_t), bufSize, pFile));
		pOffset = paBuf;

		pSPRFileHeader = reinterpret_cast<SPRFileHeader*>(pOffset);
		pOffset += sizeof(SPRFileHeader);

		pOffset += pSPRFileHeader->SpriteCount * sizeof(SpriteRect);

		for (spriteIndex = 0; spriteIndex < pSPRFileHeader->SpriteCount; ++spriteIndex)
		{
			pSpriteInfo = reinterpret_cast<SpriteInfo*>(pOffset);

			wprintf(L"[%3d] width: %3hd, height: %3hd, padding: %llu\n", pSpriteInfo->Index, pSpriteInfo->Width, pSpriteInfo->Height, pSpriteInfo->Padding);

			pOffset += sizeof(SpriteInfo);
		}

		pSpriteRect = reinterpret_cast<SpriteRect*>(paBuf + sizeof(SPRFileHeader));
		for (spriteIndex = 0; spriteIndex < pSPRFileHeader->SpriteCount; ++spriteIndex)
		{
			areaByteCount = pSpriteRect[spriteIndex].Width * pSpriteRect[spriteIndex].Height * sizeof(uint16_t);
			wprintf(L"[%3d] width: %3hd, height: %3hd, areaByteCount: %u\n", spriteIndex, pSpriteRect->Width, pSpriteRect->Height, areaByteCount);

			wchar_t imgFileName[512];
			StringCbPrintfW(imgFileName, sizeof(imgFileName), L"../samples/%s_%02hu", pFileName, spriteIndex);
			FILE* pOutput = nullptr;
			_wfopen_s(&pOutput, imgFileName, L"wb");
			if (pOutput == nullptr)
			{
				continue;
			}

			fwrite(&pSpriteRect[spriteIndex].Width, sizeof(uint16_t), 1, pOutput);
			fwrite(&pSpriteRect[spriteIndex].Height, sizeof(uint16_t), 1, pOutput);
			fwrite(pOffset, 1, areaByteCount, pOutput);
			fclose(pOutput);

			pOffset += areaByteCount;
		}

		if (pOffset != paBuf + bufSize)
		{
			__debugbreak();
		}

	} while (0);

	delete[] paBuf;
}

int wmain()
{
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_btexit.SPR");
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\Status\\esc.SPR");
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_bg.SPR");
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_bg.SPR!");
	AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_panel_notice.SPR");

	return 0;
}