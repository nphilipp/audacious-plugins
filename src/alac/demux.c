/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is the quicktime container demuxer.
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <glib.h>

#include "stream.h"
#include "demux.h"

typedef struct
{
    stream_t *stream;
    demux_res_t *res;
    long saved_mdat_pos;
} qtmovie_t;


/* chunk handlers */
static void read_chunk_ftyp(qtmovie_t *qtmovie, size_t chunk_len)
{
    fourcc_t type;
    uint32_t minor_ver;
    size_t size_remaining = chunk_len - 8; /* FIXME: can't hardcode 8, size may be 64bit */

    type = stream_read_uint32(qtmovie->stream);
    size_remaining-=4;
    if (type != MAKEFOURCC('M','4','A',' '))
    {
        return;
    }
    minor_ver = stream_read_uint32(qtmovie->stream);
    size_remaining-=4;

    /* compatible brands */
    while (size_remaining)
    {
        /* unused */
        /*fourcc_t cbrand =*/ stream_read_uint32(qtmovie->stream);
        size_remaining-=4;
    }
}

static void read_chunk_tkhd(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_mdhd(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_edts(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_elst(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

/* media handler inside mdia */
static void read_chunk_hdlr(qtmovie_t *qtmovie, size_t chunk_len)
{
    fourcc_t comptype, compsubtype;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    int strlen;
    char str[256] = {0};

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    /* component type */
    comptype = stream_read_uint32(qtmovie->stream);
    compsubtype = stream_read_uint32(qtmovie->stream);
    size_remaining -= 8;

    /* component manufacturer */
    stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    /* flags */
    stream_read_uint32(qtmovie->stream);
    stream_read_uint32(qtmovie->stream);
    size_remaining -= 8;

    /* name */
    strlen = stream_read_uint8(qtmovie->stream);
    stream_read(qtmovie->stream, strlen, str);
    size_remaining -= 1 + strlen;

    if (size_remaining)
    {
        stream_skip(qtmovie->stream, size_remaining);
    }

}

static void read_chunk_stsd(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    if (numentries != 1)
    {
        return;
    }

    for (i = 0; i < numentries; i++)
    {
        uint32_t entry_size;
        uint16_t version;

        uint32_t entry_remaining;

        entry_size = stream_read_uint32(qtmovie->stream);
        qtmovie->res->format = stream_read_uint32(qtmovie->stream);
        entry_remaining = entry_size;
        entry_remaining -= 8;

        /* sound info: */

        stream_skip(qtmovie->stream, 6); /* reserved */
        entry_remaining -= 6;

        version = stream_read_uint16(qtmovie->stream);
        if (version != 1)
		return;
        entry_remaining -= 2;

        /* revision level */
        stream_read_uint16(qtmovie->stream);
        /* vendor */
        stream_read_uint32(qtmovie->stream);
        entry_remaining -= 6;

        /* EH?? spec doesn't say theres an extra 16 bits here.. but there is! */
        stream_read_uint16(qtmovie->stream);
        entry_remaining -= 2;

        qtmovie->res->num_channels = stream_read_uint16(qtmovie->stream);

        qtmovie->res->sample_size = stream_read_uint16(qtmovie->stream);
        entry_remaining -= 4;

        /* compression id */
        stream_read_uint16(qtmovie->stream);
        /* packet size */
        stream_read_uint16(qtmovie->stream);
        entry_remaining -= 4;

        /* sample rate - 32bit fixed point = 16bit?? */
        qtmovie->res->sample_rate = stream_read_uint16(qtmovie->stream);
        entry_remaining -= 2;

        /* skip 2 */
        stream_skip(qtmovie->stream, 2);
        entry_remaining -= 2;

        /* remaining is codec data */

        /* 12 = audio format atom, 8 = padding */
        qtmovie->res->codecdata_len = entry_remaining + 12 + 8;
        qtmovie->res->codecdata = malloc(qtmovie->res->codecdata_len);
        memset(qtmovie->res->codecdata, 0, qtmovie->res->codecdata_len);
        /* audio format atom */
        ((unsigned int*)qtmovie->res->codecdata)[0] = 0x0c000000;
        ((unsigned int*)qtmovie->res->codecdata)[1] = MAKEFOURCC('a','m','r','f');
        ((unsigned int*)qtmovie->res->codecdata)[2] = MAKEFOURCC('c','a','l','a');

        stream_read(qtmovie->stream,
                entry_remaining,
                ((char*)qtmovie->res->codecdata) + 12);
        entry_remaining -= entry_remaining;

        if (entry_remaining)
            stream_skip(qtmovie->stream, entry_remaining);

        if (qtmovie->res->format != MAKEFOURCC('a','l','a','c'))
            return;
    }
}

static void read_chunk_stts(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_time_to_samples = numentries;
    qtmovie->res->time_to_sample = malloc(numentries * sizeof(*qtmovie->res->time_to_sample));

    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->time_to_sample[i].sample_count = stream_read_uint32(qtmovie->stream);
        qtmovie->res->time_to_sample[i].sample_duration = stream_read_uint32(qtmovie->stream);
        size_remaining -= 8;
    }

    if (size_remaining)
        stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_stsz(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    /* default sample size */
    if (stream_read_uint32(qtmovie->stream) != 0)
    {
        stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
        return;
    }
    size_remaining -= 4;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_sample_byte_sizes = numentries;
    qtmovie->res->sample_byte_size = malloc(numentries * sizeof(*qtmovie->res->sample_byte_size));

    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->sample_byte_size[i] = stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
    }

    if (size_remaining)
        stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_stbl(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
            return;

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('s','t','s','d'):
            read_chunk_stsd(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('s','t','t','s'):
            read_chunk_stts(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('s','t','s','z'):
            read_chunk_stsz(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('s','t','s','c'):
        case MAKEFOURCC('s','t','c','o'):
            /* skip these, no indexing for us! */
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        default:
            return;
        }

        size_remaining -= sub_chunk_len;
    }
}

static void read_chunk_minf(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t dinf_size, stbl_size;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    if (stream_read_uint32(qtmovie->stream) != 16)
        return;

    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('s','m','h','d'))
        return;

    /* now skip the rest */
    stream_skip(qtmovie->stream, 16 - 8);
    size_remaining -= 16;


  /**** DINF CHUNK ****/
    dinf_size = stream_read_uint32(qtmovie->stream);
    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('d','i','n','f'))
        return;

    /* skip it */
    stream_skip(qtmovie->stream, dinf_size - 8);
    size_remaining -= dinf_size;
  /****/


  /**** SAMPLE TABLE ****/
    stbl_size = stream_read_uint32(qtmovie->stream);
    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('s','t','b','l'))
        return;

    read_chunk_stbl(qtmovie, stbl_size);
    size_remaining -= stbl_size;

    if (size_remaining)
        stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_mdia(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
            return;

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('m','d','h','d'):
            read_chunk_mdhd(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('h','d','l','r'):
            read_chunk_hdlr(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('m','i','n','f'):
            read_chunk_minf(qtmovie, sub_chunk_len);
            break;
        default:
            return;
        }

        size_remaining -= sub_chunk_len;
    }
}

/* 'trak' - a movie track - contains other atoms */
static void read_chunk_trak(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
            return;

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('t','k','h','d'):
            read_chunk_tkhd(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('m','d','i','a'):
            read_chunk_mdia(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('e','d','t','s'):
            read_chunk_edts(qtmovie, sub_chunk_len);
            break;
        default:
            return;
        }

        size_remaining -= sub_chunk_len;
    }
}

/* 'mvhd' movie header atom */
static void read_chunk_mvhd(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

enum
{
	UDTA_NIL = 0,
	UDTA_NAM,
	UDTA_ART,
	UDTA_ALB,
	UDTA_GEN,
	UDTA_DAY
};

/* 'udta' user data.. contains tag info. this routine is utterly fucked because Apple's
 * lovely container format is utterly fucked itself...
 */
void read_chunk_udta(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */
    char *buf = g_malloc0(chunk_len);
    char *bptr;
    fourcc_t sub_chunk_id;   /* bptr[4] << 24 | bptr[3] << 16 | bptr[2] << 8 | bptr[1] */
    int udta_tgt = UDTA_NIL;

    /* read the stream in for scanning -nenolod */
    stream_read(qtmovie->stream, size_remaining, buf);

    /* this is (bptr + 3) to account for the MAKEFOURCC() macro. -nenolod */
    for (bptr = buf; (bptr + 3) - buf < size_remaining; bptr++)
    {
        sub_chunk_id = MAKEFOURCC(*bptr, *(bptr + 1), *(bptr + 2), *(bptr + 3));

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('m','e','t','a'):
             bptr += 4; /* skip meta */
             break;
        case MAKEFOURCC(0xA9,'n','a','m'):
             udta_tgt = UDTA_NAM;
	     bptr += 4;
             break;
        case MAKEFOURCC(0xA9,'A','R','T'):
             udta_tgt = UDTA_ART;
	     bptr += 4;
             break;
        case MAKEFOURCC(0xA9,'a','l','b'):
             udta_tgt = UDTA_ALB;
	     bptr += 4;
             break;
        case MAKEFOURCC(0xA9,'g','e','n'):
             udta_tgt = UDTA_GEN;
	     bptr += 4;
             break;
        case MAKEFOURCC(0xA9,'d','a','y'):
             udta_tgt = UDTA_DAY;
	     bptr += 4;
             break;
        case MAKEFOURCC('d','a','t','a'):
             switch(udta_tgt)
             {
             case UDTA_NAM:
                 qtmovie->res->tuple.nam = g_strdup(bptr + 12);
                 break;
             case UDTA_ART:
                 qtmovie->res->tuple.art = g_strdup(bptr + 12);
                 break;
             case UDTA_ALB:
                 qtmovie->res->tuple.alb = g_strdup(bptr + 12);
                 break;
             case UDTA_DAY:
                 qtmovie->res->tuple.day = g_strdup(bptr + 12);
                 break;
             case UDTA_GEN:
                 qtmovie->res->tuple.gen = g_strdup(bptr + 12);
                 break;
             default:
	         break;
             }

             bptr += 12;
	     bptr += strlen(bptr);
             break;
        default:
             break;
        }
    }

    g_free(buf);
}

/* 'moov' movie atom - contains other atoms */
static void read_chunk_moov(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
            return;

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('m','v','h','d'):
            read_chunk_mvhd(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('t','r','a','k'):
            read_chunk_trak(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('u','d','t','a'):
            read_chunk_udta(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('e','l','s','t'):
            read_chunk_elst(qtmovie, sub_chunk_len);
            break;
        default:
            return;
        }

        size_remaining -= sub_chunk_len;
    }
}

static void read_chunk_mdat(qtmovie_t *qtmovie, size_t chunk_len, int skip_mdat)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    qtmovie->res->mdat_len = (uint32_t)size_remaining;
    if (skip_mdat)
    {
        qtmovie->saved_mdat_pos = stream_tell(qtmovie->stream);
        stream_skip(qtmovie->stream, size_remaining);
    }
#if 0
    qtmovie->res->mdat = malloc(size_remaining);

    stream_read(qtmovie->stream, size_remaining, qtmovie->res->mdat);
#endif
}

static int set_saved_mdat(qtmovie_t *qtmovie)
{
    if (qtmovie->saved_mdat_pos == -1)
        return 0;

    if (stream_setpos(qtmovie->stream, qtmovie->saved_mdat_pos))
        return 0;

    return 1;
}

int qtmovie_read(stream_t *file, demux_res_t *demux_res)
{
    int found_moov = 0;
    int found_mdat = 0;
    qtmovie_t *qtmovie;

    qtmovie = (qtmovie_t*)malloc(sizeof(qtmovie_t));

    /* construct the stream */
    qtmovie->stream = file;
    qtmovie->res = demux_res;

    memset(demux_res, 0, sizeof(demux_res_t));

    /* read the chunks */
    while (1)
    {
        size_t chunk_len;
        fourcc_t chunk_id;

        chunk_len = stream_read_uint32(qtmovie->stream);
        if (stream_eof(qtmovie->stream))
        {
            return 0;
        }

        if (chunk_len == 1)
            return 0;

        chunk_id = stream_read_uint32(qtmovie->stream);

        switch (chunk_id)
        {
        case MAKEFOURCC('f','t','y','p'):
            read_chunk_ftyp(qtmovie, chunk_len);
            break;
        case MAKEFOURCC('m','o','o','v'):
            read_chunk_moov(qtmovie, chunk_len);
            if (found_mdat)
            {
                return set_saved_mdat(qtmovie);
            }
            found_moov = 1;
            break;
            /* if we hit mdat before we've found moov, record the position
             * and move on. We can then come back to mdat later.
             * This presumes the stream supports seeking backwards.
             */
        case MAKEFOURCC('m','d','a','t'):
            read_chunk_mdat(qtmovie, chunk_len, !found_moov);
            if (found_moov)
                return 1;
            found_mdat = 1;
            break;

            /*  these following atoms can be skipped !!!! */
        case MAKEFOURCC('f','r','e','e'):
            stream_skip(qtmovie->stream, chunk_len - 8); /* FIXME not 8 */
            break;
        default:
            return 0;
        }

    }
    return 0;
}


