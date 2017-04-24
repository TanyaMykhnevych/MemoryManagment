#include "stdafx.h"
#include <list>
#include <Windows.h>
#include <atomic>
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

LPVOID AllocateMemory(size_t memorySize)
{
	if (memorySize == 0)
		return nullptr;

	HANDLE h = GetCurrentProcess();

	RLIST listFree[1];
	RLIST listBusy[1];

	CreateRegionList(listFree, listBusy);

	if (listFree->size() == 0)
		return nullptr;

	listFree->sort(compare_region_by_size);

	RLIST::iterator i;

	for (i = listFree->begin(); i != listFree->end(); i++) {
		if (i->size > memorySize)
			break;
	}

	if (i == listFree->end())
		return nullptr;

	LPVOID res = VirtualAllocEx(h, i->address, memorySize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	/* ��� ��������, ���� ���������� �������� ����������
	RLIST listFree1[1];
	RLIST listBusy1[1];
	CreateRegionList(listFree1, listBusy1);
	*/

	return res;
}

BOOL FreeMemory(LPVOID address)
{
	if (address == nullptr)
		return FALSE;

	/* ��� ��������, ���� ���������� �������� ����������
	MEMORY_BASIC_INFORMATION mInfo;
	RLIST listFree2[1];
	RLIST listBusy2[1];
	CreateRegionList(listFree2, listBusy2);
	 */

	HANDLE h = GetCurrentProcess();

	/* ��� ��������, ���� ���������� �������� ����������
	VirtualQueryEx(h, (LPVOID)address, &mInfo, sizeof(mInfo));	 
	BOOL b = mInfo.State == MEM_FREE;
	*/

	BOOL res = VirtualFreeEx(h, address, 0, MEM_RELEASE);

	/* ��� ��������, ���� ���������� �������� ����������
	VirtualQueryEx(h, (LPVOID)address, &mInfo, sizeof(mInfo));
	b = mInfo.State == MEM_FREE;
	RLIST listFree3[1];
	RLIST listBusy3[1];
	CreateRegionList(listFree3, listBusy3);
	*/

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

// count - ���-�� �������, ��� ����� ��������
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

	LPVOID address = AllocateMemory(99999);

	FreeMemory(address);

	RLIST rfree[1];
	RLIST rbusy[1];

	CreateRegionList(rfree, rbusy);

	PrintRegionsList(rfree, rbusy);

	BYTE flags;
	BYTE index = 1;

	ChangeFlags(flags, index);

	BYTE res = FindIndex(flags);

	_tprintf(_T("\n%d\n"), res);


	system("pause");
	return 0;
}

