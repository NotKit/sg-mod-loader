#include <cstdint>
#include <fstream>
#include <list>
#include "SGModLoader.h"

#ifndef CODES_H
#define CODES_H
enum CodeType : uint8_t
{
	write8, write16, write32, writefloat,
	and8, and16, and32,
	or8, or16, or32,
	xor8, xor16, xor32,
	ifeq8, ifeq16, ifeq32, ifeqfloat,
	ifne8, ifne16, ifne32, ifnefloat,
	ifltu8, ifltu16, ifltu32, ifltfloat,
	iflts8, iflts16, iflts32,
	ifltequ8, ifltequ16, ifltequ32, iflteqfloat,
	iflteqs8, iflteqs16, iflteqs32,
	ifgtu8, ifgtu16, ifgtu32, ifgtfloat,
	ifgts8, ifgts16, ifgts32,
	ifgtequ8, ifgtequ16, ifgtequ32, ifgteqfloat,
	ifgteqs8, ifgteqs16, ifgteqs32,
	ifmask8, ifmask16, ifmask32,
	ifkbkey,
	_else,
	endif
};

struct Code
{
	CodeType type;
	void *address;
	bool pointer;
	int offsetcount;
	int32_t *offsets;
	uint32_t value;
	std::list<Code> trueCodes;
	std::list<Code> falseCodes;
};

void *GetAddress(Code &code);
void ProcessCodeList(std::list<Code> &codes);
unsigned char ReadCodes(std::istream &stream, std::list<Code> &list);
#endif
