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
	for(size_t i = startAddr; i < endAddr; )
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
		if(mInfo.State == MEM_FREE)
		{
			listFree->push_back(curReg);
		} else {
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
	while(it != list->end())
	{
		// ���� �������� ���� ��������� � ������,
		// �� ������� ��, ���� ��������  ����� � �����
		if(it->logAddress == page->logAddress)
		{
			page->phAddress = it->phAddress;
			list->erase(it);
			break;
		}
		++it;
	}

	// ����� �� �����, ������ ��� ����� �� �����
	if(it == list->end())
	{
		if(list->size() < count) // ���� ��������� �������
		{
			page->phAddress = phAddresses[list->size()];
		} else {
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
VOID ChangeFlags(BYTE &flags, BYTE bitIndex)
{
	if(bitIndex < 2)
	{
		flags |= 1;
		if(bitIndex == 0)
		{
			flags |= 2;
		} else {
			flags &= ~2;
		}
	} else {
		flags &= ~1;
		if(bitIndex == 2)
		{
			flags |= 4;
		} else {
			flags &= ~4;
		}
	}
}

BYTE FindIndex(BYTE flags)
{
	BYTE index;
	if(flags & 1) // �������� �� ������ ���������
	{
		index = 2;
		if(flags & 4)
		{
			index++;
		} else {
			index = 0;
			if(flags & 2)
			{
				index++;
			}
		}
	}
	return index;
}


int main()
{
	PrintSystemInfo();
	RLIST rfree[500];
	RLIST rbusy[500];

	CreateRegionList(rfree, rbusy);
	PrintRegionsList(rfree, rbusy);

	BYTE flags[1];
	size_t index = 1;
	
	ChangeFlags(*flags, index);

	BYTE res = FindIndex(*flags);

	_tprintf(_T("\n%d\n"), res);




	system("pause");
	return 0;
}

