#ifndef _COMMS_CHANNEL_H
#define _COMMS_CHANNEL_H

class CNetwork;

enum eChannelIndex
{
	eChannelLiveEdit,
	eMaxChannels
};

static const unsigned int scMaxCommsBufferSize = 64 * 1024; // The maximum number of bytes that can be sent in a single packet

class ICommsChannel
{
	public:
		ICommsChannel(eChannelIndex ChannelIndex, unsigned char* pCommsBuffer, unsigned int Size, CNetwork*	pNetwork);
		virtual ~ICommsChannel();
		void Send(unsigned char* pData, unsigned int Size);
		virtual void OnReceive(unsigned int Size) = 0;
		unsigned char* GetCommsBuffer();
		unsigned int   GetChannelIndex();

	protected:
		eChannelIndex	m_ChannelIndex;	
		unsigned char*  m_pCommsBuffer;
		unsigned int	m_Size;
		CNetwork*		m_pNetwork;
};

#endif