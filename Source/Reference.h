#ifndef _CREFERENCE_H_
#define _CREFERENCE_H_

template<class DataType>
class CReference
{
	public:
		CReference(DataType* Type)
		{
			m_ppReference = Type;
		}

		DataType operator->()
		{
			return *m_ppReference;
		}

		DataType& operator*()
		{
			return *m_ppReference;
		}

	private:
		DataType* m_ppReference;
};
#endif
