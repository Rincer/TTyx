#include "stdafx.h"
#include "Network.h"

#include "CommsChannel.h"

//--------------------------------------------------------------------------------
ICommsChannel::ICommsChannel(eChannelIndex ChannelIndex, unsigned char* pCommsBuffer, unsigned int Size, CNetwork* pNetwork) : m_ChannelIndex(ChannelIndex),
																															   m_pCommsBuffer(pCommsBuffer),
																															   m_Size(Size),
																															   m_pNetwork(pNetwork)
{			
	Assert(Size < scMaxCommsBufferSize);
}

//--------------------------------------------------------------------------------
ICommsChannel::~ICommsChannel()
{
	m_pCommsBuffer = NULL;
}

//--------------------------------------------------------------------------------
unsigned char* ICommsChannel::GetCommsBuffer()
{
	return m_pCommsBuffer;
}

//--------------------------------------------------------------------------------
void ICommsChannel::Send(unsigned char* pData, unsigned int Size)
{
	m_pNetwork->Send(m_ChannelIndex, pData, Size);
}

//--------------------------------------------------------------------------------
unsigned int ICommsChannel::GetChannelIndex()
{
	return m_ChannelIndex;
}