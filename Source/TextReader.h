#ifndef _TEXTREADER_H_
#define _TEXTREADER_H_

// No data, so all functions are static
class CTextReader
{		
	public:			
		static void ReadString(char* pString, const char** ppData);
		static void ReadString(char* pString, const char** ppStart, const char* pEnd);
		static void ReadFilename(char* pFilename, const char** ppData);
		static const char AdvanceToBracket(const char** ppData);
		static bool AdvanceToChar(char c, const char** ppData, const char* pEndData);
		static bool ReadInt(int& i, const char** ppData);
		static bool ReadUInt(unsigned int& i, const char** ppData);
		static void ReadFloat(float& f, const char** ppData);
		static void SkipComments(char CommentDesignator, const char** ppData);
		static void SkipSpacesEOLAndTab(const char** ppData);	
		static void SkipQuotes(const char** ppData);			
	private:
		// Can't have instances of this
		CTextReader()
		{
		};
		
		
		
};


#endif