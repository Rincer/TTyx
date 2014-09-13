#ifndef _VERTEXPROCESSING_H_
#define _VERTEXPROCESSING_H_
#include "VertexFormats.h"

class CHeapAllocator;


class CVertexProcessing
{
	public:
		class CFace
		{
			public:
				unsigned short m_Indices[3];
				unsigned short m_SmoothingGroup;
		};
		
		CVertexProcessing();
		~CVertexProcessing();
		
		static inline CVertexProcessing& Instance()
		{
			static CVertexProcessing s_VertexProcessing;
			return s_VertexProcessing;
		}
				
		void CreateTangents(SVertexObj* pVertices, unsigned short NumVertices, const unsigned short* pIndices, unsigned int NumIndices);
		void CreateSphere(SPos* pVertices, unsigned short* pIndices, unsigned short Segments);
					
	private:
		CHeapAllocator* m_pHeapAllocator;
};


#endif
