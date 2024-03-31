/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */




#include "GenericGlmCompat.hpp"

#include <glib.h>
#include <fontconfig/fontconfig.h>

#include <cmath>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <PositionDbl.hpp>  // float2fixed

#include "Font2.hpp"
#include "NaviContext.hpp"

namespace psc {
namespace gl {


Font2::Font2(std::string name, GLfloat point_size)
: m_face{nullptr}
, m_aFace{nullptr}
, glyphes()
, m_point_size{point_size}
, m_MutexGlyph()
{
    FT_Error err = FT_Init_FreeType(&library);
    if (err) {
        std::cerr << "FreeType no init!" << std::endl;
        return;
    }

    std::string path = buildByName(name);
    m_face = build(path);
}

Font2::~Font2()
{
    if (m_face) {
        FT_Done_Face(m_face);
    }
    if (m_aFace) {
        FT_Done_Face(m_aFace);
    }
    //FcFini();  leads to crash ! gtk is using FC____ as well ! Better to leave this to gtk
    // Clean up the library
    FT_Done_FreeType(library);
}

FT_Face
Font2::build(const std::string& path)
{
    FT_Face face;
    //std::cout << "Loading font " << path << std::endl;
    FT_Error err = FT_New_Face(library, path.c_str(), 0, &face);
    if (err) {
        std::cerr << "No face " << path << "!" << std::endl;
        return nullptr;
    }
    //std::cout << "Num glyph " << face->num_glyphs << std::endl;
    //std::cout << "Num faces " << face->num_faces << std::endl;
    //std::cout << "Units EM " << face->units_per_EM << std::endl;

    err = FT_Set_Char_Size(face,
            0, float2fixed(m_point_size),     /* char_width/char_height in points  */
            90, 90);     /* horizontal/vertical device resolution    */
    if (err) {
        std::cerr << "Error on sizing " << path << std::endl;
        return nullptr;
    }
    return face;
}

std::string
Font2::buildByName(const std::string &name)
{
    FcConfig* config = FcInitLoadConfigAndFonts();
    std::string fontFile;
    // configure the search pattern,
    // assume "name" is a std::string with the desired font name in it
    FcPattern* pat = FcNameParse((const FcChar8*) (name.c_str()));
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    // find the font
    FcResult result;
    FcPattern* font = FcFontMatch(config, pat, &result);
    if (font
     && result == FcResultMatch) {
        FcChar8* file = nullptr;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
            // save the file to another std::string
            fontFile = std::string((char*) file);
        }
        FcPatternDestroy(font);
    }

    FcPatternDestroy(pat);
    return fontFile;
}

std::string
Font2::buildByGlyph(gunichar glyph)
{
    FcConfig* config = FcInitLoadConfigAndFonts();
    std::string fontFile;

    FcCharSet *cs = FcCharSetCreate();
    //std::cout << "Font2::buildByGlyph glyph 0x" << std::hex << (int)glyph << std::dec << std::endl;
    FcCharSetAddChar(cs, glyph);

    FcPattern* pat = FcPatternCreate();
    FcPatternBuild(pat, FC_CHARSET, FcTypeCharSet, cs, (char *) 0);

    FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_LANG, FC_CHARSET, FC_FILE, (char *) 0);

    //FcPattern* font = FcFontMatch(config, pat, &result);
    // find the fonts -> results in a list of nearly all but as we where no specific ...
    FcFontSet* fs = FcFontList(config, pat, os);
    //if (result == FcResultMatch) {
        //FcFontSetPrint(fs);
    if (fs) {
        int max = 0;
        FcPattern* best = NULL;
        //std::cout << "Found nfont " << fs->nfont << " sfont " << fs->sfont << std::endl;
        for (int i = 0; i < fs->nfont; ++i) {
            FcPattern* font = fs->fonts[i];
            FcValue val;
            FcResult result = FcPatternGet(font, FC_CHARSET, 0, &val);
            //FcChar8 *file, *family, *style;
            //FcPatternGetString(font, FC_FILE, 0, &file);
            //FcPatternGetString(font, FC_FAMILY, 0, &family);
            //FcPatternGetString(font, FC_STYLE, 0, &style);
            //FT_Int32 slant = 0;
            //FT_Int32 weight = 0;
            //FcPatternGetInteger(font, FC_SLANT, 0, &slant);
            //FcPatternGetInteger(font, FC_WEIGHT, 0, &weight);
            //std::cout << "Checking " << i << "/" << fs->nfont << " slant " << slant << " weight " << weight << std::endl;
            if (result == FcResultMatch) {
                const FcCharSet *fcs = val.u.c;
                int cnt = FcCharSetCount(fcs);
                //std::cout << "   Cnt " << cnt << std::endl;
                if (cnt > max) {        // try to use font with best coverage if we have a choice
                    max = cnt;
                    best = font;
                }
            }
        }
        if (best != NULL) {
            FcChar8* family = nullptr;
            // use family to prefer regular
            if (FcPatternGetString(best, FC_FAMILY, 0, &family) == FcResultMatch) {
                FcPattern* fam = FcNameParse(family);
                FcConfigSubstitute(config, fam, FcMatchPattern);
                FcDefaultSubstitute(fam);

                // find the font
                FcResult result;
                FcPattern* fnt = FcFontMatch(config, fam, &result);
                if (fnt
                 && result == FcResultMatch) {
                    FcChar8* file = nullptr;
                    if (FcPatternGetString(fnt, FC_FILE, 0, &file) == FcResultMatch) {
                        fontFile = std::string((char*) file);
                    }
                    FcPatternDestroy(fnt);
                }
                FcPatternDestroy(fam);
            }
        }
    }

    FcObjectSetDestroy(os);
    FcFontSetDestroy(fs);
    FcPatternDestroy(pat);
    FcCharSetDestroy(cs);
    return fontFile;
}

void
Font2::createDefault(ShaderContext *ctx, GLenum textType)
{
    for (int i = 32; i < 256; ++i) {
        gunichar c = i;
        checkGlyph(c, ctx, textType, false);   // Glyph *g =
    }
}

void
Font2::debug(ShaderContext *ctx, Matrix &persView, Position &pos, GLfloat scale)
{
    // include ?
    //if (m_textType == GL_QUADS) {
    //    scale *= 0.035;
    //}
    Position p = pos;
    for (int i = 32; i < 256; i+= 16) {
        for (int j = 0; j < 16; ++j) {
            gunichar c = i + j;
            auto g = checkGlyph(c, ctx, GL_TRIANGLES, false);
            if (g) {
                Matrix mvp = ctx->setScalePos(persView, p, scale);
                g->display(mvp);
                p.x += g->getAdvance() * scale;
            }
            else
                std::cout << "0x" << std::hex << (int)c << std::dec << " no glyph!" << std::endl;
        }
        p.y -= getLineHeight() * scale;
        p.x = pos.x;
    }
}


float
Font2::getLineHeight()
{
    return fixed2float(m_face->size->metrics.height);
}

float
Font2::getMWidth()
{
    return m_face->size->metrics.x_ppem;
}

ptrGlyph2
Font2::checkGlyph(gunichar glyph, GeometryContext *geometryContext, GLenum textType, bool tryAlternate)
{
    auto s = glyphes.find(glyph);
    if (s != glyphes.end()) {
        return (s->second);
    }
    std::lock_guard<std::mutex> lock(m_MutexGlyph); // even if all drawing shoud be on the ui thread and therefore will be sequential this may help (the extraction is definetly not thread-save)
    FT_Face face = getDefaultFace();
    FT_UInt glyph_index = FT_Get_Char_Index(face, glyph);

    if (glyph_index == 0
     && tryAlternate) {
        face = getAlternateFace(glyph);
        if (face) {
            glyph_index = FT_Get_Char_Index(face, glyph);
        }
    }

    if (glyph_index > 0) {
        auto pGlyph = std::make_shared<Glyph2>(glyph, geometryContext);
        if (pGlyph->extractGlyph(library, face, glyph_index, textType)) {
            glyphes.emplace(std::pair(glyph, pGlyph));
            return pGlyph;
        }
    }
    return nullptr;
}

FT_Face
Font2::getDefaultFace()
{
    return m_face;
}

FT_Face
Font2::getAlternateFace(gunichar glyph)
{
    if (m_aFace == nullptr) {
        //std::cout << "Font2::getAlternateFace glyph 0x" << std::hex << (int)glyph  << std::dec << std::endl;
        std::string path = buildByGlyph(glyph); // determine second font by glyph , coud be more flexible
        if (!path.empty()) {
            std::cout << "Font alternative found " << path << " for 0x" << std::hex << (int)glyph << std::dec << std::endl;
            m_aFace = build(path);
        }
    }
    return m_aFace;
}



} /* namespace gl */
} /* namespace psc */