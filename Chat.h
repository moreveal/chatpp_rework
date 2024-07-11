#ifndef CHAT_H
#define CHAT_H

// TODO: Spaces at the end of all files
// TODO: Delete unused includes

#include "pch.h"
#include "GameStructures.h"
#include "ChatEntryManager.h"

class ChatEntryManager;

// Enums
enum SampVersion {
	SAMP_NOT_LOADED,
	SAMP_UNKNOWN,
	SAMP_037_R1,
	SAMP_037_R3_1,
	SAMP_037_R5
};

enum SAMPAddressesType {
	// Addresses
	SAMP_ADDRESS_CHATINPUT_WNDPROC,
	SAMP_ADDRESS_CHAT_RENDER,
	SAMP_ADDRESS_CHAT_RENDER_ENTRY,
	SAMP_ADDRESS_CHAT_ADD_ENTRY,
	SAMP_ADDRESS_CHAT_FONTNAME,
	SAMP_ADDRESS_COMMAND_SET_PAGESIZE,
	SAMP_ADDRESS_COMMAND_SET_PAGESIZE_HINT,
	SAMP_ADDRESS_GLOBAL_GAME_PTR,
	SAMP_ADDRESS_FONTSIZE_VALUE,
	SAMP_ADDRESS_RECALC_FONTSIZE,
	SAMP_ADDRESS_GAME_SET_CURSOR_MODE,
	// Offsets
	SAMP_OFFSET_GLOBAL_GAME_MOUSEMODE,
	// Counter
	SAMP_ADDRESS_AMOUNT
};

static uintptr_t SAMPAddresses[][SAMP_ADDRESS_AMOUNT]
{
	{0x5DB40, 0x63D70, 0x638A0, 0x64010, 0xB3D40, 0x64A20, 0xD7AC8, 0x21A10C, 0xB3C60, 0x63550, 0x9BD30, 0x55}, // SAMP 0.3.7-R1,
	{0x60EE0, 0x671C0, 0x66CF0, 0x67460, 0xC5C00, 0x67E80, 0xE9DA8, 0x26E8F4, 0xC5B20, 0x669A0, 0x9FFE0, 0x61}, // SAMP 0.3.7-R3-1
	{} // SAMP 0.3.7-R5
};

// Definitions
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using dMainLoop = void(__stdcall*)();
using dOnPresent = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using dOnReset = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using dChatRender = void* (__fastcall*)(void*, void*);
using dChatRenderEntry = int(__fastcall*)(void*, void*, const char*, CRect, uint32_t);
using dChatAddEntry = void(__fastcall*)(void*, void*, int, const char*, const char*, DWORD, DWORD);
using dRecalcFontSize = int(__fastcall*)(void*, void*);
using dGameSetCursorMode = int(__thiscall*)(void*, int nMode, int hideImmediately);

class Chat
{
private:
	Chat() = default;

	static std::uintptr_t find_device(std::uint32_t Len) {
		static std::uintptr_t base = [](std::size_t Len) {
			std::string path_to(MAX_PATH, '\0');
			if (auto size = GetSystemDirectoryA(path_to.data(), MAX_PATH)) {
				path_to.resize(size);
				path_to += "\\d3d9.dll";
				std::uintptr_t dwObjBase = reinterpret_cast<std::uintptr_t>(LoadLibraryA(path_to.c_str()));
				while (dwObjBase++ < dwObjBase + Len) {
					if (*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x00) == 0x06C7 &&
						*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x06) == 0x8689 &&
						*reinterpret_cast<std::uint16_t*>(dwObjBase + 0x0C) == 0x8689) {
						dwObjBase += 2;
						break;
					}
				}
				return dwObjBase;
			}
			return std::uintptr_t(0);
		}(Len);
		return base;
	}

	static void* get_function_address(int VTableIndex) {
		return (*reinterpret_cast<void***>(find_device(0x128000)))[VTableIndex];
	}
public:
	// Hook variables
	kthook::kthook_simple<dMainLoop>				mainLoopHook;
	kthook::kthook_simple<WNDPROC>					mWndProcHook;
	kthook::kthook_signal<dOnPresent>				mOnPresentHook{ get_function_address(17) };
	kthook::kthook_signal<dOnReset>					mOnResetHook{ get_function_address(16) };
	kthook::kthook_simple<dChatRender>				mChatRenderHook;
	kthook::kthook_simple<dChatRenderEntry>			mChatRenderEntryHook;
	kthook::kthook_simple<dChatAddEntry>			mChatAddEntryHook;
	kthook::kthook_simple<dRecalcFontSize>			mChatRecalcFontSizeHook;

	// Instance
	static Chat& getInstance()
	{
		static Chat instance;
		return instance;
	}

	// Variables
	CChat*								pChat = nullptr;
	ChatEntryManager					mChatEntryManager;
	int									mSelectedLine = -1;

	// Functions
	static uintptr_t					getSampBaseAddress();
	static bool							isSampAvailable();
	static bool							isGTAMenuActive();
	static SampVersion					getSampVersion();
	static HWND							getGameHWND();
	static ChatEntryManager&			getChatEntryManager();
	static int							getSampCursorMode();
	static void							setSampCursorMode(int nMode);
	static float						getSampFontSize(); // real fontsize
	static float						getSampFontSizeParam(); // [ /fontsize ] param
	static char*						getSampFontName();
	static void							sampDeleteChatLine(const int& id);
	static void							sampDeleteChatLineAll();
	static void							chatUpdate();

	static std::string					convertToUTF8(const std::string& cp1251_str);
	static std::string					convertFromUTF8(const std::string& utf8_str);

	template							<typename HookType, typename CallbackType>
	static void							SetHook(HookType& hook, uintptr_t dest, CallbackType callback);

	// Callbacks
	static void							OnScroll();

	// Hooks
	static void							MainLoop(const decltype(mainLoopHook)& hook);

	static HRESULT __stdcall			OnWndProc(const decltype(mWndProcHook)& hook, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static std::optional<HRESULT>		OnPresent(const decltype(mOnPresentHook)& hook, IDirect3DDevice9* pDevice, const RECT*, const RECT*, HWND, const RGNDATA*);
	static std::optional<HRESULT>		OnLost(const decltype(mOnResetHook)& hook, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters);
	static void							OnReset(const decltype(mOnResetHook)& hook, HRESULT& return_value, IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* parameters);

	static void* __fastcall				CChat__Render(const decltype(mChatRenderHook)& hook, void* ptr, void*);
	static int __fastcall				CChat__RenderEntry(const decltype(mChatRenderEntryHook)& hook, void* ptr, void*, const char* src, CRect rect, uint32_t color);
	static void __fastcall				CChat__AddEntry(const decltype(mChatAddEntryHook)& hook, void* ptr, void*, int nType, const char* szText, const char* szPrefix, D3DCOLOR textColor, D3DCOLOR prefixColor);
	static int __fastcall				CChat__RecalcFontSize(const decltype(mChatRecalcFontSizeHook)& hook, void* ptr, void*);
};

template<typename HookType, typename CallbackType>
inline void Chat::SetHook(HookType& hook, uintptr_t dest, CallbackType callback)
{
	hook.set_dest(dest);
	hook.set_cb(callback);
	hook.install();
}

inline bool SetPatch(uintptr_t address, const std::vector<unsigned char>& bytes)
{
	
	DWORD oldProtect; const size_t size = bytes.size();
	const auto patchAddress = reinterpret_cast<void*>(address);
	VirtualProtect(patchAddress, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(patchAddress, bytes.data(), size);
	VirtualProtect(patchAddress, size, oldProtect, nullptr);

	return true;
}

#endif
