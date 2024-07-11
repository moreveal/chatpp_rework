#ifndef MENU_H
#define MENU_H

#include "pch.h"
#include <freetype/imgui_freetype.h>

#include "Chat.h"
#include "imgui_impl_dx9.h"

class Menu
{
private:
	ImVec4			ARGBToImVec4(const uint32_t& color);
	uint32_t		ImVec4ToARGB(const ImVec4& color);
public:
	bool			imguiInited			= false;
	bool			openPopup			= false;

	bool			popupActive			= false;
	bool			editLineActive		= false;
	bool			colorPopupActive	= false;

	char			editLineBuffer[256];
	ImVec4			editLineColor{1.0f, 1.0f, 1.0f, 1.0f};

	Menu() = default;

	static Menu& getInstance()
	{
		static auto menu = Menu();
		return menu;
	}

	void						Render();
	static void					RebuildFonts();

	void						ShowPopup();
	void						ClosePopup();
	void						CloseEditLine();

	bool						IsPopupActive() const;
	bool						IsEditLineActive() const;
	bool						IsColorPopupActive() const;
};

#endif