/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME (prot_sma.cpp) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

void lithogen_kof99_decrypt_68k(uint8_t* base)
{
	uint16_t *rom = (uint16_t *)(base + 0x100000);

	// swap data lines on the whole ROMs
	for (int i = 0; i < 0x800000/2; i++)
		rom[i] = nc_bitswap(rom[i], 16,13,7,3,0,9,4,5,6,1,12,8,14,10,11,2,15);

	// swap address lines for the banked part
	for (int i = 0; i < 0x600000/2; i += 0x800/2)
	{
		uint16_t buffer[0x800/2];
		memcpy(buffer, &rom[i], 0x800);
		for (int j = 0; j < 0x800/2; j++)
			rom[i+j] = buffer[nc_bitswap(j, 10,6,2,4,9,8,3,1,7,0,5)];
	}

	// swap address lines & relocate fixed part
	rom = (uint16_t *)base;
	for (int i = 0; i < 0x0c0000/2; i++)
		rom[i] = rom[0x700000/2 + nc_bitswap(i, 19,18,11,6,14,17,16,5,8,10,12,0,4,3,2,7,9,15,13,1)];
}

void lithogen_garou_decrypt_68k(uint8_t* base)
{
	uint16_t *rom = (uint16_t *)(base + 0x100000);

	// swap data lines on the whole ROMs
	for (int i = 0; i < 0x800000/2; i++)
		rom[i] = nc_bitswap(rom[i], 16,13,12,14,10,8,2,3,1,5,9,11,4,15,0,6,7);

	// swap address lines & relocate fixed part
	rom = (uint16_t *)base;
	for (int i = 0; i < 0x0c0000/2; i++)
		rom[i] = rom[0x710000/2 + nc_bitswap(i, 19,18,4,5,16,14,7,9,6,13,17,15,3,1,2,12,11,8,10,0)];

	// swap address lines for the banked part
	rom = (uint16_t *)(base + 0x100000);
	for (int i = 0; i < 0x800000/2; i += 0x8000/2)
	{
		uint16_t buffer[0x8000/2];
		memcpy(buffer, &rom[i], 0x8000);
		for (int j = 0; j < 0x8000/2; j++)
			rom[i+j] = buffer[nc_bitswap(j, 14,9,4,8,3,13,6,2,7,0,12,1,11,10,5)];
	}
}

void lithogen_garouh_decrypt_68k(uint8_t* base)
{
	uint16_t *rom = (uint16_t *)(base + 0x100000);

	// swap data lines on the whole ROMs
	for (int i = 0; i < 0x800000/2; i++)
		rom[i] = nc_bitswap(rom[i], 16,14,5,1,11,7,4,10,15,3,12,8,13,0,2,9,6);

	// swap address lines & relocate fixed part
	rom = (uint16_t *)base;
	for (int i = 0; i < 0x0c0000/2; i++)
		rom[i] = rom[0x7f8000/2 + nc_bitswap(i, 19,18,5,16,11,2,6,7,17,3,12,8,14,4,0,9,1,10,15,13)];

	// swap address lines for the banked part
	rom = (uint16_t *)(base + 0x100000);
	for (int i = 0; i < 0x800000/2; i += 0x8000/2)
	{
		uint16_t buffer[0x8000/2];
		memcpy(buffer, &rom[i], 0x8000);
		for (int j = 0; j < 0x8000/2; j++)
			rom[i+j] = buffer[nc_bitswap(j, 14,12,8,1,7,11,3,13,10,6,9,5,4,0,2)];
	}
}

void lithogen_mslug3_decrypt_68k(uint8_t* base)
{
	uint16_t *rom = (uint16_t *)(base + 0x100000);

	// swap data lines on the whole ROMs
	for (int i = 0; i < 0x800000/2; i++)
		rom[i] = nc_bitswap(rom[i], 16,4,11,14,3,1,13,0,7,2,8,12,15,10,9,5,6);

	// swap address lines & relocate fixed part
	rom = (uint16_t *)base;
	for (int i = 0; i < 0x0c0000/2; i++)
		rom[i] = rom[0x5d0000/2 + nc_bitswap(i, 19,18,15,2,1,13,3,0,9,6,16,4,11,5,7,12,17,14,10,8)];

	// swap address lines for the banked part
	rom = (uint16_t *)(base + 0x100000);
	for (int i = 0; i < 0x800000/2; i += 0x10000/2)
	{
		uint16_t buffer[0x10000/2];
		memcpy(buffer, &rom[i], 0x10000);
		for (int j = 0; j < 0x10000/2; j++)
			rom[i+j] = buffer[nc_bitswap(j, 15,2,11,0,14,6,4,13,8,9,3,10,7,5,12,1)];
	}
}

void lithogen_mslug3a_decrypt_68k(uint8_t* base)
{
	uint16_t *rom = (uint16_t *)(base + 0x100000);

	/* swap data lines on the whole ROMs */
	for (int i = 0;i < 0x800000/2;i++)
		rom[i] = nc_bitswap(rom[i], 16,2,11,12,14,9,3,1,4,13,7,6,8,10,15,0,5);

	/* swap address lines & relocate fixed part */
	rom = (uint16_t *)base;
		for (int i = 0; i < 0x0c0000/2; i++)
		   rom[i] = rom[0x5d0000/2 + nc_bitswap(i, 19,18,1,16,14,7,17,5,8,4,15,6,3,2,0,13,10,12,9,11)];

	rom = (uint16_t *)(base + 0x100000);
	/* swap address lines for the banked part */
	for (int i = 0;i < 0x800000/2; i += 0x10000/2)
	{
		uint16_t buffer[0x10000/2];
		memcpy(buffer,&rom[i],0x10000);
		for (int j = 0;j < 0x10000/2;j++)
			rom[i+j] = buffer[nc_bitswap(j, 15,12,0,11,3,4,13,6,8,14,7,5,2,10,9,1)];
	}
}

void lithogen_kof2000_decrypt_68k(uint8_t* base)
{
	uint16_t *rom = (uint16_t *)(base + 0x100000);

	// swap data lines on the whole ROMs
	for (int i = 0; i < 0x800000/2; i++)
		rom[i] = nc_bitswap(rom[i], 16,12,8,11,3,15,14,7,0,10,13,6,5,9,2,1,4);

	// swap address lines for the banked part
	for (int i = 0; i < 0x63a000/2; i += 0x800/2)
	{
		uint16_t buffer[0x800/2];
		memcpy(buffer, &rom[i], 0x800);
		for (int j = 0; j < 0x800/2; j++)
			rom[i+j] = buffer[nc_bitswap(j, 10,4,1,3,8,6,2,7,0,9,5)];
	}

	// swap address lines & relocate fixed part
	rom = (uint16_t *)base;
	for (int i = 0; i < 0x0c0000/2; i++)
		rom[i] = rom[0x73a000/2 + nc_bitswap(i, 19,18,8,4,15,13,3,14,16,2,6,17,7,12,10,0,5,11,1,9)];
}

