#include "Chat.h"

#include <iostream>
#include <algorithm>
#include <array>
#include <sstream>

#undef min
#undef max

#include "ChatEntryManager.h"
#include "Menu.h"

struct LineVertex
{
	float x, y, z, rhw;
	D3DCOLOR color;
};

#define LINE_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

struct Rect
{
	DWORD top, bottom, left, right;
};

namespace {
std::array<uintptr_t, SAMP_ADDRESS_AMOUNT> sampAddresses{};

uintptr_t findUniquePattern(HMODULE module, const char* pattern, bool executable)
{
	if (!module) return 0;
	std::vector<int> bytes;
	std::istringstream stream(pattern);
	std::string token;
	while (stream >> token)
		bytes.push_back(token == "??" ? -1 : static_cast<int>(std::strtoul(token.c_str(), nullptr, 16)));
	if (bytes.empty()) return 0;

	const auto* base = reinterpret_cast<const uint8_t*>(module);
	const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
	const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

	uintptr_t match = 0;
	const auto* section = IMAGE_FIRST_SECTION(nt);
	for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
	{
		if (!(section->Characteristics & IMAGE_SCN_MEM_READ) ||
			(executable && !(section->Characteristics & IMAGE_SCN_MEM_EXECUTE))) continue;
		const size_t length = section->Misc.VirtualSize;
		if (length < bytes.size()) continue;
		const auto* start = base + section->VirtualAddress;
		for (size_t pos = 0; pos <= length - bytes.size(); ++pos)
		{
			size_t j = 0;
			for (; j < bytes.size(); ++j)
				if (bytes[j] >= 0 && start[pos + j] != bytes[j]) break;
			if (j != bytes.size()) continue;
			if (match) return 0; // Ambiguous signatures must never install a hook.
			match = reinterpret_cast<uintptr_t>(start + pos);
		}
	}
	return match;
}

uintptr_t SAMPGetAddress(SAMPAddressesType type) { return sampAddresses[type]; }
uintptr_t SAMPGetOffset(SAMPAddressesType type) { return sampAddresses[type]; }

bool resolveSampSymbols()
{
	const auto module = reinterpret_cast<HMODULE>(Chat::getSampBaseAddress());
	if (!module) return false;
	struct Signature { SAMPAddressesType type; const char* pattern; bool code; };
	static constexpr Signature signatures[] = {
		{ SAMP_ADDRESS_CHATINPUT_WNDPROC, "A1 ?? ?? ?? ?? 83 EC 10 83 F8 0A", true },
		{ SAMP_ADDRESS_CHAT_RENDER, "55 8B EC 83 E4 F8 83 EC 70", true },
		{ SAMP_ADDRESS_CHAT_RENDER_ENTRY, "55 8B EC 83 E4 F8 81 EC 0C 02 00 00", true },
		{ SAMP_ADDRESS_CHAT_ADD_ENTRY, "55 56 8B E9 57 8D BD ?? ?? ?? ?? 8D B5 ?? ?? ?? ?? B9 9C 18 00 00", true },
		{ SAMP_ADDRESS_CHAT_GET_FONTFACE, "8B 0D ?? ?? ?? ?? 85 C9 74 1F 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 74 11", true },
		{ SAMP_ADDRESS_COMMAND_SET_PAGESIZE, "51 56 8B 74 24 ?? 8B C6 8D 50 ?? EB ?? 8D 49 ?? 8A 08 40 84 C9 75 ?? 2B C2 89 44 24 ?? 74 ?? 56 E8 ?? ?? ?? ?? 8B F0 83 C4 04 83 FE 0A", true },
		{ SAMP_ADDRESS_COMMAND_SET_PAGESIZE_HINT, "70 61 67 65 73 69 7A 65 20 5B 31 30 2D 32 30 5D 20 28 6C 69 6E 65 73 29", false },
		{ SAMP_ADDRESS_FONTSIZE_VALUE, "A1 ?? ?? ?? ?? 3D 00 04 00 00", true },
		{ SAMP_ADDRESS_RECALC_FONTSIZE, "83 EC 10 56 68 00 00 00 FF", true },
		{ SAMP_ADDRESS_GAME_SET_CURSOR_MODE, "55 8B EC 8B 45 ?? 83 F8 02", true },
		{ SAMP_ADDRESS_CHAT_DRAW, "56 8B F1 8B 86 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 4E", true },
		{ SAMP_ADDRESS_INPUT_OPEN, "83 EC 10 56 8B F1 8B 86", true },
		{ SAMP_ADDRESS_GAME_MENU_VISIBLE, "8B 0D ?? ?? ?? ?? 33 C0 85 C9 0F 95 C0", true }
	};
	std::array<uintptr_t, SAMP_ADDRESS_AMOUNT> resolved{};
	for (const auto& sig : signatures)
	{
		resolved[sig.type] = findUniquePattern(module, sig.pattern, sig.code);
		if (!resolved[sig.type]) return false;
	}
	const auto gameReference = findUniquePattern(module,
		"8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 74 ?? C7 86 D6 63 00 00 00 00 00 00", true);
	if (!gameReference) return false;
	resolved[SAMP_ADDRESS_GLOBAL_GAME_PTR] = *reinterpret_cast<const uint32_t*>(gameReference + 2);
	const auto cursorModeWrite = findUniquePattern(module,
		"C7 46 ?? 02 00 00 00 5F 5E 5D C2 08 00", true);
	if (!cursorModeWrite) return false;
	resolved[SAMP_OFFSET_GLOBAL_GAME_MOUSEMODE] = *reinterpret_cast<const uint8_t*>(cursorModeWrite + 2);
	if (resolved[SAMP_OFFSET_GLOBAL_GAME_MOUSEMODE] == 0) return false;
	sampAddresses = resolved;
	return true;
}

bool patchBytes(uintptr_t address, const std::vector<unsigned char>& expected, const std::vector<unsigned char>& replacement)
{
	if (!address || expected.size() != replacement.size()) return false;
	if (memcmp(reinterpret_cast<void*>(address), replacement.data(), replacement.size()) == 0) return true;
	if (memcmp(reinterpret_cast<void*>(address), expected.data(), expected.size()) != 0) return false;
	return SetPatch(address, replacement);
}
}

void Chat::InitializeSamp()
{
	static bool initialized = false;
	if (initialized || !isSampAvailable() || !resolveSampSymbols()) return;
	auto& instance = getInstance();
	const auto pageSize = SAMPGetAddress(SAMP_ADDRESS_COMMAND_SET_PAGESIZE);
	const auto pageHint = SAMPGetAddress(SAMP_ADDRESS_COMMAND_SET_PAGESIZE_HINT);
	if (!patchBytes(pageSize + 0x2F, {0x83, 0xFE, 0x14}, {0x83, 0xFE, 0x40}) ||
		!patchBytes(pageHint + 0xD, {'2', '0'}, {'6', '4'})) return;
	SetHook(instance.mChatRenderHook, SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER), &CChat__Render);
	SetHook(instance.mChatRenderEntryHook, SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER_ENTRY), &CChat__RenderEntry);
	SetHook(instance.mWndProcHook, SAMPGetAddress(SAMP_ADDRESS_CHATINPUT_WNDPROC), &OnWndProc);
	SetHook(instance.mChatAddEntryHook, SAMPGetAddress(SAMP_ADDRESS_CHAT_ADD_ENTRY), &CChat__AddEntry);
	SetHook(instance.mChatRecalcFontSizeHook, SAMPGetAddress(SAMP_ADDRESS_RECALC_FONTSIZE), &CChat__RecalcFontSize);
	initialized = true;
}

uintptr_t Chat::getSampBaseAddress()
{
	return reinterpret_cast<uintptr_t>(GetModuleHandleA("samp.dll"));
}

bool Chat::isSampAvailable()
{
	return getSampBaseAddress() != 0;
}

bool Chat::isGTAMenuActive()
{
	const auto game = *reinterpret_cast<void**>(SAMPGetAddress(SAMP_ADDRESS_GLOBAL_GAME_PTR));
	if (!game) return false;
	return reinterpret_cast<int(__thiscall*)(void*)>(SAMPGetAddress(SAMP_ADDRESS_GAME_MENU_VISIBLE))(game) != 0;
}

HWND Chat::getGameHWND()
{
	return getInstance().mGameWindow;
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
	return reinterpret_cast<char* (*)()>(SAMPGetAddress(SAMP_ADDRESS_CHAT_GET_FONTFACE))();
}

std::string Chat::getFontRelativePathByName(const std::string& fontName)
{
	HKEY hKey;
	const char* subKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
	LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey);

	if (result != ERROR_SUCCESS) return std::string{};

	char value[512];
	DWORD valueLength = sizeof(value);
	const std::string fontRegistryName = fontName + " (TrueType)";

	result = RegQueryValueExA(hKey, fontRegistryName.c_str(), nullptr, nullptr, (LPBYTE)value, &valueLength);

	if (result != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return std::string{};
	}

	RegCloseKey(hKey);

	std::string fontPath = value;
	return fontPath;
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

	// ImGui key handle
	if (menu.imguiInited) ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

	// Check for mouse move
	if (msg == WM_MOUSEMOVE || msg == WM_RBUTTONDOWN) {
		const auto cursorMode = getInstance().getSampCursorMode();
		auto& mSelectedEntry = getInstance().mSelectedEntry;
		const bool isEdit = menu.IsPopupActive() || menu.IsEditLineActive();

		if (cursorMode >= 2 && cursorMode <= 3) // SAMP Cursor is enabled
		{
			if (!isEdit)
			{
				auto xPos = GET_X_LPARAM(lParam);
				auto yPos = GET_Y_LPARAM(lParam);

				const auto entryId = getChatEntryManager().getEntryIdByScreenCoords(xPos, yPos);
				auto& cchat = Chat::getInstance().pChat;
				if (
					entryId > -1 &&
					cchat && cchat->m_entry[entryId].m_textColor != 0
				) {
					mSelectedEntry = entryId;

					if (msg == WM_RBUTTONDOWN) menu.ShowPopup();
				}
				else mSelectedEntry = -1;
			}
		}
		else if (!isEdit) mSelectedEntry = -1;
	}

	// Close editline window by esc/enter + block keys for game
	if (menu.IsEditLineActive())
	{
		if ((msg == WM_CHAR || msg == WM_KEYUP || msg == WM_KEYDOWN) && (wParam == VK_RETURN || wParam == VK_ESCAPE))
		{
			if (msg != WM_KEYUP) return TRUE;
			if (!menu.IsColorPopupActive()) menu.CloseEditLine();
		}
	}

	return hook.call_trampoline(hwnd, msg, wParam, lParam);
}

std::optional<HRESULT> Chat::OnPresent(const decltype(mOnPresentHook)& hook, IDirect3DDevice9* pDevice, const RECT*, const RECT*, HWND, const RGNDATA*) {
	auto& menu = Menu::getInstance();
	D3DDEVICE_CREATION_PARAMETERS creation{};
	if (SUCCEEDED(pDevice->GetCreationParameters(&creation))) getInstance().mGameWindow = creation.hFocusWindow;
	InitializeSamp();
	if (!SAMPGetAddress(SAMP_ADDRESS_CHAT_RENDER) || !getInstance().pChat) return std::nullopt;

	if (!menu.imguiInited) {
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(getGameHWND());
		ImGui_ImplDX9_Init(pDevice);

		ImGui_ImplDX9_InvalidateDeviceObjects();
		ImGui_ImplDX9_CreateDeviceObjects();

		Menu::getInstance().RebuildFonts();

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
	if (Menu::getInstance().imguiInited) ImGui_ImplDX9_InvalidateDeviceObjects();
	return std::nullopt;
}

void Chat::OnReset(const decltype(mOnResetHook)& hook, HRESULT& return_value, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters) {
	if (Menu::getInstance().imguiInited) ImGui_ImplDX9_InvalidateDeviceObjects();
}

void DrawRect(
	IDirect3DDevice9* device,
	float x1, float y1,
	float x2, float y2,
	D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 0, 0))
{
	LineVertex v[8] =
	{
		{ x1, y1, 0.f, 1.f, color }, { x2, y1, 0.f, 1.f, color },
		{ x2, y1, 0.f, 1.f, color }, { x2, y2, 0.f, 1.f, color },
		{ x2, y2, 0.f, 1.f, color }, { x1, y2, 0.f, 1.f, color },
		{ x1, y2, 0.f, 1.f, color }, { x1, y1, 0.f, 1.f, color }
	};

	device->SetFVF(LINE_FVF);
	device->DrawPrimitiveUP(
		D3DPT_LINELIST,
		4,
		v,
		sizeof(LineVertex)
	);
}

void* __fastcall Chat::CChat__Render(const decltype(mChatRenderHook)& hook, void* ptr, void*)
{
	auto* chat = reinterpret_cast<CChat*>(ptr);
	auto& instance = getInstance();
	instance.pChat = chat;
	instance.mChatEntryManager.clear();
	return hook.call_trampoline(ptr, nullptr);
}

int __fastcall Chat::CChat__RenderEntry(const decltype(mChatRenderEntryHook)& hook, void* ptr, void*, const char* src, CRect rect, uint32_t color)
{
	auto& instance = getInstance();
	auto* chat = reinterpret_cast<CChat*>(ptr);
	const auto source = reinterpret_cast<uintptr_t>(src);
	const auto first = reinterpret_cast<uintptr_t>(&chat->m_entry[0]);
	const auto last = reinterpret_cast<uintptr_t>(&chat->m_entry[100]);
	if (source >= first && source < last)
	{
		const size_t relative = source - first;
		const size_t field = relative % sizeof(CChatEntry);
		if (field == offsetof(CChatEntry, m_szPrefix) || field == offsetof(CChatEntry, m_szText))
		{
			CRect hit{};
			hit.x1 = std::min<size_t>(45, rect.x1);
			hit.y1 = rect.y1;
			hit.x2 = rect.x2;
			hit.y2 = rect.y1 + chat->m_nCharHeight + 1;
			instance.mChatEntryManager.observe(static_cast<int>(relative / sizeof(CChatEntry)), hit);
		}
	}
	return hook.call_trampoline(ptr, nullptr, src, rect, color);
}

void __fastcall Chat::CChat__AddEntry(const decltype(mChatAddEntryHook)& hook, void* ptr, void*, int nType, const char* szText, const char* szPrefix, D3DCOLOR textColor, D3DCOLOR prefixColor)
{
	auto& mSelected = getInstance().mSelectedEntry;
	if (mSelected > -1) mSelected--;

	return hook.call_trampoline(ptr, nullptr, nType, szText, szPrefix, textColor, prefixColor);
}

int __fastcall Chat::CChat__RecalcFontSize(const decltype(mChatRecalcFontSizeHook)& hook, void* ptr, void*)
{
	Menu::getInstance().RebuildFonts();
	return hook.call_trampoline(ptr, nullptr);
}
