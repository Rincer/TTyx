#ifndef _DRAWPRIMITIVE_H_
#define _DRAWPRIMITIVE_H_

#include "MaterialSystem.h"
#include "ElementList.h"
#include "MultiStageAssetContainer.h"
#include "Reference.h"

static const unsigned int scMaxTrianglesPerPrimitive = 16 * 1024; // 16k
static const unsigned int scMaxVertsPerPrimitive = scMaxTrianglesPerPrimitive * 3;

class CVBuffer;
class CIBuffer;
class CStringDictionary;

//-----------------------------------------------------------------------------------
class CBasePrimitiveState
{
	public:
		enum eStateType
		{
			eUnloaded,
			eReference,
			eLoaded,
			eMaxStages
		};
};

//-----------------------------------------------------------------------------------
class CBasePrimitive : public CStateAccessor<CBasePrimitiveState::eStateType>
{
	public:
		CBasePrimitive() : m_pMaterial(NULL)
		{
			SetState(CBasePrimitiveState::eUnloaded);
		}

		CBasePrimitive(const CMaterial*	pMaterial) : m_pMaterial(pMaterial)
		{
			SetState(CBasePrimitiveState::eUnloaded);
		}

		void SetMaterial(const CMaterial* pMaterial)
		{
			Assert(pMaterial->IsLoaded());
			m_pMaterial = pMaterial;
		}

		// creates a clone of itself and material into the provided memory
		void Clone(void* pMemory) const;

		virtual void Draw(ID3D11DeviceContext* m_pDeviceContext) = 0;

		virtual unsigned int GetSizeWithMaterial() const = 0;

	protected:
		virtual bool AllResourcesLoaded() const = 0;
		const CMaterial* m_pMaterial;
};


//-----------------------------------------------------------------------------------
class CDrawPrimitive final: public CBasePrimitive
{
	public:
		
		static const unsigned int scMaxStreams = 2;

		CDrawPrimitive() : CBasePrimitive(),
			m_pIBuffer(NULL),
			m_IndexCount(0),
			m_NumStreams(0)
		{
			for(unsigned int Stream = 0; Stream < scMaxStreams; Stream++)
			{
				m_pVBuffer[Stream] = 0;
			}
		}
		
		CDrawPrimitive(	CVBuffer* pVBuffer[scMaxStreams], 
						unsigned int NumStreams, 
						CIBuffer* pIBuffer, 
						unsigned int IndexCount, 
						const CMaterial* pMaterial) :	CBasePrimitive(pMaterial),
														m_NumStreams(NumStreams),	
														m_pIBuffer(pIBuffer),
														m_IndexCount(IndexCount)
		{
			for(unsigned int Stream = 0; Stream < m_NumStreams; Stream++)
			{
				m_pVBuffer[Stream] = pVBuffer[Stream];
			}
		}
		virtual void Draw(ID3D11DeviceContext* m_pDeviceContext);
		
		virtual unsigned int GetSizeWithMaterial() const
		{
			return sizeof(*this) + m_pMaterial->GetSize();
		}

	protected:
		virtual bool AllResourcesLoaded() const final;

	private:
		CVBuffer*					m_pVBuffer[scMaxStreams];
		CIBuffer*					m_pIBuffer;
		unsigned int				m_IndexCount;
		unsigned int				m_NumStreams;

	friend class CMultiStageAssetContainer<CDrawPrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference>;

};


//-----------------------------------------------------------------------------------
class CComputePrimitive final : public CBasePrimitive
{
	public:
		CComputePrimitive() : CBasePrimitive()
								
		{
		}

		CComputePrimitive(	const CMaterial* pMaterial,
							unsigned int	 DispatchX,
							unsigned int	 DispatchY,
							unsigned int	 DispatchZ) :	CBasePrimitive(pMaterial),
															m_DispatchX(DispatchX),
															m_DispatchY(DispatchY),
															m_DispatchZ(DispatchZ)
		{
		}

		virtual void Draw(ID3D11DeviceContext* m_pDeviceContext);

		virtual unsigned int GetSizeWithMaterial() const
		{
			return sizeof(*this) + m_pMaterial->GetSize();
		}

		void SetThreadGroupCount(unsigned int DispatchX, unsigned int DispatchY, unsigned int DispatchZ);

	protected:
		virtual bool AllResourcesLoaded() const final;

	private:
		unsigned int				m_DispatchX;
		unsigned int				m_DispatchY;
		unsigned int				m_DispatchZ;

	friend class CMultiStageAssetContainer<CComputePrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference>;
};

//-----------------------------------------------------------------------------------
class CDrawPrimitiveSystem
{
	public:	
		CDrawPrimitiveSystem(CStringDictionary** ppStringDictionary);
		~CDrawPrimitiveSystem();
							
		CDrawPrimitive& Add(const char* pName, CVBuffer* pVBuffer[CDrawPrimitive::scMaxStreams], unsigned int NumStreams, CIBuffer* pIBuffer, unsigned int IndexCount, const CMaterial* pMaterial);
		CComputePrimitive& Add(const char* pName, unsigned int DispatchX, unsigned int DispatchY, unsigned int DispatchZ, const CMaterial* pMaterial);

		void AcquireMutex();
		void ReleaseMutex();
		void Tick(float DeltaSec);		
						
	private:	
		CLWMutex m_Mutex;
		CMultiStageAssetContainer<CDrawPrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference> m_DrawPrimitives;
		CMultiStageAssetContainer<CComputePrimitive, CBasePrimitiveState::eMaxStages, CBasePrimitiveState::eStateType, CBasePrimitiveState::eUnloaded, CBasePrimitiveState::eReference> m_ComputePrimitives;
		CReference <CStringDictionary*> m_rStringDictionary;
};



#endif