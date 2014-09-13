#ifndef _SKELETALOBJECT_H_
#define _SKELETALOBJECT_H_

#include "Animation.h"

class CRenderObjectSystem;
class CAnimationSystem;
class CRenderer;
class CConstantsSystem;
class CSkeletalRenderObject;

class CSkeletalObject
{
	public:
		CSkeletalObject();
		void Create(const char* pName, CRenderObjectSystem* pRenderObjectSystem, CAnimationSystem* pAnimationSystem);
		void TickAnimation(float DeltaSec);
		void Draw(CRenderer* pRenderer, CConstantsSystem* pConstantsSystem);
		void SetScaleRotationPosition(float ScaleX, float ScaleY, float ScaleZ, float Roll, float Pitch, float Yaw, float OffsetX, float OffsetY, float OffsetZ);

	private:
		bool IsLoaded();

		CSkeletalRenderObject* m_pRenderObject;
		CAnimationController m_AnimationController;
};

#endif
