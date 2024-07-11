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
			chat.SetHook(chat.mainLoopHook, 0x53E968, chat.MainLoop);

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
