#include "main.hpp"
#include "hypreco/hyprland.cpp"

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

[[__gnu__::__always_inline__]]
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
