#include "pch.h"
#include "Chat.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hModule);

			auto& chat = Chat::getInstance();
			chat.setModule(hModule);
			chat.mOnPresentHook.before += chat.OnPresent;
			chat.mOnResetHook.before += chat.OnLost;
			chat.mOnResetHook.after += chat.OnReset;

			//AllocConsole(); freopen("CONOUT$", "w", stdout); // Only logging
			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
