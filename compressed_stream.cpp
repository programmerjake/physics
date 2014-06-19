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
#include "compressed_stream.h"
#include "util.h"
#include <iostream>
#include <cstdlib>
#include <thread>

#if 0 // use demo code
namespace
{
class DumpWriter final : public Writer
{
public:
    virtual void writeByte(uint8_t v) override
    {
        cout << hex << (unsigned)(v >> 4) << (unsigned)(v & 0xF) << endl << dec;
    }
};

void dumpRead(shared_ptr<Reader> preader)
{
    ExpandReader reader(preader);

    try
    {
        while(true)
        {
            cout << (char)reader.readByte();
        }
    }
    catch(EOFException &e)
    {
    }
    cout << endl;
}

initializer init1([]()
{
    cout << "test compression :\n";
    thread readerThread;
    {
        StreamPipe pipe;
        readerThread = thread(dumpRead, pipe.preader());
        CompressWriter w(pipe.pwriter());

        for(int i = 0; i < 65536; i++)
        {
            for(const char *str =
                        "abcdefghij012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123abcdefg\n";
                    *str; str++)
            {
                w.writeByte(*str);
            }
            w.flush();
        }

        w.flush();
    }
    readerThread.join();
    exit(0);
});
}
#endif // use demo code

