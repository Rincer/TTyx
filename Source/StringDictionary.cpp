#include "stdafx.h"

#include "HashMap.h"
#include "StringDictionary.h"
#include "HeapAllocator.h"
#include "Hash64.h"

static const unsigned int sc_StringMemory = 128 * 1024;

//---------------------------------------------------------------------------------
CStringDictionary::CStringDictionary()
{
	m_pAllocator = new CHeapAllocator(sc_StringMemory, false);
	m_pHashMap = new(m_pAllocator->Alloc(sizeof(CHashMap))) CHashMap(m_pAllocator, 2048);
}

//---------------------------------------------------------------------------------
CStringDictionary::~CStringDictionary()
{
	delete m_pAllocator;
}

//---------------------------------------------------------------------------------
unsigned long long CStringDictionary::AddString(const char* pString)
{
	unsigned long long Key = CHash64::GetHash(pString);
	m_pHashMap->AddEntry(Key, pString, strlen(pString) + 1); // +1 for null terminator
	return Key;
}

//---------------------------------------------------------------------------------
const char* CStringDictionary::GetString(unsigned long long Key)
{
	return (const char*)m_pHashMap->GetValue(Key);
}

//---------------------------------------------------------------------------------
void CStringDictionary::PrintAllStrings()
{
	for(unsigned int EntryIndex = 0; EntryIndex < m_pHashMap->GetNumEntries(); EntryIndex++)
	{
		unsigned long long Key;
		char* pValue;
		m_pHashMap->GetEntry(EntryIndex, Key, (void**)&pValue);
		DebugPrintf("%llu %s\n", Key, pValue);
	}
}