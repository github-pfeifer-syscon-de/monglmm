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

#include <glibmm.h>
#include <math.h>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H
#include FT_OUTLINE_H
#include FT_BBOX_H

#ifndef CALLBACK
#define CALLBACK
#endif

#include "Glyph2.hpp"


// scan outline ourself (less c pointer magic) or use functions FT_ if undefined
#define DIRECT_SCAN 1

// use poly2tri if defined [https://github.com/greenm01/poly2tri place src poly2tri.h in
//   this dir and common & sweep in subdirs. Source not provided to avoid licence hassels]
//    just to mention as the glu implementation might not be available for all other plattforms...
// glu will be used otherwise which does overall a excellent job related to the effort.
#undef POLY2TRI

#ifdef POLY2TRI
#include "poly2tri.h"
#endif


namespace psc {
namespace gl {
class Glyph2;
} /* namespace psc */
} /* namespace gl */

static psc::gl::Glyph2 *pGlyph;


const char* getPrimitiveType(GLenum type)
{
    switch(type)
    {
    case 0x0000:
        return "GL_POINTS";
        break;
    case 0x0001:
        return "GL_LINES";
        break;
    case 0x0002:
        return "GL_LINE_LOOP";
        break;
    case 0x0003:
        return "GL_LINE_STRIP";
        break;
    case 0x0004:
        return "GL_TRIANGLES";
        break;
    case 0x0005:
        return "GL_TRIANGLE_STRIP";
        break;
    case 0x0006:
        return "GL_TRIANGLE_FAN";
        break;
    case 0x0007:
        return "GL_QUADS";
        break;
    case 0x0008:
        return "GL_QUAD_STRIP";
        break;
    case 0x0009:
        return "GL_POLYGON";
        break;
    }
    return "?";
}

#ifndef POLY2TRI
static GLenum whichTess;
static Position lastTess[2];
static unsigned int lastTessIndex;
static std::shared_ptr<PositionDbl> last;

static void CALLBACK
tessBeginCB(GLenum _which)
{
    //std::cout << "glBegin(" << getPrimitiveType(_which) << ");" << std::endl;
    whichTess = _which;
    lastTessIndex = 0;
}

static void CALLBACK
tessEndCB()
{

    //std::cout << "glEnd();" << std::endl;
}

static void CALLBACK
tessErrorCB(GLenum errorCode)
{
   const GLubyte *errorStr;

    errorStr = gluErrorString(errorCode);
    std::cerr << "[ERROR]: " << errorStr << std::endl;
}

static void CALLBACK
tessVertexCB(const GLvoid *data)
{
  // cast back to double type
    const GLdouble *ptr = (const GLdouble*)data;

    Position pos;
    pos.x = (GLfloat)*ptr;
    pos.y = (GLfloat)*(ptr+1);
    pos.z = (GLfloat)*(ptr+2);
    Color c(1.0f);
    Vector n(0.0f, 1.0f, 0.0f);

    switch(whichTess) {
    case GL_TRIANGLES :
        pGlyph->addFillPoint(pos, c, n);
        break;
    case GL_TRIANGLE_FAN:   // to use uniform geometry convert to triangles
        if (lastTessIndex < 2) {
            lastTess[lastTessIndex] = pos;
            ++lastTessIndex;
        }
        else {
            pGlyph->addFillPoint(lastTess[0], c, n);
            pGlyph->addFillPoint(lastTess[1], c, n);
            pGlyph->addFillPoint(pos, c, n);

            // with a fan point 0 stays
            lastTess[1] = pos;
        }
        break;
    case GL_TRIANGLE_STRIP:   // to use uniform geometry convert to triangles
        if (lastTessIndex < 2) {
            lastTess[lastTessIndex] = pos;
            ++lastTessIndex;
        }
        else {
            pGlyph->addFillPoint(lastTess[0], c, n);
            pGlyph->addFillPoint(lastTess[1], c, n);
            pGlyph->addFillPoint(pos, c, n);

            lastTess[0] = lastTess[1];
            lastTess[1] = pos;
        }
        break;
    default:
        std::cerr << "Undefined " << getPrimitiveType(whichTess) << std::endl;
    }

    // DEBUG //
    //std::cout << "  glVertex3d(" << *ptr << ", " << *(ptr+1) << ", " << *(ptr+2) << ");" << std::endl;
}

static void CALLBACK
tessCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4],
                            const GLfloat neighborWeight[4], GLdouble **outData)
{
   //std::cerr << "Undefined tessCombineCB" << std::endl;
}

void
p2t::Polygon::gluTess(GLUtesselator *tess)
{
    gluTessBeginContour(tess);
    for (auto p : positions) {
        //std::cout << "Tess line " << i->x << "," << i->y << "," << i->z << std::endl;
        //GLdouble cords[3] = {i->x, i->y, i->z};
        gluTessVertex(tess, (GLdouble *)p.get(), (void *)p.get());
    }
    gluTessEndContour(tess);
    for (auto h : holes) {
        h->gluTess(tess);
    }
}

#endif


namespace psc {
namespace gl {


class CompareByArea {
    public:
    bool operator()(std::shared_ptr<struct p2t::Polygon> a, std::shared_ptr<struct p2t::Polygon> b) {
        const Rect ra = a->bounds();    // better use Bound as these will be more accurate or event all points...
        const Rect rb = b->bounds();
        bool gt = ra.area() > rb.area();
        return gt;
    }
};

Glyph2::Glyph2(gunichar _glyph, GeometryContext *geometryContext)
: glyph{_glyph}
, m_lineGeom{GL_LINES, geometryContext}
, m_fillGeom{GL_TRIANGLES, geometryContext}
, m_tex{0}
, m_ctx{geometryContext}
{
}

Glyph2::~Glyph2()
{
    if (m_tex > 0) {
        glDeleteTextures(1, &m_tex);
        m_tex = 0;
    }
}


#ifndef DIRECT_SCAN

static int
move_to(const FT_Vector *to, Polygons *lines)
{
    SharedPoly next = std::make_shared<Polygon>();
    std::shared_ptr<PositionDbl> pos = std::make_shared<PositionDbl>(*to);
    next->positions.push_back(pos);
    lines->push_back(next);

    last = pos;
    return 0;
}

static int
line_to(const FT_Vector *to, Polygons *lines)
{
    std::shared_ptr<PositionDbl> pos = std::make_shared<PositionDbl>(*to);
    SharedPoly poly = lines->back();
    poly->positions.push_back(pos);

    last = pos;
    return 0;
}

static int
conic_to( const FT_Vector *control, const FT_Vector *to, Polygons *lines)
{
    std::shared_ptr<PositionDbl> p0 = last;
    std::shared_ptr<PositionDbl> p1 = std::make_shared<PositionDbl>(*control);
    std::shared_ptr<PositionDbl> p2 = std::make_shared<PositionDbl>(*to);
    SharedPoly poly = lines->back();
    poly->quadric_bezier(p0, p1, p2, BEZIER_POINTS);
    last = p2;
    return 0;
}

static int
cubic_to( const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, Polygons *lines)
{
    std::shared_ptr<PositionDbl> p0 = last;
    std::shared_ptr<PositionDbl> p1 = std::make_shared<PositionDbl>(*control1);
    std::shared_ptr<PositionDbl> p2 = std::make_shared<PositionDbl>(*control2);
    std::shared_ptr<PositionDbl> p3 = std::make_shared<PositionDbl>(*to);
    SharedPoly poly = lines->back();
    poly->cubic_bezier(p0, p1, p2, p3, BEZIER_POINTS);
    last = p3;
    return 0;
}
#endif

void
Glyph2::buildLineTriangels(FT_Library library, FT_Face face, FT_UInt glyph_index)
{
    pGlyph = this;
    FT_Outline* outline = &face->glyph->outline;
    p2t::Polygons polys;
    #ifdef DIRECT_SCAN
    // allows using contour info from structure
    //std::cout << std::hex << "0x" << (int)glyph << std::dec
    //          << " points " << outline->n_points
    //          << " contours " << outline->n_contours
    //          << std::endl;
    int startIdx = 0;      // start index for contour
    for (int cont = 0; cont < outline->n_contours; ++cont) {
        int endIdx = outline->contours[cont];       // this index is inclusive
        if (endIdx - startIdx >= 2) {                // ignore those not making a shape e.g. dejavue 'u' anchor ?
            auto poly = std::make_shared<struct p2t::Polygon>();
            poly->addPoints(outline, startIdx, endIdx, cont);
            polys.push_back(poly);
        }
        startIdx = endIdx + 1;
    }
    #else
    FT_Outline_Funcs func_interface;
    func_interface.move_to = (FT_Outline_MoveToFunc)move_to;
    func_interface.line_to = (FT_Outline_LineToFunc)line_to;
    func_interface.conic_to = (FT_Outline_ConicToFunc)conic_to;
    func_interface.cubic_to = (FT_Outline_CubicToFunc)cubic_to;
    func_interface.shift = 0;
    func_interface.delta = 0;
    err = FT_Outline_Decompose(outline, &func_interface, &polys);
    if (err) {
        std::cerr << "No decompose 0x" << std::hex << (int)glyph << std::dec << std::endl;
    }
    #endif
    Color col(1.0f, 1.0f, 1.0f);
    Vector v(0.0f, 1.0f, 0.0f); // make glyphs look up toward light (if need)
    for (auto poly : polys) { // build lines
        auto pLast = std::make_shared<PositionDbl>();
        for (auto p : poly->positions) {
            if (pLast) {
                addLine(pLast, p, col, &v);
            }
            pLast = p;
        }
    }
    try {
        tesselate(polys);
    }
    catch (...) {
        std::cout << "Tesselation failed 0x" << std::hex << (int)glyph << std::dec << std::endl;
    }
}

bool
Glyph2::extractGlyph(FT_Library library, FT_Face face, FT_UInt glyph_index, GLenum textType)
{

//  std::cout << "Glyph indx " << glyph_index << std::endl;

    FT_Error err = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
    if (err) {
        std::cerr << __FILE__ << "::extractGlyph no glyph 0x" << std::hex << (int)glyph << std::dec
                  << " index " << glyph_index << std::endl;
        return false;
    }
    setAdvance(fixed2float(face->glyph->advance.x));
    if (textType == GL_TRIANGLES) {
        buildLineTriangels(library, face, glyph_index);
    }
    else if (textType == GL_QUADS) {
        render2tex(library, face, glyph_index);
    }
    else {  // in case of unknown build all
        std::cout << __FILE__ << "::extractGlyph unknown build " << textType
                  << " " << glyph_index << std::endl;
    }
    return true;                // allow also glyphs without shape e.g. space
}

void
Glyph2::displayLine(Matrix &mv)
{
    if (m_lineGeom.getNumVertex() > 0) {     // allow glyphes without shape e.g. space
        m_lineGeom.display(mv);
    }
}

void
Glyph2::display(Matrix &mv)
{
    if (m_fillGeom.getNumVertex() > 0) {     // allow glyphes without shape e.g. space
        m_fillGeom.display(mv);
    }
}

GLint
Glyph2::bindTexture()
{
    if (m_tex > 0) {
        glBindTexture(GL_TEXTURE_2D, m_tex);
        checkError("TexShaderCtx glBindTextures");
    }
    return m_tex;
}

void
Glyph2::render2tex(FT_Library library, FT_Face face, FT_UInt glyph_index)
{
    FT_Error err = 0;
    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        //face->glyph->bitmap_top = face->glyph->metrics->vertAdvance;
        //face->glyph->bitmap_left = 0;
        err = FT_Render_Glyph(face->glyph,   /* glyph slot  */
                              FT_RENDER_MODE_NORMAL); /* render mode 256 gray levels */
    }
    if (err) {
        std::cerr << "No render 0x" << std::hex << (int)glyph << std::dec << std::endl;
    }
    else {
        glGenTextures(1, &m_tex);
        checkError("TexShaderCtx glGenTextures");
        glBindTexture(GL_TEXTURE_2D, m_tex);
        checkError("TexShaderCtx glBindTextures");

        FT_Size fontMetric = face->size;
        FT_Size_Metrics fontSize = fontMetric->metrics;
        FT_GlyphSlot pglyph = face->glyph;
        FT_Bitmap bmp = pglyph->bitmap;
        GLuint ascender = fixed2float(fontSize.ascender);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
        GLfloat descender = fixed2float(fontSize.descender);
#pragma GCC diagnostic pop
        GLuint rows = bmp.rows;
        GLuint cols = bmp.width;
        GLuint orows = fixed2float(fontSize.height);
        GLuint ocols = fontSize.y_ppem;
        //std::cout << std::hex << "0x" << (int)glyph < <std::dec
        //          << " top: " <<  pglyph->bitmap_top
        //          << " left: " << pglyph->bitmap_left
        //          << " orows: " << orows << " ocols: " << ocols
        //          << " masc: " << asc << " mdesc: " << desc
        //          << std::endl;
        GLuint osize = orows * ocols;
        auto colors{new GLuint[osize]};
        if (colors) {
            memset(colors, 0, osize * sizeof(GLuint));
            for (GLuint row = 0; row < rows; ++row) {
                GLuint rowoffs = row * cols;
                GLuint orowoffs = (ascender - pglyph->bitmap_top + row) * ocols;
                for (GLuint col = 0; col < cols; ++col) {
                    GLuint idx = rowoffs + col;
                    GLuint oidx = orowoffs + (col + pglyph->bitmap_left);
                    unsigned char val = pglyph->bitmap.buffer[idx];
                    //std::cout << std::hex << std::setfill('0') << std::setw(2) << (val & 0xff) ;
                    unsigned int c = 0x0;
                    if (val > 0) {
                        c = 0xffffff00;
                    }
                    colors[oidx] = g_htonl( c | (val & 0xff) );  //  htobe32 Graphics card works in bigendian, replaced gtypes for interop
                }
                //std::cout << std::dec << std::endl;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA
                    , ocols, orows, 0, GL_RGBA
                    , GL_UNSIGNED_BYTE, colors);
            checkError("glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA");
            glGenerateMipmap(GL_TEXTURE_2D);    // required !
            checkError("glGenerateMipmap(GL_TEXTURE_2D)");

            // Setting these min/mag parameters is important, otherwise the result will be all black !!!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // linear shoud give optimal result, weightened to effort
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            checkError("TexShaderCtx glTexParameteri");

            delete[] colors;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

/**
 *
 * @param outline
 * @param geometryContext
 * @return TRUE has shape, FALSE empty shape e.g space
 */
bool
Glyph2::tesselate(p2t::Polygons outline)
{
    bool ret = false;
    if (outline.size() > 0) {
        p2t::Polygons nested;        // build nested structure with holes assigned, here only separate polys
        ret = true;
        outline.sort(CompareByArea());  // sort largest first -> must be outer
        // fine for latin, but we need something better for other languages
        while (!outline.empty()) {
            //std::cout << "Outline size " << outline.size() << std::endl;
            p2t::SharedPoly outer = outline.front();
            outline.pop_front();
            nested.push_back(outer);
            outer->nested(outline);
        }
#ifdef POLY2TRI
        for (SharedPoly nest : nested) {
            std::vector<p2t::Point*> polyline = nest->toTess();
            std::cout << std::hex << "0x" << (int)glyph << std::dec
                      << " bnd " << nest->bounds().info()
                      << std::endl;

            p2t::CDT cdt(polyline);
            for (SharedPoly hole : nest->holes) {
                std::cout << "   add hole" << std::endl;
                std::vector<p2t::Point*> tHole = hole->toTess();
                cdt.AddHole(tHole);
            }
            cdt.Triangulate();
            Color c(1.0f);
            Vector n(0.0f, 1.0f, 0.0f);
            for (auto tri : cdt.GetTriangles()) {
                for (int i = 0; i < 3; ++i) {
                    p2t::Point* p = tri->GetPoint(i);
                    Position pos(p->x, p->y, 0.0);
                    pGlyph->addFillPoint(pos, c, n);
                }
            }
            nest->clearTemp();
        }
#else
        int n = 0;
        GLUtesselator *tess = gluNewTess();
        gluTessNormal(tess, 0.0, 0.0, -1.0);  // our data lies in x-y plane

        gluTessCallback(tess, GLU_TESS_BEGIN, (void (CALLBACK *)())tessBeginCB);
        gluTessCallback(tess, GLU_TESS_END, (void (CALLBACK *)())tessEndCB);
        gluTessCallback(tess, GLU_TESS_ERROR, (void (CALLBACK *)())tessErrorCB);
        gluTessCallback(tess, GLU_TESS_VERTEX, (void (CALLBACK *)())tessVertexCB);
        // If your primitive is self-intersecting, you must also specify a callback to create new vertices:
        gluTessCallback(tess, GLU_TESS_COMBINE, (void (CALLBACK *)())tessCombineCB);
        //gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
        gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
        for (auto poly : nested) {
            gluTessBeginPolygon(tess, nullptr);
            poly->gluTess(tess);
            gluTessEndPolygon(tess);
            ++n;
        }
        gluDeleteTess(tess);
#endif
        create_vao();
    }
    return ret;
}

void
Glyph2::setAdvance(GLfloat advance)
{
    m_advance = advance;
}

void
Glyph2::addLine(std::shared_ptr<PositionDbl> posD, std::shared_ptr<PositionDbl> endD, Color &c, Vector *n)
{
    Position pos;
    pos.x = (GLfloat)posD->x;
    pos.y = (GLfloat)posD->y;
    pos.z = (GLfloat)posD->z;
    Position end;
    end.x = (GLfloat)endD->x;
    end.y = (GLfloat)endD->y;
    end.z = (GLfloat)endD->z;

    m_lineGeom.addLine(pos, end, c, n);
}

void
Glyph2::addFillPoint(const Position& pos, const Color &c, const Vector &n)
{
    //std::cout <<  "addFillPoint" << glyph << " " << c.r << " " << c.g << " " << c.b << std::endl;
    m_fillGeom.addPoint(&pos, &c, &n);
}

GLuint
Glyph2::getNumVertex()
{
    return m_lineGeom.getNumVertex();
}

void
Glyph2::create_vao()
{
    //std::cout << " 0x"
    //          << std::hex << (int)glyph << std::dec
    //          << " fill vertexes: " << m_fillGeom.getNumVertex()
    //          << " line vertexes: " << m_lineGeom.getNumVertex()
    //          << std::endl;

    m_lineGeom.create_vao();
    m_fillGeom.create_vao();
}



} /* namespace gl */
} /* namespace psc */