#include "../main.hpp"

//BSD 3-Clause License
//
//Copyright (c) 2022-2025, vaxerski
//All rights reserved.
// Adapted from CHyprOpenGLImpl::loadAsset
[[__gnu__::__always_inline__]]
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
