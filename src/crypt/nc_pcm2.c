/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME (prot_pcm2.cpp) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

void lithogen_pcm2_decrypt(uint8_t* ymrom, uint32_t ymsize, int value)
{
	// thanks to Elsemi for the NEO-PCM2 info
	uint16_t *rom = (uint16_t *)ymrom;
	int size = ymsize;

	if (rom != NULL)
	{
		// swap address lines on the whole ROMs
		uint16_t *buffer = (uint16_t *)calloc((size_t)(value / 2), sizeof(uint16_t)); 

		for (int i = 0; i < size / 2; i += (value / 2))
		{
			memcpy(buffer, &rom[i], value);
			for (int j = 0; j < (value / 2); j++)
			{
				rom[i + j] = buffer[j ^ (value/4)];
			}
		}
	free(buffer);
	}
}

void lithogen_pcm2_swap(uint8_t* ymrom, uint32_t ymsize, int value)
{
	static const uint32_t addrs[7][2]={
		{0x000000,0xa5000},
		{0xffce20,0x01000},
		{0xfe2cf6,0x4e001},
		{0xffac28,0xc2000},
		{0xfeb2c0,0x0a000},
		{0xff14ea,0xa7001},
		{0xffb440,0x02000}};
	static const uint8_t xordata[7][8]={
		{0xf9,0xe0,0x5d,0xf3,0xea,0x92,0xbe,0xef},
		{0xc4,0x83,0xa8,0x5f,0x21,0x27,0x64,0xaf},
		{0xc3,0xfd,0x81,0xac,0x6d,0xe7,0xbf,0x9e},
		{0xc3,0xfd,0x81,0xac,0x6d,0xe7,0xbf,0x9e},
		{0xcb,0x29,0x7d,0x43,0xd2,0x3a,0xc2,0xb4},
		{0x4b,0xa4,0x63,0x46,0xf0,0x91,0xea,0x62},
		{0x4b,0xa4,0x63,0x46,0xf0,0x91,0xea,0x62}};

	uint8_t *buf = (uint8_t *)calloc((size_t)(0x1000000), sizeof(uint8_t)); 
	int j, d;
	uint8_t* src = ymrom;
	memcpy(buf, src, 0x1000000);

	for (int i = 0; i < 0x1000000; i++)
	{
		j = nc_bitswap(i, 24,23,22,21,20,19,18,17,0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,16);
		j ^= addrs[value][1];
		d = ((i + addrs[value][0]) & 0xffffff);
		src[j] = buf[d] ^ xordata[value][j & 0x7];
	}
free(buf);
	}

