#include <hyprland/src/plugins/PluginAPI.hpp>

#include <filesystem>
#include <hyprland/src/config/ConfigManager.hpp>

#define private public
#include <hyprland/src/render/OpenGL.hpp>
#undef private

inline HANDLE PHANDLE = nullptr;

inline CFunctionHook* g_pApplyScreenShaderHook = nullptr;
typedef void (*origApplyScreenShader)(void*, const std::string&);

SP<CTexture> m_lutTexture;

// Adapted from CHyprOpenGLImpl::loadAsset
SP<CTexture> loadAsset(const std::string& path) {
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
    auto       tex         = makeShared<CTexture>();

    tex->allocate();
    tex->m_size = {cairo_image_surface_get_width(CAIROSURFACE), cairo_image_surface_get_height(CAIROSURFACE)};

    const GLint glIFormat = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_RGB32F : GL_RGBA;
    const GLint glFormat  = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_RGB : GL_RGBA;
    const GLint glType    = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_FLOAT : GL_UNSIGNED_BYTE;

    const auto  DATA = cairo_image_surface_get_data(CAIROSURFACE);
    tex->bind();
    tex->setTexParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->setTexParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    static const float border[] = { 1.0, 0.0, 1.0, 1.0 };
    tex->setTexParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    tex->setTexParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    if (CAIROFORMAT != CAIRO_FORMAT_RGB96F) {
        tex->setTexParameter(GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        tex->setTexParameter(GL_TEXTURE_SWIZZLE_B, GL_RED);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, glIFormat, tex->m_size.x, tex->m_size.y, 0, glFormat, glType, DATA);

    cairo_surface_destroy(CAIROSURFACE);

    return tex;
}

void createLUTTexture() {
    static auto* const PLUT = (Hyprlang::STRING* const)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hyprlut:texture")->getDataStaticPtr();

    if (!**PLUT)
        return;

    std::string texPath = absolutePath(*PLUT, g_pConfigManager->getMainConfigPath());
    glActiveTexture(GL_TEXTURE2);
    m_lutTexture = loadAsset(texPath);
    if (m_lutTexture == g_pHyprOpenGL->m_missingAssetTexture || m_lutTexture->m_size.x == 0.0) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprlut] missing LUT!", CHyprColor{1.0,0.0,0.0,1.0}, 5000);
    } else {
        std::string const x = std::to_string(static_cast<int>(m_lutTexture->m_size.x));
        std::string const y = std::to_string(static_cast<int>(m_lutTexture->m_size.y));
        HyprlandAPI::addNotification(PHANDLE,
                                     "[hyprlut] loaded LUT:\n"
                                     "path: " + texPath + "\n"
                                     "size: " + x + "x" + y,
                                     CHyprColor{0.0,1.0,0.0,1.0}, 5000);
    }

    glActiveTexture(GL_TEXTURE0);
}

void hkApplyScreenShader(void* thisptr, const std::string& path) {
    {
        Debug::log(INFO, "[hyprlut] Running hkApplyScreenShader");

        createLUTTexture();
    }

    (*(origApplyScreenShader)g_pApplyScreenShaderHook->m_original)(thisptr, path);

    {
        uint8_t lut = glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut");
        uint8_t lutSize = glGetUniformLocation(g_pHyprOpenGL->m_finalScreenShader.program, "lut_size");
        g_pHyprOpenGL->useProgram(g_pHyprOpenGL->m_finalScreenShader.program);
        if (m_lutTexture) {
            glUniform1i(lut, 2);
            glUniform2f(lutSize, m_lutTexture->m_size.x, m_lutTexture->m_size.y);
        }

        Debug::log(INFO, "[hyprlut] Ran hkApplyScreenShader");
    }
}

static SDispatchResult setTexture(std::string in) {
    HyprlandAPI::invokeHyprctlCommand("keyword plugin:hyprlut:texture", in);

    g_pHyprOpenGL->m_reloadScreenShader = true;

    return SDispatchResult{};
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    // ALWAYS add this to your plugins. It will prevent random crashes coming from
    // mismatched header versions.
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprlut] Mismatched headers! Can't proceed.",
                                     CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprlut] Version mismatch");
    }

    static const auto METHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "applyScreenShader");
    if (METHODS.size() < 1)
        throw std::runtime_error("[hyprlut] applyScreenShader not found!");
    g_pApplyScreenShaderHook = HyprlandAPI::createFunctionHook(PHANDLE, METHODS[0].address, (void*)&hkApplyScreenShader);
    g_pApplyScreenShaderHook->hook();

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprlut:texture", Hyprlang::STRING{""});

    HyprlandAPI::addDispatcherV2(PHANDLE, "plugin:hyprlut:settexture", ::setTexture);

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
