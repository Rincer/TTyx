#ifndef _RUNMODETEST_H_
#define _RUNMODETEST_H_
#include "RunMode.h"
#include "ViewController.h"
#include "View.h"

class CRenderObjectSystem;
class CLoadingSystem;
class CLightSystem;
class CTextureSystem;
class CMaterialSystem;
class CUtilityDraw;
class CShaderPipeline;
class CStringDictionary;


class CRunModeTest : public CRunMode
{
	public:
		CRunModeTest();
		CRunModeTest(CInput* pInput, 
					 CConstantsSystem* pConstantsSystem, 
					 CRenderObjectSystem* pRenderObjectSystem,
					 CLoadingSystem* pLoadingSystem,
					 CLightSystem* pLightSystem,
					 CTimeLine* pTimeLine,
					 CRenderer* pRenderer,
					 CAnimationSystem* pAnimationSystem,
					 CTextureSystem* pTextureSystem,
					 CMaterialSystem* pMaterialSystem,
					 CUtilityDraw* pUtilityDraw,
					 CShaderPipeline* pShaderSystem,
					 CStringDictionary* pStringDictionary);
		virtual ~CRunModeTest();
		virtual void Tick(float DeltaSec);

	private:
		CView m_View;
		CViewController m_ViewController;
		CInput* pInput;
		CConstantsSystem* m_pConstantsSystem;
		CRenderObjectSystem* m_pRenderObjectSystem;
		CLoadingSystem* m_pLoadingSystem;
		CLightSystem* m_pLightSystem;
		CTimeLine* m_pTimeLine;
		CRenderer* m_pRenderer;
		CAnimationSystem* m_pAnimationSystem;
		CTextureSystem* m_pTextureSystem;
		CMaterialSystem* m_pMaterialSystem;
		CUtilityDraw* m_pUtilityDraw;
		CShaderPipeline* m_pShaderPipeline;
		CStringDictionary* m_pStringDictionary;
};

#endif
