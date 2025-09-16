#pragma once

#include <array>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <filesystem>
#include <hyprland/src/config/ConfigManager.hpp>

#define private public
#include <hyprland/src/render/OpenGL.hpp>
#undef private

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook* g_pApplyScreenShaderHook = nullptr;
typedef void (*origApplyScreenShader)(void*, const std::string&);

using std::array;

// Hyprlang types
using HInt = Hyprlang::INT;
using HString = Hyprlang::STRING;

// HyprlandAPI functions
constexpr auto *H_fFBN = HyprlandAPI::findFunctionsByName;
constexpr auto *H_cFH = HyprlandAPI::createFunctionHook;
constexpr auto *H_aCV = HyprlandAPI::addConfigValue;
constexpr auto *H_gCV = HyprlandAPI::getConfigValue;
constexpr auto *H_rCD = HyprlandAPI::registerCallbackDynamic;
constexpr auto *H_aDv2 = HyprlandAPI::addDispatcherV2;
constexpr auto *H_aNv1 = HyprlandAPI::addNotification;
constexpr auto *H_iHC = HyprlandAPI::invokeHyprctlCommand;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

static array<SP<CTexture>, 16> m_lutTexture;
static array<HString, 16> loadedLUT;
static array<GLint, 16> lut;
static array<GLint, 16> lut_size;
#pragma GCC diagnostic pop
