/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

#include <cstdint>
#include <ostream>
#include "util.h"

using namespace std;

struct Color final
{
    float r, g, b, a; /// a is opacity -- 0 is transparent and 1 is opaque
    Color(float v, float a = 1)
    {
        r = g = b = v;
        this->a = a;
    }
    Color()
    {
        r = g = b = a = 0;
    }
    Color(float r, float g, float b, float a = 1)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }
    uint8_t ri() const
    {
        return ifloor(limit(r * 256.0f, 0.0f, 255.0f));
    }
    uint8_t gi() const
    {
        return ifloor(limit(g * 256.0f, 0.0f, 255.0f));
    }
    uint8_t bi() const
    {
        return ifloor(limit(b * 256.0f, 0.0f, 255.0f));
    }
    uint8_t ai() const
    {
        return ifloor(limit(a * 256.0f, 0.0f, 255.0f));
    }
    void ri(uint8_t v)
    {
        r = (unsigned)v * (1.0f / 255.0f);
    }
    void gi(uint8_t v)
    {
        g = (unsigned)v * (1.0f / 255.0f);
    }
    void bi(uint8_t v)
    {
        b = (unsigned)v * (1.0f / 255.0f);
    }
    void ai(uint8_t v)
    {
        a = (unsigned)v * (1.0f / 255.0f);
    }
    friend Color scale(Color l, Color r)
    {
        return Color(l.r * r.r, l.g * r.g, l.b * r.b, l.a * r.a);
    }
    friend Color scale(float l, Color r)
    {
        return Color(l * r.r, l * r.g, l * r.b, r.a);
    }
    friend Color scale(Color l, float r)
    {
        return Color(l.r * r, l.g * r, l.b * r, l.a);
    }
    friend ostream & operator <<(ostream & os, const Color & c)
    {
        return os << "RGBA(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
    }
};

template <>
inline const Color interpolate<Color>(const float t, const Color a, const Color b)
{
    return Color(interpolate(t, a.r, b.r), interpolate(t, a.g, b.g), interpolate(t, a.b, b.b), interpolate(t, a.a, b.a));
}

#endif // COLOR_H_INCLUDED
