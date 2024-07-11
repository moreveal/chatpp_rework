#ifndef PCH_H
#define PCH_H

#define DIRECTINPUT_VERSION 0x0800

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <ShlObj.h>

#include "kthook/kthook.hpp"

#include <d3d9.h>
#include <d3d9types.h>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <freetype/imgui_freetype.h>

#include <vector>

#endif //PCH_H
