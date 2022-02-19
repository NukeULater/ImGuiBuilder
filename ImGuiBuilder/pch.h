#pragma once

// ===========================
// Windows IMPORTS
// ===========================

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

// ===========================
// Windows IMPORTS /
// ===========================

// ===========================
// Standard C++ IMPORTS
// ===========================
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <regex>
#include <fstream>
#include <sstream>
#include <memory>
#include <mutex>
#include <unordered_set>

// ===========================
// Standard C++ IMPORTS /
// ===========================

// ===========================
// OPENGL Framework
// ===========================

#ifndef _OPENGL2
#include <opengl/imgui_impl_opengl3_loader.h>
#endif // _OPENGL2
#include <GLFW/glfw3.h>

// ===========================
// IMGUI Framework
// ===========================

#include <imgui.h>
#include <opengl/imgui_impl_glfw.h>

#ifdef _OPENGL2
#include <opengl/imgui_impl_opengl2.h>
#else
#include <opengl/imgui_impl_opengl3.h>
#endif

// ===========================
// IMGUI Framework /
// ===========================

// ===========================
// MISC
// ===========================

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (defined(__cplusplus) && (__cplusplus >= 201703L)))
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif

// ===========================
// MISC /
// ===========================