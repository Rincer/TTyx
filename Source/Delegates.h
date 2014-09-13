#ifndef _DELEGATES_H_
#define _DELEGATES_H_

#include "ElementList.h"

class IAllocator;
//----------------------------------------------------------------------------------
// Execute code once 'ExecuteDeived' and delete the data
class IDelegate
{
	public:
		bool Execute();
		
	protected:
		// true if execution succeeded
		virtual bool ExecuteDerived() = 0;
};

//----------------------------------------------------------------------------------
static const unsigned int scDelegatesMemory = 64 * 1024;

// Manages memory for once-off 'callbacks'
class CDelegates
{
	public:
		CDelegates(unsigned int DelegatesMemory);
		~CDelegates();
		
		static inline CDelegates& Instance()
		{
			static CDelegates s_Delegates(scDelegatesMemory);
			return s_Delegates;
		}
		
		IDelegate* CreateDelegate(unsigned int Size);	
		
	private:
		void ReleaseDelegate(IDelegate* pDelegate);				
		IAllocator* m_pAllocator;	
		
	friend class IDelegate;	
};

#endif