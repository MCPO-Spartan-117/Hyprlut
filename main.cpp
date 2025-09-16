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

static void notify(eLogLevel level, const std::string& text) {
    Debug::log(level, "[hyprlut] " + text);

    static const auto PNOTIFY = reinterpret_cast<HInt* const*>(H_gCV(PHANDLE, "plugin:hyprlut:notify")->getDataStaticPtr());

    if (!**PNOTIFY)
        return;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    CHyprColor color;
    switch (level) {
        case INFO:
            color = CHyprColor{0.0, 1.0, 0.0, 1.0};
            break;
        case ERR:
            color = CHyprColor{1.0, 0.0, 0.0, 1.0};
            break;
        default:
            color = CHyprColor{0.0, 1.0, 0.0, 1.0};
    }
    H_aNv1(PHANDLE, "[hyprlut] " + text, color, 5000);
#pragma GCC diagnostic pop
}

// Adapted from CHyprOpenGLImpl::loadAsset
inline SP<CTexture> loadAsset(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        Debug::log(LOG, "[hyprlut] loadAsset: looking at {} unsuccessful: ec {}", path, ec.message());
        return g_pHyprOpenGL->m_missingAssetTexture;
    }

    if (path.empty()) {
        Debug::log(ERR, "[hyprlut] loadAsset: looking for {} failed (no provider found)", path);
        return g_pHyprOpenGL->m_missingAssetTexture;
    }

    const auto CAIROSURFACE = cairo_image_surface_create_from_png(path.c_str());

    if (!CAIROSURFACE) {
        Debug::log(ERR, "[hyprlut] loadAsset: failed to load {} (corrupt / inaccessible / not png)", path);
        return g_pHyprOpenGL->m_missingAssetTexture;
    }

    const auto CAIROFORMAT = cairo_image_surface_get_format(CAIROSURFACE);
    auto       &&tex       = makeShared<CTexture>();

    tex->allocate();
    tex->m_size = {cairo_image_surface_get_width(CAIROSURFACE), cairo_image_surface_get_height(CAIROSURFACE)};

    const GLint glIFormat = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_RGB32F : GL_RGBA;
    const GLint glFormat  = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_RGB : GL_RGBA;
    const GLint glType    = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_FLOAT : GL_UNSIGNED_BYTE;

    const auto  DATA = cairo_image_surface_get_data(CAIROSURFACE);
    tex->bind();
    tex->setTexParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->setTexParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    const float border[] = { 1.0, 0.0, 1.0, 1.0 };
    tex->setTexParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    tex->setTexParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    if (CAIROFORMAT != CAIRO_FORMAT_RGB96F) {
        tex->setTexParameter(GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        tex->setTexParameter(GL_TEXTURE_SWIZZLE_B, GL_RED);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
    glTexImage2D(GL_TEXTURE_2D, 0, glIFormat, tex->m_size.x, tex->m_size.y, 0, glFormat, glType, DATA);
#pragma GCC diagnostic pop

    cairo_surface_destroy(CAIROSURFACE);

    return tex;
}

inline void createLUTTexture(uint loopvar) {
    std::string loopvarstring = std::to_string(loopvar);
    auto PLUT = reinterpret_cast<HString const*>(H_gCV(PHANDLE, "plugin:hyprlut:texture" + loopvarstring)->getDataStaticPtr());
    auto &parray = loadedLUT[loopvar];

    if (!**PLUT || parray == *PLUT)
        return;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    std::string texPath = absolutePath(*PLUT, g_pConfigManager->getMainConfigPath());
    glActiveTexture(GL_TEXTURE16 + loopvar);
    m_lutTexture[loopvar] = loadAsset(texPath);
    if (m_lutTexture[loopvar] == g_pHyprOpenGL->m_missingAssetTexture || m_lutTexture[loopvar]->m_size.x == 0.0) {
        notify(ERR, "missing LUT!");
    } else {
        std::string const x = std::to_string(static_cast<int>(m_lutTexture[loopvar]->m_size.x));
        std::string const y = std::to_string(static_cast<int>(m_lutTexture[loopvar]->m_size.y));
        notify(INFO, "loaded LUT " + loopvarstring + ":\n"
               "path: " + texPath + "\n"
               "size: " + x + "x" + y);
    }
#pragma GCC diagnostic pop

    parray = *PLUT;
    glActiveTexture(GL_TEXTURE0);
}

static void hkApplyScreenShader(void* thisptr, const std::string& path) {
    {
        Debug::log(INFO, "[hyprlut] Running hkApplyScreenShader");
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wconditionally-supported"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
    (*reinterpret_cast<origApplyScreenShader>(g_pApplyScreenShaderHook->m_original))(thisptr, path);
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
    {
        lut = {{
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut0"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut1"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut2"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut3"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut4"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut5"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut6"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut7"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut8"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut9"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut10"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut11"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut12"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut13"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut14"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut15"),
        }};

        lut_size = {{
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size0"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size1"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size2"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size3"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size4"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size5"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size6"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size7"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size8"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size9"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size10"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size11"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size12"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size13"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size14"),
            glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size15")
        }};

        for(uint loopvar = 0; loopvar < m_lutTexture.max_size(); loopvar++) {
            std::string loopvarstring = std::to_string(loopvar);
            createLUTTexture(loopvar);

            g_pHyprOpenGL->useProgram(g_pHyprOpenGL->m_finalScreenShader.program);
            if (m_lutTexture[loopvar]) {
                notify(INFO, std::to_string(lut[loopvar]) + " " + loopvarstring);
                glUniform1i(lut[loopvar], 16 + loopvar);
                glUniform2f(lut_size[loopvar], m_lutTexture[loopvar]->m_size.x, m_lutTexture[loopvar]->m_size.y);
            }
        }

        Debug::log(INFO, "[hyprlut] Ran hkApplyScreenShader");
    }
#pragma GCC diagnostic pop
}

static SDispatchResult reloadTextures(std::string in) {
    loadedLUT.fill(nullptr);

    g_pHyprOpenGL->m_reloadScreenShader = true;

    return SDispatchResult{};
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    // ALWAYS add this to your plugins. It will prevent random crashes coming from
    // mismatched header versions.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wimplicit-float-conversion"
    if (HASH != GIT_COMMIT_HASH) {
        H_aNv1(PHANDLE, "[hyprlut] Mismatched headers! Can't proceed.",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprlut] Version mismatch");
    }
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wconditionally-supported"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
    static const auto METHODS = H_fFBN(PHANDLE, "applyScreenShader");
    if (METHODS.size() < 1)
        throw std::runtime_error("[hyprlut] applyScreenShader not found!");
    g_pApplyScreenShaderHook = H_cFH(PHANDLE, METHODS[0].address, reinterpret_cast<void*>(&hkApplyScreenShader));
    g_pApplyScreenShaderHook->hook();
#pragma GCC diagnostic pop
    for(uint loopvar = 0; loopvar < m_lutTexture.max_size(); loopvar++) {
        H_aCV(PHANDLE, "plugin:hyprlut:texture" + std::to_string(loopvar), HString{""});
    }
    H_aCV(PHANDLE, "plugin:hyprlut:notify", HInt{1});

    H_aDv2(PHANDLE, "plugin:hyprlut:reload", ::reloadTextures);
    return {
        "hyprlut",
        "A Plugin to load LUTs for shaders",
        "MCPO-Spartan-117 and gnusenpai",
        "0.1"
    };
}

APICALL EXPORT void PLUGIN_EXIT() {}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}
