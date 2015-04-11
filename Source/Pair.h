#ifndef _PAIR_H_
#define _PAIR_H_

template<class ValueType0, class ValueType1>
class CPair
{
	public:
		CPair(ValueType0 Value0, ValueType1	Value1) : m_Value0(Value0),
			m_Value1(Value1)
		{
		}

		ValueType0	m_Value0;
		ValueType1	m_Value1;
};

#endif