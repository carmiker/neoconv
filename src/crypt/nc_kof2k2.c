/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME (prot_kof2k2.cpp) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

void lithogen_kof2002_decrypt_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[]={0x100000,0x280000,0x300000,0x180000,0x000000,0x380000,0x200000,0x080000};
	uint8_t *src = cpurom + 0x100000;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x400000), sizeof(uint8_t)); 
	memcpy(dst, src, 0x400000);

	for (int i = 0; i < 8; ++i)
		memcpy(src + i * 0x80000, &dst[sec[i]], 0x80000);
free(dst);
	}

void lithogen_matrim_decrypt_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[]={0x100000,0x280000,0x300000,0x180000,0x000000,0x380000,0x200000,0x080000};
	uint8_t *src = cpurom + 0x100000;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x400000), sizeof(uint8_t)); 
	memcpy(dst, src, 0x400000);

	for (int i = 0; i < 8; ++i)
		memcpy(src + i * 0x80000, &dst[sec[i]], 0x80000);
free(dst);
	}

void lithogen_samsho5_decrypt_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[]={0x000000,0x080000,0x700000,0x680000,0x500000,0x180000,0x200000,0x480000,0x300000,0x780000,0x600000,0x280000,0x100000,0x580000,0x400000,0x380000};
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x800000), sizeof(uint8_t)); 
	memcpy(dst, src, 0x800000);
	for (int i = 0; i < 16; ++i)
		memcpy(src + i * 0x80000, &dst[sec[i]], 0x80000);
free(dst);
	}

void lithogen_samsh5sp_decrypt_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[]={0x000000,0x080000,0x500000,0x480000,0x600000,0x580000,0x700000,0x280000,0x100000,0x680000,0x400000,0x780000,0x200000,0x380000,0x300000,0x180000};
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x800000), sizeof(uint8_t)); 

	memcpy(dst, src, 0x800000);
	for (int i = 0; i < 16; ++i)
		memcpy(src + i * 0x80000, &dst[sec[i]], 0x80000);
free(dst);
	}

