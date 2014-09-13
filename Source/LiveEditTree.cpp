#include "stdafx.h"
#include "StackAllocator.h"
#include "ElementList.h"
#include "LiveEditVariable.h"
#include "Hash64.h"
#include "HashMap.h"
#include "Color.h"
#include "CommsChannel.h"
#include "Network.h"
#include "MemoryStream.h"

#include "LiveEditTree.h"

// some const sizes
static const int scNumListPoolEntries = 1 * 1024;
static const int scNumMapEntries = 1 * 1024;
static const int scLiveEditTreeMemory = 64 * 1024;
static const int scMaxSubPathLength = 256;
static const int scCommsBufferSize = 2 * 1024;
static const int scCommsBufferAlignment = 16;

// disable warning for deprecated string functions
#pragma warning( disable : 4996 )


//----------------------------------------------------------------------------------------------
class CCommsChannelLiveEdit : public ICommsChannel
{	
	public:
		enum eReceiveMessageType
		{
			eSendTree,
			eVariableUpdate,
			eMaxReceiveMessageTypes
		};	

		enum eSendMessageType
		{
			eAddVariable,
			eMaxSendMessageTypes
		};	
	
		
		CCommsChannelLiveEdit(eChannelIndex ChannelIndex, unsigned char* pCommsBuffer, unsigned int Size, CNetwork* pNetwork) : ICommsChannel(ChannelIndex, pCommsBuffer, Size, pNetwork)
		{
		}
		
		virtual ~CCommsChannelLiveEdit()
		{
		}
		
		virtual void OnReceive(unsigned int Size)
		{
			CMemoryStreamReader MemoryStream(m_pCommsBuffer, 0, Size);
			unsigned int MessageType = MemoryStream.Read<unsigned int>();
			switch(MessageType)
			{
				case eSendTree:
				{
					CLiveEditTree::Instance().SendTreeToConsole();
					break;
				}
				
				case eVariableUpdate:
				{
					CLiveEditTree::Instance().UpdateDebugVariable(&MemoryStream);
					break;
				}
				
				default:
					Assert(0);
					break;
			}			
		}
};


//----------------------------------------------------------------------------------------------
// Node in the debug variable tree		
class CDebugVariableTreeNode
{
	public:
		CDebugVariableTreeNode()
		{
			m_pName = NULL;
			m_Key = 0;
			// allocate in place using memory from our stack allocator
			m_pNodes = new (CLiveEditTree::Instance().GetAllocator()->Alloc(sizeof(CElementList))) CElementList(CLiveEditTree::Instance().GetElementPool());
			m_pDebugVariables = new (CLiveEditTree::Instance().GetAllocator()->Alloc(sizeof(CElementList))) CElementList(CLiveEditTree::Instance().GetElementPool());			
		}

		CDebugVariableTreeNode(unsigned long long Key) :	m_Key(Key)																				
		{
			// allocate in place using memory from our stack allocator
			m_pNodes = new (CLiveEditTree::Instance().GetAllocator()->Alloc(sizeof(CElementList))) CElementList(CLiveEditTree::Instance().GetElementPool());
			m_pDebugVariables = new (CLiveEditTree::Instance().GetAllocator()->Alloc(sizeof(CElementList))) CElementList(CLiveEditTree::Instance().GetElementPool());
			m_pName = ((char*)CLiveEditTree::Instance().GetMap()->GetValue(m_Key));			
		}
		
		~CDebugVariableTreeNode()
		{
		}		
		
		void AddDebugVariable(const unsigned long long* pKeys, int Level, int Depth, ILiveEditVariable* pLiveEditVariable)
		{
			for (CElement* pElement = m_pNodes->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
			{
				CDebugVariableTreeNode* pNode = (CDebugVariableTreeNode*)pElement->m_pData;
				// Node with this path already exists
				if(pNode->m_Key == pKeys[Level])
				{
					// are we at the leaf node?
					if (Level == Depth - 1)
					{
						pNode->m_pDebugVariables->Add(pLiveEditVariable);
					}
					else // traverse down to the next level
					{
						pNode->AddDebugVariable(pKeys, Level + 1, Depth, pLiveEditVariable);
					}					
					return;
				}
			}	

			// add a new node to the tree
			CDebugVariableTreeNode* pNewNode = new (CLiveEditTree::Instance().GetAllocator()->Alloc(sizeof(CDebugVariableTreeNode))) CDebugVariableTreeNode(pKeys[Level]);
			m_pNodes->Add(pNewNode);
			
			// are we at the leaf node?
			if(Level == Depth - 1)
			{
				pNewNode->m_pDebugVariables->Add(pLiveEditVariable);
			}
			else
			{
				// traverse down to the next level
				pNewNode->AddDebugVariable(pKeys, Level + 1, Depth, pLiveEditVariable);
			}
		}
	
		void SendToConsole(unsigned char* pSendBuffer, unsigned int CurrPos, CCommsChannelLiveEdit* pCommsChannel)
		{			
			for (CElement* pElement = m_pNodes->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
			{
				CDebugVariableTreeNode* pNode = (CDebugVariableTreeNode*)pElement->m_pData;
				memcpy(&pSendBuffer[CurrPos], pNode->m_pName, strlen(pNode->m_pName));
				int Len = strlen(pNode->m_pName);
				pSendBuffer[CurrPos + Len] = '/';
				pNode->SendToConsole(pSendBuffer, CurrPos + Len + 1, pCommsChannel);
			}
			
			for (CElement* pElement = m_pDebugVariables->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
			{
				ILiveEditVariable* pVariable = (ILiveEditVariable*)pElement->m_pData;
				CMemoryStreamWriter MemoryStream(pSendBuffer, CurrPos, scBiggestVariableSize);
				pVariable->Serialize(&MemoryStream);	
				pCommsChannel->Send(pSendBuffer, MemoryStream.GetOffset());		
			}				
		}
	
		ILiveEditVariable*  FindVariable(unsigned long long* pPathKeys, int Depth, int Level, unsigned long long NameKey)
		{
			if(Level == Depth)
			{
				for (CElement* pElement = m_pDebugVariables->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
				{
					ILiveEditVariable* pVariable = (ILiveEditVariable*)pElement->m_pData;
					if(pVariable->GetKey() == NameKey)
					{
						return pVariable;
					}
				}				
			}
			else
			{
				for (CElement* pElement = m_pNodes->GetFirst(); pElement != NULL; pElement = pElement->m_pNext)
				{
					CDebugVariableTreeNode* pNode = (CDebugVariableTreeNode*)pElement->m_pData;
					if(pNode->m_Key == pPathKeys[Level])
					{
						return pNode->FindVariable(pPathKeys, Depth, Level + 1, NameKey);
					}
				}			
			}
			return NULL;
		}
//		unsigned int Serialize(void* pData) const
//		{
//		}
//		
//		void Deserialize(const void* pData);				
//		{
//		}
		
	private:		
		unsigned long long	m_Key;
		const char*			m_pName;
		CElementList*		m_pNodes;
		CElementList*		m_pDebugVariables;											
};


//--------------------------------------------------------------------------------------------
CLiveEditTree::CLiveEditTree() : m_pNetwork(NULL)
{
	m_pStackAllocator = new CStackAllocator(scLiveEditTreeMemory);
	// allocate in place using memory from our stack allocator
	m_pElementPool = new (m_pStackAllocator->Alloc(sizeof(CElementPool))) CElementPool(scNumListPoolEntries, m_pStackAllocator);	
	m_pMap = new (m_pStackAllocator->Alloc(sizeof(CHashMap))) CHashMap(m_pStackAllocator, scNumMapEntries);	
	m_pRoot	= new (m_pStackAllocator->Alloc(sizeof(CDebugVariableTreeNode))) CDebugVariableTreeNode;
}

//--------------------------------------------------------------------------------------------
CLiveEditTree::~CLiveEditTree()
{
	// all memory allocations were in place using the stack allocator, so to clean up we only need to delete it
	// and the memory will be given back to the run-time in stack allocator destructor
	delete m_pStackAllocator;
}

//--------------------------------------------------------------------------------------------
void CLiveEditTree::Initialize(CNetwork* pNetwork)
{
	m_pNetwork = pNetwork;
	unsigned char *pCommsBufferMemory = new (m_pStackAllocator->AlignedAlloc(scCommsBufferSize, scCommsBufferAlignment)) unsigned char[scCommsBufferSize];
	m_pCommsChannel = new (m_pStackAllocator->Alloc(sizeof(CCommsChannelLiveEdit))) CCommsChannelLiveEdit(eChannelLiveEdit, pCommsBufferMemory, scCommsBufferSize, m_pNetwork);
	m_pNetwork->RegisterCommsChannel(m_pCommsChannel);
}

//--------------------------------------------------------------------------------------------
CLiveEditTree& CLiveEditTree::Instance()
{
	static CLiveEditTree s_LiveEditTree;	// singleton
	return s_LiveEditTree;					// get the instance
}

//--------------------------------------------------------------------------------------------
CStackAllocator* CLiveEditTree::GetAllocator()
{
	return m_pStackAllocator;
}

//--------------------------------------------------------------------------------------------
CElementPool* CLiveEditTree::GetElementPool()
{
	return m_pElementPool;
}

//--------------------------------------------------------------------------------------------
const CHashMap* CLiveEditTree::GetMap() const
{
	return m_pMap;
}

//--------------------------------------------------------------------------------------------
// Parses a '/' or '\' delimited string and splits it into sub strings
// each substring is has a 64 bit hash value computed for it and added to a map (if doesnt exist alread) as a key value pair
void CLiveEditTree::CalcKeysAndDepth(unsigned long long* pKeys, int& Depth, const char* pPath)
{
	char SubPath[scMaxSubPathLength];
	const char* pSrc = pPath;
	char* pDst = SubPath;
	Depth = 0;
	while(*pSrc)
	{
		if((*pSrc == '/') || (*pSrc == '\\'))
		{
			*pDst = 0; // zero terminate the current substring
			pKeys[Depth] = CHash64::GetHash(SubPath);
			m_pMap->AddEntry(pKeys[Depth], SubPath, strlen(SubPath) + 1);
			pDst = SubPath;
			Depth++;
		}
		else
		{
			*pDst++ = *pSrc;
		}
		pSrc++;
	}
	*pDst = 0; // zero terminate the current substring
	pKeys[Depth] = CHash64::GetHash(SubPath);
	m_pMap->AddEntry(pKeys[Depth], SubPath, strlen(SubPath) + 1);	
	Depth++;
}



//--------------------------------------------------------------------------------------------
void Pad(int Spaces)
{
	for(int i = 0; i < Spaces; i++)
	{
		printf(" ");
	}
}


//--------------------------------------------------------------------------------------------
void CLiveEditTree::DebugPrint()
{	
	printf("------------------------------------------------\n");
//	m_pRoot->DrawTreePart(0);
}

//--------------------------------------------------------------------------------------------
void CLiveEditTree::AddInt(const char* pPath, const char* pName, int Minimum, int Maximum, int Increment, int* pVariable, LiveEditCB pUserCB)
{
	unsigned long long Keys[scMaxDepth];
	int Depth;
	CalcKeysAndDepth(Keys, Depth, pPath);	
	unsigned long long NameKey = CHash64::GetHash(pName);
	m_pMap->AddEntry(NameKey, pName, strlen(pName) + 1);
	ILiveEditVariable* pLiveEditVariable = new (m_pStackAllocator->Alloc(sizeof(CLiveEditVariableInt))) CLiveEditVariableInt(Minimum, Maximum, Increment, pVariable, NameKey, pName, pUserCB );
	// recursively adds the variable to the tree
	m_pRoot->AddDebugVariable(Keys, 0, Depth, pLiveEditVariable);
}

//--------------------------------------------------------------------------------------------
void CLiveEditTree::AddFloat(const char* pPath, const char* pName, float Minimum, float Maximum, float Increment, float* pVariable, LiveEditCB pUserCB)
{
	unsigned long long Keys[scMaxDepth];
	int Depth;
	CalcKeysAndDepth(Keys, Depth, pPath);
	unsigned long long NameKey = CHash64::GetHash(pName);
	m_pMap->AddEntry(NameKey, pName, strlen(pName) + 1);	
	ILiveEditVariable* pLiveEditVariable = new (m_pStackAllocator->Alloc(sizeof(CLiveEditVariableFloat))) CLiveEditVariableFloat(Minimum, Maximum, Increment, pVariable, NameKey, pName, pUserCB );
	// recursively adds the variable to the tree
	m_pRoot->AddDebugVariable(Keys, 0, Depth, pLiveEditVariable);
}

//--------------------------------------------------------------------------------------------
void CLiveEditTree::AddBool(const char* pPath, const char* pName, bool* pVariable, LiveEditCB pUserCB)
{
	unsigned long long Keys[scMaxDepth];
	int Depth;
	CalcKeysAndDepth(Keys, Depth, pPath);
	unsigned long long NameKey = CHash64::GetHash(pName);
	m_pMap->AddEntry(NameKey, pName, strlen(pName) + 1);		
	ILiveEditVariable* pLiveEditVariable = new (m_pStackAllocator->Alloc(sizeof(CLiveEditVariableFloat))) CLiveEditVariableBool(pVariable, NameKey, pName, pUserCB );
	// recursively adds the variable to the tree
	m_pRoot->AddDebugVariable(Keys, 0, Depth, pLiveEditVariable);
}


//--------------------------------------------------------------------------------------------
void CLiveEditTree::AddColor(const char* pPath, const char* pName, CColor* pVariable, LiveEditCB pUserCB)		
{
	unsigned long long Keys[scMaxDepth];
	int Depth;
	CalcKeysAndDepth(Keys, Depth, pPath);
	unsigned long long NameKey = CHash64::GetHash(pName);
	m_pMap->AddEntry(NameKey, pName, strlen(pName) + 1);		
	
	ILiveEditVariable* pLiveEditVariable = new (m_pStackAllocator->Alloc(sizeof(CLiveEditVariableColor))) CLiveEditVariableColor(pVariable, NameKey, pName, pUserCB );
	// recursively adds the variable to the tree
	m_pRoot->AddDebugVariable(Keys, 0, Depth, pLiveEditVariable);
}


//--------------------------------------------------------------------------------------------
ILiveEditVariable* CLiveEditTree::FindVariable(const char* pPath, const char* pName)
{
	unsigned long long PathKeys[scMaxDepth];
	int Depth;
	CalcKeysAndDepth(PathKeys, Depth, pPath);
	long long NameKey = CHash64::GetHash(pName);
	return m_pRoot->FindVariable(PathKeys, Depth, 0, NameKey);
}
		
//--------------------------------------------------------------------------------------------
// Received an update to debug variable
void CLiveEditTree::UpdateDebugVariable(CMemoryStreamReader* pMemoryStream)
{
	char* pPath = (char*)pMemoryStream->ReadStringInPlace();
	char* pName = (char*)pMemoryStream->ReadStringInPlace();
	ILiveEditVariable* pVariable = FindVariable(pPath, pName);
	Assert(pVariable);
	pVariable->Deserialize(pMemoryStream);
}


//--------------------------------------------------------------------------------------------
void CLiveEditTree::SendTreeToConsole()
{
	unsigned char* pSendBuffer = m_pCommsChannel->GetCommsBuffer();
	*(unsigned int*)pSendBuffer = CCommsChannelLiveEdit::eAddVariable;
	m_pRoot->SendToConsole(pSendBuffer, sizeof(unsigned int), m_pCommsChannel);
}



