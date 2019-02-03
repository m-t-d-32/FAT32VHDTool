#include <stdio.h>
#include "exception.h"
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#ifndef DirectRead_H
#define DirectRead_H

#define SECTION_SIZE 512
#ifdef _MSC_VER
#define CreateFile CreateFileA
#endif

HANDLE hDev = INVALID_HANDLE_VALUE;

void _strcat(wchar_t * dest, const char * src){
	while (*dest) dest++;
	while (*src) *dest++ = *src++;
	*dest = 0;
}
void init_file(char * devname) {
	wchar_t full_devname[SECTION_SIZE] = L"\\\\.\\";
	_strcat(full_devname, devname);
	hDev = CreateFile(full_devname, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
}

unsigned read_bytes(unsigned begin, unsigned size){
	if (size > sizeof(unsigned))
		throw rdr;
	if (hDev == INVALID_HANDLE_VALUE)
		throw idr;
	
	unsigned char buffer[SECTION_SIZE] = {0};
	unsigned move_high = begin / SECTION_SIZE;
	unsigned move_low = begin % SECTION_SIZE;
	unsigned result_high;
	long unsigned result_size;
	
	unsigned long long reset_move = move_high * SECTION_SIZE;
	unsigned long long div_t = 1;
	div_t <<= sizeof(unsigned) * 8;
	long reset_move_high = reset_move / div_t;
	unsigned reset_move_low = reset_move % div_t;
	result_high = SetFilePointer(hDev, reset_move_low, &reset_move_high, FILE_BEGIN);
	if (reset_move_low != result_high)
		throw rdr;
	ReadFile(hDev, buffer, SECTION_SIZE, &result_size, NULL);
	if (result_size != SECTION_SIZE)
		throw rdr;
	if (result_size != SECTION_SIZE)
		throw rdr;
	unsigned result = 0;
	/* Read the begin bytes */
	unsigned i, j, temp_data;
	for (i = 0; i < (size + move_low <= SECTION_SIZE ? 
		size : (SECTION_SIZE - move_low))
		; ++i){
		temp_data = buffer[move_low + i];
		result |= (temp_data << (8 * i));
	}
	/* Read the after bytes (if exists) */
	if ( size + move_low > SECTION_SIZE ) {
		move_high = (size + move_low) / SECTION_SIZE; /* 1 */
		result_high = SetFilePointer(hDev, move_high * SECTION_SIZE, NULL, FILE_BEGIN);
		if (move_high * SECTION_SIZE != result_high)
			throw rdr;
		ReadFile(hDev, buffer, SECTION_SIZE, &result_size, NULL);	
		if (result_size != SECTION_SIZE)
			throw rdr;
		for (j = 0; j < size + move_low - SECTION_SIZE; ++j){
			temp_data = buffer[j];
			result |= (temp_data << (8 * (i + j)));
		}
	}
	return result;
}

void close_file() {
	CloseHandle(hDev);
	hDev = INVALID_HANDLE_VALUE;
}

#endif