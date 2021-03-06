/*
 *  RNGstats worker bee.
 *  Copyright 2013 Zack Weinberg <zackw@panix.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "worker.h"
#include "ciphers.h"

#include <string.h>

/* Nothing up my sleeve: first 32 hexadecimal digits of pi, as
   computed by wolfram alpha. I'd rather use a Stoneham number,
   but there's no off-the-shelf web thingy that will calculate
   one of those. */
static const uint8_t keygen_key[16] = {
    0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
    0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34
};

#define BLOCKSIZE 8192
_Static_assert(KEYSTREAM_LENGTH % BLOCKSIZE == 0,
               "keystream length must be a multiple of blocksize");

void
worker_run(const work_order *in, work_results *out)
{
    const cipher *ciph = all_ciphers[in->cipher_index];
    uint64_t i, j, k;
    uint8_t stream_block[BLOCKSIZE];

    uint8_t keygen_ctx[aes128_cipher.ctxsize];
    uint8_t stream_ctx[ciph->ctxsize];
    uint8_t stream_key[ciph->keysize];

    memset(out, 0, sizeof(work_results));
    aes128_cipher.init(keygen_ctx, keygen_key);

    for (i = in->base; i < in->limit; i++)
    {
        /* aes128_cipher runs in counter mode, so asking for keystream
           from i * ciph->keysize through (i+1)*ciph->keysize produces
           the encipherment of 000... || i when ciph is a 128-bit cipher,
           and of 000... || 2i || 000... || 2i+1 when ciph is 256-bit.
           Either way, we'll never reuse keys within or between workers,
           but each key should be satisfactorily random. */
        aes128_cipher.gen_keystream(keygen_ctx, i * ciph->keysize,
                                    stream_key, ciph->keysize);

        ciph->init(stream_ctx, stream_key);

        for (j = 0; j < KEYSTREAM_LENGTH; j += BLOCKSIZE)
        {
            ciph->gen_keystream(stream_ctx, j, stream_block, BLOCKSIZE);

            for (k = 0; k < BLOCKSIZE; k++)
                out->epmf[j+k][stream_block[k]] += 1;
        }
    }
}

/*
 * Local Variables:
 * indent-tabs-mode: nil
 * c-basic-offset: 4
 * c-file-offsets: ((substatement-open . 0))
 * End:
 */
