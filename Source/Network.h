#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "Timer.h"
#include "Thread.h"
#include "CommsChannel.h"
#include "RingBuffer.h"
#include "Reference.h"

class CTimeLine;

class CNetwork
{
	enum eCommandType
	{
		eData,
		eDisconnect,
		eMaxCommands
	};

	enum eBufferReadState
	{
		eReadingHeader,
		eReadingData
	};
	
	public:
		class CNetworkHeader
		{
			public:
				CNetworkHeader()
				{
					m_Magic[0] = 'x';
					m_Magic[1] = 'y';
					m_Magic[2] = 'T';
					m_Magic[3] = 'T';
					m_Type = eData;
					m_ChannelIndex = 0;
					m_Size = 0;
				}
				CNetworkHeader(eChannelIndex ChannelIndex, eCommandType Type, unsigned int Size)
				{
					m_Magic[0] = 'x';
					m_Magic[1] = 'y';
					m_Magic[2] = 'T';
					m_Magic[3] = 'T';
					m_Type = Type;
					m_ChannelIndex = ChannelIndex;
					m_Size = Size;
				}
				bool				VerifyMagic();
				char				m_Magic[4];
				eCommandType		m_Type;
				unsigned int		m_ChannelIndex;
				unsigned int		m_Size;
		};

		CNetwork(CTimeLine** ppTimeLine);
		~CNetwork();
				
		void Tick(unsigned int CurrentFrame, float DeltaTime);
		void WaitForConnection();
		void RegisterCommsChannel(ICommsChannel* pCommsChannel);
		void Send(eChannelIndex ChannelIndex, unsigned char* pData, unsigned int Size);
		
	private:
		static const unsigned int scMaxConnections = 16;

		void Startup();
		void InitializeSockets();
		void Broadcast();
		void UpdateConnections();
		void ProcessClosedConnections();
		void ProcessNewConnections();
		void ProcessReceivedData();

		static void AcceptConnection(void* pContext);
		static void ReceiveData(void* pContext);
		static void SendData(void* pContext);

		IN_ADDR			m_BroadcastAddress;	
		IN_ADDR			m_Ip;
		SOCKET			m_BroadcastSocket;
		SOCKET			m_ListenSocket;		
		SOCKET			m_NewConnection;
		SOCKET			m_ConnectedSockets[scMaxConnections];
		unsigned int	m_NumConnections;
		unsigned int	m_PendingConnection;
		unsigned int    m_CachedConnection;
		SOCKADDR_IN		m_BroadcastSocketAddress;
		CThread			m_AcceptThread;
		CThread			m_RecvThread[scMaxConnections];
		CThread			m_SendThread;
		int				m_RecvStatus[scMaxConnections];
		CTimer			m_BroadcastTimer;
		CTimer			m_TestTimer;
		char			m_HostName[128];	
		ICommsChannel*  m_pCommsChannels[eMaxChannels];
		CRingBuffer*	m_pRecvBuffer;
		CRingBuffer*	m_pSendBuffer;
		CNetworkHeader  m_CurrentHeader;
		eBufferReadState m_CurrentReadState;	
		CReference<CTimeLine*>	m_rTimeLine;
};

#endif
