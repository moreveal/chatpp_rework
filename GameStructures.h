#ifndef GAME_STRUCTURES_H
#define GAME_STRUCTURES_H

#include <cstdint>
#include <d3d9.h>

#pragma pack(push, 1)
struct CChatEntry
{
	uint32_t            m_timestamp;
	char                m_szPrefix[28];
	char                m_szText[144];
	char                unk_1[64];
	uint32_t            m_nType;
	uint32_t            m_textColor;
	uint32_t            m_prefixColor;
};

struct CScrollBar
{
	uint8_t pad0[142];
	uint32_t m_nPos;
};

struct CFont
{
	void** m_lpVtbl;
	uint32_t* m_pFont;
};

struct CFonts
{
	CFont* m_pFont;
	CFont* m_pLittleFont;
	CFont* m_pShadow;
	CFont* m_pLittleShadow;
	CFont* m_pLicensePlateFont;
	uint32_t m_pDefaultSprite;
	IDirect3DDevice9* m_pDevice;
	char* m_szTempBuffer;
	int m_nCharHeight;
	int m_nLittleCharHeight;
};

struct CChat
{
	int                 m_nPageSize;
	uint32_t            m_szLastMessage;
	int                 m_nMode;
	bool                m_bTimestamp;
	bool                m_bDoesLogExist;
	char                pad_[3];
	char                m_szLogPath[261];
	uint32_t            m_pGameUi;
	uint32_t            m_pEditbox;
	CScrollBar*         m_pScrollbar;
	uint32_t            m_textColor;
	uint32_t            m_infoColor;
	uint32_t            m_debugColor;
	long                m_nWindowBottom;
	CChatEntry          m_entry[100];
	CFonts*             m_pFontRenderer;
	uint32_t            m_pTextSprit;
	uint32_t            m_pSprite;
	uint32_t            m_pDevice;
	uint32_t            m_bRenderToSurface;
	uint32_t            m_pRenderToSurface;
	uint32_t            m_pTexture;
	uint32_t            m_pSurface;
	uint32_t            m_displayMode[4];
	uint32_t            pad_2[2];
	int                 m_bRedraw;
	uint32_t            m_nScrollbarPos;
	long                m_nCharHeight;
	long                m_nTimestampWidth;
};

struct CRect
{
	size_t x1, y1, x2, y2;

	CRect()
		: x1(SIZE_MAX), y1(SIZE_MAX),
		x2(SIZE_MAX), y2(SIZE_MAX)
	{}

	bool operator==(const CRect& other) const {
		return x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2;
	}
};
#pragma pack(pop)

#endif
