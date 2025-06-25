#include <cstdio>
#include <cstdint>
#include <shlwapi.h>
#include <strsafe.h>

#pragma comment(lib, "Shlwapi.lib")

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

struct BGR888
{
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
};

void AnalyzeSPR(const wchar_t* const pWszFilePath);
void SaveBMP(wchar_t* pFileName, const LONG width, const LONG height, const BGR888* const paBGR);
void GetFileNameWithoutExtension(const wchar_t* const pWszFilePath, wchar_t* pDstFileName);

int wmain()
{
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_btexit.SPR");
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\Status\\esc.SPR");
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_bg.SPR");
	//AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_bg.SPR!");
	AnalyzeSPR(L"D:\\Yapcar\\Sprite\\LOGIN_9STAR\\login_panel_notice.SPR");

	return 0;
}

void AnalyzeSPR(const wchar_t* const pWszFilePath)
{
	wchar_t fileNameWithoutExtension[MAX_PATH];
	wchar_t bmpFileName[MAX_PATH];
	FILE* pFile = nullptr;
	errno_t err;
	uint32_t bufSize;
	uint8_t* paBuf = nullptr;
	uint32_t readByteCount;
	SPRFileHeader* pSPRFileHeader;
	SpriteRect* pSpriteRect;
	SpriteInfo* pSpriteInfo;
	uint8_t* pOffset = 0;
	uint16_t spriteIndex;
	uint32_t area;
	BGR888* paBGR888 = nullptr;
	BGR888* pBGR888Iterator;

	GetFileNameWithoutExtension(pWszFilePath, fileNameWithoutExtension);

	wprintf(L"-- FileName: \"%s\" --\n", fileNameWithoutExtension);

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

		paBuf = new uint8_t[bufSize];
		readByteCount = static_cast<uint32_t>(fread_s(paBuf, bufSize, sizeof(uint8_t), bufSize, pFile));
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
			area = pSpriteRect[spriteIndex].Width * pSpriteRect[spriteIndex].Height;
			wprintf(L"[%3d] width: %3hd, height: %3hd, area: %u\n", spriteIndex, pSpriteRect->Width, pSpriteRect->Height, area);

			pOffset += area * sizeof(uint16_t);

			paBGR888 = new BGR888[area];
			pBGR888Iterator = paBGR888;

			uint8_t* pColorOffset = pOffset;

			for (uint16_t y = 0; y < pSpriteRect->Height; ++y)
			{
				pColorOffset -= pSpriteRect->Width * sizeof(uint16_t);
				uint8_t* pReversedRow = pColorOffset;
				for (uint16_t x = 0; x < pSpriteRect->Width; ++x)
				{
					uint16_t rgb565 = *reinterpret_cast<uint16_t*>(pReversedRow);

					BGR888 bgr888;
					bgr888.Blue = (((rgb565 & 0x1F) * 527) + 23) >> 6;
					bgr888.Green = ((((rgb565 >> 5) & 0x3F) * 259) + 33) >> 6;
					bgr888.Red = ((((rgb565 >> 11) & 0x1F) * 527) + 23) >> 6;

					*pBGR888Iterator++ = bgr888;

					pReversedRow += sizeof(uint16_t);
				}
			}

			StringCbPrintfW(bmpFileName, sizeof(bmpFileName), L"../samples/%s_%03hu.bmp", fileNameWithoutExtension, spriteIndex);
			SaveBMP(bmpFileName, pSpriteRect[spriteIndex].Width, pSpriteRect[spriteIndex].Height, paBGR888);

			delete[] paBGR888;
		}

		if (pOffset != paBuf + bufSize)
		{
			__debugbreak();
		}

	} while (0);

	delete[] paBuf;
}

void SaveBMP(wchar_t* pFileName, const LONG width, const LONG height, const BGR888* const paBGR)
{
	FILE* pOutput = nullptr;
	_wfopen_s(&pOutput, pFileName, L"wb");
	if (pOutput == nullptr)
	{
		return;
	}

	BITMAPFILEHEADER fileHeader;
	fileHeader.bfType = 0x4D42;
	fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	fileHeader.bfSize = fileHeader.bfOffBits + (width * height * sizeof(BGR888));
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;

	BITMAPINFOHEADER infoHeader;
	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth = width;
	infoHeader.biHeight = height;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 8 * sizeof(BGR888);
	infoHeader.biCompression = 0;
	infoHeader.biSizeImage = 0;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	fwrite(&fileHeader, sizeof(fileHeader), 1, pOutput);
	fwrite(&infoHeader, sizeof(infoHeader), 1, pOutput);
	fwrite(paBGR, sizeof(BGR888), width * height, pOutput);

	fclose(pOutput);
}

void GetFileNameWithoutExtension(const wchar_t* pWszFilePath, wchar_t* pDstFileName)
{
	const wchar_t* pFileName;
	const wchar_t* pFileExt;
	const wchar_t* pSrc;

	pFileName = PathFindFileNameW(pWszFilePath);
	pFileExt = PathFindExtensionW(pWszFilePath);
	pSrc = pFileName;

	while (pSrc < pFileExt)
	{
		*pDstFileName++ = *pSrc++;
	}
	*pDstFileName = L'\0';
}