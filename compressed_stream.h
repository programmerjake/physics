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
#ifndef COMPRESSED_STREAM_H_INCLUDED
#define COMPRESSED_STREAM_H_INCLUDED

#include <deque>
#include "stream.h"
#include <iostream>

class LZ77FormatException final : public IOException
{
public:
    LZ77FormatException()
        : IOException("LZ77 format error")
    {
    }
};

struct LZ77CodeType final
{
    static constexpr int lengthBits = 6, offsetBits = 16 - lengthBits;
    static constexpr size_t maxLength = (1 << lengthBits) - 1, maxOffset = (1 << offsetBits) - 1;
    size_t length;
    size_t offset;
    uint8_t nextByte;
    LZ77CodeType(size_t length, size_t offset, uint8_t nextByte)
        : length(length), offset(offset), nextByte(nextByte)
    {
    }
    LZ77CodeType() // initialize with EOF
        : length(0), offset(1), nextByte(0)
    {
    }
    LZ77CodeType(uint8_t nextByte)
        : length(0), offset(0), nextByte(nextByte)
    {
    }
    bool hasNextByte()
    {
        return length != 0 || offset == 0;
    }
    bool eof()
    {
        return length == 0 && offset != 0;
    }
    static LZ77CodeType read(Reader &reader)
    {
        LZ77CodeType retval;
        retval.nextByte = reader.readByte();

        try
        {
            uint16_t v = reader.readU16();
            retval.length = v >> offsetBits;
            retval.offset = v & maxOffset;
        }
        catch(EOFException &e)
        {
            throw LZ77FormatException();
        }

        return retval;
    }
    void write(Writer &writer)
    {
        //cout << "Write code : 0x" << hex << (unsigned)nextByte << dec << " : length : " << length << " : offset : " << offset << endl;
        writer.writeByte(nextByte);
        uint16_t v = (offset & maxOffset) | (length << offsetBits);
        writer.writeU16(v);
    }
};

class ExpandReader final : public Reader
{
private:
    shared_ptr<Reader> reader;
    static constexpr size_t bufferSize = LZ77CodeType::maxOffset + 1;
    circularDeque<uint8_t, bufferSize + 2> buffer;
    LZ77CodeType currentCode;
public:
    ExpandReader(shared_ptr<Reader> reader)
        : reader(reader)
    {
    }
    ExpandReader(Reader &reader)
        : ExpandReader(shared_ptr<Reader>(&reader, [](Reader *) {}))
    {
    }
    virtual ~ExpandReader()
    {
    }
    virtual uint8_t readByte() override
    {
        while(currentCode.eof())
        {
            currentCode = LZ77CodeType::read(*reader);
        }

        uint8_t retval;

        if(currentCode.length == 0)
        {
            retval = currentCode.nextByte;
            currentCode = LZ77CodeType();
        }
        else
        {
            if(currentCode.offset >= buffer.size())
            {
                throw LZ77FormatException();
            }

            retval = buffer.cbegin()[currentCode.offset];

            if(--currentCode.length == 0)
            {
                currentCode = LZ77CodeType(currentCode.nextByte);
            }
        }

        buffer.push_front(retval);

        if(buffer.size() > bufferSize)
        {
            buffer.pop_back();
        }

        return retval;
    }
};

class CompressWriter final : public Writer
{
private:
    static constexpr int uint8_max = (1 << 8) - 1;
    static constexpr size_t bufferSize = LZ77CodeType::maxOffset + 1;
    struct Match
    {
        size_t location, length;
        Match(size_t location, size_t length)
            : location(location), length(length)
        {
        }
        Match()
            : Match(0, 0)
        {
        }
    };

    size_t location;
    size_t getActualLocation(size_t l)
    {
        return location - l;
    }

    shared_ptr<Writer> writer;
    circularDeque<uint_fast8_t, bufferSize + 1> currentInput;
    circularDeque<uint_fast8_t, bufferSize + 2> buffer;
    list<size_t> nodes[uint8_max + 1];

    void addByte(uint_fast8_t v)
    {
        nodes[v].push_front(++location);
        buffer.push_front(v);
        if(buffer.size() > bufferSize)
        {
            buffer.pop_back();
            for(list<size_t> & nodeList : nodes)
            {
                for(auto i = nodeList.begin(); i != nodeList.end();)
                {
                    if(getActualLocation(*i) >= buffer.size())
                        i = nodeList.erase(i);
                    else
                        i++;
                }
            }
        }
    }

    /*void dumpLocation(size_t l)
    {
        cout << l << " : ";
        l = getActualLocation(l);
        if(l >= buffer.size())
            cout << "\"\"" << endl;
        else
        {
            cout << "\"";
            auto iter = buffer.cbegin() + l;
            for(int i = 0; i < 5 && i <= l; i++, iter--)
            {
                cout << (char)*iter;
            }
            cout << "\"" << endl;
        }
    }*/

    Match getBiggestMatch()
    {
        Match retval;
        auto ii = currentInput.cbegin();
        if(ii == currentInput.cend())
            return Match();
        const list<size_t> & curNodes = nodes[*ii++];
        if(curNodes.empty())
            return Match();
        auto startII = ii;
        for(size_t startPos : curNodes)
        {
            size_t matchLength = 1;
            size_t node = startPos;
            for(ii = startII; ii != currentInput.cend(); matchLength++, ii++, node++)
            {
                size_t pos = getActualLocation(node + 1);
                if(pos >= buffer.size())
                    break;
                if(buffer.cbegin()[pos] != *ii)
                    break;
            }
            if(matchLength > retval.length)
            {
                retval.length = matchLength;
                retval.location = getActualLocation(startPos);
            }
        }
        if(retval.length > LZ77CodeType::maxLength + 1)
            retval.length = LZ77CodeType::maxLength + 1;
        return retval;
    }

    void writeCode()
    {
        if(currentInput.empty())
            return;
        if(currentInput.size() == 1)
        {
            addByte(currentInput.front());
            LZ77CodeType(currentInput.front()).write(*writer);
            currentInput.pop_front();
            return;
        }
        Match m = getBiggestMatch();
        if(m.length <= 1)
        {
            addByte(currentInput.front());
            LZ77CodeType(currentInput.front()).write(*writer);
            currentInput.pop_front();
            return;
        }
        m.length--;
        for(size_t i = 0; i < m.length; i++)
        {
            addByte(currentInput.front());
            currentInput.pop_front();
        }
        LZ77CodeType code(m.length, m.location, currentInput.front());
        code.write(*writer);
        addByte(currentInput.front());
        currentInput.pop_front();
    }

public:
    CompressWriter(shared_ptr<Writer> writer)
        : writer(writer)
    {
    }
    CompressWriter(Writer &writer)
        : CompressWriter(shared_ptr<Writer>(&writer, [](Writer *) {}))
    {
    }
    virtual ~CompressWriter()
    {
    }
    virtual void flush() override
    {
        while(!currentInput.empty())
            writeCode();
        writer->flush();
    }
    virtual void writeByte(uint8_t v) override
    {
        currentInput.push_back(v);
        if(currentInput.size() < bufferSize)
            return;
        writeCode();
    }
};

#endif // COMPRESSED_STREAM_H_INCLUDED
