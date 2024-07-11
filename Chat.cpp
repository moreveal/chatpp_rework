#include "Chat.h"

#include <iostream>
#include <regex>

#include "ChatEntryManager.h"
#include "Menu.h"

uintptr_t SAMPGetAddress(SAMPAddressesType type) {
	static const auto versionIndex = Chat::getSampVersion() - 2;
	static const auto base = Chat::getSampBaseAddress();
	return (base + SAMPAddresses[versionIndex][type]);
}

uintptr_t SAMPGetOffset(SAMPAddressesType type)
{
	static const auto versionIndex = Chat::getSampVersion() - 2;
	return SAMPAddresses[versionIndex][type];
}

void Chat::MainLoop(const decltype(mainLoopHook)& hook)
{
	static bool init = false;
	if (!init && isSampAvailable())
	{
		// Setup hooks
		auto& instance = getInstance();

		SetHook(instance.mWndProcHook, SAMPGetAddress(SAMP_ADDRESS_CHATINPUT_WNDPROC), &OnWndProc);

		SetHook(instance.mChatRenderHook, SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER), &CChat__Render);
		SetHook(instance.mChatRenderEntryHook, SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER_ENTRY), &CChat__RenderEntry);
		SetHook(instance.mChatAddEntryHook, SAMPGetAddress(SAMP_ADDRESS_CHAT_ADD_ENTRY), &CChat__AddEntry);
		SetHook(instance.mChatRecalcFontSizeHook, SAMPGetAddress(SAMP_ADDRESS_RECALC_FONTSIZE), &CChat__RecalcFontSize);

		instance.mOnPresentHook.before += instance.OnPresent;
		instance.mOnResetHook.before += instance.OnLost;
		instance.mOnResetHook.after += instance.OnReset;

		// Setup patches
		SetPatch(SAMPGetAddress(SAMP_ADDRESS_COMMAND_SET_PAGESIZE) + 0x2F, { 0x83, 0xFE, 0x40 }); // cmp esi, 0x40
		SetPatch(SAMPGetAddress(SAMP_ADDRESS_COMMAND_SET_PAGESIZE_HINT) + 0xD, { '6', '4' }); // "20" -> "64"

		init = true;
	}
	else if (init)
	{
		// While true

	}

	return hook.get_trampoline()();
}

uintptr_t Chat::getSampBaseAddress()
{
	return reinterpret_cast<uintptr_t>(GetModuleHandleA("samp.dll"));
}

bool Chat::isSampAvailable()
{
	return *reinterpret_cast<uintptr_t*>(0x00C8D4C0) == 9 && getSampBaseAddress() > 0;
}

bool Chat::isGTAMenuActive()
{
	return *reinterpret_cast<uint32_t*>(0xBA67A4) != 0;
}

SampVersion Chat::getSampVersion()
{
	static SampVersion version = SAMP_NOT_LOADED;
	if (version <= SAMP_UNKNOWN)
	{
		const auto base = getSampBaseAddress();
		if (base == 0) return SAMP_NOT_LOADED;

		const auto* ntHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(base + reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
		switch (ntHeader->OptionalHeader.AddressOfEntryPoint)
		{
			case 0x31DF13:
			{
				version = SAMP_037_R1;
				break;
			}
			case 0xCC4D0:
			{
				version = SAMP_037_R3_1;
				break;
			}
			default:
			{
				version = SAMP_UNKNOWN;
			}
		}
	}
	return version;
}

HWND Chat::getGameHWND()
{
	static const HWND gameHWND = **reinterpret_cast<HWND**>(0xC17054);
	return gameHWND;
}

ChatEntryManager& Chat::getChatEntryManager()
{
	return getInstance().mChatEntryManager;
}

int Chat::getSampCursorMode()
{
	const auto CGame = *reinterpret_cast<uintptr_t*>(SAMPGetAddress(SAMP_ADDRESS_GLOBAL_GAME_PTR));
	if (!CGame) return 0;

	return *reinterpret_cast<int*>(CGame + SAMPGetOffset(SAMP_OFFSET_GLOBAL_GAME_MOUSEMODE));
}

void Chat::setSampCursorMode(const int nMode)
{
	const auto CGame = *reinterpret_cast<uintptr_t*>(SAMPGetAddress(SAMP_ADDRESS_GLOBAL_GAME_PTR));
	if (!CGame || getSampCursorMode() == nMode) return;

	const auto setCursorModeFunc = reinterpret_cast<dGameSetCursorMode>(SAMPGetAddress(SAMP_ADDRESS_GAME_SET_CURSOR_MODE));
	setCursorModeFunc(reinterpret_cast<void*>(CGame), nMode, 0);
}

float Chat::getSampFontSize()
{
	return static_cast<float>(reinterpret_cast<int(*)()>(SAMPGetAddress(SAMP_ADDRESS_FONTSIZE_VALUE))());
}

float Chat::getSampFontSizeParam()
{
	return (getSampFontSize() - 20) / 2;
}

char* Chat::getSampFontName()
{
	return reinterpret_cast<char* (*)()>(SAMPGetAddress(SAMP_ADDRESS_CHAT_FONTNAME))();
}

void Chat::chatUpdate()
{
	const auto& pChat = getInstance().pChat;
	if (pChat == nullptr) return;

	pChat->m_bRedraw = 1;
	reinterpret_cast<int(__thiscall*)(void*)>(SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER))(pChat);
}

void Chat::sampDeleteChatLine(const int& id)
{
	const auto& pChat = getInstance().pChat;
	
	for (int i = id; i > 0; --i)
		pChat->m_entry[i] = pChat->m_entry[i - 1];

	pChat->m_entry[0] = CChatEntry();

	getInstance().chatUpdate();
}

void Chat::sampDeleteChatLineAll()
{
	const auto& pChat = getInstance().pChat;

	for (auto& entry : pChat->m_entry)
		entry = CChatEntry();

	getInstance().chatUpdate();
}

std::string Chat::convertToUTF8(const std::string& os_str) {
	const UINT os_codepage = GetACP();
	
	const int wchars_num = MultiByteToWideChar(os_codepage, 0, os_str.c_str(), -1, nullptr, 0);
	if (wchars_num == 0) {
		return std::string{};
	}
	std::wstring wstr(wchars_num, 0);
	MultiByteToWideChar(os_codepage, 0, os_str.c_str(), -1, wstr.data(), wchars_num);
	
	const int utf8_chars_num = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (utf8_chars_num == 0) {
		return std::string{};
	}
	std::string utf8_str(utf8_chars_num, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, utf8_str.data(), utf8_chars_num, nullptr, nullptr);

	return utf8_str;
}

std::string Chat::convertFromUTF8(const std::string& utf8_str) {
	const UINT os_codepage = GetACP();
	
	const int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
	auto* utf16_buffer = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, utf16_buffer, len);
	
	const int os_chars_num = WideCharToMultiByte(os_codepage, 0, utf16_buffer, -1, nullptr, 0, nullptr, nullptr);
	if (os_chars_num == 0) {
		delete[] utf16_buffer;
		return std::string{};
	}
	auto* os_buffer = new char[os_chars_num];
	WideCharToMultiByte(os_codepage, 0, utf16_buffer, -1, os_buffer, os_chars_num, nullptr, nullptr);

	delete[] utf16_buffer;

	std::string result(os_buffer);

	delete[] os_buffer;

	return result;
}

HRESULT __stdcall Chat::OnWndProc(const decltype(mWndProcHook)& hook, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto& menu = Menu::getInstance();

	wchar_t wch;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(&wParam), 1, &wch, 1);

	// Don't send the ESCAPE key while the edit window is open
	if (menu.IsEditLineActive() && msg == WM_KEYDOWN && wParam == VK_ESCAPE) return true;

	// ImGui key handle
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

	// Check for mouse move
	if (msg == WM_MOUSEMOVE || msg == WM_RBUTTONDOWN) {
		const auto cursorMode = getInstance().getSampCursorMode();
		auto& mSelectedLine = getInstance().mSelectedLine;
		const bool isEdit = menu.IsPopupActive() || menu.IsEditLineActive();

		if (cursorMode >= 2 && cursorMode <= 3) // SAMP Cursor is enabled
		{
			if (!isEdit)
			{
				auto xPos = (uint32_t)GET_X_LPARAM(lParam);
				auto yPos = (uint32_t)GET_Y_LPARAM(lParam);

				const auto lineIndex = getChatEntryManager().getLineIndexInScreenCoords(xPos, yPos);
				if (lineIndex > -1)
				{
					mSelectedLine = lineIndex;

					if (msg == WM_RBUTTONDOWN) menu.ShowPopup();
				}
				else mSelectedLine = -1;
			}
		}
		else if (!isEdit) mSelectedLine = -1;
	}

	// Close editline window by esc/enter + block keys for game
	if (menu.IsEditLineActive())
	{
		if ((msg == WM_CHAR || msg == WM_KEYUP || msg == WM_KEYDOWN) && (wParam == VK_RETURN || wParam == VK_ESCAPE))
		{
			if (msg != WM_KEYUP) return true;
			if (!menu.IsColorPopupActive()) menu.CloseEditLine();
		}
		return true;
	}

	return hook.get_trampoline()(hwnd, msg, wParam, lParam);
}

std::optional<HRESULT> Chat::OnPresent(const decltype(mOnPresentHook)& hook, IDirect3DDevice9* pDevice, const RECT*, const RECT*, HWND, const RGNDATA*) {
	auto& menu = Menu::getInstance();

	if (!menu.imguiInited) {
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(getGameHWND());
		ImGui_ImplDX9_Init(pDevice);

		menu.imguiInited = true;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	menu.Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return std::nullopt;
}

std::optional<HRESULT> Chat::OnLost(const decltype(mOnResetHook)& hook, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters) {

	return std::nullopt;
}

void Chat::OnReset(const decltype(mOnResetHook)& hook, HRESULT& return_value, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters) {
	ImGui_ImplDX9_InvalidateDeviceObjects();
}

void* __fastcall Chat::CChat__Render(const decltype(mChatRenderHook)& hook, void* ptr, void*)
{
	auto& pChat = getInstance().pChat;
	static bool getChatPtr = false;
	if (!getChatPtr)
	{
		// Get chat pointer
		pChat = static_cast<CChat*>(ptr);
		getChatEntryManager().setChatPointer(pChat);
		getChatPtr = true;
	}

	return hook.get_trampoline()(ptr, nullptr);
}

int __fastcall Chat::CChat__RenderEntry(const decltype(mChatRenderEntryHook)& hook, void* ptr, void*, const char* src, CRect rect, uint32_t color)
{
	auto& chat = getInstance();

	if (hook.get_return_address() - SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER) != 0x13F) { // Not timestamp
		auto& chatEntryManager = chat.getChatEntryManager();
		chatEntryManager.push(rect);

		// Render was finished
		if (chatEntryManager.getCurrentLineRenderIndex() == -1)
		{
			const auto& menu = Menu::getInstance();

			if (!menu.IsPopupActive() && !menu.IsEditLineActive())
			{
				// Update line index (for scroll, etc.)
				POINT cursorPos;
				if (GetCursorPos(&cursorPos))
				{
					const auto lineIndex = getChatEntryManager().getLineIndexInScreenCoords(cursorPos.x, cursorPos.y);
					if (lineIndex > -1) {
						chat.mSelectedLine = lineIndex;
					}
				}
			}
		}
	}

	return hook.get_trampoline()(ptr, nullptr, src, rect, color);
}

void __fastcall Chat::CChat__AddEntry(const decltype(mChatAddEntryHook)& hook, void* ptr, void*, int nType, const char* szText, const char* szPrefix, D3DCOLOR textColor, D3DCOLOR prefixColor)
{
	auto& mSelectedLine = getInstance().mSelectedLine;
	if (mSelectedLine > -1) mSelectedLine--;

	return hook.get_trampoline()(ptr, nullptr, nType, szText, szPrefix, textColor, prefixColor);
}

int __fastcall Chat::CChat__RecalcFontSize(const decltype(mChatRecalcFontSizeHook)& hook, void* ptr, void*)
{
	Menu::getInstance().RebuildFonts();
	return hook.get_trampoline()(ptr, nullptr);
}
