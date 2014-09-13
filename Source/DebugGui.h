#ifndef _DEBUGGUI_H_
#define _DEBUGGUI_H_

#include "Reference.h"

class CMaterial;
class CTextureSystem;
class CUtilityDraw;
class CMaterialSystem;
class CRenderer;

class CDebugGUI
{
	public:
		CDebugGUI(CTextureSystem** ppTextureSystem, CUtilityDraw** ppUtilityDraw, CMaterialSystem**ppMaterialSystem, CRenderer** ppRenderer);
		~CDebugGUI();
				
		void DrawString(float x, float y, const char* pString, const CColor& Color);
		
	private:
		CMaterial*	m_pFontMaterial;
		float m_ScreenPixelWidth;
		float m_ScreenPixelHeight;
		float m_FontPixelWidth;
		float m_FontPixelHeight;	
		CReference<CTextureSystem*> m_rTextureSystem;
		CReference<CUtilityDraw*>	m_rUtilityDraw;
		CReference<CMaterialSystem*>m_rMaterialSystem;
		CReference<CRenderer*>		m_rRenderer;
};


#endif _DEBUGGUI_H_