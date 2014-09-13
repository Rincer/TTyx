#ifndef _HIDINPUTINTERFACE_H_
#define _HIDINPUTINTERFACE_H_

// Common non-templated base class
class IHIDInputBase
{
	public:
		virtual void ProcessHIDInput(const unsigned char* pInputBuffer) = 0;	
		IHIDInputBase* m_pNext;
		IHIDInputBase* m_pPrev;
};

// Executes movement commands coming from an HID (Human Input Device)
template<class ContextType>
class IHIDInput : public IHIDInputBase
{
	public:
		IHIDInput()
		{
			m_pNext = NULL;
			m_pPrev = NULL;			
		}
		void SetContext(ContextType* pContext)
		{
			m_pContext = pContext;
		}
		
		virtual void ProcessHIDInput(const unsigned char* pInputBuffer)
		{
			m_pContext->ProcessHIDInput(pInputBuffer);
		}

	protected:
		ContextType* m_pContext;
};


#endif