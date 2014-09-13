#ifndef _INSTANCEDOBJECT_H_
#define _INSTANCEDOBJECT_H_

class CRenderObjectSystem;
class CRenderer;
class CConstantsSystem;
class CInstancedRenderObject;

class CInstancedObject
{
	public:
		CInstancedObject();
		void Create(const char* pName, CRenderObjectSystem* pRenderObjectSystem);
		void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
		void SetScaleRotationPosition(float ScaleX, float ScaleY, float ScaleZ, float Roll, float Pitch, float Yaw, float OffsetX, float OffsetY, float OffsetZ);

	private:
		bool IsLoaded();
		CInstancedRenderObject* m_pRenderObject;
};

#endif