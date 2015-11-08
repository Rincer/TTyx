#include "stdafx.h"

#include "TextReader.h"


//-----------------------------------------------------------------------------
void CTextReader::SkipSpacesEOLAndTab(const char** ppData)
{
	while(((*ppData)[0] == ' ') || ((*ppData)[0] == 13) || ((*ppData)[0] == 10) || ((*ppData)[0] == 9))
	{
		(*ppData)++;
	}
}

//-----------------------------------------------------------------------------
void CTextReader::SkipQuotes(const char** ppData)
{
	while(((*ppData)[0] == '"'))
	{
		(*ppData)++;
	}
}

//-----------------------------------------------------------------------------
void CTextReader::ReadString(char* pString, const char** ppData)
{
	SkipSpacesEOLAndTab(ppData);
	SkipQuotes(ppData);
	char* pDst = pString;
	while(((*ppData)[0] != 13) && ((*ppData)[0] != 10) && ((*ppData)[0] != 9) && ((*ppData)[0] != ' ') && ((*ppData)[0] != '"')) 
	{
		pDst[0] = (*ppData)[0]; // Copy from data to string
		pDst++;				
		(*ppData)++;					
	}
	pDst[0] = 0;
	SkipQuotes(ppData);
}

//-----------------------------------------------------------------------------
void CTextReader::ReadString(char* pString, const char** ppStart, const char* pEnd)
{
	SkipSpacesEOLAndTab(ppStart);
	char* pDst = pString;
	while(((*ppStart) != pEnd) && ((*ppStart)[0] != 13) && ((*ppStart)[0] != 10) && ((*ppStart)[0] != 9) && ((*ppStart)[0] != ' ')) 
	{
		pDst[0] = (*ppStart)[0]; // Copy from data to string
		pDst++;				
		(*ppStart)++;					
	}
	pDst[0] = 0;
}

//-----------------------------------------------------------------------------
const char CTextReader::AdvanceToBracket(const char** ppData)
{
	while(((*ppData)[0] != '{') && ((*ppData)[0] != '}'))
	{
		(*ppData)++;
	}
	return (*ppData)[0];
}

//-----------------------------------------------------------------------------
bool CTextReader::AdvanceToChar(char c, const char** ppData, const char* pEndData)
{
	while(((*ppData)[0] != c) && ((*ppData) < pEndData))
	{
		(*ppData)++;
	}
	return ((*ppData)[0] == c);
}

//-----------------------------------------------------------------------------
bool CTextReader::ReadInt(int& i, const char** ppData)
{
	bool Res = false;
	int Sign = 1;
	i = 0;
	SkipSpacesEOLAndTab(ppData);
	while((((*ppData)[0] >= '0') && ((*ppData)[0] <= '9')) || ((*ppData)[0] == '-'))			
	{
		if((*ppData)[0] == '-')
			Sign = -1;
		if(((*ppData)[0] >= '0') && ((*ppData)[0] <= '9'))									
		{
			i = i * 10 + ((*ppData)[0] - '0');
			Res = true; // read something valid
		}					
		(*ppData)++;					
	}			
	i = i * Sign;
	return Res;
}

//-----------------------------------------------------------------------------
bool CTextReader::ReadUInt(unsigned int& i, const char** ppData)
{
	bool Res = false;
	i = 0;
	SkipSpacesEOLAndTab(ppData);
	while(((*ppData)[0] >= '0') && ((*ppData)[0] <= '9'))			
	{
		i = i * 10 + ((*ppData)[0] - '0');			
		(*ppData)++;					
		Res = true; // read something valid		
	}			
	return Res;
}

//-----------------------------------------------------------------------------
void CTextReader::ReadFloat(float& f, const char** ppData)
{
	float Sign = 1.0f;
	float Divisor = 0.0f;
	f = 0.0f;
	SkipSpacesEOLAndTab(ppData);
	while((((*ppData)[0] >= '0') && ((*ppData)[0] <= '9')) || ((*ppData)[0] == '-') || ((*ppData)[0] == '.'))
	{
		if((*ppData)[0] == '-')
			Sign = -1.0f;
		if((*ppData)[0] == '.')					
			Divisor = 1.0f;
		if(((*ppData)[0] >= '0') && ((*ppData)[0] <= '9'))									
		{
			f = f * 10.0f + ((*ppData)[0] - '0');
			Divisor *= 10.0f;
		}					
		(*ppData)++;					
	}			
	f = f * Sign;
	if (Divisor != 0.0f)
	{
		f = f / Divisor;
	}
	if ((*ppData)[0] == 'e') // Exponent
	{
		(*ppData)++;
		int Exp;
		ReadInt(Exp, ppData);
		float Pow = pow(10.0f, Exp);
		f *= Pow;
	}
}


//-----------------------------------------------------------------------------
// reads just the filename stripping off any path		
void CTextReader::ReadFilename(char* pFilename, const char** ppData)
{
	SkipSpacesEOLAndTab(ppData);
	char* pDst = pFilename;
	while(((*ppData)[0] != 13) && ((*ppData)[0] != 10) && ((*ppData)[0] != 9) && ((*ppData)[0] != ' ')) 
	{
		if(((*ppData)[0] == '/') || ((*ppData)[0] == '\\'))
		{
			pDst = pFilename; // Reset to the beginning
		}
		else
		{
			pDst[0] = (*ppData)[0]; // Copy from data to filename
			pDst++;				
		}
		(*ppData)++;					
	}
	pDst[0] = 0;
}

//-----------------------------------------------------------------------------
void CTextReader::SkipComments(char CommentDesignator, const char** ppData)
{
	if((*ppData)[0] == CommentDesignator) 
	{
		while(((*ppData)[0] != 13) && ((*ppData)[0] != 10))
			(*ppData)++;
	}	
}
