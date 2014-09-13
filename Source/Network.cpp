#include "stdafx.h"
#include "CommsChannel.h"
#include "TimeLine.h"

#include "Network.h"


static const float scBroadcastInterval = 1.0f; // sec
static unsigned short scPort = 27015;
static char s_BroadcastBuffer[256];
static const unsigned int scMaxPacketSize = scMaxCommsBufferSize;
static const unsigned int scRecvBufferSize = (scMaxPacketSize * 4);
static const unsigned int scSendBufferSize = (scMaxPacketSize * 4);


//----------------------------------------------------------------------------------------------
bool CNetwork::CNetworkHeader::VerifyMagic()
{
	return (m_Magic[0] == 'x') && (m_Magic[1] == 'y') && (m_Magic[2] == 'T') && (m_Magic[3] == 'T');
}

//----------------------------------------------------------------------------------------------
CNetwork::CNetwork(CTimeLine** ppTimeLine) : m_rTimeLine(ppTimeLine)
{	
	Startup();
}

//----------------------------------------------------------------------------------------------		
CNetwork::~CNetwork()
{
	closesocket(m_ListenSocket);	// this will also unblock the accept thread
	m_AcceptThread.WaitForWorkComplete(); // Wait for accept thread to unblock
	m_AcceptThread.Shutdown();
	for(int i = 0; i < scMaxConnections; i++)
	{
		if(m_ConnectedSockets[i] != INVALID_SOCKET)
		{
			closesocket(m_ConnectedSockets[i]); // this will unblock the recv() calls on all blocking receiver threads.
			m_RecvThread[i].Shutdown();
		}
	}
	m_SendThread.Shutdown();
	closesocket(m_BroadcastSocket);
	delete m_pRecvBuffer;
	delete m_pSendBuffer;
}

//----------------------------------------------------------------------------------------------
void CNetwork::InitializeSockets()
{
    int Err = 0;
    PMIB_IPADDRTABLE pIPAddrTable;
    unsigned long Size = 0;
    pIPAddrTable = (MIB_IPADDRTABLE *)malloc(sizeof (MIB_IPADDRTABLE));
	Err = GetIpAddrTable(pIPAddrTable, &Size, 0);
	Assert((Err == NO_ERROR) || (Err == ERROR_INSUFFICIENT_BUFFER));	
	free(pIPAddrTable);
	
    pIPAddrTable = (MIB_IPADDRTABLE *) malloc(Size);	
	Err = GetIpAddrTable(pIPAddrTable, &Size, 0);    
	Assert(Err == 0);	
	
	// try to find a valid IP.
	unsigned int LocalHostAddressIndex = 0;
	unsigned int AddressIndex;
	for (AddressIndex = 0; AddressIndex < pIPAddrTable->dwNumEntries; AddressIndex++)
	{
		if(pIPAddrTable->table[AddressIndex].dwAddr == 0x0100007F)
		{
			LocalHostAddressIndex = AddressIndex;
		}
		else if(pIPAddrTable->table[AddressIndex].wType & MIB_IPADDR_PRIMARY) 
		{		
			break;
		}			
	}

	// If no valid IP found fallback to local host
	if(AddressIndex == pIPAddrTable->dwNumEntries)
	{
		AddressIndex = LocalHostAddressIndex;
	} 	
		
	IN_ADDR Mask;
	
	Mask.S_un.S_addr = pIPAddrTable->table[AddressIndex].dwMask;
	m_Ip.S_un.S_addr = pIPAddrTable->table[AddressIndex].dwAddr;
	m_BroadcastAddress.S_un.S_addr = (m_Ip.S_un.S_addr & Mask.S_un.S_addr) | (~Mask.S_un.S_addr);
	m_BroadcastSocket = INVALID_SOCKET;	
	m_BroadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);	
	BOOL Broadcast = TRUE;
	Err = setsockopt(m_BroadcastSocket, SOL_SOCKET, SO_BROADCAST, (CHAR *) &Broadcast, sizeof (BOOL));
    Assert(Err == 0);
	memset(&m_BroadcastSocketAddress, 0, sizeof(m_BroadcastSocketAddress));
	m_BroadcastSocketAddress.sin_family = AF_INET;
    m_BroadcastSocketAddress.sin_addr.s_addr = m_BroadcastAddress.S_un.S_addr;
    m_BroadcastSocketAddress.sin_port = htons(scPort);
	
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
	SOCKADDR_IN Service;
    Service.sin_family = AF_INET;
    Service.sin_addr.s_addr = m_Ip.S_un.S_addr;
    Service.sin_port = htons(scPort);
	m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    Err = bind(m_ListenSocket, (SOCKADDR*)&Service, sizeof(Service));
	Err = listen(m_ListenSocket, SOMAXCONN);
	m_AcceptThread.Startup(AcceptConnection, this, "Net Accept", NULL, *m_rTimeLine);
	m_AcceptThread.SetWorkReady();
}

//----------------------------------------------------------------------------------------------
void CNetwork::Startup()
{
	// Initialize Winsock
	WSADATA wsaData;
	int Err = WSAStartup(MAKEWORD(2,2), &wsaData);
	Assert(Err == 0);	
	gethostname(m_HostName, sizeof(m_HostName) - 1);
	sprintf_s(s_BroadcastBuffer, 255, "%s,TTyxServer", m_HostName);
	InitializeSockets();	
	m_BroadcastTimer.SetFrequency(scBroadcastInterval);
	m_TestTimer.SetFrequency(0.016f);
	for(int i = 0; i < scMaxConnections; i++)
	{
		m_ConnectedSockets[i] = INVALID_SOCKET; 
		m_RecvStatus[i] = 1; // set to some valid value
	}
	m_NumConnections = 0;
	m_PendingConnection = 0;
	for(int i = 0; i < eMaxChannels; i++)
	{
		m_pCommsChannels[i] = NULL;
	}
	m_pRecvBuffer = new CRingBuffer(scRecvBufferSize, NULL);
	m_pSendBuffer = new CRingBuffer(scRecvBufferSize, NULL);
	m_CurrentReadState = eReadingHeader;
	m_SendThread.Startup(SendData, this, "Net Send", NULL, *m_rTimeLine);
	m_SendThread.SetWorkReady();
}

//----------------------------------------------------------------------------------------------
void CNetwork::ProcessClosedConnections()
{
	// update any closed connections first
	for(int i = 0; i < scMaxConnections; i++)
	{
		// something went wrong so close the connection
		if((m_ConnectedSockets[i] != INVALID_SOCKET) && ((m_RecvStatus[i] == 0) || (m_RecvStatus[i] == SOCKET_ERROR))) 
		{
			closesocket(m_ConnectedSockets[i]);
			m_ConnectedSockets[i] = INVALID_SOCKET;
			m_RecvStatus[i] = 1; // set to some valid value
			m_RecvThread[i].Shutdown();
			// if we were full then now that we closed one, we have space for a new connection so unblock the accept thread
			if(m_NumConnections == scMaxConnections)
			{
				m_AcceptThread.SetWorkReady();
			}
			m_NumConnections--;
		}		
	}
}

//----------------------------------------------------------------------------------------------
void CNetwork::ProcessNewConnections()
{
	// check for any incoming connections
	if(m_AcceptThread.IsWorkComplete())
	{		
		m_CachedConnection = m_PendingConnection;
		m_ConnectedSockets[m_PendingConnection] = m_NewConnection;
		m_RecvThread[m_PendingConnection].Startup(ReceiveData, this, "Net Recv", NULL, *m_rTimeLine);
		m_RecvThread[m_PendingConnection].SetWorkReady();		
		m_NumConnections++;
		// if we have space look for more connections
		if(m_NumConnections < scMaxConnections)
		{
			while(m_ConnectedSockets[m_PendingConnection] != INVALID_SOCKET)
			{
				m_PendingConnection = (m_PendingConnection + 1) % scMaxConnections;
			}
			m_AcceptThread.SetWorkReady();
		}
	}
}

//----------------------------------------------------------------------------------------------
void CNetwork::ProcessReceivedData()
{
	bool ReadRes = false;
	do
	{
		switch(m_CurrentReadState)
		{
			case eReadingHeader:
			{				
				ReadRes = m_pRecvBuffer->TryRead((unsigned char*)&m_CurrentHeader, sizeof(CNetworkHeader));	
				if(ReadRes)
				{				
					m_CurrentReadState = eReadingData;
					Assert(m_CurrentHeader.VerifyMagic());
					Assert(m_CurrentHeader.m_Size);
				}
				break;
			}

			case eReadingData:
			{
				ICommsChannel* pCommsChannel = m_pCommsChannels[m_CurrentHeader.m_ChannelIndex];
				ReadRes = m_pRecvBuffer->TryRead(pCommsChannel->GetCommsBuffer(), m_CurrentHeader.m_Size);	
				if(ReadRes)
				{				
					m_CurrentReadState = eReadingHeader;
					pCommsChannel->OnReceive(m_CurrentHeader.m_Size);
				}
				break;
			}
		}
	}
	while(ReadRes);	
}

//----------------------------------------------------------------------------------------------
void CNetwork::UpdateConnections()
{
	ProcessClosedConnections();
	ProcessNewConnections();
}	

//----------------------------------------------------------------------------------------------
void CNetwork::Tick(unsigned int CurrentFrame, float DeltaTime)
{
	CurrentFrame; // C4100
	if(m_SendThread.IsWorkComplete())
	{
		m_SendThread.SetWorkReady();
	}

	bool bDoBroadcast;
	// periodically broadcast on the subnet, so that any clients can get our IP and connect. 
	bDoBroadcast = m_BroadcastTimer.Tick(DeltaTime);
	if(bDoBroadcast)
	{
		Broadcast();		
	}
	UpdateConnections();	
	ProcessReceivedData();
}

//----------------------------------------------------------------------------------------------
void CNetwork::Broadcast()
{
	int Err;
	Err = sendto(	m_BroadcastSocket,
					s_BroadcastBuffer,
					(int)strlen(s_BroadcastBuffer) + 1,
					0,
					(SOCKADDR*)&m_BroadcastSocketAddress,
					(int)sizeof(SOCKADDR_IN));    
}

//----------------------------------------------------------------------------------------------
// this happens on the accept connection thread
void CNetwork::WaitForConnection()
{
	// only set m_NewConnection here, the actual m_ConnectedSockets will get updated in the main network tick, so
	// no threading synchornization is necessary.
	m_NewConnection = accept(m_ListenSocket, NULL, NULL);
}

//----------------------------------------------------------------------------------------------
// this happens on the accept connection thread
void CNetwork::AcceptConnection(void* pContext)
{
	((CNetwork*)pContext)->WaitForConnection();
}

//----------------------------------------------------------------------------------------------
void CNetwork::ReceiveData(void* pContext)
{
	CNetwork* pNetwork = (CNetwork*)pContext;
	unsigned int ConnectionIndex = pNetwork->m_CachedConnection;
	SOCKET ReceiveSocket = pNetwork->m_ConnectedSockets[ConnectionIndex];	
	while((pNetwork->m_RecvStatus[ConnectionIndex] != 0) && (pNetwork->m_RecvStatus[ConnectionIndex] != SOCKET_ERROR))
	{		
		CNetworkHeader NetworkHeader;	
		unsigned char  ReceiveBuffer[scMaxPacketSize]; // make sure thread creation stack size is large enough for this
		pNetwork->m_RecvStatus[ConnectionIndex] = recv(ReceiveSocket, (char*)&NetworkHeader, sizeof(CNetworkHeader), MSG_WAITALL);	
		pNetwork->m_RecvStatus[ConnectionIndex] = recv(ReceiveSocket, (char*)ReceiveBuffer, NetworkHeader.m_Size, MSG_WAITALL);	
		if((pNetwork->m_RecvStatus[ConnectionIndex] != 0) && (pNetwork->m_RecvStatus[ConnectionIndex] != SOCKET_ERROR))
		{			
			Assert(NetworkHeader.VerifyMagic());
			Assert(NetworkHeader.m_Size <= scMaxPacketSize && NetworkHeader.m_Size > 0);
			pNetwork->m_pRecvBuffer->Write((unsigned char*)&NetworkHeader, sizeof(CNetworkHeader));	
			pNetwork->m_pRecvBuffer->Write(ReceiveBuffer, NetworkHeader.m_Size);				
		}
	}		
}

//----------------------------------------------------------------------------------------------
void CNetwork::SendData(void* pContext)
{
	CNetwork* pNetwork = (CNetwork*)pContext;
	CNetworkHeader NetworkHeader;	
	while(pNetwork->m_pSendBuffer->TryRead((unsigned char*)&NetworkHeader, sizeof(CNetworkHeader)))
	{
		unsigned char SendBuffer[scMaxPacketSize]; // make sure thread creation stack size is large enough for this
		pNetwork->m_pSendBuffer->Read((unsigned char*)SendBuffer, NetworkHeader.m_Size);
		for(int i = 0; i < scMaxConnections; i++)
		{
			SOCKET SendSocket = pNetwork->m_ConnectedSockets[i];	
			if(SendSocket != INVALID_SOCKET)
			{		
				send(SendSocket, (char*)&NetworkHeader, sizeof(CNetworkHeader), 0);	
				send(SendSocket, (char*)SendBuffer,  NetworkHeader.m_Size, 0);		
			}		
		}
	}
}

//----------------------------------------------------------------------------------------------
void CNetwork::RegisterCommsChannel(ICommsChannel* pCommsChannel)
{
	Assert(m_pCommsChannels[pCommsChannel->GetChannelIndex()] == NULL);
	m_pCommsChannels[pCommsChannel->GetChannelIndex()] = pCommsChannel;
}

//----------------------------------------------------------------------------------------------
void CNetwork::Send(eChannelIndex ChannelIndex, unsigned char* pData, unsigned int Size)
{
	if(m_NumConnections > 0)
	{
		CNetworkHeader Header(ChannelIndex, eData, Size);
		m_pSendBuffer->Write((unsigned char*)&Header,sizeof(Header));
		m_pSendBuffer->Write(pData, Size);
	}
}