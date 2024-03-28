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

#include <iostream>
#include <glm/vec4.hpp> // vec4
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>   // euler
#include <glm/gtx/euler_angles.hpp> // eulerAngleYXZ

#include "Matrix.hpp"
#include "NaviContext.hpp"
#include "TextContext.hpp"
#include "Text2.hpp"

namespace psc {
namespace gl {


Text2::Text2(GLenum type, NaviContext *ctx, const psc::gl::ptrFont2& font)
: psc::gl::Geom2::Geom2(GL_TRIANGLES, ctx)
, m_font{font}
, m_shaderCtx{ctx}
, m_textCtx{nullptr}
, m_texID{0}
, m_textType{type}
{
    setMarkable(false); // keep text unmarkable for now as we didn't get the bounds right

}


void
Text2::setText(const Glib::ustring &text)
{
    m_text = text;
}

const Glib::ustring &
Text2::getText()
{
    return m_text;
}

void
Text2::display(NaviContext* context, const Matrix &projView)
{
    display(projView);
}


void
Text2::display(const Matrix &perspView)
{
    if (!m_visible) {
        return;
    }
    GLint restoreProgram = 0;
    if (m_textCtx) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &restoreProgram);
        #ifdef DEBUG
        std::cout << __FILE__ "::display curr"
          << " prog " << restoreProgram << std::endl;
        #endif
        //m_shaderCtx->unuse();   // unuse prev.ctx
        m_textCtx->use();       // prefere a simpler text context for rendering
    }
    if (m_textType == GL_QUADS
      && getNumVertex() == 0) {
        Position pc0(0.0f, 0.0f, 0.0f);
        UV uv0(0.0f, 1.0f); // invert v as bitmap defined in scanline order gl is carthesian
        Position pc1(0.0f, 1.0f, 0.0f);
        UV uv1(0.0f, 0.0f);
        Position pc2(1.0f, 1.0f, 0.0f);
        UV uv2(1.0f, 0.0f);
        Position pc3(1.0f, 0.0f, 0.0f);
        UV uv3(1.0f, 1.0f);
        addPoint(&pc0, nullptr, nullptr, &uv0);
        addPoint(&pc1, nullptr, nullptr, &uv1);
        addPoint(&pc2, nullptr, nullptr, &uv2);
        addPoint(&pc0, nullptr, nullptr, &uv0);
        addPoint(&pc2, nullptr, nullptr, &uv2);
        addPoint(&pc3, nullptr, nullptr, &uv3);
        create_vao();
    }
    //std::wcout << m_text << " phi " << getRotation().getPhi()
    //                     << " theta " << getRotation().getTheta()
    //                     << " psi " << getRotation().getPsi()
    //                     << std::endl;

    glm::mat4 rotation = glm::eulerAngleYXZ(m_rotate.phiRadians(), m_rotate.thetaRadians(), m_rotate.psiRadians());
    glm::mat4 scaling = glm::scale(glm::vec3(m_scale));    // glm::mat4(1.0f),
    glm::mat4 rotationScaling = rotation * scaling;

    if (m_textType == GL_QUADS) {
        prepareTexture();
    }
    //std::cout << "Text2::display \"" << m_text << "\"" << std::endl;
    m_min = Position(999.0f);
    m_max = Position(-999.0f);
    Position p = getPos();
    float height = m_font->getLineHeight();
    if (m_textType == GL_QUADS) {
        height *= 0.035;
    }
    glm::vec4 a(0.0f, -height, 0.0f, 0.0f);
    glm::vec3 lineskip(rotationScaling * a);
    int line = 0;
    for (gunichar c : m_text) {
        if (c == '\n') {   // Handle line break
            ++line;
            p = getPos();
            p += lineskip * (float)line;
        }
        else {
            auto g = m_font->checkGlyph(c, m_ctx, m_textType);
            if (g != nullptr) {
                if (g->getChar() != c) {
                    std::cout << "Searching 0x" << std::hex << (int)c << " got 0x" << (int)g->getChar() << std::dec << std::endl;
                }
                glm::mat4 translation = glm::translate(p);    // glm::mat4(1.0f),
                glm::mat4 model = translation * rotationScaling;

                Matrix mvp;
                if (m_textCtx)
                    mvp = m_textCtx->setModel(perspView, model);
                else
                    mvp = m_shaderCtx->setModel(perspView, model);
                if (m_textType == GL_TRIANGLES) {
                    g->display(mvp);
                }
                else if (m_textType == GL_LINES) {
                    g->displayLine(mvp);
                }
                else {
                    if (g->bindTexture() > 0) { // allow glyphes without shape e.g. space (even if this value comes from a different world)
                        Geom2::display(mvp);
                    }
                    else {
                        std::cout << __FILE__ << ".display tried to use texture for " << c << ", but was not available!" << std::endl;
                    }
                }
                float dist = g->getAdvance();
                if (m_textType == GL_QUADS) {
                    dist *= 0.035;
                }
                glm::vec4 a(dist, 0.0f, 0.0f, 0.0f);
                glm::vec3 charskip(rotationScaling * a);
                p += charskip;
                min(m_min, &p);
                max(m_max, &p);
                Position pmax = p+lineskip;
                Position pmin = p+lineskip;
                min(m_min, &pmax);
                max(m_max, &pmin);
            }
            else {
                std::cerr << "No glyph for 0x" << std::hex << c << "!" << std::dec << std::endl;
            }
        }
    }
    if (m_textType == GL_QUADS) {
        finishTexture();
    }
    // as we captured the position with min & max use here w/o position
    Matrix mvp = m_shaderCtx->setModel(perspView, rotationScaling);
    updateClickBounds(mvp);

    if (m_textCtx) {
        if (restoreProgram > 0) {   // restore previous used seems better
            glUseProgram(restoreProgram);
        }
    }
}


void
Text2::prepareTexture()
{
    if (m_textCtx) {
        m_texID = glGetUniformLocation(m_textCtx->getProgram(), "renderedTexture");
    }
    else {
        m_texID = glGetUniformLocation(m_shaderCtx->getProgram(), "renderedTexture");
    }
    glDepthMask(GL_FALSE);      // depth checking will care about existing values, but not writing new values, this is the best we can make out of transparency
    checkError("glDepthMask GL_FALSE");
    glEnable(GL_BLEND);
    checkError("Font glEnableBlend");
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    checkError("Font glBlendFunc");
    glActiveTexture(GL_TEXTURE0);
    checkError("glActiveTexture texture0");
    glUniform1i(m_texID, 0);
    checkError("glUniform1i m_texID");
}

void
Text2::finishTexture()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    checkError("Font glBindTexture 0");
    glDisable(GL_BLEND);
    checkError("Font glDisableBlend");
    glDepthMask(GL_TRUE);
    checkError("glDepthMask GL_TRUE");
}


void
Text2::setTextContext(TextContext *textCtx)
{
    m_textCtx = textCtx;
}


} /* namespace gl */
} /* namespace psc */