/**
 * @file prvhash42.h
 *
 * @brief The inclusion file for the "prvhash42" hash function.
 *
 * @mainpage
 *
 * @section intro_sec Introduction
 *
 * Description is available at https://github.com/avaneev/prvhash
 *
 * @section license License
 *
 * Copyright (c) 2020 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @version 2.20
 */

//$ nocpp

#ifndef PRVHASH42_INCLUDED
#define PRVHASH42_INCLUDED

#include <string.h>
#include "prvhash42ec.h"

/**
 * PRVHASH hash function (64-bit variables with 32-bit hash word). Produces
 * hash of the specified message. This function applies endianness correction
 * automatically (on little- and big-endian processors).
 *
 * @param Msg The message to produce hash from. The alignment of the message
 * is unimportant.
 * @param MsgLen Message's length, in bytes.
 * @param[in,out] Hash The resulting hash. The length of this buffer should be
 * equal to HashLen * 2 to supply a scratch pad for the function (for 32-bit
 * hashes, this buffer can have HashLen length). If InitVec is non-NULL, the
 * hash will not be initially reset to 0, and it should be pre-initialized
 * with uniformly-random bytes (there are no restrictions on which values to
 * use for initialization: even an all-zero value can be used). The provided
 * hash will be automatically endianness-corrected. On systems where this is
 * relevant, this address should be aligned to 32 bits.
 * @param HashLen The required hash length, in bytes, should be >= 4, in
 * increments of 4.
 * @param SeedXOR Optional value, to XOR the default seed with. To use the
 * default seed, set to 0. If InitVec is non-NULL, this SeedXOR is ignored and
 * should be set to 0. Otherwise, the SeedXOR value can have any bit length,
 * and is used only as an additional entropy source. It should be
 * endianness-corrected.
 * @param InitVec If non-NULL, an "initialization vector" for the internal
 * "lcg" and "Seed" variables. Full 16-byte uniformly-random value should be
 * supplied in this case. Since it is imperative that the initialization
 * vector is non-zero, the best strategies to generate it are: 1) compose the
 * vector from 16-bit random values that have 4 to 12 random bits set; 2)
 * compose the vector from 64-bit random values that have 28-36 random bits
 * set.
 */

inline void prvhash42( const uint8_t* const Msg, const int MsgLen,
	uint8_t* const Hash, const int HashLen, const uint64_t SeedXOR,
	const uint8_t InitVec[ 16 ])
{
	const int hlm = ( HashLen == 4 ? HashLen : HashLen << 1 );
	uint64_t lcg;
	uint64_t Seed;

	if( InitVec == 0 )
	{
		memset( Hash, 0, hlm );

		lcg = 15252113002925621231ULL;
		Seed = 17412655673657598932ULL ^ SeedXOR;
	}
	else
	{
		prvhash42_ec( Hash, hlm );

		lcg = prvhash42_u64ec( InitVec );
		Seed = prvhash42_u64ec( InitVec + 8 );
	}

	const uint8_t lb = (uint8_t) ( MsgLen > 0 ? ~Msg[ MsgLen - 1 ] : 0xFF );
	const int mlext = MsgLen + (( 4 - ( MsgLen & 3 )) & 3 );
	const int c = mlext + HashLen + ( hlm - mlext % hlm );
	int hpos = 0;
	int k;

	for( k = 0; k < c; k += 4 )
	{
		uint64_t msgw;

		if( k < MsgLen - 3 )
		{
			msgw = prvhash42_u32ec( Msg + k );
		}
		else
		{
			msgw = (uint32_t) ( k < MsgLen ? Msg[ k ] : lb ) |
				(uint32_t) ( k < MsgLen - 1 ? Msg[ k + 1 ] : lb ) << 8 |
				(uint32_t) ( k < MsgLen - 2 ? Msg[ k + 2 ] : lb ) << 16 |
				(uint32_t) lb << 24;
		}

		Seed *= lcg;
		Seed = ~Seed;
		uint32_t* const hc = (uint32_t*) &Hash[ hpos ];
		const uint64_t hl = lcg >> 32 ^ msgw;
		lcg += Seed;
		const uint64_t ph = *hc ^ ( Seed >> 32 );
		Seed ^= ph ^ hl;
		*hc = (uint32_t) ph;

		hpos += 4;

		if( hpos == hlm )
		{
			hpos = 0;
		}
	}

	if( hlm > 4 )
	{
		for( k = 0; k < HashLen; k += 4 )
		{
			*(uint32_t*) ( Hash + k ) ^= *(uint32_t*) ( Hash + HashLen + k );
		}
	}

	prvhash42_ec( Hash, HashLen );
}

#endif // PRVHASH42_INCLUDED
