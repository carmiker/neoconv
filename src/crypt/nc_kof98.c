/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME (prot_kof98.cpp) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

static uint16_t m_default_rom[2];

void lithogen_kof98_decrypt_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x200000), sizeof(uint8_t)); 
	int i, j, k;
	static const uint32_t sec[]={ 0x000000, 0x100000, 0x000004, 0x100004, 0x10000a, 0x00000a, 0x10000e, 0x00000e };
	static const uint32_t pos[]={ 0x000, 0x004, 0x00a, 0x00e };

	memcpy(dst, src, 0x200000);
	for (i = 0x800; i < 0x100000; i += 0x200)
	{
		for (j = 0; j < 0x100; j += 0x10)
		{
			for (k = 0; k < 16; k += 2)
			{
				memcpy(&src[i+j+k],       &dst[i+j+sec[k/2]+0x100], 2);
				memcpy(&src[i+j+k+0x100], &dst[i+j+sec[k/2]],       2);
			}
			if (i >= 0x080000 && i < 0x0c0000)
			{
				for (k = 0; k < 4; k++)
				{
					memcpy(&src[i+j+pos[k]],       &dst[i+j+pos[k]],       2);
					memcpy(&src[i+j+pos[k]+0x100], &dst[i+j+pos[k]+0x100], 2);
				}
			}
			else if (i >= 0x0c0000)
			{
				for (k = 0; k < 4; k++)
				{
					memcpy(&src[i+j+pos[k]],       &dst[i+j+pos[k]+0x100], 2);
					memcpy(&src[i+j+pos[k]+0x100], &dst[i+j+pos[k]],       2);
				}
			}
		}
		memcpy(&src[i+0x000000], &dst[i+0x000000], 2);
		memcpy(&src[i+0x000002], &dst[i+0x100000], 2);
		memcpy(&src[i+0x000100], &dst[i+0x000100], 2);
		memcpy(&src[i+0x000102], &dst[i+0x100100], 2);
	}
	memmove(&src[0x100000], &src[0x200000], 0x400000);

	uint16_t* mem16 = (uint16_t*)cpurom;
	m_default_rom[0] = mem16[0x100/2];
	m_default_rom[1] = mem16[0x102/2];
free(dst);
	}

