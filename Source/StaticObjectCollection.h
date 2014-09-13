#ifndef _STATICOBJECTCOLLECTION_H_
#define _STATICOBJECTCOLLECTION_H_

class CRenderObjectSystem;
class CRenderer;
class CConstantsSystem;
class CRenderObject;

class CStaticObjectCollection
{
	public:
		CStaticObjectCollection();
		void Create(const char* pName, unsigned int NumObjects, CRenderObjectSystem* pRenderObjectSystem);
		void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
	private:
		unsigned int	m_NumObjects;
		CRenderObject**	m_ppRenderObjects;
};

#endif