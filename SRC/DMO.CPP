#include <string.h>

#include "fileio.h"
#include "visual.h"
#include "player.h"
#include "s3m.h"
#include "dmo.h"


/*
#define LOWORD(x)      ((unsigned int)((unsigned long)(x) & 0xFFFF))
#define HIWORD(x)      ((unsigned int)((unsigned long)(x) >> 16))
#define LOBYTE(x)      ((unsigned char)((unsigned int)(x) & 0xFF))
#define HIBYTE(x)      ((unsigned char)((unsigned int)(x) >> 8))

#define ARRAY_AS_DWORD(a, i) \
( ((unsigned long) ( (unsigned long) (a[i + 3]) << 24))+ \
  ((unsigned long) ((unsigned long) (a[i + 2]) << 16))+ \
  ((unsigned long) ((unsigned long) (a[i + 1]) << 8))+  \
  ( (unsigned long) ((unsigned long) (a[i]))) )

#define ARRAY_AS_WORD(a, i) \
( (((unsigned int)a[i + 1]) << 8) + \
  ( (unsigned int) a[i] ) )


#define CHARP_AS_WORD(p)     (( ((unsigned int) *(p + 1)) << 8) + (*p))
*/




/*
------------------------------------------------------------------------------
*/


unsigned long as_long(char *buf, int idx)
{
    unsigned long res;
    asm {
        push    ds
        lds     si, buf
        mov     bx, [idx]
        add     si, bx
        lodsb
        mov     bl, al
        lodsb
        mov     bh, al
        lodsb
        mov     dl, al
        lodsb
        mov     dh, al
        pop     ds
        mov     word ptr [res], bx
        mov     word ptr [res+2], dx
    }
    return res;
}

unsigned int as_word(char *buf, int idx)
{
    unsigned int res;
    asm {
        push    ds
        lds     si, buf
        mov     bx, [idx]
        add     si, bx
        lodsb
        mov     bl, al
        lodsb
        mov     bh, al
        pop     ds
        mov     [res], bx
    }
    return res;
}

unsigned int charp_as_word(unsigned char *buf)
{
    unsigned int res;
    asm {
        push    ds
        lds     si, buf
        lodsb
        mov     bl, al
        lodsb
        mov     bh, al
        pop     ds
        mov     word ptr [res], bx
    }
    return res;
}

/*
void dump(char *buf, unsigned int size)
{
    int j;
    unsigned char *p = buf, *t, ch;
    unsigned int sz, addr = 0;


    dbg_Log("  --- dump ---\n");
    while (size)
    {
        t = p;
        sz = size;
        dbg_Log("%04X: ", addr);
        addr += 16;
        for (j = 0; j < 16; j++)
        {
            ch = *p;
            dbg_Log("%02hX ", ch);
            p++;
            size--;
            if (!size)
                break;
        }
        for (j = 0; j < 16; j++)
        {
            ch = *t;
            if (ch < 0x20 || ch > 0x80)
                ch = '.';
            dbg_Log("%c", ch);
            t++;
            sz--;
            if (!sz)
                break;
        }
        dbg_Log("\n");
    }
    dbg_Log("  --- end dump ---\n");
}
*/



bool CdmoLoader::load(char *filename)
{
    int i,j;
    int f;
    unsigned char chkhdr[16];
    unsigned char *packed_module;
    unsigned char *module;
    long packed_length;
    long unpacked_length;
    unsigned short my_patlen[100];
    s3mpattype    *pat;
    unsigned char *ptr;
    unsigned char *cur_pos;
    unsigned char token, chan;
    unsigned char bufbyte;

    CPlayer::load(filename);

    if ((f = io_fopen(filename, O_RDONLY)) == 0)
        return FALSE;

    // check header
    io_fread(chkhdr, 16, f);

    if (!decrypt(chkhdr, 16))
    {
        io_fclose(f);
        return FALSE;
    }

    // get file size
    packed_length = io_filesize(f);
    if (packed_length > 65536-16)
    {
        io_fclose(f);
        return FALSE;
    }
    io_fseek(f, 0, SEEK_SET);           // f->seek(0);
    packed_module = new unsigned char [packed_length];

    // load file
    io_fread(packed_module, packed_length, f);
    io_fclose(f);

    // decrypt
    decrypt(packed_module, packed_length);
    unpacked_length = 0x2000 * as_word(packed_module, 12);
    if (unpacked_length >= 65536)
    {
        delete [] packed_module;
        return FALSE;
    }
    module = new unsigned char [unpacked_length];

    // unpack
    if (!unpack(packed_module+12, module, unpacked_length))
    {
        delete [] packed_module;
        delete [] module;
        return FALSE;
    }
    delete [] packed_module;

    // "TwinTeam" - signed ?
    if (memcmp(module, "TwinTeam Module File""\x0D\x0A", 22))
    {
        delete [] module;
        return FALSE;
    }

    // load header
    ptr = module+22;

    memset(&header,0,sizeof(s3mheader));

    //ptr += 22;
    memcpy(header.name, ptr, 28);
    // header.name[27] = 0;              // ensure termination
    ptr += 28+2;
    memcpy(&header.ordnum, ptr, 2); ptr += 2;
    memcpy(&header.insnum, ptr, 2); ptr += 2;
    memcpy(&header.patnum, ptr, 2); ptr += 2+2;
    memcpy(&header.is, ptr, 2); ptr += 2;
    memcpy(&header.it, ptr, 2); ptr += 2+32;
    //ptr += 32;

    if (header.ordnum >= 256 || header.insnum > 99 || header.patnum > 99)
    {
        delete [] module;
        return FALSE;
    }

    memset(header.chanset,0xFF,32);

    for (i=0;i<9;i++)
        header.chanset[i] = 0x10 + i;

    // load orders
    memcpy(&orders, ptr, 256); ptr += 256;
    orders[header.ordnum] = 0xFF;

    // load pattern lengths
    memcpy(&my_patlen, ptr, 200); ptr += 200;

    // load instruments
    for (i = 0; i < header.insnum; i++)
    {
        memset(&inst[i],0,sizeof(s3minst));

        memcpy(inst[i].name, ptr, 28-1); ptr += 28;
        // inst[i].name[27] = 0;

        inst[i].volume = *ptr++;
        inst[i].dsk    = *ptr++;
        memcpy(&inst[i].c2spd, ptr, 4); ptr += 4;
        inst[i].type   = *ptr++;
        //memcpy(&inst[i].d00, ptr, 12); ptr += 12;

        inst[i].d00    = *ptr++;
        inst[i].d01    = *ptr++;
        inst[i].d02    = *ptr++;
        inst[i].d03    = *ptr++;
        inst[i].d04    = *ptr++;
        inst[i].d05    = *ptr++;
        inst[i].d06    = *ptr++;
        inst[i].d07    = *ptr++;
        inst[i].d08    = *ptr++;
        inst[i].d09    = *ptr++;
        inst[i].d0a    = *ptr++;
        /*
         * Originally, riven sets d0b = d0a and ignores 1 byte in the
         * stream, but i guess this was a typo, so i read it here.
         */
        inst[i].d0b    = *ptr++;
    }

    // load patterns
    for (i = 0; i < header.patnum; i++)
    {
        cur_pos = ptr;
        pat = alloc_pattern();
        if (!pat)
        {
            delete [] module;
            return FALSE;
        }
        pattern[i] = pat;

        for (j = 0; j < 64; j++)
        {
            while (1)
            {
                token = *ptr++;

                if (!token)
                    break;

                chan = token & 31;

                // note + instrument ?
                if (token & 32)
                {
                    bufbyte = *ptr++;

                    pat->rows[j][chan].note = bufbyte & 15;
                    pat->rows[j][chan].oct = bufbyte >> 4;
                    pat->rows[j][chan].instrument = *ptr++;
                }

                // volume ?
                if (token & 64)
                {
                    pat->rows[j][chan].volume = *ptr++;
                }

                // command ?
                if (token & 128)
                {
                    pat->rows[j][chan].command = *ptr++;
                    pat->rows[j][chan].info = *ptr++;
                }
            }
        }
        ptr = cur_pos + my_patlen[i];
    }

    delete [] module;
    rewind(0);
    return TRUE;
}



/*
-------- Private Methods -------------------------------
*/
/*
unsigned short CdmoLoader::brand(unsigned short range)
{
  bseed *= 0x08088405U;
  bseed++;

  return (unsigned long long)bseed * range >> 32;
}
*/

unsigned short CdmoLoader::brand(unsigned short range)
{
    unsigned short res;
    unsigned long _seed = bseed;

    asm {
        mov     cx, word ptr [_seed]
        mov     bx, word ptr [_seed+2]
        mov     ax, 0x8405
        mul     cx
        shl     cx, 1
        shl     cx, 1
        shl     cx, 1
        add     ch, cl
        add     dx, cx
        add     dx, bx
        shl     bx, 1
        shl     bx, 1
        add     dx, bx
        add     dh, bl  // _dx = (((HIBYTE(_dx) + LOBYTE(_bx)) & 0xFF) << 8) + LOBYTE(_dx);
        mov     cl, 5
        shl     bx, cl
        add     dh, bl  // _dx = (((HIBYTE(_dx) + LOBYTE(_bx)) & 0xFF) << 8) + LOBYTE(_dx);
        inc     ax
        jnz     skip_dx
        inc     dx
    } skip_dx:
    asm {
        mov     word ptr [_seed], ax
        mov     word ptr [_seed+2], dx

        mov     bx, dx          // bx = HIWORD(bseed)
        mul     [range]
        mov     cx, dx          // cx = HIWORD( LOWORD(bseed) * range )
        mov     ax, bx
        mul     [range]         // dx:ax = HIWORD(bseed) * range
        add     ax, cx
        adc     dx, 0
        mov     [res], dx       // HIWORD ( HIWORD( LOWORD(bseed) * range ) + HIWORD(bseed) * range )
    }
    bseed = _seed;
    return res;                 //HIWORD(HIWORD(LOWORD(bseed) * range) + HIWORD(bseed) * range);
}


bool CdmoLoader::decrypt(unsigned char *buf, long len)
{
    unsigned long seed = 0;
    int i;

    if (len < 12)
        return FALSE;

    bseed = as_long(buf, 0);
    for (i=0; i <= as_word(buf, 4); i++)
        seed += brand(0xffff);

    bseed = seed ^ as_long(buf, 6);

    if (as_word(buf, 10) != brand(0xffff))
        return FALSE;

    for (i = 12; i < len; i++)
        buf[i] ^= brand(0x100);

    buf[len - 2] = buf[len - 1] = 0;

    return TRUE;
}



short CdmoLoader::unpack_block(unsigned char *ibuf, long ilen, unsigned char *obuf)
{
    unsigned char code,par1,par2;
    unsigned short ax,bx,cx;

    unsigned char *ipos = ibuf;
    unsigned char *opos = obuf;

    int i;

    // LZ77 child
    while (ipos - ibuf < ilen)
    {
        code = *ipos++;

        // 00xxxxxx: copy (xxxxxx + 1) bytes
        if ((code >> 6) == 0)
        {
            cx = (code & 0x3F) + 1;

            if (opos + cx >= oend)
                return -1;

            for (i = 0; i < cx; i++)
                *opos++ = *ipos++;

            continue;
        }

        // 01xxxxxx xxxyyyyy: copy (Y + 3) bytes from (X + 1)
        if ((code >> 6) == 1)
        {
            par1 = *ipos++;

            ax = ((code & 0x3F) << 3) + ((par1 & 0xE0) >> 5) + 1;
            cx = (par1 & 0x1F) + 3;

            if (opos + cx >= oend)
                return -1;

            for (i = 0; i < cx; i++)
                *opos++ = *(opos - ax);

            continue;
        }

        // 10xxxxxx xyyyzzzz: copy (Y + 3) bytes from (X + 1); copy Z bytes
        if ((code >> 6) == 2)
        {
            par1 = *ipos++;

            ax = ((code & 0x3F) << 1) + (par1 >> 7) + 1;
            cx = ((par1 & 0x70) >> 4) + 3;
            bx = par1 & 0x0F;

            if (opos + bx + cx >= oend)
                return -1;

            for (i = 0; i < cx; i++)
                *opos++ = *(opos - ax);

            for (i = 0; i < bx; i++)
                *opos++ = *ipos++;

            continue;
        }

        // 11xxxxxx xxxxxxxy yyyyzzzz: copy (Y + 4) from X; copy Z bytes
        if ((code >> 6) == 3)
        {
            par1 = *ipos++;
            par2 = *ipos++;

            bx = ((code & 0x3F) << 7) + (par1 >> 1);
            cx = ((par1 & 0x01) << 4) + (par2 >> 4) + 4;
            ax = par2 & 0x0F;

            if (opos + ax + cx >= oend)
                return -1;

            for (i = 0; i < cx; i++)
                *opos++ = *(opos - bx);

            for (i = 0; i < ax; i++)
                *opos++ = *ipos++;

            continue;
        }
    }
    return opos - obuf;
}

long CdmoLoader::unpack(unsigned char *ibuf, unsigned char *obuf,
                                      unsigned long outputsize)
{
    long           olen = 0;
    unsigned short block_count;
    unsigned short bul;
    unsigned char *block_length;

    block_count = charp_as_word(ibuf);
    ibuf += 2;
    block_length = ibuf;
    ibuf += 2 * block_count;
    oend = obuf + outputsize;

    for (int i=0;i<block_count;i++)
    {
        bul = charp_as_word(ibuf);

        if (unpack_block(ibuf + 2,charp_as_word(block_length) - 2,obuf) != bul)
            return 0;
        obuf += bul;
        olen += bul;

        ibuf += charp_as_word(block_length);
        block_length += 2;
    }
    return olen;
}
