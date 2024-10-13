// This wasn't written by me
// You know who you are

#include "dmincludes.h"

// I need to clean this up ;_;

// Sorry
const BYTE HvPeekPokeExp[] = {
	0x00, 0x00, 0x00, 0x00
};

// Sorry
const BYTE HvPeekPokeExp2[] = { // Encrypted
	0x00, 0x00, 0x00, 0x00
};

#define HvxCall QWORD _declspec(naked)
static HvxCall HvxExpansionInstall(DWORD PhysicalAddress, DWORD CodeSize) {
	
	if(XboxKrnlVersion->Build == 9199) {
		__asm { li r0, 0x76 }
	} else if(XboxKrnlVersion->Build == 12611 || XboxKrnlVersion->Build == 12625) {
		__asm { li r0, 0x70 }
	} else if(XboxKrnlVersion->Build >= 13110) {
		__asm { li r0, 0x6F }
	}

	__asm {
		sc
		blr
	}
}

static HvxCall HvxExpansionCall(DWORD ExpansionId, QWORD Param1, QWORD Param2, QWORD Param3, QWORD Param4) {
	
	if(XboxKrnlVersion->Build == 9199) {
		__asm { li r0, 0x77 }
	} else if(XboxKrnlVersion->Build == 12611 || XboxKrnlVersion->Build == 12625) {
		__asm { li r0, 0x71 }
	} else if(XboxKrnlVersion->Build >= 13110) {
		__asm { li r0, 0x70 }
	}

	__asm {
		sc	
		blr
	}
}

HRESULT HvInitialize() {

	// Allocate physcial memory for this expansion
	VOID* pPhysExp = XPhysicalAlloc(0x1000, MAXULONG_PTR, 0, PAGE_READWRITE);
	DWORD physExpAdd = MmGetPhysicalAddress(pPhysExp);
	HRESULT result;

	// Copy over our expansion data
	ZeroMemory(pPhysExp, 0x1000);
	
	memcpy(pPhysExp, HvPeekPokeExp, sizeof(HvPeekPokeExp));
	if(XboxKrnlVersion->Build >= 13110)
		*(LPDWORD)pPhysExp = 'HXPR';

	// Now we can install our expansion
	result = (HRESULT)HvxExpansionInstall(physExpAdd, 0x1000);

	// Free our allocated data
	XPhysicalFree(pPhysExp);

	if(FAILED(result))
		DbgPrint("[hvx] failed to install (0x%08x)\n", result);

	// Return our install result
	return result;
}
	
BYTE HvPeekBYTE(QWORD Address) {
	return (BYTE)HvxExpansionCall(HvPeekPokeExpID, PEEK_BYTE, Address, 0, 0);
}
WORD HvPeekWORD(QWORD Address) {
	return (WORD)HvxExpansionCall(HvPeekPokeExpID, PEEK_WORD, Address, 0, 0);
}
DWORD HvPeekDWORD(QWORD Address) {
	return (DWORD)HvxExpansionCall(HvPeekPokeExpID, PEEK_DWORD, Address, 0, 0);
}
QWORD HvPeekQWORD(QWORD Address) {
	return HvxExpansionCall(HvPeekPokeExpID, PEEK_QWORD, Address, 0, 0);
}
	
HRESULT HvPeekBytes(QWORD Address, PVOID Buffer, DWORD Size) {	
	
	// Create a physical buffer to peek into
	VOID* data = XPhysicalAlloc(Size, MAXULONG_PTR, 0, PAGE_READWRITE);
	HRESULT result;
	ZeroMemory(data, Size);
	
	result = (HRESULT)HvxExpansionCall(HvPeekPokeExpID, 
		PEEK_BYTES, Address, MmGetPhysicalAddress(data), Size);

	// If its successful copy it bacl
	if(result == S_OK) memcpy(Buffer, data, Size);

	// Free our physical data and return our result
	XPhysicalFree(data);
	return result;
}

HRESULT HvPokeBYTE(QWORD Address, BYTE Value) {
	return (HRESULT)HvxExpansionCall(HvPeekPokeExpID, POKE_BYTE, Address, Value, 0);
}
HRESULT HvPokeWORD(QWORD Address, WORD Value) {
	return (HRESULT)HvxExpansionCall(HvPeekPokeExpID, POKE_WORD, Address, Value, 0);
}
HRESULT HvPokeDWORD(QWORD Address, DWORD Value) {
	return (HRESULT)HvxExpansionCall(HvPeekPokeExpID, POKE_DWORD, Address, Value, 0);
}
HRESULT HvPokeQWORD(QWORD Address, QWORD Value) {
	return (HRESULT)HvxExpansionCall(HvPeekPokeExpID, POKE_QWORD, Address, Value, 0);
}
HRESULT HvPokeBytes(QWORD Address, const void* Buffer, DWORD Size) {

	// Create a physical buffer to poke from
	VOID* data = XPhysicalAlloc(Size, MAXULONG_PTR, 0, PAGE_READWRITE);
	HRESULT result;
	memcpy(data, Buffer, Size);
	
	result = (HRESULT)HvxExpansionCall(HvPeekPokeExpID, 
		POKE_BYTES, Address, MmGetPhysicalAddress(data), Size);

	// Free our physical data and return our result
	XPhysicalFree(data);
	return result;
}

	
QWORD HvPeekSPR(SOC_SPRS SPR) {
	return HvxExpansionCall(HvPeekPokeExpID, PEEK_SPR, SPR, 0, 0);
}
QWORD HvPokeSPR(SOC_SPRS SPR, QWORD Value) {
	return HvxExpansionCall(HvPeekPokeExpID, POKE_SPR, SPR, Value, 0);
}

VOID HvGetFuses(QWORD *Out)
{
	int x;

	for (x = 0; x < 12; x++) 
	{
		Out[x] = HvPeekQWORD(0x8000020000020000 + (x * 0x200));
		
	}
}
	
VOID HvDumpFromMemory(CHAR* FilePath) {

	// Create our output file
	HANDLE fHandle = CreateFile(FilePath, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Read our our HV from memory
	BYTE hvPart[0x10000]; DWORD pageSize = 0x10000;DWORD x;
	QWORD address = 0;
	QWORD rtoc = 0x0000000200000000;
	for(x = 0; x < 4; x++) {
		DWORD bytesWritten  = 0;

		// Read our section from our HV
		ZeroMemory(hvPart, pageSize);
		HvPeekBytes(address, hvPart, pageSize);

		// Write out the section
		WriteFile(fHandle, hvPart, pageSize, &bytesWritten, NULL);

		// Now increase our address
		address += (rtoc + pageSize);
	}

	// Close our output file
	CloseHandle(fHandle);
}
	
VOID HvDumpFuses(CHAR* FilePath) {

	// Create our output file
	HANDLE fHandle = CreateFile(FilePath, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// Read each fuse set and dump it
	QWORD fuseSet = 0; DWORD bytesWritten = 0;DWORD x;
	for (x = 0; x < 12; x++) {
		fuseSet = HvPeekQWORD(0x8000020000020000 + (x * 0x200));
		WriteFile(fHandle, &fuseSet, 8, &bytesWritten, NULL);
	}

	// Close our output file
	CloseHandle(fHandle);
}
VOID HvDump1Bl(CHAR* FilePath) {

	// Create our output file
	HANDLE fHandle = CreateFile(FilePath, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD bytesWritten = 0;
	BYTE bl1[0x8000];
	ZeroMemory(bl1, 0x8000);
	
	HvPeekBytes(0x8000020000000000, bl1, 0x8000);
	WriteFile(fHandle, bl1, 0x8000, &bytesWritten, NULL);

	// Close our output file
	CloseHandle(fHandle);
}