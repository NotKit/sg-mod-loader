// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <DbgHelp.h>
#include <cstdio>
#include "SGModLoader.h"
#include "codes.h"
using namespace std;

typedef unordered_map<string, string> IniGroup;
struct IniGroupStr { IniGroup Element; };
typedef unordered_map<string, IniGroupStr> IniDictionary;
IniDictionary LoadINI(istream &textfile)
{
	IniDictionary result = IniDictionary();
	result[""] = IniGroupStr();
	IniGroupStr *curent = &result[""];
	while (textfile.good())
	{
		string line;
		getline(textfile, line);
		string sb = string();
		sb.reserve(line.length());
		bool startswithbracket = false;
		int firstequals = -1;
		int endbracket = -1;
		for (int c = 0; c < (int)line.length(); c++)
			switch (line[c])
			{
			case '\\': // escape character
				if (c + 1 == line.length())
					goto appendchar;
				c++;
				switch (line[c])
				{
				case 'n': // line feed
					sb += '\n';
					break;
				case 'r': // carriage return
					sb += '\r';
					break;
				case '\\':
					goto appendchar;
				default: // literal character
					sb += '\\';
					goto appendchar;
				}
				break;
			case '=':
				if (firstequals == -1)
					firstequals = sb.length();
				goto appendchar;
			case '[':
				if (c == 0)
					startswithbracket = true;
				goto appendchar;
			case ']':
				endbracket = sb.length();
				goto appendchar;
			case ';': // comment character, stop processing this line
				c = line.length();
				break;
			default:
appendchar:
				sb += line[c];
				break;
			}
		line = sb;
		if (startswithbracket && endbracket != -1)
		{
			line = line.substr(1, endbracket - 1);
			result[line] = IniGroupStr();
			curent = &result[line];
		}
		else if (!line.empty())
		{
			string key;
			string value = "";
			if (firstequals > -1)
			{
				key = line.substr(0, firstequals);
				value = line.substr(firstequals + 1);
				// strip quotaion marks around values
				if (value[0] == '"' && value[value.length() - 1] == '"')
					value = value.substr(1, value.length() - 2);
			}
			else
				key = line;
			(*curent).Element[key] = value;
		}
	}
	return result;
}

HMODULE myhandle;
HMODULE datadllhandle;
unordered_map<string, void *> dataoverrides = unordered_map<string, void *>();
FARPROC __stdcall MyGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	if (hModule == myhandle)
	{
		unordered_map<string, void *>::iterator iter = dataoverrides.find(lpProcName);
		if (iter != dataoverrides.end())
			return (FARPROC)iter->second;
		else
			return GetProcAddress(datadllhandle, lpProcName);
	}
	else
		return GetProcAddress(hModule, lpProcName);
}

inline int backslashes(int c)
{
	if (c == '/')
		return '\\';
	else
		return c;
}

string &basedir(string &path)
{
	string dir = "";
	size_t lastslash = path.find_last_of("/\\");
	if (lastslash != string::npos)
		dir = path.substr(0, lastslash + 1);
	return dir;
}


IniGroup settings;
IniGroup cpkredir;
unordered_map<string, char *> filemap = unordered_map<string, char *>();
unordered_map<string, string> filereplaces = unordered_map<string, string>();
const string resourcedir = "resource\\gd_pc\\";
string sa2dir;
const string savedatadir = "resource\\gd_pc\\savedata\\";
CRITICAL_SECTION filereplacesection;
string exefilename;
list<Code> codes = list<Code>();

const char *_ReplaceFile(const char *lpFileName)
{
	EnterCriticalSection(&filereplacesection);
	string path = lpFileName;
	transform(path.begin(), path.end(), path.begin(), backslashes);
	if (path.length() > 2 && (path[0] == '.' && path[1] == '\\'))
		path = path.substr(2, path.length() - 2);
	transform(path.begin(), path.end(), path.begin(), ::tolower);
	if (path.length() > sa2dir.length() && path.compare(0, sa2dir.length(), sa2dir) == 0)
		path = path.substr(sa2dir.length(), path.length() - sa2dir.length());
	unordered_map<string, string>::iterator replIter = filereplaces.find(path);
	if (replIter != filereplaces.end())
		path = replIter->second;
	unordered_map<string, char *>::iterator fileIter = filemap.find(path);
	if (fileIter != filemap.end())
		lpFileName = fileIter->second;
	LeaveCriticalSection(&filereplacesection);
	return lpFileName;
}

HANDLE __stdcall MyCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	return CreateFileA(_ReplaceFile(lpFileName), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

void HookTheAPI()
{
    ULONG ulSize = 0;
    PROC pNewFunction = NULL;
    PROC pActualFunction = NULL;

    PSTR pszModName = NULL;

    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = NULL;

    pNewFunction = (PROC)MyGetProcAddress ;
	PROC pNewCreateFile = (PROC)MyCreateFileA;
    pActualFunction = GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "GetProcAddress");
	PROC pActualCreateFile = GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "CreateFileA");

    pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryEntryToData(
                                                    hModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);

    if(NULL != pImportDesc)
    {
        for (; pImportDesc->Name; pImportDesc++)
        {
            // get the module name
            pszModName = (PSTR) ((PBYTE) hModule + pImportDesc->Name);

            if(NULL != pszModName)
            {
                // check if the module is kernel32.dll
                if (lstrcmpiA(pszModName, "Kernel32.dll") == 0)
                {
                    // get the module
                    PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA) ((PBYTE) hModule + pImportDesc->FirstThunk);

                     for (; pThunk->u1.Function; pThunk++) 
                     {
                        PROC* ppfn = (PROC*) &pThunk->u1.Function;
                        if(*ppfn == pActualFunction)
                        {
                                DWORD dwOldProtect = 0;
                                VirtualProtect(ppfn, sizeof(pNewFunction), PAGE_WRITECOPY,&dwOldProtect);
                                     WriteProcessMemory(GetCurrentProcess(), ppfn, &pNewFunction, sizeof(pNewFunction), NULL);
                                     VirtualProtect(ppfn, sizeof(pNewFunction), dwOldProtect,&dwOldProtect);
                        } // Function that we are looking for
						else if (*ppfn == pActualCreateFile)
						{
                                DWORD dwOldProtect = 0;
                                VirtualProtect(ppfn, sizeof(pNewCreateFile), PAGE_WRITECOPY,&dwOldProtect);
                                     WriteProcessMemory(GetCurrentProcess(), ppfn, &pNewCreateFile, sizeof(pNewCreateFile), NULL);
                                     VirtualProtect(ppfn, sizeof(pNewCreateFile), dwOldProtect,&dwOldProtect);
						}
                     }
                } // Compare module name
            } // Valid module name
        }
    }
}

string NormalizePath(string path)
{
	string pathlower = path;
	if (pathlower.length() > 2 && (pathlower[0] == '.' && pathlower[1] == '\\'))
		pathlower = pathlower.substr(2, pathlower.length() - 2);
	transform(pathlower.begin(), pathlower.end(), pathlower.begin(), ::tolower);
	return pathlower;
}

void ScanFolder(string path, int length)
{
	_WIN32_FIND_DATAA data;
	HANDLE hfind = FindFirstFileA((path + "\\*").c_str(), &data);
	if (hfind == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (data.cFileName[0] == '.')
			continue;
		else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			ScanFolder(path + "\\" + data.cFileName, length);
		else
		{
			string filebase = path + "\\" + data.cFileName;
			transform(filebase.begin(), filebase.end(), filebase.begin(), ::tolower);
			string modfile = filebase;
			filebase = filebase.substr(length);
			string origfile = resourcedir + filebase;
			char *buf = new char[modfile.length() + 1];
			if (filemap.find(origfile) != filemap.end())
				delete[] filemap[origfile];
			filemap[origfile] = buf;
			modfile.copy(buf, modfile.length());
			buf[modfile.length()] = 0;
			printf("Replaced file: \"%s\" = \"%s\"\n", origfile.c_str(), buf);
		}
	}
	while (FindNextFileA(hfind, &data) != 0);
	FindClose(hfind);
}

void LoadMod(IniDictionary &modini, IniGroup &modinfo, string &dir, bool console)
{
	IniDictionary::iterator gr = modini.find("IgnoreFiles");
	if (gr != modini.end())
	{
		IniGroup replaces = gr->second.Element;
		for (IniGroup::iterator it = replaces.begin(); it != replaces.end(); it++)
		{
			filemap[NormalizePath(it->first)] = "nullfile";
			if (console)
				printf("Ignored file: %s\n", it->first.c_str());
		}
	}
	gr = modini.find("ReplaceFiles");
	if (gr != modini.end())
	{
		IniGroup replaces = gr->second.Element;
		for (IniGroup::iterator it = replaces.begin(); it != replaces.end(); it++)
			filereplaces[NormalizePath(it->first)] = NormalizePath(it->second);
	}
	gr = modini.find("SwapFiles");
	if (gr != modini.end())
	{
		IniGroup replaces = gr->second.Element;
		for (IniGroup::iterator it = replaces.begin(); it != replaces.end(); it++)
		{
			filereplaces[NormalizePath(it->first)] = NormalizePath(it->second);
			filereplaces[NormalizePath(it->second)] = NormalizePath(it->first);
		}
	}
	/*string sysfol = dir;
	transform(sysfol.begin(), sysfol.end(), sysfol.begin(), ::tolower);
	if (GetFileAttributesA(sysfol.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
		ScanFolder(sysfol, sysfol.length() + 1);*/
	if (modinfo.find("EXEFile") != modinfo.end())
	{
		string modexe = modinfo["EXEFile"];
		transform(modexe.begin(), modexe.end(), modexe.begin(), ::tolower);
		if (modexe.compare(exefilename) != 0)
		{
			const char *msg = ("Mod \"" + modinfo["Name"] + "\" should be run from \"" + modexe + "\", but you are running \"" + exefilename + "\".\n\nContinue anyway?").c_str();
			if (MessageBoxA(NULL, msg, "SG Mod Loader", MB_ICONWARNING | MB_YESNO) == IDNO)
				ExitProcess(1);
		}
	}
	string filename = modinfo["DLLFile"];
	if (!filename.empty())
	{
		filename = dir + "\\" + filename;
		HMODULE module = LoadLibraryA(filename.c_str());
		if (module)
		{
			ModInfo *info = (ModInfo *)GetProcAddress(module, "SGModInfo");
			if (info)
			{
				if (info->Patches)
					for (int i = 0; i < info->PatchCount; i++)
						WriteData(info->Patches[i].address, info->Patches[i].data, info->Patches[i].datasize);
				if (info->Jumps)
					for (int i = 0; i < info->JumpCount; i++)
						WriteJump(info->Jumps[i].address, info->Jumps[i].data);
				if (info->Calls)
					for (int i = 0; i < info->CallCount; i++)
						WriteCall(info->Calls[i].address, info->Calls[i].data);
				if (info->Pointers)
					for (int i = 0; i < info->PointerCount; i++)
						WriteData(info->Pointers[i].address, &info->Pointers[i].data, sizeof(void*));
				if (info->Exports)
					for (int i = 0; i < info->ExportCount; i++)
						dataoverrides[info->Exports[i].name] = info->Exports[i].data;
				if (info->Init)
					info->Init(dir.c_str());
			}
			else if (console)
				printf("File \"%s\" was loaded, but is not a valid mod file. Ignore this if it's stand-alone DLL.\n", filename.c_str());
		}
		else if (console)
			printf("Failed loading file \"%s\".\n", filename.c_str());
	}	
}

void __cdecl ProcessCodes()
{
	ProcessCodeList(codes);
}

void __cdecl InitMods(void)
{
	datadllhandle = LoadLibrary(L".\\cpkredir.dll");
	if (!datadllhandle)
	{
		if (MessageBoxA(NULL, "cpkredir.dll could not be loaded!\n\nContinue anyway?", "SG Mod Loader", MB_ICONWARNING | MB_YESNO) == IDNO)
			ExitProcess(1);
	}
	LoadLibrary(L".\\D3dHook.dll");
	//HookTheAPI();
	ifstream str = ifstream("cpkredir.ini");
	if (!str.is_open())
	{
		MessageBox(NULL, L"cpkredir.ini could not be read!", L"SG Mod Loader", MB_ICONWARNING);
		return;
	}
	IniDictionary ini = LoadINI(str);
	str.close();

	string modsdbpath = "mods\\ModsDB.ini";
	IniDictionary::iterator gr = ini.find("CPKREDIR");
	if (gr != ini.end()) {
		cpkredir = ini["CPKREDIR"].Element;
		string value = cpkredir["ModsDbIni"];
		if (!value.empty())
			modsdbpath = value;
	}

	bool console = false;
	gr = ini.find("ModLoader");
	if (gr != ini.end()) {
		settings = ini["ModLoader"].Element;
		string item = settings["ShowConsole"];
		transform(item.begin(), item.end(), item.begin(), ::tolower);

		if (item == "true")
		{
			AllocConsole();
			SetConsoleTitle(L"SG Mod Loader output");
			freopen("CONOUT$", "wb", stdout);
			console = true;
			printf("SG Mod Loader version %d, built %s\n", ModLoaderVer, __TIMESTAMP__);
			printf("Loading mods from %s...\n", modsdbpath.c_str());
		}
	}

	str = ifstream(modsdbpath);
	if (!str.is_open())
	{
		MessageBox(NULL, (LPCWSTR)(modsdbpath + " could not be read!").c_str(), L"SG Mod Loader", MB_ICONWARNING);
		return;
	}
	IniDictionary modsdb = LoadINI(str);
	str.close();
	IniGroup modsdbmain = modsdb["Main"].Element;
	IniGroup modsdbmods = modsdb["Mods"].Element;

	char pathbuf[MAX_PATH];
	GetModuleFileNameA(NULL, pathbuf, MAX_PATH);
	exefilename = pathbuf;
	exefilename = exefilename.substr(exefilename.find_last_of("/\\") + 1);
	transform(exefilename.begin(), exefilename.end(), exefilename.begin(), ::tolower);	
	string modsdbdir = basedir(modsdbpath);

	InitializeCriticalSection(&filereplacesection);
	DWORD oldprot;
	VirtualProtect((void *)0x400100, 0xfc1000, PAGE_WRITECOPY, &oldprot);
	char key[12];
	for (int i = 0; i < 999; i++)
	{
		sprintf_s(key, "ActiveMod%d", i);
		if (modsdbmain.find(key) == modsdbmain.end())
			break;
		string modinipath = modsdbdir + modsdbmods[modsdbmain[key]];
		string dir = basedir(modinipath);
		str = ifstream(modinipath);
		if (!str.is_open())
		{
			if (console)
				printf("Could not open %s.\n", modinipath);
			continue;
		}
		IniDictionary modini = LoadINI(str);
		IniGroup modinfo = modini["Main"].Element;	
		if (console)
			printf("%d. %s\n", i, modini["Desc"].Element["Title"].c_str());
		LoadMod(modini, modinfo, dir, console);
	}
	printf("Mod loading finished.\n");
	str = ifstream("mods\\Codes.dat", ifstream::binary);
	if (str.is_open())
	{
		int32_t codecount;
		str.read((char *)&codecount, sizeof(int32_t));
		printf("Loading %d codes...\n", codecount);
		ReadCodes(str, codes);
	}
	str.close();
	// WriteJump((void *)0x77E897, ProcessCodes);
	
	// Continue game's initilization
	jump((void *)0xA6BD56);
}

BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	int bufsize;
	char *buf;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		myhandle = hModule;
		bufsize = GetCurrentDirectoryA(0, NULL);
		buf = new char[bufsize];
		GetCurrentDirectoryA(bufsize, buf);
		sa2dir = buf;
		delete[] buf;
		transform(sa2dir.begin(), sa2dir.end(), sa2dir.begin(), ::tolower);
		sa2dir += "\\";
		WriteJump((void *)0xA6BED9, InitMods);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}