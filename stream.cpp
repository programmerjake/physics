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
#include "stream.h"
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "util.h"

using namespace std;

namespace
{
const size_t bufferSize = 32768;

struct Pipe
{
    mutex lock;
    condition_variable_any cond;
    bool closed = false;
    queue<uint8_t, circularDeque<uint8_t, bufferSize + 1>> buffer;
};

class PipeReader final : public Reader
{
private:
    shared_ptr<Pipe> pipe;
public:
    PipeReader(shared_ptr<Pipe> pipe)
        : pipe(pipe)
    {
    }
    virtual ~PipeReader()
    {
        pipe->lock.lock();
        pipe->closed = true;
        pipe->cond.notify_all();
        pipe->lock.unlock();
    }
    virtual uint8_t readByte() override
    {
        pipe->lock.lock();
        if(pipe->buffer.empty())
        {
            pipe->cond.notify_all();
        }
        while(true)
        {
            if(!pipe->buffer.empty())
                break;
            if(pipe->closed)
            {
                pipe->lock.unlock();
                throw EOFException();
            }
            pipe->cond.wait(pipe->lock);
        }
        uint8_t retval = pipe->buffer.front();
        pipe->buffer.pop();
        pipe->lock.unlock();
        return retval;
    }
};

class PipeWriter final : public Writer
{
private:
    shared_ptr<Pipe> pipe;
public:
    PipeWriter(shared_ptr<Pipe> pipe)
        : pipe(pipe)
    {
    }

    virtual ~PipeWriter()
    {
        pipe->lock.lock();
        pipe->closed = true;
        pipe->cond.notify_all();
        pipe->lock.unlock();
    }

    virtual void writeByte(uint8_t v) override
    {
        pipe->lock.lock();
        while(true)
        {
            if(pipe->closed)
            {
                pipe->lock.unlock();
                throw IOException("IO Error : can't write to pipe");
            }
            if(pipe->buffer.size() < bufferSize)
                break;
            pipe->cond.notify_all();
            pipe->cond.wait(pipe->lock);
        }
        pipe->buffer.push(v);
        pipe->lock.unlock();
    }

    virtual void flush() override
    {
        pipe->lock.lock();
        pipe->cond.notify_all();
        pipe->lock.unlock();
    }
};
}

StreamPipe::StreamPipe()
{
    shared_ptr<Pipe> pipe = make_shared<Pipe>();
    readerInternal = shared_ptr<Reader>(new PipeReader(pipe));
    writerInternal = shared_ptr<Writer>(new PipeWriter(pipe));
}

uint8_t DumpingReader::readByte()
{
    uint8_t retval = reader.readByte();
    cerr << "Read Byte : " << (unsigned)retval << "\n";
    return retval;
}
