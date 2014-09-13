#ifndef LIVEEDITTREE_H_
#define LIVEEDITTREE_H_

#include "LiveEditVariable.h"
//----------------------------------------------------------------------------------------------
// The live edit tree class. Each debug variable has a unique path to it from the root of the tree. 
// The path is initially specified as a '/' or '\' delimited string in one of the Add<Variable type> 
// functions. Each substring is hashed to a 64 bit key and the key and string pair is stored in a map. 
// Thus only the 64 bit key value needs to be stored for each node reducing memory requirements and 
// eliminating the need for string compares when traversing the tree.
class ILiveEditVariable;
class CElementList;
class CStackAllocator;
class CElementPool;
class CHashMap;
class CColor;
class CCommsChannelLiveEdit;
class CDebugVariableTreeNode;
class CNetwork;

class CLiveEditTree
{
	public:
		static const int scMaxDepth = 256;	// Max depth of the tree	
		CLiveEditTree();
		~CLiveEditTree();
		void Initialize(CNetwork* pNetwork);
		static CLiveEditTree& Instance();	// singleton accessor
		CStackAllocator* GetAllocator();	// accessor for memory allocator, all memory allocations within CLiveEditTree are done using this
		CElementPool* GetElementPool();		// accessor for the element pool
		const CHashMap* GetMap() const;		// accessor for the map of (64bit hash key, string value) pairs
		// the variable adding interface
		void AddInt(const char* pPath, const char* pName, int Minimum, int Maximum, int Increment, int* pVariable, LiveEditCB pUserCB);		
		void AddFloat(const char* pPath, const char* pName, float Minimum, float Maximum, float Increment, float* pVariable, LiveEditCB pUserCB);
		void AddBool(const char* pPath, const char* pName, bool* pVariable, LiveEditCB pUserCB);
		void AddColor(const char* pPath, const char* pName, CColor* pVariable, LiveEditCB pUserCB);		
		void DebugPrint();
		void UpdateDebugVariable(CMemoryStreamReader* pMemoryStream);	// will be called by the communication protocol when update data is received from client	
		
	private:		
		ILiveEditVariable* FindVariable(const char* pPath, const char* pName);
		void CalcKeysAndDepth(unsigned long long* pKeys, int& Depth, const char* pPath); // utility function	
		void SendTreeToConsole();	
		CStackAllocator*				m_pStackAllocator;	// handles all memory allocations on the live edit server side
		CElementPool*					m_pElementPool;		// all CElementList instances share this pool
		CHashMap*						m_pMap;				// map of strings and their 64bit hash values
		class CDebugVariableTreeNode*	m_pRoot;			// the root of the tree	
		CCommsChannelLiveEdit*	m_pCommsChannel;			// The channel used for network comms 
		CNetwork*				m_pNetwork;
		friend class CCommsChannelLiveEdit;
};
#endif