#pragma once

// ===========================
// Windows IMPORTS
// ===========================

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

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

// ===========================
// Windows IMPORTS /
// ===========================


// ===========================
// OPENGL Framework
// ===========================

#ifndef _OPENGL2
#include <GL/gl3w.h>
#endif
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