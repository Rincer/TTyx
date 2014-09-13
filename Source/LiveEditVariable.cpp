#include "stdafx.h"
#include "MemoryStream.h"

#include "LiveEditVariable.h"


//----------------------------------------------------------------------------------------------
unsigned long long ILiveEditVariable::GetKey() const
{
	return m_Key;
}


//----------------------------------------------------------------------------------------------
void ILiveEditVariable::Serialize(CMemoryStreamWriter* pMemoryStream) const
{
	pMemoryStream->WriteString(m_pName);	
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableBool::Serialize(CMemoryStreamWriter* pMemoryStream) const
{
	ILiveEditVariable::Serialize(pMemoryStream);
	pMemoryStream->Write<int>(eBool);
	pMemoryStream->Write<int>(*m_pVariable ? 1 : 0);
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableBool::Deserialize(CMemoryStreamReader* pMemoryStream)
{
	*m_pVariable = pMemoryStream->Read<int>() ? true : false;
	Print();
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableBool::Print()
{
	printf(" = ");
	if(*m_pVariable)
	{
		printf("true\n");
	}
	else
	{
		printf("false\n");	
	}
}

//----------------------------------------------------------------------------------------------
// We are sending this to the UI system so need to store bounds and increment step info
void CLiveEditVariableInt::Serialize(CMemoryStreamWriter* pMemoryStream) const
{
	ILiveEditVariable::Serialize(pMemoryStream);
	pMemoryStream->Write<int>(eInt);
	pMemoryStream->Write<int>(*m_pVariable);
	pMemoryStream->Write<int>(m_Minimum);
	pMemoryStream->Write<int>(m_Maximum);	
	pMemoryStream->Write<int>(m_Increment);	
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableInt::Deserialize(CMemoryStreamReader* pMemoryStream)
{
	*m_pVariable = pMemoryStream->Read<int>();
	Print();	
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableInt::Print()
{
	printf(" = %d\n", *m_pVariable);
}

//----------------------------------------------------------------------------------------------
// We are sending this to the UI system so need to store bounds and increment step info
void CLiveEditVariableFloat::Serialize(CMemoryStreamWriter* pMemoryStream) const
{
	ILiveEditVariable::Serialize(pMemoryStream);
	pMemoryStream->Write<int>(eFloat);
	pMemoryStream->Write<float>(*m_pVariable);
	pMemoryStream->Write<float>(m_Minimum);
	pMemoryStream->Write<float>(m_Maximum);	
	pMemoryStream->Write<float>(m_Increment);	
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableFloat::Deserialize(CMemoryStreamReader* pMemoryStream)
{
	*m_pVariable = pMemoryStream->Read<float>();
	Print();	
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableFloat::Print()
{
	printf(" = %.2f\n", *m_pVariable);
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableColor::Serialize(CMemoryStreamWriter* pMemoryStream) const
{
	ILiveEditVariable::Serialize(pMemoryStream);
	pMemoryStream->Write<int>(eColor);
	pMemoryStream->Write<CColor>(m_pVariable);
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableColor::Deserialize(CMemoryStreamReader* pMemoryStream)
{
	*m_pVariable = pMemoryStream->Read<CColor>();
}

//----------------------------------------------------------------------------------------------
void CLiveEditVariableColor::Print()
{
}


