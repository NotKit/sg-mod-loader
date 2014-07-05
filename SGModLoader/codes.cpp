#include "stdafx.h"
#include <cstdint>
#include <fstream>
#include <string>
#include "codes.h"

using namespace std;

void *GetAddress(Code &code)
{
	if (!code.pointer)
		return code.address;
	void *addr = code.address;
	addr = *(void **)addr;
	if (code.offsetcount == 0 || addr == nullptr)
		return addr;
	for (int i = 0; i < code.offsetcount - 1; i++)
	{
		addr = (void *)((uint32_t)addr + code.offsets[i]);
		addr = *(void **)addr;
		if (addr == nullptr)
			return nullptr;
	}
	addr = (void *)((uint32_t)addr + code.offsets[code.offsetcount - 1]);
	return addr;
}

#define ifcode(size,op) if (*(uint##size##_t *)address op (uint##size##_t)it->value) \
	ProcessCodeList(it->trueCodes); \
else \
	ProcessCodeList(it->falseCodes);

#define ifcodes(size,op) if (*(int##size##_t *)address op (int##size##_t)it->value) \
	ProcessCodeList(it->trueCodes); \
else \
	ProcessCodeList(it->falseCodes);

#define ifcodef(op) if (*(float *)address op *(float *)&it->value) \
	ProcessCodeList(it->trueCodes); \
else \
	ProcessCodeList(it->falseCodes);

void ProcessCodeList(list<Code> &codes)
{
	for (list<Code>::iterator it = codes.begin(); it != codes.end(); it++)
	{
		void *address = GetAddress(*it);
		if (it->type != ifkbkey && address == nullptr)
		{
			if (distance(it->falseCodes.begin(), it->falseCodes.end()) > 0)
				ProcessCodeList(it->falseCodes);
			continue;
		}
		switch (it->type)
		{
		case write8:
			WriteData(address, (uint8_t)it->value);
			break;
		case write16:
			WriteData(address, (uint16_t)it->value);
			break;
		case write32:
		case writefloat:
			WriteData(address, (uint32_t)it->value);
			break;
		case and8:
			WriteData(address, (uint8_t)(*(uint8_t *)address & it->value));
			break;
		case and16:
			WriteData(address, (uint16_t)(*(uint16_t *)address & it->value));
			break;
		case and32:
			WriteData(address, (uint32_t)(*(uint32_t *)address & it->value));
			break;
		case or8:
			WriteData(address, (uint8_t)(*(uint8_t *)address | it->value));
			break;
		case or16:
			WriteData(address, (uint16_t)(*(uint16_t *)address | it->value));
			break;
		case or32:
			WriteData(address, (uint32_t)(*(uint32_t *)address | it->value));
			break;
		case xor8:
			WriteData(address, (uint8_t)(*(uint8_t *)address ^ it->value));
			break;
		case xor16:
			WriteData(address, (uint16_t)(*(uint16_t *)address ^ it->value));
			break;
		case xor32:
			WriteData(address, (uint32_t)(*(uint32_t *)address ^ it->value));
			break;
		case ifeq8:
			ifcode(8,==)
			break;
		case ifeq16:
			ifcode(16,==)
			break;
		case ifeq32:
			ifcode(32,==)
			break;
		case ifeqfloat:
			ifcodef(==)
			break;
		case ifne8:
			ifcode(8,!=)
			break;
		case ifne16:
			ifcode(16,!=)
			break;
		case ifne32:
			ifcode(32,!=)
			break;
		case ifnefloat:
			ifcodef(!=)
			break;
		case ifltu8:
			ifcode(8,<)
			break;
		case ifltu16:
			ifcode(16,<)
			break;
		case ifltu32:
			ifcode(32,<)
			break;
		case ifltfloat:
			ifcodef(<)
			break;
		case iflts8:
			ifcodes(8,<)
			break;
		case iflts16:
			ifcodes(16,<)
			break;
		case iflts32:
			ifcodes(32,<)
			break;
		case ifltequ8:
			ifcode(8,<=)
			break;
		case ifltequ16:
			ifcode(16,<=)
			break;
		case ifltequ32:
			ifcode(32,<=)
			break;
		case iflteqfloat:
			ifcodef(<=)
			break;
		case iflteqs8:
			ifcodes(8,<=)
			break;
		case iflteqs16:
			ifcodes(16,<=)
			break;
		case iflteqs32:
			ifcodes(32,<=)
			break;
		case ifgtu8:
			ifcode(8,>)
			break;
		case ifgtu16:
			ifcode(16,>)
			break;
		case ifgtu32:
			ifcode(32,>)
			break;
		case ifgtfloat:
			ifcodef(>)
			break;
		case ifgts8:
			ifcodes(8,>)
			break;
		case ifgts16:
			ifcodes(16,>)
			break;
		case ifgts32:
			ifcodes(32,>)
			break;
		case ifgtequ8:
			ifcode(8,>=)
			break;
		case ifgtequ16:
			ifcode(16,>=)
			break;
		case ifgtequ32:
			ifcode(32,>=)
			break;
		case ifgteqfloat:
			ifcodef(>=)
			break;
		case ifgteqs8:
			ifcodes(8,>=)
			break;
		case ifgteqs16:
			ifcodes(16,>=)
			break;
		case ifgteqs32:
			ifcodes(32,>=)
			break;
		case ifmask8:
			if ((*(uint8_t *)address & (uint8_t)it->value) == (uint8_t)it->value)
				ProcessCodeList(it->trueCodes);
			else
				ProcessCodeList(it->falseCodes);
			break;
		case ifmask16:
			if ((*(uint16_t *)address & (uint16_t)it->value) == (uint16_t)it->value)
				ProcessCodeList(it->trueCodes);
			else
				ProcessCodeList(it->falseCodes);
			break;
		case ifmask32:
			if ((*(uint32_t *)address & it->value) == it->value)
				ProcessCodeList(it->trueCodes);
			else
				ProcessCodeList(it->falseCodes);
			break;
		case ifkbkey:
			if (GetAsyncKeyState(it->value))
				ProcessCodeList(it->trueCodes);
			else
				ProcessCodeList(it->falseCodes);
			break;
		}
	}
}

unsigned char ReadCodes(istream &stream, list<Code> &list)
{
	while (true)
	{
		uint8_t t = stream.get();
		if (t == 0xFF || t == _else || t == endif)
			return t;
		Code code = { };
		code.pointer = (t & 0x80) == 0x80;
		code.type = (CodeType)(t & 0x7F);
		stream.read((char *)&code.address, sizeof(void *));
		if (code.pointer)
		{
			code.offsetcount = stream.get();
			code.offsets = new int[code.offsetcount];
			for (int i = 0; i < code.offsetcount; i++)
				stream.read((char *)&code.offsets[i], sizeof(int32_t));
		}
		stream.read((char *)&code.value, sizeof(uint32_t));
		if (code.type >= ifeq8 && code.type <= ifkbkey)
			switch (ReadCodes(stream, code.trueCodes))
			{
			case _else:
				if (ReadCodes(stream, code.falseCodes) == 0xFF)
					return 0xFF;
				break;
			case 0xFF:
				return 0xFF;
			}
		list.push_back(code);
	}
	return 0;
}
