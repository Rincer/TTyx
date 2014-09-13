#ifndef LIVEEDITVARIABLE_H_
#define LIVEEDITVARIABLE_H_

#include "Color.h"

//----------------------------------------------------------------------------------------------
// Object hierarchy for debug menu variables. 
//----------------------------------------------------------------------------------------------

typedef void (*LiveEditCB)(void); 	// Optional user callback function when variable value changes	

static const unsigned int scMaxVarNameSize = 32; // longest allowable variable name

class CMemoryStreamWriter;
class CMemoryStreamReader;
//----------------------------------------------------------------------------------------------
// Simple interface class for LiveEditVariable
class ILiveEditVariable
{
	public:
	
		enum
		{
			eBool,
			eInt,
			eFloat,
			eColor
		};
		
		ILiveEditVariable( unsigned long long Key, const char* pName, LiveEditCB pUserCB) :	m_Key(Key),
																							m_pName(pName),
																							m_pUserCB(pUserCB)
		{
			Assert(strlen(m_pName) < scMaxVarNameSize);
		}
		
		virtual ~ILiveEditVariable()
		{
		}		
				
		virtual void Serialize(CMemoryStreamWriter* pMemoryStream) const;
		virtual void Deserialize(CMemoryStreamReader* pMemoryStream) = 0;			
		virtual void Print() = 0;
		unsigned long long GetKey() const;
		
	private:		
		unsigned long long	m_Key;		// The actual name of this variable as a key value pair in a map
		const char*			m_pName;	
		LiveEditCB m_pUserCB; 			// Optional user callback function when variable value changes	
};

//----------------------------------------------------------------------------------------------
// 'Bool' variant
class CLiveEditVariableBool : public ILiveEditVariable
{
	public:
		CLiveEditVariableBool(bool* pVariable, unsigned long long Key, const char* pName, LiveEditCB pUserCB) :	ILiveEditVariable(Key, pName, pUserCB),
																												m_pVariable(pVariable)
		{
		}
		void Serialize(CMemoryStreamWriter* pMemoryStream) const;		
		void Deserialize(CMemoryStreamReader* pMemoryStream);
		void Print();
		
	private:
		bool* m_pVariable;
};

//----------------------------------------------------------------------------------------------
// 'Int' variant
class CLiveEditVariableInt : public ILiveEditVariable
{
	public:
		CLiveEditVariableInt(int Minimum, int Maximum, int Increment, int* pVariable, unsigned long long Key, const char* pName, LiveEditCB pUserCB) :	ILiveEditVariable(Key, pName, pUserCB),
																																						m_Minimum(Minimum),
																																						m_Maximum(Maximum),
																																						m_Increment(Increment),
																																						m_pVariable(pVariable)
		{
		}		
		virtual void Serialize(CMemoryStreamWriter* pMemoryStream) const;		
		void Deserialize(CMemoryStreamReader* pMemoryStream);
		void Print();		
		
	private:
		int		m_Minimum;
		int		m_Maximum;
		int		m_Increment;
		int*	m_pVariable;
};

//----------------------------------------------------------------------------------------------
// 'Float' variant
class CLiveEditVariableFloat : public ILiveEditVariable
{
	public:
		CLiveEditVariableFloat(float Minimum, float Maximum, float Increment, float* pVariable, unsigned long long Key, const char* pName, LiveEditCB pUserCB) :ILiveEditVariable(Key, pName, pUserCB),
																																								m_Minimum(Minimum),
																																								m_Maximum(Maximum),
																																								m_Increment(Increment),
																																								m_pVariable(pVariable)
		{
		}
		void Serialize(CMemoryStreamWriter* pMemoryStream) const;		
		void Deserialize(CMemoryStreamReader* pMemoryStream);
		void Print();		
		
	private:
		float	m_Minimum;
		float	m_Maximum;
		float	m_Increment;
		float*	m_pVariable;
};


//----------------------------------------------------------------------------------------------
// 'Color' variant
class CLiveEditVariableColor : public ILiveEditVariable
{
	public:
		CLiveEditVariableColor( CColor* pVariable, unsigned long long Key, const char* pName, LiveEditCB pUserCB) :	ILiveEditVariable(Key, pName, pUserCB),
																													m_pVariable(pVariable)
		{
		}
		void Serialize(CMemoryStreamWriter* pMemoryStream) const;		
		void Deserialize(CMemoryStreamReader* pMemoryStream);
		void Print();		
		
	private:
		CColor*	m_pVariable;
};

static const unsigned int scBiggestVariableSize = sizeof(CLiveEditVariableFloat) + scMaxVarNameSize;

#endif