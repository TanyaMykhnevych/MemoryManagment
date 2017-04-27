#include "stdafx.h"
#include <list>
#include <Windows.h>
#include <iterator>
using namespace std;

// выравнивание блоков памяти
// чтоб адрес начала был кратен 2^16
#define ALIGN(addr) ((addr+65536-1)/65536*65536)


// --------------------Task1--------------------
// формирование системной информации про виртуальную память
BOOL GetMemoryInfo(SYSTEM_INFO *s, MEMORYSTATUSEX *m)
{
	// записывает результат в структуру SYSTEM_INFO
	GetSystemInfo(s);
	// задать размер структуры до ее использования
	(*m).dwLength = sizeof(*m);
	// записывает результат в структуру MEMORYSTATUSEX
	return GlobalMemoryStatusEx(m);
}

// распечатка информации (полей структур) о виртуальной памяти
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
// список свободных и занятых блоков памяти

// блок памяти (регион)
typedef struct
{
	LPVOID address; // адрес начала блока
	size_t size; // размер блока
} REGION, *PREGION;

// список блоков памяти
typedef list<REGION> RLIST, *PRLIST;

VOID CreateRegionList(PRLIST listFree, PRLIST listBusy)
{
	SYSTEM_INFO s;
	GetSystemInfo(&s);

	// адрес начала приложения
	size_t startAddr = (size_t)s.lpMinimumApplicationAddress;

	// адрес конца приложения
	size_t endAddr = (size_t)s.lpMaximumApplicationAddress;

	// грануляция, определяет границу выравнивания памяти, 
	// которая выделяется функцией VirtualAlloc
	size_t gran = (size_t)s.dwAllocationGranularity;

	HANDLE h = GetCurrentProcess();

	// проходим по всем адресам приложения
	for (size_t i = startAddr; i < endAddr; )
	{
		MEMORY_BASIC_INFORMATION mInfo;

		// исследование адресного пространства заданного процесса
		// записывает в структуру MEMORY_BASIC_INFORMATION
		// адрес начала памяти, размер региона, состояние
		VirtualQueryEx(h, (LPVOID)i, &mInfo, sizeof(mInfo));

		REGION curReg;
		curReg.address = mInfo.AllocationBase;
		curReg.size = mInfo.RegionSize;

		//в зависимости от состояния записываем в 1 из 2 списков
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

// вывод списков свободных и занятых блоков
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

BOOL AllocMemory(PRLIST listFree, PRLIST listBusy, LPVOID memPtr, DWORD memSize, PREGION firstSufficient)
{
	listFree->sort(compare_region_by_size);   
	RLIST::iterator it = listFree->begin();
	while (it != listFree->end())
	{
		if (it->size >= memSize)
		{
			*firstSufficient = *it;
			break;
		}
	}

	if (it == listFree->end())
	{
		return FALSE;
	}

	if (it->size > memSize)
	{
		it->size -= memSize;
		it->address = (PVOID)((PBYTE)it->address + memSize);
		firstSufficient->size = memSize;
	}

	memPtr = it->address;
	listBusy->push_back(*firstSufficient);
	return memPtr != NULL;
}

VOID CombineBlocks(PRLIST listFree)
{
	list<RLIST::iterator> iterators;
	for (RLIST::iterator it1 = listFree->begin(); it1 != listFree->end(); it1++)
	{
		for (RLIST::iterator it2 = listFree->begin(); it2 != listFree->end(); it2++)
		{
			if ((PBYTE)(it1->address) + it1->size == (PBYTE)(it2->address) && it1 != it2)
			{
				it1->size += it2->size;
				iterators.push_back(it2);
			}
		}
	}

	for (list<RLIST::iterator>::iterator iterator = iterators.begin(); iterator != iterators.end(); iterator++)
	{
		listFree->erase(*iterator);
	}
}

BOOL FreeMemory(PRLIST listFree, PRLIST listBusy, const PREGION region)
{
	if (!region)
	{
		return FALSE;
	}
	
	listFree->push_back(*region);
	CombineBlocks(listFree);

	for (RLIST::iterator it = listBusy->begin(); it != listBusy->end(); it++)
	{
		if (it->address == region->address)
		{
			listBusy->erase(it);
		}
	}

	return TRUE;
}



// --------------------Task3--------------------
// страница
typedef struct
{
	PVOID phAddress; // физический адрес - в оперативной памяти
	PVOID logAddress; // логический адрес - с которым приходит страница
} PAGE, *PPAGE;

// список страниц
typedef list<PAGE> PAGES, *PPAGES;

// count - кол-во страниц, которое можно выделить
PVOID AddPage(PVOID *phAddresses, size_t count, PPAGES list, PPAGE page)
{
	PAGES::iterator it = list->begin();
	bool ifListContainsPage = false;
	while (it != list->end())
	{
		// если страница была загружена в память,
		// то удаляем ее, чтоб добавить  потом в конец
		if (it->logAddress == page->logAddress)
		{
			page->phAddress = it->phAddress;
			list->erase(it);
			ifListContainsPage = true;
			break;
		}
		++it;
	}

	// вышли из цикла, потому как дошли до конца
	if (!ifListContainsPage)
	{
		if (list->size() < count) // есть свободные позиции
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
// четырехассоциативный кэш
// функция изменения флагов

/*
 * b0, b1, b2 - биты в массиве flags
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

 // изменение битов по индксу
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

// нахождение индекса по битам
BYTE FindIndex(BYTE flags)
{
	BYTE index;
	if (!(flags & 1))
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

	SYSTEM_INFO s;
	MEMORYSTATUSEX m;
	GetMemoryInfo(&s, &m);

	REGION r1;
	r1.address = (PVOID)8;
	r1.size = 15;
	REGION r2;
	r2.address = (PVOID)22;
	r2.size = 3;
	REGION r3;
	r3.address = 0;
	r3.size = 8;
	RLIST lst = { r1,r2,r3 };
	CombineBlocks(&lst);


	PAGE p1;
	p1.logAddress = (PVOID)0;
	p1.phAddress = (PVOID)0;

	PAGE p2;
	p2.logAddress = (PVOID)5;
	p2.phAddress = (PVOID)1;

	PAGE p3;
	p3.logAddress = (PVOID)10;
	p3.phAddress = (PVOID)2;

	PAGE p4;
	p4.logAddress = (PVOID)20;
	p4.phAddress = (PVOID)0;

	PAGES pages = {};
	PVOID phAddresses[] = { (PVOID)0, (PVOID)5, (PVOID)10 };

	size_t count = 3;

	AddPage(phAddresses, count, &pages, &p4);


	PrintSystemInfo();

	RLIST rfree[1];
	RLIST rbusy[1];

	CreateRegionList(rfree, rbusy);


	PrintRegionsList(rfree, rbusy);

	BYTE flags = 3;

	BYTE res = FindIndex(flags);

	ChangeFlags(flags, res);

	_tprintf(_T("\n%d\n"), res);


	system("pause");
	return 0;
}


