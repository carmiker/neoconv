/* SPDX-License-Identifier: BSD-3-Clause
 * Ported to ISO C99 from MAME (prot_misc.cpp) for liblithogen.
 * Original copyright-holders: S. Smith, David Haywood, Fabio Priuli
 * and the MAME development team.  This file is a mechanical translation;
 * algorithms and tables are reproduced from the MAME source.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"

void lithogen_cx_decrypt(uint8_t*sprrom, uint32_t sprrom_size)
{
	int cx_size = sprrom_size;
	uint8_t *rom = sprrom;
	uint8_t *buf = (uint8_t *)calloc((size_t)(cx_size), sizeof(uint8_t)); 

	memcpy(buf, rom, cx_size);

	for (int i = 0; i < cx_size / 0x40; i++)
		memcpy(&rom[i * 0x40], &buf[(i ^ 1) * 0x40], 0x40);
free(buf);
	}

void lithogen_sx_decrypt(uint8_t* fixed, uint32_t fixed_size, int value)
{
	int sx_size = fixed_size;
	uint8_t *rom = fixed;

	if (value == 1)
	{
		uint8_t *buf = (uint8_t *)calloc((size_t)(sx_size), sizeof(uint8_t)); 
		memcpy(buf, rom, sx_size);

		for (int i = 0; i < sx_size; i += 0x10)
		{
			memcpy(&rom[i], &buf[i + 8], 8);
			memcpy(&rom[i + 8], &buf[i], 8);
		}
	free(buf);
	}
	else if (value == 2)
	{
		for (int i = 0; i < sx_size; i++)
			rom[i] = nc_bitswap(rom[i], 8, 7, 6, 0, 4, 3, 2, 1, 5);
	}
}

void lithogen_kof97oro_px_decode(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint16_t *tmp = (uint16_t *)calloc((size_t)(0x500000), sizeof(uint16_t)); 
	uint16_t *src = (uint16_t*)cpurom;

	for (int i = 0; i < 0x500000/2; i++)
		tmp[i] = src[i ^ 0x7ffef];

	memcpy(src, tmp, 0x500000);
free(tmp);
	}

void lithogen_kf10thep_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint16_t *rom = (uint16_t*)cpurom;
	uint16_t *buf = (uint16_t *)calloc((size_t)(0x100000/2), sizeof(uint16_t)); 

	memcpy(&buf[0x000000/2], &rom[0x060000/2], 0x20000);
	memcpy(&buf[0x020000/2], &rom[0x100000/2], 0x20000);
	memcpy(&buf[0x040000/2], &rom[0x0e0000/2], 0x20000);
	memcpy(&buf[0x060000/2], &rom[0x180000/2], 0x20000);
	memcpy(&buf[0x080000/2], &rom[0x020000/2], 0x20000);
	memcpy(&buf[0x0a0000/2], &rom[0x140000/2], 0x20000);
	memcpy(&buf[0x0c0000/2], &rom[0x0c0000/2], 0x20000);
	memcpy(&buf[0x0e0000/2], &rom[0x1a0000/2], 0x20000);
	memcpy(&buf[0x0002e0/2], &rom[0x0402e0/2], 0x6a);  // copy banked code to a new memory region
	memcpy(&buf[0x0f92bc/2], &rom[0x0492bc/2], 0xb9e); // copy banked code to a new memory region
	memcpy(rom, buf, 0x100000);

	for (int i = 0xf92bc/2; i < 0xf9e58/2; i++)
	{
		if (rom[i+0] == 0x4eb9 && rom[i+1] == 0x0000) rom[i+1] = 0x000F; // correct JSR in moved code
		if (rom[i+0] == 0x4ef9 && rom[i+1] == 0x0000) rom[i+1] = 0x000F; // correct JMP in moved code
	}
	rom[0x00342/2] = 0x000f;

	memmove(&rom[0x100000/2], &rom[0x200000/2], 0x600000);
free(buf);
	}

void lithogen_kf2k5uni_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *src = cpurom;
	uint8_t dst[0x80];

	for (int i = 0; i < 0x800000; i += 0x80)
	{
		for (int j = 0; j < 0x80; j += 2)
		{
			int ofst = nc_bitswap(j, 8, 0, 3, 4, 5, 6, 1, 2, 7);
			memcpy(&dst[j], src + i + ofst, 2);
		}
		memcpy(src + i, dst, 0x80);
	}

	memcpy(src, src + 0x600000, 0x100000); // Seems to be the same as kof10th
}

void lithogen_kf2k5uni_sx_decrypt(uint8_t* fixedrom, uint32_t fixedrom_size)
{
	uint8_t *srom = fixedrom;

	for (int i = 0; i < 0x20000; i++)
		srom[i] = nc_bitswap(srom[i], 8, 4, 5, 6, 7, 0, 1, 2, 3);
}

void lithogen_kf2k5uni_mx_decrypt(uint8_t* audiorom, uint32_t audiorom_size)
{
	uint8_t *mrom = audiorom;

	for (int i = 0; i < 0x30000; i++)
		mrom[i] = nc_bitswap(mrom[i], 8, 4, 5, 6, 7, 0, 1, 2, 3);
}

void lithogen_decrypt_kof2k4se_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *src = cpurom + 0x100000;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x400000), sizeof(uint8_t)); 
	static const int sec[] = {0x300000,0x200000,0x100000,0x000000};
	memcpy(dst, src, 0x400000);

	for (int i = 0; i < 4; ++i)
		memcpy(src + i * 0x100000, &dst[sec[i]], 0x100000);
free(dst);
	}

void lithogen_lans2004_vx_decrypt(uint8_t* ymsndrom, uint32_t ymsndrom_size)
{
	uint8_t *rom = ymsndrom;
	for (int i = 0; i < 0xA00000; i++)
		rom[i] = nc_bitswap(rom[i], 8, 0, 1, 5, 4, 3, 2, 6, 7);
}

void lithogen_lans2004_decrypt_68k(uint8_t* cpurom, uint32_t cpurom_size)
{
	// Descrambling P ROMs - Thanks to Razoola for the info
	uint8_t *src = cpurom;
	uint16_t *rom = (uint16_t*)cpurom;

	static const int sec[] = { 0x3, 0x8, 0x7, 0xc, 0x1, 0xa, 0x6, 0xd };
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x600000), sizeof(uint8_t)); 

	for (int i = 0; i < 8; i++)
		memcpy (&dst[i * 0x20000], src + sec[i] * 0x20000, 0x20000);

	memcpy (&dst[0x0bbb00], src + 0x045b00, 0x001710);
	memcpy (&dst[0x02fff0], src + 0x1a92be, 0x000010);
	memcpy (&dst[0x100000], src + 0x200000, 0x400000);
	memcpy (src, dst, 0x600000);

	for (int i = 0xbbb00/2; i < 0xbe000/2; i++)
	{
		if ((((rom[i] & 0xffbf)==0x4eb9) || ((rom[i] & 0xffbf)==0x43b9)) && (rom[i+1]==0x0000))
		{
			rom[i + 1] = 0x000b;
			rom[i + 2] += 0x6000;
		}
	}

	/* Patched by protection chip (Altera) ? */
	rom[0x2d15c/2] = 0x000b;
	rom[0x2d15e/2] = 0xbb00;
	rom[0x2d1e4/2] = 0x6002;
	rom[0x2ea7e/2] = 0x6002;
	rom[0xbbcd0/2] = 0x6002;
	rom[0xbbdf2/2] = 0x6002;
	rom[0xbbe42/2] = 0x6002;
free(dst);
	}

void lithogen_samsho5b_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	int px_size = cpurom_size;
	uint8_t *rom = cpurom;
	uint8_t *buf = (uint8_t *)calloc((size_t)(px_size), sizeof(uint8_t)); 

	memcpy(buf, rom, px_size);

	for (int i = 0; i < px_size / 2; i++)
	{
		int ofst = nc_bitswap((i & 0x000ff), 8, 7, 6, 5, 4, 3, 0, 1, 2);
		ofst += (i & 0xfffff00);
		ofst ^= 0x060005;

		memcpy(&rom[i * 2], &buf[ofst * 2], 0x02);
	}

	memcpy(buf, rom, px_size);

	memcpy(&rom[0x000000], &buf[0x700000], 0x100000);
	memcpy(&rom[0x100000], &buf[0x000000], 0x700000);
free(buf);
	}

void lithogen_samsho5b_vx_decrypt(uint8_t* ymsndrom, uint32_t ymsndrom_size)
{
	int vx_size = ymsndrom_size;
	uint8_t *rom = ymsndrom;

	for (int i = 0; i < vx_size; i++)
		rom[i] = nc_bitswap(rom[i], 8, 0, 1, 5, 4, 3, 2, 6, 7);
}

void lithogen_mslug5b_vx_decrypt(uint8_t* ymsndrom, uint32_t ymsndrom_size)
{
	// only odd bytes are scrambled
	int ym_size = ymsndrom_size;
	uint8_t *rom = ymsndrom;
	for (int i = 1; i < ym_size; i += 2)
		rom[i] = nc_bitswap(rom[i], 8, 3, 2, 4, 1, 5, 0, 6, 7);
}

void lithogen_mslug5b_cx_decrypt(uint8_t* sprrom, uint32_t sprrom_size)
{
	// rom a18/a19 lines are swapped
	int cx_size = sprrom_size;
	uint8_t *rom = sprrom;
	uint8_t *buf = (uint8_t *)calloc((size_t)(cx_size), sizeof(uint8_t)); 

	memcpy(buf, rom, cx_size);

	for (int i = 1; i < 128; i += 4)
	{
		memcpy(&rom[i * 0x80000], &buf[(i + 1) * 0x80000], 0x80000);
		memcpy(&rom[(i + 1) * 0x80000], &buf[i * 0x80000], 0x80000);
	}
free(buf);
	}

void lithogen_kog_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	// the protection chip does some *very* strange things to the rom
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x600000), sizeof(uint8_t)); 
	uint16_t *rom = (uint16_t *)cpurom;
	static const int sec[] = { 0x3, 0x8, 0x7, 0xc, 0x1, 0xa, 0x6, 0xd };

	for (int i = 0; i < 8; i++)
		memcpy (&dst[i * 0x20000], src + sec[i] * 0x20000, 0x20000);

	memcpy (&dst[0x0007a6], src + 0x0407a6, 0x000006);
	memcpy (&dst[0x0007c6], src + 0x0407c6, 0x000006);
	memcpy (&dst[0x0007e6], src + 0x0407e6, 0x000006);
	memcpy (&dst[0x090000], src + 0x040000, 0x004000);
	memcpy (&dst[0x100000], src + 0x200000, 0x400000);
	memcpy (src, dst, 0x600000);

	for (int i = 0x90000/2; i < 0x94000/2; i++)
	{
		if (((rom[i] & 0xffbf) == 0x4eb9 || rom[i] == 0x43f9) && !rom[i + 1])
			rom[i + 1] = 0x0009;

		if (rom[i] == 0x4eb8)
			rom[i] = 0x6100;
	}

	rom[0x007a8/2] = 0x0009;
	rom[0x007c8/2] = 0x0009;
	rom[0x007e8/2] = 0x0009;
	rom[0x93408/2] = 0xf168;
	rom[0x9340c/2] = 0xfb7a;
	rom[0x924ac/2] = 0x0009;
	rom[0x9251c/2] = 0x0009;
	rom[0x93966/2] = 0xffda;
	rom[0x93974/2] = 0xffcc;
	rom[0x93982/2] = 0xffbe;
	rom[0x93990/2] = 0xffb0;
	rom[0x9399e/2] = 0xffa2;
	rom[0x939ac/2] = 0xff94;
	rom[0x939ba/2] = 0xff86;
	rom[0x939c8/2] = 0xff78;
	rom[0x939d4/2] = 0xfa5c;
	rom[0x939e0/2] = 0xfa50;
	rom[0x939ec/2] = 0xfa44;
	rom[0x939f8/2] = 0xfa38;
	rom[0x93a04/2] = 0xfa2c;
	rom[0x93a10/2] = 0xfa20;
	rom[0x93a1c/2] = 0xfa14;
	rom[0x93a28/2] = 0xfa08;
	rom[0x93a34/2] = 0xf9fc;
	rom[0x93a40/2] = 0xf9f0;
	rom[0x93a4c/2] = 0xfd14;
	rom[0x93a58/2] = 0xfd08;
	rom[0x93a66/2] = 0xf9ca;
	rom[0x93a72/2] = 0xf9be;

free(dst);
	}

void lithogen_svcboot_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const uint8_t sec[] = { 0x06, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00 };
	int size = cpurom_size;
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(size), sizeof(uint8_t)); 

	for (int i = 0; i < size / 0x100000; i++)
		memcpy(&dst[i * 0x100000], &src[sec[i] * 0x100000], 0x100000);

	for (int i = 0; i < size / 2; i++)
	{
		int ofst = nc_bitswap((i & 0x0000ff), 8, 7, 6, 1, 0, 3, 2, 5, 4);
		ofst += (i & 0xffff00);
		memcpy(&src[i * 2], &dst[ofst * 2], 0x02);
	}
free(dst);
	}

void lithogen_svcboot_cx_decrypt(uint8_t* sprrom, uint32_t sprrom_size)
{
	static const uint8_t idx_tbl[ 0x10 ] = { 0, 1, 0, 1, 2, 3, 2, 3, 3, 4, 3, 4, 4, 5, 4, 5, };
	static const uint8_t bitswap4_tbl[ 6 ][ 4 ] = {
		{ 3, 0, 1, 2 },
		{ 2, 3, 0, 1 },
		{ 1, 2, 3, 0 },
		{ 0, 1, 2, 3 },
		{ 3, 2, 1, 0 },
		{ 3, 0, 2, 1 },
	};
	int size = sprrom_size;
	uint8_t *src = sprrom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(size), sizeof(uint8_t)); 

	memcpy(dst, src, size);
	for (int i = 0; i < size / 0x80; i++)
	{
		int idx = idx_tbl[(i & 0xf00) >> 8];
		int bit0 = bitswap4_tbl[idx][0];
		int bit1 = bitswap4_tbl[idx][1];
		int bit2 = bitswap4_tbl[idx][2];
		int bit3 = bitswap4_tbl[idx][3];
		int ofst = nc_bitswap((i & 0x0000ff), 8, 7, 6, 5, 4, bit3, bit2, bit1, bit0);
		ofst += (i & 0xfffff00);
		memcpy(&src[i * 0x80], &dst[ofst * 0x80], 0x80);
	}
free(dst);
	}

void lithogen_svcplus_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[] = { 0x00, 0x03, 0x02, 0x05, 0x04, 0x01 };
	int size = cpurom_size;
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(size), sizeof(uint8_t)); 

	memcpy(dst, src, size);
	for (int i = 0; i < size / 2; i++)
	{
		int ofst = nc_bitswap((i & 0xfffff), 24, 0x17, 0x16, 0x15, 0x14, 0x13, 0x00, 0x01, 0x02,
								0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
								0x07, 0x06, 0x05, 0x04, 0x03, 0x10, 0x11, 0x12);
		ofst ^= 0x0f0007;
		ofst += (i & 0xff00000);
		memcpy(&src[i * 0x02], &dst[ofst * 0x02], 0x02);
	}

	memcpy(dst, src, size);
	for (int i = 0; i < 6; i++)
		memcpy(&src[i * 0x100000], &dst[sec[i] * 0x100000], 0x100000);
free(dst);
	}

void lithogen_svcplus_px_hack(uint8_t* cpurom, uint32_t cpurom_size)
{
	/* patched by the protection chip? */
	uint16_t *mem16 = (uint16_t *)cpurom;
	mem16[0x0f8016/2] = 0x33c1;
}

void lithogen_svcplusa_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x00 };
	int size = cpurom_size;
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(size), sizeof(uint8_t)); 

	memcpy(dst, src, size);
	for (int i = 0; i < 6; i++)
		memcpy(&src[i * 0x100000], &dst[sec[i] * 0x100000], 0x100000);
free(dst);
	}

void lithogen_svcsplus_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	static const int sec[] = { 0x06, 0x07, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00 };
	int size = cpurom_size;
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(size), sizeof(uint8_t)); 

	memcpy(dst, src, size);
	for (int i = 0; i < size / 2; i++)
	{
		int ofst = nc_bitswap((i & 0x007fff), 16, 0x0f, 0x00, 0x08, 0x09, 0x0b, 0x0a, 0x0c, 0x0d,
								0x04, 0x03, 0x01, 0x07, 0x06, 0x02, 0x05, 0x0e);

		ofst += (i & 0x078000);
		ofst += sec[(i & 0xf80000) >> 19] << 19;
		memcpy(&src[i * 2], &dst[ofst * 2], 0x02);
	}
free(dst);
	}

void lithogen_svcsplus_px_hack(uint8_t* cpurom, uint32_t cpurom_size)
{
	/* patched by the protection chip? */
	uint16_t *mem16 = (uint16_t *)cpurom;
	mem16[0x9e90/2] = 0x000f;
	mem16[0x9e92/2] = 0xc9c0;
	mem16[0xa10c/2] = 0x4eb9;
	mem16[0xa10e/2] = 0x000e;
	mem16[0xa110/2] = 0x9750;
}

void lithogen_kof2002b_gfx_decrypt(uint8_t *src, int size)
{
	static const uint8_t t[8][6] =
	{
		{ 0, 8, 7, 6, 2, 1 },
		{ 1, 0, 8, 7, 6, 2 },
		{ 2, 1, 0, 8, 7, 6 },
		{ 6, 2, 1, 0, 8, 7 },
		{ 7, 6, 2, 1, 0, 8 },
		{ 0, 1, 2, 6, 7, 8 },
		{ 2, 1, 0, 6, 7, 8 },
		{ 8, 0, 7, 6, 2, 1 },
	};

	uint8_t *dst = (uint8_t *)calloc((size_t)(0x10000), sizeof(uint8_t)); 

	for (int i = 0; i < size; i += 0x10000)
	{
		memcpy(dst, src + i, 0x10000);

		for (int j = 0; j < 0x200; j++)
		{
			int n = (j & 0x38) >> 3;
			int ofst = nc_bitswap(j, 16, 15, 14, 13, 12, 11, 10, 9, t[n][0], t[n][1], t[n][2], 5, 4, 3, t[n][3], t[n][4], t[n][5]);
			memcpy(src + i + ofst * 128, &dst[j * 128], 128);
		}
	}
free(dst);
	}

void lithogen_kf2k2mp_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *src = cpurom;
	uint8_t dst[0x80];

	memmove(src, src + 0x300000, 0x500000);

	for (int i = 0; i < 0x800000; i+=0x80)
	{
		for (int j = 0; j < 0x80 / 2; j++)
		{
			int ofst = nc_bitswap(j, 8, 6, 7, 2, 3, 4, 5, 0, 1 );
			memcpy(dst + j * 2, src + i + ofst * 2, 2);
		}
		memcpy(src + i, dst, 0x80);
	}
}

void lithogen_kf2k2mp2_px_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *src = cpurom;
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x600000), sizeof(uint8_t)); 

	memcpy(&dst[0x000000], &src[0x1C0000], 0x040000);
	memcpy(&dst[0x040000], &src[0x140000], 0x080000);
	memcpy(&dst[0x0C0000], &src[0x100000], 0x040000);
	memcpy(&dst[0x100000], &src[0x200000], 0x400000);
	memcpy(&src[0x000000], &dst[0x000000], 0x600000);
free(dst);
	}

void lithogen_kof10th_decrypt(uint8_t* cpurom, uint32_t cpurom_size)
{
	uint8_t *dst = (uint8_t *)calloc((size_t)(0x900000), sizeof(uint8_t)); 
	uint8_t *src = cpurom;

	memcpy(&dst[0x000000], src + 0x700000, 0x100000); // Correct (Verified in Uni-bios)
	memcpy(&dst[0x100000], src + 0x000000, 0x800000);

	for (int i = 0; i < 0x900000; i++)
	{
		int j = nc_bitswap(i, 24,23,22,21,20,19,18,17,16,15,14,13,12,11,2,9,8,7,1,5,4,3,10,6,0);
		src[j] = dst[i];
	}
free(dst);
	}

