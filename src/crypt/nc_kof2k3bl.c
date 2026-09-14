/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME (prot_kof2k3bl.cpp) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

static uint16_t m_overlay; /* runtime member; value re-read by protection sim */

void lithogen_kf2k3pl_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint16_t *tmp = (uint16_t *)calloc((size_t)(0x100000/2), sizeof(uint16_t)); 
	uint16_t*rom16 = (uint16_t*)cpurom;

	for (int i = 0; i < 0x700000/2; i += 0x100000/2)
	{
		memcpy(tmp, &rom16[i], 0x100000);
		for (int j = 0; j < 0x100000/2; j++)
			rom16[i+j] = tmp[nc_bitswap(j, 24,23,22,21,20,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18)];
	}

	/* patched by Altera protection chip on PCB */
	rom16[0xf38ac/2] = 0x4e75;

	m_overlay = rom16[0x58196 / 2];
free(tmp);
	}

void lithogen_kf2k3upl_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *src = cpurom;
	memmove(src + 0x100000, src, 0x600000);
	memmove(src, src + 0x700000, 0x100000);

	uint8_t *rom = cpurom + 0xfe000;
	uint8_t *buf = cpurom + 0xd0610;
	for (int i = 0; i < 0x2000 / 2; i++)
	{
		int ofst = (i & 0xff00) + nc_bitswap((i & 0x00ff), 8, 7, 6, 0, 4, 3, 2, 1, 5);
		memcpy(&rom[i * 2], &buf[ofst * 2], 2);
	}

	uint16_t* rom16 = (uint16_t*)cpurom;
	m_overlay = rom16[0x58196 / 2];
}

