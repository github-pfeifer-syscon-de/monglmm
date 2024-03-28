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

#include <glib.h>
#include <iostream>
#include <math.h>
#include <cmath>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp> // triangleNormal

#include "Font2.hpp"
#include "ShaderContext.hpp"
#include "NaviContext.hpp"

#include "Geom2.hpp"

namespace psc {
namespace gl {



void
checkError(const char *where)
{
#ifdef DEBUG
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        const GLubyte *errmsg = gluErrorString(err);
        g_warning("Glerror: %s %s", where, errmsg);
    }
#endif
}


Geom2::Geom2(GLenum type, GeometryContext *_ctx)
: Displayable(_ctx)
, geometries()
, m_master{nullptr}
, m_type{type}
, debugGeom{nullptr}
, m_sensitivity{0.0f}
, m_min{999.0f}
, m_max{-999.0f}
, m_removeChildren{true}
, m_vao{0}
, m_vertexes()
, m_indexes()
, m_numVertex{0}
, m_stride_size{0}
, m_numIndex{0}
{
    m_vertexes.reserve(100);    // give it some space as we store floats and expect many of them
}


Geom2::~Geom2()
{
    remove();
}

void
Geom2::remove()
{
    // geometries may belong to three categories
    //   - standalone (these we shoud eliminate, has to be whightened for complex structures it may exist a
    //                 external structure to attach points to, avoid duplicated references as these are hard to maintain if they change)
    //   - belonging to a context
    //   - belonging to a geometry
    if (m_master) {
        //std::cout << "remove " << std::hex << this << " from master " << std::hex << m_master << std::endl;
        m_master->removeGeometry(this);
    }
    else {
      if (m_ctx && m_removeFromctx) {
          //std::cout << "remove " << std::hex << this << " from ctx " << std::hex << m_ctx << std::endl;
          m_ctx->removeGeometry(this);
      }
    }
    //std::cout << "remove " << std::hex << this << " size " << geometries.size() << " remove " << (m_removeChildren ? "y" : "n") << std::endl;
    int n = 0;
    for (auto p = geometries.begin(); p != geometries.end();  ++n) {
        auto chld = *p;
        //std::cout << "loop " << std::hex << this << " remove " << std::hex << chld << " pos " << n << std::endl;
        auto lchld = chld.lease();
        if (lchld) {
            lchld->resetMaster();  // prevent iterator hassles, by preventing inner remove from list
        }
        if (m_removeChildren) {
            chld.reset();
        }
        p = geometries.erase(p);
    }
    geometries.clear();
    //std::cout << "remove " << std::hex << this << " destr lsnr " << destructionListeners.size() << std::endl;
    //for (auto p = destructionListeners.begin(); p != destructionListeners.end(); ++p) {
    //    GeometryDestructionListener *lsnr = *p;
    //    lsnr->geometryDestroyed(this);
    //}
    //destructionListeners.clear();
    //std::cout << "remove " << std::hex << this << " delete vertex arr " << std::endl;
    deleteVertexArray();
}

void
Geom2::deleteVertexArray()
{
    /* destroy all the resources we created */
    if (m_vao != 0) {
        //std::cout << "deleteVertexArray " << std::hex << this << " delete vertex arr " << std::endl;
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void
Geom2::resetMaster()
{
    //std::cout << "resetMaster " << std::hex << this << std::endl;
    m_master = nullptr;
}

void
Geom2::setRemoveChildren(bool removeChildren) {
    m_removeChildren = removeChildren;
}

bool
Geom2::isRemoveChildren() {
    return m_removeChildren;
}

void
Geom2::addPoint(
    const Position *p
  , const Color *c
  , const Vector *n
  , const UV *uv
  , const Vector *tangent
  , const Vector *bitangent)
{
    m_vertexes.push_back(p->x);
    m_vertexes.push_back(p->y);
    m_vertexes.push_back(p->z);
    min(m_min, p);
    max(m_max, p);
    if (m_ctx->useColor()) {
        auto defCol{Color(1.0f)};
        auto tp = (c != nullptr ? c : &defCol);
        m_vertexes.push_back(tp->r);
        m_vertexes.push_back(tp->g);
        m_vertexes.push_back(tp->b);
    }
    if (m_ctx->useNormal()) {
        auto defNor{Vector(0.0f, 1.0f, 0.0f)};
        auto tn = (n != nullptr ? n : &defNor);
        m_vertexes.push_back(tn->x);
        m_vertexes.push_back(tn->y);
        m_vertexes.push_back(tn->z);
    }
    if (m_ctx->useUV()) {
        auto defUV{UV(0.0f)};
        auto tuv = (uv != nullptr ? uv : &defUV);
        m_vertexes.push_back(tuv->s);
        m_vertexes.push_back(tuv->t);
    }
    if (m_ctx->useNormalMap()) {
        auto defTan{Vector(1.0f, 0.0f, 0.0f)};
        auto ttan = (tangent != nullptr ? tangent : &defTan);
        m_vertexes.push_back(ttan->x);
        m_vertexes.push_back(ttan->y);
        m_vertexes.push_back(ttan->z);
        auto defBitan{Vector(0.0f, 0.0f, 1.0f)};
        auto tbitan = (bitangent != nullptr ? bitangent : &defBitan);
        m_vertexes.push_back(tbitan->x);
        m_vertexes.push_back(tbitan->y);
        m_vertexes.push_back(tbitan->z);
    }
}


void
Geom2::addIndex(int idx)
{
    m_indexes.push_back(idx);
}


void
Geom2::addIndex(int idx1, int idx2)
{
    m_indexes.push_back(idx1);
    m_indexes.push_back(idx2);
}

void
Geom2::addIndex(int idx1, int idx2, int idx3)
{
    m_indexes.push_back(idx1);
    m_indexes.push_back(idx2);
    m_indexes.push_back(idx3);
}

void
Geom2::addLine(Position &p1, Position &p2, Color &c, Vector *n) {
    addPoint(&p1, &c, n);
    addPoint(&p2, &c, n);
}


// Creates rect from diagonally opposite points
//  works for laying out in pane x-z
void
Geom2::addRect(Position &p1, Position &p2, Color &c)
{
    Position pos[6];
    pos[0] = p1;

    pos[1] = p2;

    pos[2].x = p1.x;
    pos[2].y = p1.y;
    pos[2].z = p2.z;

    pos[3] = p2;

    pos[4] = p1;

    pos[5].x = p2.x;
    pos[5].y = p2.y;
    pos[5].z = p1.z;

    Vector norm;
    if (m_ctx->useNormal()) {
        norm = glm::triangleNormal(pos[0], pos[1], pos[2]);
    } else {
        norm.x = 0.0f;
        norm.y = 1.0f;
        norm.z = 0.0f;
    }
    for (int i = 0; i < 6; ++i) {
        // this realizes flat shading
        addPoint(&pos[i], &c, &norm);
    }
}

// Creates triangle
//   remember right hand rule
void
Geom2::addTri(Position &p1, Position &p2, Position &p3, Color &c)
{
    Position pos[3];
    pos[0] = p1;
    pos[1] = p2;
    pos[2] = p3;

    Vector norm;
    if (m_ctx->useNormal()) {
        norm = glm::triangleNormal(pos[0], pos[1], pos[2]);
    } else {
        norm.x = 0.0f;
        norm.y = 1.0f;
        norm.z = 0.0f;
    }
    for (int i = 0; i < 3; ++i) {
        // this realizes flat shading
        addPoint(&pos[i], &c, &norm);
    }
}

void
Geom2::addCube(float dist, Color &c)
{
    Position p0(-dist, -dist, -dist);
    Position p1(dist, dist, -dist);
    Position p2(dist, -dist, -dist);
    Position p3(-dist, dist, -dist);
    addPoint(&p0, &c);
    addPoint(&p1, &c);
    addPoint(&p2, &c);
    addPoint(&p3, &c);
    Position p4(-dist, -dist, dist);
    Position p5(dist, dist, dist);
    Position p6(dist, -dist, dist);
    Position p7(-dist, dist, dist);
    addPoint(&p4, &c);
    addPoint(&p5, &c);
    addPoint(&p6, &c);
    addPoint(&p7, &c);
    // check these with culling
    addIndex(0, 2, 1); // front low
    addIndex(0, 1, 3); // front high

    addIndex(1, 5, 3); // top low
    addIndex(3, 5, 7); // top high

    addIndex(2, 6, 5); // right low
    addIndex(2, 5, 1); // right high

    addIndex(0, 2, 4); // left low
    addIndex(0, 3, 2); // left high

    addIndex(0, 6, 2); // bottom low
    addIndex(0, 4, 6); // bottom high

    addIndex(4, 5, 6); // back low
    addIndex(4, 7, 5); // back high

}

guint
Geom2::getStrideSize() {
    if (m_stride_size == 0) {
        m_stride_size = sizeof (GLfloat) * 3; // at least we have positions
        if (m_ctx->useColor()) {
            m_stride_size += sizeof (GLfloat) * 3;
        }
        if (m_ctx->useNormal()) {
            m_stride_size += sizeof (GLfloat) * 3;
        }
        if (m_ctx->useUV())  {
            m_stride_size += sizeof (GLfloat) * 2;
        }
        if (m_ctx->useNormalMap()) {
            m_stride_size += sizeof (GLfloat) * 6;
        }
    }
    return m_stride_size;
}

void
Geom2::create_vao() {
    guint buffer;
    GLfloat *data = (GLfloat *) & m_vertexes[0];
    guint sizeof_vertex_data = m_vertexes.size() * sizeof (GLfloat);
    m_numVertex = sizeof_vertex_data / getStrideSize();
    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    checkError("geom bindArr");

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof_vertex_data, data, GL_STATIC_DRAW);
    checkError("geom arrBuf");

    guint struct_offs = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    // as we didnt invent this convention cast int to pointer...
    /* enable and set the position attribute */
    glEnableVertexAttribArray(m_ctx->getPositionIndex());
    glVertexAttribPointer(m_ctx->getPositionIndex(), 3, GL_FLOAT, GL_FALSE,
            getStrideSize(),
            (GLvoid *) struct_offs);
    struct_offs += sizeof (GLfloat) * 3;
    checkError("geom Pos");
    if (m_ctx->useColor()) {
        /* enable and set the color attribute */
        glEnableVertexAttribArray(m_ctx->getColorIndex());
        glVertexAttribPointer(m_ctx->getColorIndex(), 3, GL_FLOAT, GL_FALSE,
                getStrideSize(),
                (GLvoid *) struct_offs);
        struct_offs += sizeof (GLfloat) * 3;
        checkError("geom Color");
    }
    if (m_ctx->useNormal()) {
        //std::cout << "Offs color " << (G_STRUCT_OFFSET (struct vertex_info, color)) << std::endl;
        glEnableVertexAttribArray(m_ctx->getNormalIndex());
        glVertexAttribPointer(m_ctx->getNormalIndex(), 3, GL_FLOAT, GL_FALSE,
                getStrideSize(),
                (GLvoid *) struct_offs);
        struct_offs += sizeof (GLfloat) * 3;
        checkError("geom Normal");
    }
    if (m_ctx->useUV()) {
        //std::cout << "Offs color " << (G_STRUCT_OFFSET (struct vertex_info, color)) << std::endl;
        glEnableVertexAttribArray(m_ctx->getUVIndex());
        glVertexAttribPointer(m_ctx->getUVIndex(), 2, GL_FLOAT, GL_FALSE,
                getStrideSize(),
                (GLvoid *) struct_offs);
        struct_offs += sizeof (GLfloat) * 2;
        checkError("geom UV");
    }
    if (m_ctx->useNormalMap()) {
        //std::cout << "Offs color " << (G_STRUCT_OFFSET (struct vertex_info, color)) << std::endl;
        glEnableVertexAttribArray(m_ctx->getNormMapTangentIndex());
        glVertexAttribPointer(m_ctx->getNormMapTangentIndex(), 3, GL_FLOAT, GL_FALSE,
                getStrideSize(),
                (GLvoid *) struct_offs);
        struct_offs += sizeof (GLfloat) * 3;
        checkError("geom TangentIndex");
        glEnableVertexAttribArray(m_ctx->getNormMapBitangentIndex());
        glVertexAttribPointer(m_ctx->getNormMapBitangentIndex(), 3, GL_FLOAT, GL_FALSE,
                getStrideSize(),
                (GLvoid *) struct_offs);
        struct_offs += sizeof (GLfloat) * 3;
        checkError("geom BitangentIndex");
    }
#pragma GCC diagnostic pop
    /* reset the state; we will re-enable the VAO when needed */
    m_numIndex = m_indexes.size();
    GLuint iboID = 0;
    if (m_numIndex > 0) {
        glGenBuffers(1, &iboID);
        checkError("add index genBuf");
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
        checkError("add index bindBuf");
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndex * sizeof (GLushort), &m_indexes[0], GL_STATIC_DRAW);
        checkError("add buf_data");
        m_indexes.clear();
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* the VBO is referenced by the VAO */
    // the opinion's on that vary a bit "renderdoc" reports this as an api contradiction.
    //   See https://registry.khronos.org/OpenGL/specs/gl/glspec33.core.pdf D.1.2 Deleted Object and Object Name Lifetimes
    //   Following the above doc, and as it works fine after all. This change will break the library compatibility, so keep it.
    //   If you want a "clean" interface move this and the following delete to ::remove
    glDeleteBuffers(1, &buffer);
    if (iboID > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &iboID);
    }
    //std::cerr << "Create vao " << m_vao << std::endl;

    m_vertexes.clear(); // these we don't need any longer
}

guint
Geom2::getNumVertex()
{
    return m_numVertex;
}

guint
Geom2::getNumIndex()
{
    return m_numIndex;
}

void
Geom2::setName(Glib::UStringView name)
{
    m_name = name.c_str();
}

Glib::ustring&
Geom2::getName()
{
    return m_name;
}

void
Geom2::display(NaviContext* context, const Matrix &projView)
{
    glm::mat4 mvp = context->setModel(projView, getTransform());
    display(projView);
    updateClickBounds(mvp);
    for (auto it = geometries.begin(); it != geometries.end(); ) {
        auto cgeo = *it;
        if (cgeo) {
            auto lgeo = cgeo.lease();
            if (lgeo) {
                //if (lgeo->getName() != "") {
                //    std::cout << "display " << lgeo->getName() << std::endl;
                //}
                lgeo->display(context, mvp);
            }
            ++it;
        }
        else {
            it = geometries.erase(it);
        }
    }
}


void
Geom2::display(const Matrix &mvp)
{
    if (!m_visible) {
        return;
    }
    if (m_vao == 0)
    {
        std::cerr << "Geometry vao is 0 num"
                  << " vertexes " <<  m_numVertex
                  << " type " << typeid(this).name()
                  << " name " << m_name
                  << std::endl;
		return;
    }
    if (m_numVertex == 0)
    {
        std::cerr << "Geometry primitivs is 0 " <<  m_numVertex << std::endl;
		return;
    }
    //std::cout << "Geom2::display vao " << m_vao << std::endl;
    glBindVertexArray(m_vao);

    /* draw the vertices or use pointer */
    if (m_numIndex == 0) {
	    //std::cout << "Geom2::display arr " << std::endl;
        glDrawArrays(m_type, 0, m_numVertex);
    }
    else {
	    //std::cout << "Geom2::display idx " << m_numIndex << " type " << m_type << std::endl;
        glDrawElements(m_type, m_numIndex, GL_UNSIGNED_SHORT, 0);
    }

    /* we finished using the buffers */
#ifdef DEBUG
    glBindVertexArray (0);
#endif
	//std::cout << "Geom2::display end " << std::endl;
}

void
Geom2::addGeometry(aptrGeom2 geo)
{
    if (geo.get() == this) {
        std::cerr << "Adding geometry to itself " << typeid(geo.get()).name() << "!" << std::endl;
        return;     // or throw exception ?
    }
    auto lgeo = geo.lease();
    if (lgeo) {
        if (lgeo->m_master == this) {     // already contained (cheaper than iterating)
            return;
        }
        if (lgeo->m_master != nullptr) {  // only keep once in tree
            lgeo->m_master->removeGeometry(lgeo.get());
        }
        lgeo->setMaster(this);
    }
    geometries.push_back(geo);
}

void
Geom2::setMaster(Geom2* geo)
{
    m_master = geo;
}


void
Geom2::removeGeometry(Geom2* geo)
{
    //std::cout << "removeGeometry " << std::hex << geo << " from " << std::hex << this << " size "  << std::dec  << geometries.size() << std::endl;
    for (auto i = geometries.begin(); i != geometries.end(); ) {
        auto cgeo = *i;
        if (cgeo.get() == geo) {
            geo->m_master = nullptr;
            i = geometries.erase(i);
        }
        else {
            ++i;
        }
    }
}

std::list<Displayable *> &
Geom2::getGeometries()
{
    std::cout << "Geom2::getGeometries not implemented!!!" << std::endl;
    return m_legacyGeometries;
}

void
Geom2::addSphere(float radius, unsigned int rings, unsigned int sectors)
{
// thanks to (meanwhile heavily modified)
// https://stackoverflow.com/questions/5988686/creating-a-3d-sphere-in-opengl-using-visual-c
    unsigned int r, s;

    // as these values are expensive and will get used often precalculate
    double sinLon[sectors];
    double cosLon[sectors];
    double tCosLon[sectors];
    double tSinLon[sectors];
    for(s = 0; s < sectors; s++) {
        double const sms = ((double)s / (double)(sectors-1));
        double const lon = 2.0 * M_PI * sms;
        sinLon[s] = std::sin(lon);
        cosLon[s] = std::cos(lon);
        double const tlon = lon + M_PI_2;     // tangent is the normal at lon offset 90°
        tSinLon[s] = std::sin(tlon);
        tCosLon[s] = std::cos(tlon);
    }
    for(r = 0; r < rings; r++) {
        double const rmr = ((double)r / (double)(rings-1));
        double const lat = M_PI * rmr;
        double const sinLat = std::sin(lat);    // the trig(double) is more accurate than needed (but as we precalculate values that shoud not matter)
        double const sinLat2 = std::sin(-M_PI_2 + lat); // -PI_2 makes the latitudes to mach equator
        double const btlat = lat + M_PI_2;    // bitangent is the normal at lat offset 90°
        double const btSinLat = std::sin(btlat);
        double const btSinLat2 = std::sin(-M_PI_2 + btlat);
        for(s = 0; s < sectors; s++) {
            double const sms = ((double)s / (double)(sectors-1));
            double const x = sinLon[s] * sinLat;    // match definition of texture at 180° from greenwich and wrap (date border)
            double const y = sinLat2;
            double const z = cosLon[s] * sinLat;


            Vector normal{x, y, z};
            UV uv{sms,            // u matches our geometry above
                         1.0f-rmr};		 // v inverted to match our bottom up geometry, (images defined top down)
            Position position{x * radius, y * radius, z * radius};
            Vector tangent{tSinLon[s] * sinLat, y, tCosLon[s] * sinLat};
            Vector bitangent{sinLon[s] * btSinLat, btSinLat2, cosLon[s] * btSinLat};
            addPoint(&position, nullptr, &normal, &uv, &tangent, &bitangent);
            if (debugGeom != nullptr) {
                // debug vectors
                Color red(1.0f, 0.0f, 0.0f);
                Vector nm(0.0f, 1.0f, 0.0f);
                Position p0 = position * 1.1f;
                Position p1 = p0 + normal;
                debugGeom->addLine(p0, p1, red, &nm);
                Color gn(0.0f, 1.0f, 0.0f);
                Position t = p0 + tangent;
                debugGeom->addLine(p0, t, gn, &nm);
                Color bl(0.0f, 0.0f, 1.0f);
                Position b = p0 + bitangent;
                debugGeom->addLine(p0, b, bl, &nm);
            }
        }
    }

    m_indexes.reserve(rings * sectors * 6);
    for(r = 0; r < rings; r++) {
        for(s = 0; s < sectors; s++) {
            GLushort i0 = (GLushort)(r * sectors + s);
            GLushort i1 = (GLushort)(r * sectors + (s+1));
            GLushort i2 = (GLushort)((r+1) * sectors + (s+1));
            GLushort i3 = (GLushort)((r+1) * sectors + s);
            m_indexes.push_back(i0);    // first triangle
            m_indexes.push_back(i1);
            m_indexes.push_back(i2);
            m_indexes.push_back(i2);    // second triangle
            m_indexes.push_back(i3);
            m_indexes.push_back(i0);
        }
    }
    if (debugGeom != nullptr) {
        debugGeom->create_vao();
    }
}

// tangents,bitangent for dynamic geometry
//    for(r = 0; r < rings; ++r) {
//        for(s = 0; s < sectors; s+=2) {
//            GLushort i0 = (GLushort)(r * sectors + s);
//            GLushort i1 = (GLushort)(r * sectors + (s+1));
//            GLushort i2 = (GLushort)((r+1) * sectors + (s+1));
//            GLushort i3 = (GLushort)((r+1) * sectors + s);
//            // thanks to http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/
//            // Shortcuts for vertices
//            glm::vec3 & v0 = vertices[i0].position;
//            glm::vec3 & v1 = vertices[i1].position;
//            glm::vec3 & v2 = vertices[i2].position;
//
//            // Shortcuts for UVs
//            glm::vec2 & uv0 = vertices[i0].uv;
//            glm::vec2 & uv1 = vertices[i1].uv;
//            glm::vec2 & uv2 = vertices[i2].uv;
//
//            // Edges of the triangle : position delta
//            glm::vec3 deltaPos1 = v1-v0;
//            glm::vec3 deltaPos2 = v2-v0;
//
//            // UV delta
//            glm::vec2 deltaUV1 = uv1-uv0;
//            glm::vec2 deltaUV2 = uv2-uv0;
//
//            float rf = 1.0f;
//            if (deltaUV1.x != 0.0f && deltaUV2.y != 0.0f) {
//                rf = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
//            }
//            glm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*rf;
//            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x)*rf;
//
//            //std::cout << r << " " << s << " tang " << tangent.x << ", " << tangent.y << ", " << tangent.z << std::endl;
//            //std::cout << r << " " << s << " bitan " << bitangent.x << ", " << bitangent.y << ", " << bitangent.z << std::endl;
//            vertices[i0].tangent += tangent;
//            vertices[i0].bitangent += bitangent;
//        }
//    }
//    for (auto& vert : vertices) {
//        glm::vec3 & n = vert.normal;
//        glm::vec3 & t = vert.tangent;
//        glm::vec3 & b = vert.bitangent;
//
//        // Gram-Schmidt orthogonalize
//        t = glm::normalize(t - n * glm::dot(n, t));
//
//        // Calculate handedness
//        if (glm::dot(glm::cross(n, t), b) < 0.0f) {
//            t = t * -1.0f;
//        }
//        //b = glm::normalize(b);  // this seems necassary to get a equaly sized vector, for dynamic geometry
//        //t *= 6.0f;  // account for the myterious sum in tut 13
//        //b *= 6.0f;
//
//        addVertex(vert);
//    }


void
Geom2::setDebugGeometry(Geom2* geo)
{
    debugGeom = geo;
}
/*
 * get the transform for geometry that includes all parent transforms.
 */
Matrix
Geom2::getConcatTransform()
{
    if (m_master == nullptr) {
        return m_transform;
    }
    else {
        // possible optimization store concat and invalidate if parent changes ...
        return m_master->getConcatTransform() * m_transform;    // the order for this matters
    }
}

void
Geom2::min(Position &set, const Position* pos)
{
    set.x = std::min(set.x, pos->x);
    set.y = std::min(set.y, pos->y);
    set.z = std::min(set.z, pos->z);
}

void
Geom2::max(Position &set, const Position* pos)
{
    set.x = std::max(set.x, pos->x);
    set.y = std::max(set.y, pos->y);
    set.z = std::max(set.z, pos->z);
}

void
Geom2::updateClickBounds(glm::mat4 &mvp)
{
    if (!m_markable) {
        return;
    }
    glm::vec4 pm1(m_min.x, m_min.y, m_min.z, 1.0f);
    glm::vec4 pm2(m_max.x, m_min.y, m_min.z, 1.0f);
    glm::vec4 pm3(m_max.x, m_max.y, m_min.z, 1.0f);
    glm::vec4 pm4(m_min.x, m_max.y, m_min.z, 1.0f);
    glm::vec4 pm5(m_min.x, m_min.y, m_max.z, 1.0f);
    glm::vec4 pm6(m_max.x, m_min.y, m_max.z, 1.0f);
    glm::vec4 pm7(m_max.x, m_max.y, m_max.z, 1.0f);
    glm::vec4 pm8(m_min.x, m_max.y, m_max.z, 1.0f);
    glm::vec4 pv1 = mvp * pm1;
    glm::vec4 pv2 = mvp * pm2;
    glm::vec4 pv3 = mvp * pm3;
    glm::vec4 pv4 = mvp * pm4;
    glm::vec4 pv5 = mvp * pm5;
    glm::vec4 pv6 = mvp * pm6;
    glm::vec4 pv7 = mvp * pm7;
    glm::vec4 pv8 = mvp * pm8;
    Position p1(glm::vec3(pv1)/pv1.w);   // to ogl view box
    Position p2(glm::vec3(pv2)/pv2.w);
    Position p3(glm::vec3(pv3)/pv3.w);
    Position p4(glm::vec3(pv4)/pv4.w);
    Position p5(glm::vec3(pv5)/pv5.w);
    Position p6(glm::vec3(pv6)/pv6.w);
    Position p7(glm::vec3(pv7)/pv7.w);
    Position p8(glm::vec3(pv8)/pv8.w);
    v_min = Position(999.0f, 999.0f, 999.0f);
    v_max = Position(-999.0f, -999.0f, -999.0f);
    min(v_min, &p1); max(v_max, &p1);
    min(v_min, &p2); max(v_max, &p2);
    min(v_min, &p3); max(v_max, &p3);
    min(v_min, &p4); max(v_max, &p4);
    min(v_min, &p5); max(v_max, &p5);
    min(v_min, &p6); max(v_max, &p6);
    min(v_min, &p7); max(v_max, &p7);
    min(v_min, &p8); max(v_max, &p8);
    //std::cout << "updateClick bounds " << std::hex << this << std::dec
    //          << " min " << v_min.x << " " << v_min.y << " " << v_min.z
    //          << " max " << v_max.x << " " << v_max.y << " " << v_max.z << std::endl;
    // coud distribute upwards but for the moment this seems not necessary
}

/**
 * gets actual index of vertexes e.g. to be used by addIndex
 */
unsigned int
Geom2::getVertexIndex() {
    return m_vertexes.size() / (getStrideSize() / sizeof (GLfloat));
}

void
Geom2::addCylinder(float radius, float start, float end, Color *c, unsigned int approx) {
    short idxs = getVertexIndex();  // allow stacking figures
    Position ps(0.0f, start, 0.0f);
    Position pe(0.0f, end, 0.0f);
    Vector ns(0.0f,  1.0f, 0.0f);       // normal start
    Vector ne(0.0f, -1.0f, 0.0f);       // normal end
    addPoint(&ps, c, &ns);
    addPoint(&pe, c, &ne);
    auto si{new double[approx]};
    auto co{new double[approx]};
    for (unsigned int i = 0; i < approx; ++i) {
        double r = 2.0 * G_PI * (double)i / (double)approx;    // approximate circle by approx segments
        si[i] = sin(r);
        co[i] = cos(r);
    }

    short idx = idxs + 2;
    for (unsigned int i = 0; i <= approx; ++i) {
        //Vector n(si, -0.5f + si / 2.0f, co);       // normal side + give some shine (y) ...
        double sin = si[i % approx];
        double cos = co[i % approx];
        Position p0(sin * radius, start, cos * radius);
        Position p1(sin * radius, end, cos * radius);
        double sin2 = si[(i+1) % approx];
        double cos2 = co[(i+1) % approx];
        Position p2(sin2 * radius, end, cos2 * radius);
        Vector n = glm::triangleNormal(p0, p1, p2);

        addPoint(&p0, c, &n);
        addPoint(&p1, c, &n);
        addPoint(&p0, c, &ns);
        addPoint(&p1, c, &ne);
        if (i > 0) {
            addIndex(idx + 0, idx - 4, idx - 3);   // side bottom
            addIndex(idx + 0, idx - 3, idx + 1);   // side top
            addIndex(idxs+0, idx - 2, idx + 2);    // bottom triangle
            addIndex(idx + 3, idx - 1, idxs+1);    // top triangle
        }
        idx += 4;
    }
    delete[] si;
    delete[] co;
}

bool
Geom2::hit(float x, float y)
{
    if (!m_markable || !m_visible) {
        return false;
    }
    if (x >= v_min.x - m_sensitivity
     && x <= v_max.x + m_sensitivity
     && y >= v_min.y - m_sensitivity
     && y <= v_max.y + m_sensitivity) {
        return true;
    }
    return false;
}

void
Geom2::setVisible(bool visible)
{
    Displayable::setVisible(visible);
    for (auto i = geometries.begin(); i != geometries.end(); ) {
        auto g = *i;
        if (g) {
            auto lg = g.lease();
            if (lg) {
                lg->setVisible(visible);
            }
            ++i;
        }
        else {  // remove obsolet
            i = geometries.erase(i);
        }
    }
}

void
Geom2::setSensitivity(float sensitivity)
{
    m_sensitivity =  sensitivity;
}

//void
//Geom2::addDestructionListener(GeometryDestructionListener *listener)
//{
//    destructionListeners.push_back(listener);
//}

//void
//Geom2::removeDestructionListener(GeometryDestructionListener *listener)
//{
//    destructionListeners.remove(listener);
//}

Position &
Geom2::getViewMin()
{
    return v_min;
}

Position &
Geom2::getModelMin()
{
    return m_min;
}

Position &
Geom2::getModelMax()
{
    return m_max;
}


Tex::Tex()
: m_tex{0}
{
}

Tex::~Tex()
{
    if (m_tex > 0) {
        glDeleteTextures(1, &m_tex);
        m_tex = 0;
    }
}

void
Tex::create(GLuint width, GLuint height, const void *data, GLuint type)
{
    glGenTextures(1, &m_tex);
    checkError("TexShaderCtx glGenTextures");
    glBindTexture(GL_TEXTURE_2D, m_tex);
    // Indicate that pixel rows are tightly packed
    //  (defaults to stride of 4 which is kind of only good for RGBA )
    glPixelStorei(GLenum(GL_UNPACK_ALIGNMENT), 1);
    checkError("TexShaderCtx glBindTextures");
    glTexImage2D(GL_TEXTURE_2D, 0, type
                , width, height, 0, type
                , GL_UNSIGNED_BYTE, data);
    checkError("TexShaderCtx glTexImage2D");
    // Setting these min/mag parameters is important, otherwise the result will be all black !!!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getMagFilter());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getMinFilter());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, getTexWrap());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, getTexWrap());
    checkError("TexShaderCtx glTexParameteri");
    // Create mipmaps for this texture for better image quality
    glGenerateMipmap(GLenum(GL_TEXTURE_2D));
    checkError("TexShaderCtx glGenerateMipmap");

    glBindTexture(GL_TEXTURE_2D, 0);
}

GLint
Tex::getMagFilter()
{
    return GL_LINEAR;       // linear shoud give optimal result, weightened to effort
}

GLint
Tex::getMinFilter()
{
	return GL_LINEAR_MIPMAP_LINEAR;
}

GLint
Tex::getTexWrap()
{
    return GL_CLAMP;
}

void
Tex::use(GLuint textureUnit)
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_tex);
}

void
Tex::unuse() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint
Tex::getTex()
{
    return m_tex;
}

void
Tex::create(Glib::RefPtr<Gdk::Pixbuf> pix)
{
	GLint maxTextureSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	if (pix->get_width() > maxTextureSize ||
		pix->get_height() > maxTextureSize) {	// Keep reported limit
		printf("Scale w %d h %d to %d GL_MAX_TEXTURE_SIZE\n", pix->get_width(), pix->get_height(), maxTextureSize);
		pix = pix->scale_simple(maxTextureSize, maxTextureSize, Gdk::InterpType::INTERP_BILINEAR);
	}
	#if USE_GLES
	// more sensitive to power of two or not,
	// as it seems not true 2048x1024 is accept nicely
	// but more sensitive to weired UV coords
	// e.g. for raspi GL_ARB_texture_non_power_of_two extension is indicated
	#endif

	const guint8* data = pix->get_pixels();	// use const to avoid copying
    //std::cout  << "pix width: " << pix->get_width()
    //           << " height: " << pix->get_height()
    //           << " channels: " << pix->get_n_channels()
    //           << " bits: " << pix->get_bits_per_sample()
    //           << " alpha: " << (pix->get_has_alpha() ? "yes" : "no")
    //           << " pix: 0x" << std::hex << data << std::dec
    //           << std::endl;

    create(pix->get_width(), pix->get_height(), data,
            pix->get_has_alpha() ? GL_RGBA : GL_RGB);	// coud use  GL_SRGB8_ALPHA8 /  GL_SRGB8 but for now RGB spares use gamma correction in shader ...
}

Tex *
Tex::fromResource(const std::string res_name)
{
    Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_resource(res_name);
    Tex *tex = new Tex();
    tex->create(pix);
    return tex;
}

Tex *
Tex::fromFile(const std::string file_name)
{
    Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create_from_file(file_name);
    Tex *tex = new Tex();
    tex->create(pix);
    return tex;
}



} /* namespace gl */
} /* namespace psc */