#include "stdafx.h"
#include "RenderObject.h"
#include "Renderer.h"
#include "ConstantsSystem.h"

#include "StaticObjectCollection.h"

//------------------------------------------------------------------------------------
CStaticObjectCollection::CStaticObjectCollection() : m_NumObjects(0),
m_ppRenderObjects(NULL)
{
}

//------------------------------------------------------------------------------------
void CStaticObjectCollection::Create(const char* pName, unsigned int NumObjects, CRenderObjectSystem* pRenderObjectSystem)
{
	m_NumObjects = NumObjects;
	m_ppRenderObjects = new CRenderObject*[NumObjects];
	char Name[256];
	for (unsigned int ObjectIndex = 0; ObjectIndex < m_NumObjects; ObjectIndex++)
	{
		sprintf_s(Name, "%s_%d.%s", pName, ObjectIndex, CRenderObject::GetTypeName());
		m_ppRenderObjects[ObjectIndex] = &pRenderObjectSystem->GetRenderObjectRef(Name);
	}
}

//------------------------------------------------------------------------------------
void CStaticObjectCollection::Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem)
{
	for (unsigned int ObjectIndex = 0; ObjectIndex < m_NumObjects; ObjectIndex++)
	{
		if (m_ppRenderObjects[ObjectIndex]->IsLoaded())
		{
			m_ppRenderObjects[ObjectIndex]->Draw(pRenderer, pConstantsSystem);
		}
	}
}

