#include <cstdio>
#include <cstdint>
#include <clocale>
#include <shlwapi.h>
#include <strsafe.h>
#include <pathcch.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Pathcch.lib")

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
    uint64_t Unknown;
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

typedef uint16_t RGB565;

void ConvertSPRToBMP(const wchar_t* const pWszFilePath);
void SaveBMP(const wchar_t* const pFileName, const LONG width, const LONG height, const BGR888* const paBGR888);
void GetFileNameWithoutExtension(const wchar_t* const pWszFilePath, wchar_t* pDstFileName);

int wmain()
{
	_wsetlocale(LC_ALL, L"ko-KR");

	wchar_t path[1024];

	ConvertSPRToBMP(L"D:\\Yapcar\\Sprite\\CAR\\MTD0\\01.spr");

	while (fgetws(path, _countof(path), stdin) != nullptr)
	{
		size_t len = wcsnlen_s(path, _countof(path));
		path[len - 1] = L'\0';
		ConvertSPRToBMP(path);
	}

	return 0;
}

void ConvertSPRToBMP(const wchar_t* const pWszFilePath)
{
	wchar_t directoryPath[MAX_PATH];
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
	RGB565* pColorOffset;
	uint16_t x;
	uint16_t y;
	RGB565 rgb565;
	BGR888 bgr888;
	uint16_t width;
	uint16_t height;
	int stride;
	int biSizeImage;
	int padding;

	GetFileNameWithoutExtension(pWszFilePath, fileNameWithoutExtension);

	wprintf(L"\n-- \"%s\" --\n", pWszFilePath);

	do
	{
		err = _wfopen_s(&pFile, pWszFilePath, L"rb");
		if (err != 0 || pFile == nullptr)
		{
			wprintf(L"open rb error");
			break;
		}

		fseek(pFile, 0, SEEK_END);
		bufSize = static_cast<uint32_t>(ftell(pFile));
		fseek(pFile, 0, SEEK_SET);

		paBuf = new uint8_t[bufSize];
		readByteCount = static_cast<uint32_t>(fread_s(paBuf, bufSize, sizeof(uint8_t), bufSize, pFile));

		fclose(pFile);

		pOffset = paBuf;

		pSPRFileHeader = reinterpret_cast<SPRFileHeader*>(pOffset);
		pOffset += sizeof(SPRFileHeader);

		pSpriteRect = reinterpret_cast<SpriteRect*>(pOffset);
		pOffset += pSPRFileHeader->SpriteCount * sizeof(SpriteRect);

		pSpriteInfo = reinterpret_cast<SpriteInfo*>(pOffset);
		pOffset += pSPRFileHeader->SpriteCount * sizeof(SpriteInfo);

		wprintf(L"MetaData = { %u, %u, SpriteCount: %hu, %hu }\n", pSPRFileHeader->Unknown1, pSPRFileHeader->Unknown2, pSPRFileHeader->SpriteCount, pSPRFileHeader->Unknown4);

		for (spriteIndex = 0; spriteIndex < pSPRFileHeader->SpriteCount; ++spriteIndex)
		{
			wprintf(L"[%3hu] SpriteRect / width: %3hu, height: %3hu, area: %u\n"
				"      SpriteInfo / width: %3hd, height: %3hd, Unknown: %p\n",
			pSpriteInfo[spriteIndex].Index, pSpriteRect[spriteIndex].Width, pSpriteRect[spriteIndex].Height, pSpriteRect[spriteIndex].Width * pSpriteRect[spriteIndex].Height,
			pSpriteInfo[spriteIndex].Width, pSpriteInfo[spriteIndex].Height, (void*)pSpriteInfo->Unknown);
		}

		for (spriteIndex = 0; spriteIndex < pSPRFileHeader->SpriteCount; ++spriteIndex)
		{
			width = pSpriteRect[spriteIndex].Width;
			height = pSpriteRect[spriteIndex].Height;
			area = width * height;
			stride = ((((width * 24) + 31) & ~31) >> 3);
			biSizeImage = height * stride;
			padding = stride - width * 3;

			pOffset += area * sizeof(RGB565);

			paBGR888 = new BGR888[biSizeImage];
			pBGR888Iterator = paBGR888;

			pColorOffset = reinterpret_cast<RGB565*>(pOffset);

			for (y = 0; y < height; ++y)
			{
				pColorOffset -= width;
				for (x = 0; x < width; ++x)
				{
					rgb565 = *reinterpret_cast<RGB565*>(&pColorOffset[x]);

					bgr888.Blue = (((rgb565 & 0x1F) * 527) + 23) >> 6;
					bgr888.Green = ((((rgb565 >> 5) & 0x3F) * 259) + 33) >> 6;
					bgr888.Red = ((((rgb565 >> 11) & 0x1F) * 527) + 23) >> 6;

					*pBGR888Iterator++ = bgr888;
				}
				pBGR888Iterator = (BGR888*)((uint8_t*)pBGR888Iterator + padding);
			}

			StringCchCopyW(directoryPath, _countof(directoryPath), pWszFilePath);
			PathCchRemoveFileSpec(directoryPath, _countof(directoryPath));
			StringCbPrintfW(bmpFileName, sizeof(bmpFileName), L"%s\\%s_%03hu.bmp", directoryPath, fileNameWithoutExtension, spriteIndex);
			SaveBMP(bmpFileName, width, height, paBGR888);

			delete[] paBGR888;
		}

	} while (0);

	delete[] paBuf;
}

void SaveBMP(const wchar_t* const pFileName, const LONG width, const LONG height, const BGR888* const paBGR888)
{
	FILE* pBitmapFile = nullptr;
	BITMAPFILEHEADER bmFileHeader;
	BITMAPINFOHEADER bmInfoHeader;

	_wfopen_s(&pBitmapFile, pFileName, L"wb");
	if (pBitmapFile == nullptr)
	{
		return;
	}

	LONG stride = ((((width * 24) + 31) & ~31) >> 3);
	LONG biSizeImage = height * stride;

	bmFileHeader.bfType = 0x4D42;
	bmFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmFileHeader.bfSize = bmFileHeader.bfOffBits + (biSizeImage * sizeof(BGR888));
	bmFileHeader.bfReserved1 = 0;
	bmFileHeader.bfReserved2 = 0;

	bmInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfoHeader.biWidth = width;
	bmInfoHeader.biHeight = height;
	bmInfoHeader.biPlanes = 1;
	bmInfoHeader.biBitCount = 8 * sizeof(BGR888);
	bmInfoHeader.biCompression = 0;
	bmInfoHeader.biSizeImage = 0;
	bmInfoHeader.biXPelsPerMeter = 0;
	bmInfoHeader.biYPelsPerMeter = 0;
	bmInfoHeader.biClrUsed = 0;
	bmInfoHeader.biClrImportant = 0;

	fwrite(&bmFileHeader, sizeof(bmFileHeader), 1, pBitmapFile);
	fwrite(&bmInfoHeader, sizeof(bmInfoHeader), 1, pBitmapFile);
	fwrite(paBGR888, 1, biSizeImage, pBitmapFile);

	fclose(pBitmapFile);
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