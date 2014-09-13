#ifndef _CVIEWCONTROLLER_H_
#define _CVIEWCONTROLLER_H_

#include "HIDInputInterface.h"

class CInput;
class CView;


class CViewController
{
	public:
		CViewController(CInput*	pInput, CView* pView);
		void Tick(float DeltaSec);

		class CViewControllerInput : public IHIDInput<CViewController>
		{
			public:
				CViewControllerInput()
				{
				}
		};

		void ProcessHIDInput(const unsigned char* pInputBuffer);

	protected:
		virtual void Forward(float Val);
		virtual void Lateral(float Val);
		virtual void Pitch(float Val);
		virtual void Yaw(float Val);
		virtual void Roll(float Val);

	private:
		bool	m_LeftMouseButtonPressed;
		bool	m_IsConsoleMode;
		CInput*	m_pInput;
		CView*  m_pView;

		float m_PitchVelocity;
		float m_YawVelocity;
		float m_RollVelocity;
		float m_PitchChange;
		float m_YawChange;
		float m_RollChange;
		float m_ForwardVelocity;
		float m_LateralVelocity;
		float m_ForwardChange;
		float m_LateralChange;

		CViewControllerInput m_ViewControllerInput;

};


#endif