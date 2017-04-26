#include "stdafx.h"
#include <list>
#include <Windows.h>
#include <iterator>
using namespace std;

// ������������ ������ ������
// ���� ����� ������ ��� ������ 2^16
#define ALIGN(addr) ((addr+65536-1)/65536*65536)


// --------------------Task1--------------------
// ������������ ��������� ���������� ��� ����������� ������
BOOL GetMemoryInfo(SYSTEM_INFO *s, MEMORYSTATUSEX *m)
{
	// ���������� ��������� � ��������� SYSTEM_INFO
	GetSystemInfo(s);
	// ������ ������ ��������� �� �� �������������
	(*m).dwLength = sizeof(*m);
	// ���������� ��������� � ��������� MEMORYSTATUSEX
	return GlobalMemoryStatusEx(m);
}

// ���������� ���������� (����� ��������) � ����������� ������
VOID PrintSystemInfo()
{
	SYSTEM_INFO systemInfo;
	MEMORYSTATUSEX memStatus;

	GetMemoryInfo(&systemInfo, &memStatus);

	_tprintf(_T("\n**********System information**********\n"));

	_tprintf(_T("dwPageSize = %d\n"), systemInfo.dwPageSize);
	_tprintf(_T("dwAllocationGranularity = %d\n"), systemInfo.dwAllocationGranularity);
	_tprintf(_T("lpMinimumApplicationAddress = %#x\n"),
		systemInfo.lpMinimumApplicationAddress);
	_tprintf(_T("lpMaximumApplicationAddress = %#x\n"),
		systemInfo.lpMaximumApplicationAddress);

	_tprintf(_T("ullAvailPhys = %I64x\n"), memStatus.ullAvailPhys);
	_tprintf(_T("ullAvailVirtual = %I64x\n"), memStatus.ullAvailVirtual);
	_tprintf(_T("ullAvailPageFile = %I64x\n"), memStatus.ullAvailPageFile);
	_tprintf(_T("ullullTotalPhys = %I64x\n"), memStatus.ullTotalPhys);
	_tprintf(_T("ullTotalVirtual = %I64x\n"), memStatus.ullTotalVirtual);
	_tprintf(_T("ullTotalPageFile = %I64x\n"), memStatus.ullTotalPageFile);
}

// --------------------Task2--------------------
// ������ ��������� � ������� ������ ������

// ���� ������ (������)
typedef struct
{
	LPVOID address; // ����� ������ �����
	size_t size; // ������ �����
} REGION, *PREGION;

// ������ ������ ������
typedef list<REGION> RLIST, *PRLIST;

VOID CreateRegionList(PRLIST listFree, PRLIST listBusy)
{
	SYSTEM_INFO s;
	GetSystemInfo(&s);

	// ����� ������ ����������
	size_t startAddr = (size_t)s.lpMinimumApplicationAddress;

	// ����� ����� ����������
	size_t endAddr = (size_t)s.lpMaximumApplicationAddress;

	// ����������, ���������� ������� ������������ ������, 
	// ������� ���������� �������� VirtualAlloc
	size_t gran = (size_t)s.dwAllocationGranularity;

	HANDLE h = GetCurrentProcess();

	// �������� �� ���� ������� ����������
	for (size_t i = startAddr; i < endAddr; )
	{
		MEMORY_BASIC_INFORMATION mInfo;

		// ������������ ��������� ������������ ��������� ��������
		// ���������� � ��������� MEMORY_BASIC_INFORMATION
		// ����� ������ ������, ������ �������, ���������
		VirtualQueryEx(h, (LPVOID)i, &mInfo, sizeof(mInfo));

		REGION curReg;
		curReg.address = mInfo.AllocationBase;
		curReg.size = mInfo.RegionSize;

		//� ����������� �� ��������� ���������� � 1 �� 2 �������
		if (mInfo.State == MEM_FREE)
		{
			listFree->push_back(curReg);
		}
		else {
			listBusy->push_back(curReg);
		}

		i += ALIGN(curReg.size);
	}

}

// ����� ������� ��������� � ������� ������
VOID PrintRegionsList(PRLIST listFree, PRLIST listBusy) {
	_tprintf(_T("\n**********Free blocks**********\n"));
	for (RLIST::iterator i = listFree->begin(); i != listFree->end(); i++) {
		_tprintf(_T("%p\t%x\n"),
			i->address, i->size);
	}

	_tprintf(_T("\n**********Busy blocks**********\n"));
	for (RLIST::iterator i = listBusy->begin(); i != listBusy->end(); i++) {
		_tprintf(_T("%p\t%x\n"),
			i->address, i->size);
	}
}


bool compare_region_by_size(const REGION& lhs, const REGION& rhs) {
	return lhs.size < rhs.size;
}

LPVOID AllocateMemory(PRLIST listFree, PRLIST listBusy, size_t memorySize)
{
	if (memorySize == 0 | listFree->size() == 0)
		return nullptr;

	HANDLE h = GetCurrentProcess();

	listFree->sort(compare_region_by_size);

	RLIST::iterator i;

	for (i = listFree->begin(); i != listFree->end(); i++) {
		if (i->size >= memorySize) // ����� ���������� �����������
			break;
	}

	if (i == listFree->end()) // ���� �� ����� ���������� �����������
		return nullptr;

	LPVOID res = VirtualAllocEx(h, i->address, memorySize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	MEMORY_BASIC_INFORMATION mInfo;
	VirtualQueryEx(h, res, &mInfo, sizeof(mInfo));

	REGION r;
	r.address = res;
	r.size = mInfo.RegionSize;

	// ���������� � ������ ������� ������ ������
	listBusy->push_back(r);

	for (RLIST::iterator j = listFree->begin(); j != listFree->end(); j++) {
		// i - ���, �� �������� �� ����� ����� ������
		if (j->size == i->size) {
			REGION reg;
			reg.size = j->size - r.size;
			reg.address = &(r.address) + r.size;
			listFree->erase(++j);
			listFree->push_back(reg);
			break;
		}
	}
	return res;
}

BOOL FreeMemory(PRLIST listFree, PRLIST listBusy, LPVOID address)
{
	if (address == nullptr | listBusy->size() == 0)
		return FALSE;

	HANDLE h = GetCurrentProcess();

	BOOL res = VirtualFreeEx(h, address, 0, MEM_RELEASE);

	MEMORY_BASIC_INFORMATION mInfo;
	VirtualQueryEx(h, address, &mInfo, sizeof(mInfo));

	// ����, ������� ���� ���������
	REGION r;
	r.address = mInfo.BaseAddress;
	r.size = mInfo.RegionSize;

	// �������� ����� �� ������ ������� ����� ��� ������������
	for (RLIST::iterator j = listBusy->begin(); j != listBusy->end(); j++) {
		if (j->address == mInfo.BaseAddress) {
			listBusy->erase(j);
			break;
		}
	}

	// ���������� 2 ������ ��������� ����� � 1	
	// ����� ���
	VirtualQueryEx(h, &(r.address) - 1, &mInfo, sizeof(mInfo));
	REGION free;
	if(mInfo.State == MEM_FREE)
	{
		free.address = mInfo.BaseAddress;
		free.size = mInfo.RegionSize + r.size;
	}
	else // ����� ����
	{
		VirtualQueryEx(h, &(r.address) + r.size + 1, &mInfo, sizeof(mInfo));
		if (mInfo.State == MEM_FREE)
		{			
			free.address = r.address;
			free.size = mInfo.RegionSize + r.size;			
		}
		else
		{
			free = r;
		}
	}

	listFree->push_back(free);
		
	return res;
}


// --------------------Task3--------------------
// ��������
typedef struct
{
	PVOID phAddress; // ���������� ����� - � ����������� ������
	PVOID logAddress; // ���������� ����� - � ������� �������� ��������
} PAGE, *PPAGE;

// ������ �������
typedef list<PAGE> PAGES, *PPAGES;

// count - ���-�� �������, ������� ����� ��������
PVOID AddPage(PVOID *phAddresses, size_t count, PPAGES list, PPAGE page)
{
	PAGES::iterator it = list->begin();
	while (it != list->end())
	{
		// ���� �������� ���� ��������� � ������,
		// �� ������� ��, ���� ��������  ����� � �����
		if (it->logAddress == page->logAddress)
		{
			page->phAddress = it->phAddress;
			list->erase(it);
			break;
		}
		++it;
	}

	// ����� �� �����, ������ ��� ����� �� �����
	if (it == list->end())
	{
		if (list->size() < count) // ���� ��������� �������
		{
			page->phAddress = phAddresses[list->size()];
		}
		else {
			page->phAddress = list->begin()->phAddress;
			list->pop_front();
		}
	}
	list->push_back(*page);
	return page->phAddress;
}


// --------------------Task4--------------------
// �������������������� ���
// ������� ��������� ������

/*
 * b0, b1, b2 - ���� � ������� flags
 *
 * 1 - left, 0 - right
 *
 *           b0
 *         /    \
 *		b1		  b2
 *     /  \      /  \
 *    0    1    2    3     <---- indexes
 *
 */

 // ��������� ����� �� ������
VOID ChangeFlags(BYTE &flags, BYTE bitIndex)
{
	if (bitIndex < 2)
	{
		flags |= 1; // |0|0|0|1|
		if (bitIndex == 0)
		{
			flags |= 2; // |0|0|1|1| left, left
		}
		else {
			flags &= ~2; // |1|1|0|1| left, right
		}
	}
	else {
		flags &= ~1;  // |1|1|1|0|
		if (bitIndex == 2)
		{
			flags |= 4; // |1|1|1|0| right, left
		}
		else {
			flags &= ~4; // |1|0|1|0| right, right
		}
	}
}

// ���������� ������� �� �����
BYTE FindIndex(BYTE flags)
{
	BYTE index;
	if (flags & 1)
	{
		index = 2;
		if (flags & 4)
		{
			index++;
		}
	}
	else {
		index = 0;
		if (flags & 2)
		{
			index++;
		}
	}
	return index;
}


int main()
{
	PrintSystemInfo();

	RLIST rfree[1];
	RLIST rbusy[1];

	CreateRegionList(rfree, rbusy);

	LPVOID address = AllocateMemory(rfree, rbusy, 102354);

	FreeMemory(rfree, rbusy, address);

	PrintRegionsList(rfree, rbusy);

	BYTE flags = 3;	

	BYTE res = FindIndex(flags);

	ChangeFlags(flags, res);

	_tprintf(_T("\n%d\n"), res);


	system("pause");
	return 0;
}

