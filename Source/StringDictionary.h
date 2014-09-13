#ifndef _STRINGDICTIONARY_H_
#define _STRINGDICTIONARY_H_


class CHashMap;
class IAllocator;

class CStringDictionary
{
	public:
		CStringDictionary();
		~CStringDictionary();
		
		unsigned long long AddString(const char* pString);
		const char* GetString(unsigned long long Key);	
		void PrintAllStrings();
	
	private:
		CHashMap*	m_pHashMap;
		IAllocator* m_pAllocator;
		
};

#endif