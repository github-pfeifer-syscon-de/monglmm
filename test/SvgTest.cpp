/*
 * Copyright (C) 2025 RPf 
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
#include <cstdlib>
#include <cstdint>
#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/trigonometric.hpp>  //radians
#include <psc_format.hpp>
#include <fstream>

#include <GeometryContext.hpp>
#include <Geom2.hpp>

// This resembles the graph rendering to create a svg-icon.

class SvgContext
: public GeometryContext
{
public:
    SvgContext()
    : GeometryContext()
    {
    }
    virtual ~SvgContext() = default;

    bool useNormal() override
    {
        return true;
    }

    bool useColor() override
    {
        return true;
    }

    bool useUV() override
    {
        return false;
    }
};

class SvgGeom
: public psc::gl::Geom2
{
public:
    SvgGeom(SvgContext *ctx)
    :  psc::gl::Geom2(GL_TRIANGLES, ctx)
    {
    }
    virtual ~SvgGeom()
    {
    }

    void put(std::fstream& out, Matrix& mvp, Vector& light)
    {
        out << "<g>" << std::endl;
        for (size_t i = 0; i < m_vertexes.size(); i += 27) {    // a triangle consists of 27 values 3*(3 pos, 3 color, 3normal)
            glm::vec4 pos0{m_vertexes[i+0], m_vertexes[i+1], m_vertexes[i+2], 1.0f};
            glm::vec4 proj0 = mvp * pos0;
            glm::vec2 scr0{proj0.x / proj0.w, proj0.y / proj0.w};
            //std::cout << "x " << scr.x << " y " << scr.y << std::endl;
            glm::vec4 pos1{m_vertexes[i+9], m_vertexes[i+10], m_vertexes[i+11], 1.0f};
            glm::vec4 proj1 = mvp * pos1;
            glm::vec2 scr1{proj1.x / proj1.w, proj1.y / proj1.w};
            glm::vec4 pos2{m_vertexes[i+18], m_vertexes[i+19], m_vertexes[i+20], 1.0f};
            glm::vec4 proj2 = mvp * pos2;
            glm::vec2 scr2{proj2.x / proj2.w, proj2.y / proj2.w};

            glm::vec3 color{m_vertexes[i+3], m_vertexes[i+4], m_vertexes[i+5]};
            glm::vec3 normal{m_vertexes[i+6], m_vertexes[i+7], m_vertexes[i+8]};
            float x = glm::dot(glm::normalize(light), glm::normalize(normal));
            x = (x < 0.2f) ? 0.2f : x;
            auto cx = color * x;

            std::string hexColor = psc::fmt::format("#{:02x}{:02x}{:02x}", static_cast<int>(cx.r * 255.0f), static_cast<int>(cx.g * 255.0f), static_cast<int>(cx.b * 255.0f));
            out << "<path d=\"M" << std::fixed << scr0.x << "," << scr0.y
                            << " " << scr1.x << "," << scr1.y
                            << " " << scr2.x << "," << scr2.y << " \""
                            << " style=\"fill:" << hexColor << ";"
                            << "stroke:" << hexColor <<";stroke-width: 0;\" />" << std::endl;

        }
        out << "</g>" << std::endl;
    }
};

class SvgTest {
public:
    std::fstream m_out;
    SvgContext ctx;
    float stripeWidth{0.3f};
    static constexpr gfloat diagramHeight{0.7f};
    static constexpr gfloat diagramWidth{2.0f};
    Position pos{-1.0f, -2.8f, 0.0f};
    static constexpr Vector light{0.f, -1.f, 0.f};
    static constexpr Position initPos{0.0f, 4.4f, 8.0f};
    Rotational initAngel{-0.0f, 0.0f, 0.0f};
    Matrix projection = glm::perspective(
            glm::radians(glm::radians(60.0f)),  // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
            0.75f,                  // Aspect Ratio.
            0.1f,                   // Near clipping plane. Keep as big as possible, or you'll get precision issues.
            100.0f);                // Far clipping plane. Keep as little as possible.

    float verticalAngle = initAngel.phiRadians();
    float horizontalAngle = initAngel.thetaRadians();
    float cosVert = std::cos(verticalAngle);
    float sinVert = std::sin(verticalAngle);
    float sinHorz = std::sin(horizontalAngle);
    float cosHorz = std::cos(horizontalAngle);
    glm::vec3 direction{cosVert * sinHorz, sinVert, cosVert * cosHorz};
    glm::vec3 right = glm::vec3{std::sin(horizontalAngle - (float)M_PI_2), 0.0f, std::cos(horizontalAngle - (float)M_PI_2)};
    glm::vec3 up = glm::cross(right, direction);
    Matrix view = glm::lookAt(
            initPos,           // Camera is here
            initPos+direction, // better than tweaking the sign of direction is change inital rotation to 180.
            up);
    Matrix projView = projection * view;

    SvgTest()
    {
        m_out = std::fstream{"monglmm.svg", m_out.trunc | m_out.out};
        m_out << "<svg height=\"80mm\" width=\"80mm\" viewBox=\"0 0 80 80\" xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
    }
    virtual ~SvgTest()
    {
        m_out << "</svg>" << std::endl;
        m_out.close();
    }

    gfloat
    index2x(guint size, guint i)
    {
        return ((gfloat) i / (gfloat)(size-1)) * diagramWidth + pos.x;
    }

    gfloat
    value2y(gfloat val)
    {
        return val * diagramHeight + pos.y;
    }

    float
    value2z(gfloat val)
    {
        return val + pos.z;
    }

    void
    render(std::array<float, 10>& values, Color& c)
    {
        SvgGeom lgeom(&ctx);
        //lgeom->deleteVertexArray();
        for (guint i = 0; i < values.size() - 1; ++i) {
            Position p1(index2x(values.size(), i    ), value2y(values[i    ]), value2z(0.0));
            Position p2(index2x(values.size(), i + 1), value2y(values[i + 1]), value2z(stripeWidth));
            lgeom.addRect(p1, p2, c);
        }
        glm::vec3 l{light};
        lgeom.put(m_out, projView, l);
    }

    void
    base()
    {
        SvgGeom lgeom(&ctx);
        //lgeom->deleteVertexArray();
        Color gray(0.15f, 0.15f, 0.15f);
        Position p1(index2x(10,  0), value2y(0.0f), value2z(0.0));
        Position p2(index2x(10, 10), value2y(0.0f), value2z(stripeWidth));
        lgeom.addRect(p1, p2, gray);
        Position p3(index2x(10,  0), value2y(0.0f), value2z(0.0));
        Position p4(index2x(10,  0), value2y(1.0f), value2z(stripeWidth));
        lgeom.addRect(p3, p4, gray);
        glm::vec3 l{light};
        lgeom.put(m_out, projView, l);
    }

    void
    next()
    {
        pos += glm::vec3{0.0f, 1.0f, 0.0};
    }

    bool test()
    {
        base();
        std::array<float, 10> bvalues{0.1f, 0.2f, 0.25f, 0.4f, 0.2f, 0.3f, 0.4f, 0.3f, 0.5f, 0.3f};
        Color blue(0.3f, 0.2f, 0.85f);
        render(bvalues, blue);
        std::array<float, 10> yvalues{0.5f, 0.6f, 0.3f, 0.5f, 0.6f, 0.5f, 0.7f, 0.6f, 0.8f, 0.9f};
        Color yellow(0.65f, 0.6f, 0.2f);
        render(yvalues, yellow);
        next();

        base();
        std::array<float, 10> rvalues{0.2f, 0.1f, 0.4f, 0.2f, 0.25f, 0.4f, 0.3f, 0.5f, 0.3f, 0.4f};
        Color red(0.85f, 0.3f, 0.2f);
        render(rvalues, red);
        std::array<float, 10> gvalues{0.6f, 0.5f, 0.4f, 0.3f, 0.5f, 0.6f, 0.5f, 0.7f, 0.9f, 0.8f};
        Color green(0.3f, 0.85f, 0.2f);
        render(gvalues, green);

        return true;
    }
};

/*
 *
 */
int main(int argc, char** argv)
{
    Glib::init();
    SvgTest svgTest;
    if (!svgTest.test()) {
        return 1;
    }

    return 0;
}

