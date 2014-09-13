#ifndef _MULTISTAGEASSETCONTAINER_H_
#define _MULTISTAGEASSETCONTAINER_H_

#include "Allocator.h"
#include "ElementList.h"
#include "HashMap.h"
#include "LWMutex.h"

// container class for assets that can be in different stages of processing
class IAllocator;
class CRenderer;
class CConstantsSystem;
	
template<class DataType, unsigned int NumStages, typename StateType, StateType InitialState, StateType ReferenceState>
class CMultiStageAssetContainer 
{
	public:	
		CMultiStageAssetContainer(CLWMutex* pAddAssetMutex) : m_pAddAssetMutex(pAddAssetMutex)
		{
			m_pAllocator = NULL;
			m_pElementArrayPool = NULL;
			for(unsigned int Stage = 0; Stage < NumStages; Stage++)
			{
				m_pStages[Stage] = NULL;
			}									
		}			
				
		~CMultiStageAssetContainer()
		{
			if(!m_pAllocator)
			{
				delete m_pElementArrayPool;
				for(unsigned int Stage = 0; Stage < NumStages; Stage++)
				{
					delete m_pStages[Stage];
				}									
			}
		}
	
		void Startup(IAllocator* pAllocator, unsigned int NumSegments, unsigned int SegmentSize)
		{
			m_pAllocator = pAllocator;
			if(pAllocator)
			{
				m_pElementArrayPool = new(pAllocator->Alloc(sizeof(CElementArrayPool<DataType>))) CElementArrayPool<DataType>(pAllocator, NumSegments, SegmentSize);	
				for(unsigned int Stage = 0; Stage < NumStages; Stage++)
				{
					m_pStages[Stage] = new(pAllocator->Alloc(sizeof(CElementArray<DataType>))) CElementArray<DataType>(m_pElementArrayPool);
				}		
				m_pHashMap = new(m_pAllocator->Alloc(sizeof(CStaticHashMap<DataType*>))) CStaticHashMap<DataType*>(m_pAllocator, NumSegments * SegmentSize);
			}
			else
			{
				m_pElementArrayPool = new CElementArrayPool<DataType>(NULL, NumSegments, SegmentSize);	
				for(unsigned int Stage = 0; Stage < NumStages; Stage++)
				{
					m_pStages[Stage] = new CElementArray<DataType>(m_pElementArrayPool);
				}						
				m_pHashMap = new CStaticHashMap<DataType*>(NULL, NumSegments * SegmentSize);				
			}
		}		
				
		DataType& AddAsset(unsigned long long Key, bool& Exists)
		{	
			DataType*const* ppData = m_pHashMap->GetValue(Key);
			if(ppData)
			{
				Exists = true;
				return **ppData;
			}
			DataType& Data = m_pStages[InitialState]->Add();	 // Initial state 
			Data.SetState(InitialState);
			m_pHashMap->AddEntry(Key, &Data);
			Exists = false;
			return Data;
		}

		DataType& AddReference(unsigned long long Key)
		{	
			DataType*const* ppData = m_pHashMap->GetValue(Key);
			if(ppData)
			{
				return **ppData;
			}
			DataType& Data = m_pStages[ReferenceState]->Add();	// The reference stage 
			Data.SetState(ReferenceState);
			m_pHashMap->AddEntry(Key, &Data);
			return Data;
		}
		
		void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem); // For debugging
		
		void Tick();
		void Shutdown();
				
		bool IsShutdown()
		{
			for(unsigned int Stage = 0; Stage < NumStages; Stage++) 
			{
				if(!m_pStages[Stage]->IsEmpty())
					return false;
			}
			return true;
		}		
		
				
	private:
		IAllocator* 					m_pAllocator;
		CElementArrayPool<DataType>*	m_pElementArrayPool;
		CElementArray<DataType>*		m_pStages[NumStages];		
		CStaticHashMap<DataType*>*		m_pHashMap;	
		CLWMutex*						m_pAddAssetMutex;
};


#endif