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
#include "image.h"
#include "png_decoder.h"
#include "platform.h"
#include <cstring>
#include <iostream>

Image::Image(wstring resourceName)
{
    try
    {
        shared_ptr<Reader> preader = getResourceReader(resourceName);
        PngDecoder decoder(*preader);
        data = shared_ptr<data_t>(new data_t(decoder.removeData(), decoder.width(), decoder.height(), TopToBottom));
    }
    catch(IOException &e)
    {
        throw ImageLoadError(e.what());
    }
}

Image::Image(unsigned w, unsigned h)
{
    data = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel * w * h], w, h, TopToBottom));
    memset((void *)data->data, 0, BytesPerPixel * w * h);
}

Image::Image(Color c)
{
    data = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel], 1, 1, TopToBottom));
    setPixel(0, 0, c);
}

Image::Image()
    : data(nullptr)
{
}

Image::data_t::~data_t()
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");
    if(texture != 0)
    {
        glDeleteTextures(1, (const GLuint *)&texture);
    }
    delete []data;
}

void Image::setPixel(int x, int y, Color c)
{
    if(!data)
    {
        return;
    }

    data->lock.lock();

    if(data->rowOrder == BottomToTop)
    {
        y = data->h - y - 1;
    }

    if(y < 0 || (unsigned)y >= data->h || x < 0 || (unsigned)x >= data->w)
    {
        data->lock.unlock();
        return;
    }

    copyOnWrite();
    data->textureValid = false;
    uint8_t *pixel = &data->data[BytesPerPixel * (x + y * data->w)];
    pixel[0] = c.ri();
    pixel[1] = c.gi();
    pixel[2] = c.bi();
    pixel[3] = c.ai();
    data->lock.unlock();
}

Color Image::getPixel(int x, int y) const
{
    if(!data)
    {
        return Color();
    }

    Color retval;
    data->lock.lock();

    if(data->rowOrder == BottomToTop)
    {
        y = data->h - y - 1;
    }

    if(y < 0 || (unsigned)y >= data->h || x < 0 || (unsigned)x >= data->w)
    {
        data->lock.unlock();
        return Color();
    }

    uint8_t *pixel = &data->data[BytesPerPixel * (x + y * data->w)];
    retval.ri(pixel[0]);
    retval.gi(pixel[1]);
    retval.bi(pixel[2]);
    retval.ai(pixel[3]);
    data->lock.unlock();
    return retval;
}

void Image::bind() const
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");

    if(!data)
    {
        unbind();
        return;
    }

    data->lock.lock();
    setRowOrder(BottomToTop);

    if(data->textureValid)
    {
        glBindTexture(GL_TEXTURE_2D, data->texture);
        data->lock.unlock();
        return;
    }

    if(data->texture == 0)
    {
        glGenTextures(1, (GLuint *)&data->texture);
    }

    glBindTexture(GL_TEXTURE_2D, data->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0);
    glPixelTransferf(GL_ALPHA_BIAS, 0.0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data->w, data->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)data->data);
    data->textureValid = true;
    data->lock.unlock();
}

void Image::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Image::setRowOrder(RowOrder newRowOrder) const
{
    if(data->rowOrder == newRowOrder)
    {
        return;
    }

    data->rowOrder = newRowOrder;

    for(unsigned y1 = 0, y2 = data->h - 1; y1 < y2; y1++, y2--)
    {
        swapRows(y1, y2);
    }
}

void Image::swapRows(unsigned y1, unsigned y2) const
{
    size_t index1 = y1 * BytesPerPixel * data->w, index2 = y2 * BytesPerPixel * data->w;

    for(size_t i = 0; i < data->w * BytesPerPixel; i++)
    {
        uint8_t t = data->data[index1];
        data->data[index1++] = data->data[index2];
        data->data[index2++] = t;
    }
}

void Image::copyOnWrite()
{
    if(data.unique())
    {
        return;
    }

    shared_ptr<data_t> newData = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel * data->w * data->h], data));

    for(size_t i = 0; i < BytesPerPixel * data->w * data->h; i++)
    {
        newData->data[i] = data->data[i];
    }

    data->lock.unlock();
    data = newData;
    data->lock.lock();
}

