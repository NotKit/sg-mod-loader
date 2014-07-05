#include "ninja.h"
#ifndef SGMODLOADER_H
#define SGMODLOADER_H

static const int ModLoaderVer = 2;

#define arrayptrandlength(data) data, LengthOfArray(data)
#define arraylengthandptr(data) LengthOfArray(data), data
#define arrayptrandsize(data) data, SizeOfArray(data)
#define arraysizeandptr(data) SizeOfArray(data), data

struct PatchInfo
{
	void *address;
	void *data;
	int datasize;
};

#define patchdecl(address,data) { (void*)address, arrayptrandsize(data) }

struct PointerInfo
{
	void *address;
	void *data;
};

#define ptrdecl(address,data) { (void*)address, (void*)data }

struct ExportInfo
{
	const char *const name;
	void *data;
};

struct ModInfo
{
	int Version;
	void (*Init)(const char *path);
	PatchInfo *Patches;
	int PatchCount;
	PointerInfo *Jumps;
	int JumpCount;
	PointerInfo *Calls;
	int CallCount;
	PointerInfo *Pointers;
	int PointerCount;
	ExportInfo *Exports;
	int ExportCount;
};

// Utility Functions
template <typename T, size_t N>
inline size_t LengthOfArray( const T(&)[ N ] )
{
	return N;
}

template <typename T, size_t N>
inline size_t SizeOfArray( const T(&)[ N ] )
{
	return N * sizeof(T);
}

static BOOL WriteData(void *writeaddress, void *data, SIZE_T datasize, SIZE_T *byteswritten)
{
	return WriteProcessMemory(GetCurrentProcess(), writeaddress, data, datasize, byteswritten);
}

static BOOL WriteData(void *writeaddress, void *data, SIZE_T datasize)
{
	SIZE_T written;
	return WriteData(writeaddress, data, datasize, &written);
}

template<typename T> static BOOL WriteData(void *writeaddress, T data, SIZE_T *byteswritten)
{
	return WriteData(writeaddress, &data, sizeof(data), byteswritten);
}

template<typename T> static BOOL WriteData(void *writeaddress, T data)
{
	SIZE_T written;
	return WriteData(writeaddress, data, &written);
}

template <typename T, size_t N> static BOOL WriteData(void *writeaddress, T(&data)[N], SIZE_T *byteswritten)
{
	return WriteData(writeaddress, data, SizeOfArray(data), byteswritten);
}

template <typename T, size_t N> static BOOL WriteData(void *writeaddress, T(&data)[N])
{
	SIZE_T written;
	return WriteData(writeaddress, data, &written);
}

static BOOL WriteJump(void *writeaddress, void *funcaddress)
{
	unsigned char data[5];
	data[0] = 0xE9;
	*(signed int *)(data + 1) = (unsigned int)funcaddress - ((unsigned int)writeaddress + 5);
	return WriteData(writeaddress, data);
}

static BOOL WriteCall(void *writeaddress, void *funcaddress)
{
	unsigned char data[5];
	data[0] = 0xE8;
	*(signed int *)(data + 1) = (unsigned int)funcaddress - ((unsigned int)writeaddress + 5);
	return WriteData(writeaddress, data);
}

static __declspec(naked) int __fastcall jump(void *funcaddress) {
   __asm {
	 sub esp, 4
	 jmp ecx
   }
}

static

// SA2 Enums
#define makemask(enumname,membername) enumname##_##membername = (1 << enumname##Bits_##membername)
#define makemasks(enumname,membername) enumname##s_##membername = (1 << enumname##Bits_##membername)

#endif