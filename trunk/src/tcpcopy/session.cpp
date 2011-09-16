#include <map>
#include <arpa/inet.h>
#include "session.h"
#include "send.h"
#include "../communication/msg.h"
#include "../log/log.h"
#include "address.h"

using std::map;

uint32_t localhost_ip;
uint32_t company_ip;
uint32_t client_ip;
virtual_ip_addr local_ips;
uint32_t remote_ip;
uint16_t local_port;
uint16_t remote_port;

typedef map<uint64_t,session_st> SessContainer;
typedef map<uint64_t,uint32_t> IPContainer;
typedef map<uint64_t,session_st>::iterator SessIterator;
typedef map<uint64_t,uint32_t>::iterator IPIterator;

static SessContainer sessions;
static IPContainer trueIPContainer;
static int activeCount=0;
static int enterCount=0;
static int leaveCount=0;
static int deleteObsoCount=0;
static int timeCount=0;
static int totalContentPackets=0;
static int totalPackets=0;
static int totalSynPackets=0;
static int totalSendPackets=0;
static int totalResponse=0;
static int invalidPacketsFromClient=0; 

static uint64_t totalRequests=0;

/**
 * clear timeout tcp sessions
 */
static int clearTimeoutTcpSessions()
{
	//we clear old sessions every one minute 
	//this may be a problem for keepalive connections
	time_t normalBase=time(0)-60;
	time_t keepaliveBase=time(0)-3600;
	time_t tmpBase=0;
	double ratio=100.0*enterCount/(totalRequests+1);
	if(ratio<10)
	{
		normalBase=keepaliveBase;
		logInfo(LOG_NOTICE,"keepalive connection global");
	}
	for(SessIterator p=sessions.begin();p!=sessions.end();)
	{
		if(p->second.isKeepalive)
		{
			tmpBase=keepaliveBase;
		}else
		{
			tmpBase=normalBase;
		}

		if(p->second.lastUpdateTime<tmpBase)
		{
			deleteObsoCount++;
			if(!p->second.isStatClosed)
			{
				p->second.isStatClosed=true;
				activeCount--;
			}
			sessions.erase(p++);
		}else
		{
			p++;
		}
	}
	return 0;
}

/**
 * output packet info for debug
 */
void outputPacketForDebug(int level,int flag,struct iphdr *ip_header,
		struct tcphdr *tcp_header)
{
	if(output_level < level)
	{
		return;
	}
	struct in_addr srcaddr;
	struct in_addr destaddr;
	srcaddr.s_addr=ip_header->saddr;
	destaddr.s_addr=ip_header->daddr;
	char* tmpbuf=inet_ntoa(srcaddr);
	char sbuf[1024];
	memset(sbuf,0,1024);
	strcpy(sbuf,tmpbuf);
	char dbuf[1024];
	memset(dbuf,0,1024);
	tmpbuf=inet_ntoa(destaddr);
	strcpy(dbuf,tmpbuf);
	uint32_t packSize=ntohs(ip_header->tot_len);
	unsigned int seq=ntohl(tcp_header->seq);
	unsigned int ack_seq=ntohl(tcp_header->ack_seq);
	if(BACKEND_FLAG==flag)
	{
		logInfo(level,"from backend:%s:%u-->%s:%u,len %u,seq=%u,ack_seq=%u",
				sbuf,ntohs(tcp_header->source),dbuf,
				ntohs(tcp_header->dest),packSize,seq,ack_seq);
	}else if(CLIENT_FLAG==flag)
	{
		logInfo(level,"recv client:%s:%u-->%s:%u,len %u,seq=%u,ack_seq=%u",
				sbuf,ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}else if(SERVER_BACKEND_FLAG==flag)
	{
		logInfo(level,"to backend: %s:%u-->%s:%u,len %u,seq=%u,ack_seq=%u",
				sbuf,ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}else if(RESERVE_CLIENT_FLAG==flag)
	{
		logInfo(level,"send buf packet %s:%u-->%s:%u,len %u,seq=%u,ack_seq=%u",
				sbuf,ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}else if(UNKNOWN_FLAG==flag)
	{
		logInfo(level,"unkown packet %s:%u-->%s:%u,len %u,seq=%u,ack_seq=%u",
				sbuf,ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}else
	{
		logInfo(level,"%s:%u-->%s:%u,length %u,seq=%u,ack_seq=%u",
				sbuf,ntohs(tcp_header->source),dbuf,ntohs(tcp_header->dest),
				packSize,seq,ack_seq);
	}
}

static bool checkLocalIPValid(uint32_t ip)
{
	bool result=false;
	int i=0;
	for(;i<local_ips.num;i++)
	{
		if(ip==local_ips.ips[i])
		{
			result=true;
			break;
		}
	}
	return result;
}

static inline uint32_t minus_1(uint32_t seq)
{
	return htonl(ntohl(seq)-1);
}

static inline uint32_t plus_1(uint32_t seq)
{
	return htonl(ntohl(seq)+1);
}

static bool checkPacketLost(struct tcphdr *tcp_header,uint32_t oldSeq)
{
	uint32_t curSeq=ntohl(tcp_header->seq);
	if(curSeq>oldSeq)
	{
		logInfo(LOG_INFO,"seq in the packet:%u,expected seq:%u",
				curSeq,oldSeq);
		return true;
	}
	return false;
}


static bool checkRetransmission(struct tcphdr *tcp_header,uint32_t oldSeq)
{
	uint32_t curSeq=ntohl(tcp_header->seq);
	if(curSeq<=oldSeq)
	{
		return true;
	}
	return false;
}

static bool checkTcpSeg(struct tcphdr *tcp_header,uint32_t oldSeq,
		uint32_t oldAck)
{
	uint32_t curSeq=ntohl(tcp_header->seq);
	if(curSeq<=oldSeq)
	{
		logInfo(LOG_INFO,"current seq %u ,last seq:%u from client",
				curSeq,oldSeq);
		return false;
	}
	uint32_t curAck=ntohl(tcp_header->ack_seq);
	if(curAck==oldAck)
	{
		return true;
	}
	if(0==oldSeq)
	{
		return true;
	}
	logInfo(LOG_INFO,"current ack %u is not valid ,last ack:%u from client",
			curAck,oldAck);
	return false;
}

/**
 * send reserved lost packets
 */
int session_st::sendReservedLostPackets()
{
	if(chosenOutput)
	{
		logInfo(LOG_DEBUG,"lost packet size:%d",lostPackets.size());
	}
	//sort the lostPackets
	//TODO

	while(true)
	{
		int count=0;
		for(dataIterator iter=lostPackets.begin();iter!=lostPackets.end();)
		{
			unsigned char *data =*iter;
			struct iphdr *ip_header=(struct iphdr*)((char*)data);
			uint32_t size_ip = ip_header->ihl<<2;
			struct tcphdr* tcp_header = (struct tcphdr*)((char *)ip_header
					+size_ip);
			uint32_t size_tcp = tcp_header->doff<<2;
			uint32_t packSize=ntohs(ip_header->tot_len);
			uint32_t contSize=packSize-size_tcp-size_ip;
			uint32_t currentSeq=ntohl(tcp_header->seq);	
			if(nextSeq==currentSeq)
			{
				if(chosenOutput)
				{
					if(contSize==0)
					{
						logInfo(LOG_WARN,"error info reserved in lostPackets");
					}else
					{
						isWaitResponse=true;
					}
					logInfo(LOG_DEBUG,"send reserved packets for lost packet");
					send_ip_packet(true,fake_ip_addr,data,virtual_next_sequence,
							&nextSeq);
					totalSendPackets++;
				}else
				{
					totalSendPackets++;
					send_ip_packet(false,fake_ip_addr,data,
							virtual_next_sequence,&nextSeq);
				}
				lastReqContSeq=ntohl(tcp_header->seq);
				lastAck=ntohl(tcp_header->ack_seq);
				count++;
				free(data);
				lostPackets.erase(iter++);
			}else
			{
				if(chosenOutput)
				{
					logInfo(LOG_DEBUG,"cant send packets for lost packet");
				}
				iter++;
			}
		}
		if(count==0)
		{
			break;
		}
	}
	if(lostPackets.empty())
	{
		isWaitPreviousPacket=false;
	}

	return 0;
}

/**
 * send reserved packets to backend
 */
int session_st::sendReservedPackets()
{
	bool needPause=false;
	bool mayPause=false;
	unsigned char* prevPacket=NULL;
	uint32_t prePackSize=0;
	int count=0;
	while(! unsend.empty()&&!needPause)
	{
		unsigned char *data = unsend.front();
		struct iphdr *ip_header=(struct iphdr*)((char*)data);
		uint32_t size_ip = ip_header->ihl<<2;
		struct tcphdr* tcp_header = (struct tcphdr*)((char *)ip_header+size_ip);
		uint32_t size_tcp = tcp_header->doff<<2;
		uint32_t packSize=ntohs(ip_header->tot_len);
		uint32_t contSize=packSize-size_tcp-size_ip;
		if(contSize>0)
		{
			if(mayPause)
			{
				if(!checkTcpSeg(tcp_header,lastReqContSeq,lastAck))
				{
					logInfo(LOG_INFO,"checkTcpSeq is not true");
					outputPacketForDebug(LOG_INFO,RESERVE_CLIENT_FLAG,
							ip_header,tcp_header);
					break;
				}
			}
			mayPause=true;
			isWaitResponse=true;
			lastReqContSeq=ntohl(tcp_header->seq);
			lastAck=ntohl(tcp_header->ack_seq);
		}else if(isWaitBakendClosed)
		{
			//if server closed socket before it is connected with backend
			//then it should not pause,or it will never send remaining 
			//packets to backend
			if(mayPause)
			{
				if(prePackSize==packSize)
				{
					//check if it is a duplicate short packet
					if(memcmp(data,prevPacket,packSize)==0)
					{
						mayPause=false;
						free(prevPacket);
						prevPacket=NULL;
					}
				}
				if(mayPause)
				{
					if(NULL!=prevPacket)
					{
						free(prevPacket);
					}
					//if two consecutive packets has no content,
					//then it will not send the second packet
					break;	
				}
			}
			mayPause=true;
			prevPacket=copy_ip_packet(ip_header);
			prePackSize=packSize;
		}else if(tcp_header->fin)
		{
			needPause=true;
			virtual_status |= CLIENT_FIN;
			confirmed=true;
		}else if(0==contSize&&isWaitResponse)
		{
			break;
		}

		if(chosenOutput)
		{
			send_ip_packet(true,fake_ip_addr,data,
					virtual_next_sequence,&nextSeq);
			totalSendPackets++;
		}else
		{
			totalSendPackets++;
			send_ip_packet(false,fake_ip_addr,data,
					virtual_next_sequence,&nextSeq);
		}
		count++;
		free(data);
		unsend.pop_front();
	}
	return count;
}

/**
 * save header info for later use
 */
void session_st::save_header_info(struct iphdr *ip_header,
		struct tcphdr *tcp_header){                                      
	client_ip_id = ip_header->id;
	if(isHalfWayIntercepted)
	{
		tcp_header->window=65535;
	}else
	{
		client_window=tcp_header->window;
	}
}

/**
 * send faked syn packet for backend for intercepting already connected packets
 */
void session_st::sendFakedSynToBackend(struct iphdr* ip_header,
		struct tcphdr* tcp_header)
{
	isHalfWayIntercepted=true;

	unsigned char fake_syn_buf[FAKE_SYN_BUF_SIZE];
	memset(fake_syn_buf,0,FAKE_SYN_BUF_SIZE);
	struct iphdr *ip_header2 = (struct iphdr *)fake_syn_buf;
	struct tcphdr *tcp_header2 = (struct tcphdr *)(fake_syn_buf+20);

	logInfo(LOG_NOTICE,"sendFakedSynToBackend");
	outputPacketForDebug(LOG_NOTICE,CLIENT_FLAG,ip_header,tcp_header);
	ip_header2->version = 4;
	ip_header2->ihl = 5;
	ip_header2->tot_len = htons(FAKE_SYN_BUF_SIZE);
	ip_header2->frag_off = 64; 
	ip_header2->ttl = 64; 
	ip_header2->protocol = 6;
	ip_header2->id= htons(client_ip_id+2);;
	ip_header2->saddr = ip_header->saddr;
	ip_header2->daddr = ip_header->daddr;
	tcp_header2->doff= 8;
	tcp_header2->source = tcp_header->source;
	tcp_header2->dest= tcp_header->dest;
	tcp_header2->syn=1;
	tcp_header2->seq = minus_1(tcp_header->seq);
	client_window=65535;
	tcp_header2->window= client_window;
	virtual_next_sequence=tcp_header->seq;
	unsigned char *data=copy_ip_packet(ip_header2);
	handshakePackets.push_back(data);
	outputPacketForDebug(LOG_NOTICE,CLIENT_FLAG,ip_header2,tcp_header2);
	logInfo(LOG_DEBUG,"send faked syn to backend,client window:%u",
			tcp_header2->window);
	send_ip_packet(true,fake_ip_addr,fake_syn_buf,
			virtual_next_sequence,&nextSeq);
	totalSendPackets++;
}

/**
 * send faked ack packet to backend  for syn
 */
void session_st::sendFakedSynAckToBackend(struct iphdr* ip_header,
		struct tcphdr* tcp_header)
{
	static unsigned char fake_ack_buf[40];
	memset(fake_ack_buf,40,0);
	struct iphdr *ip_header2 = (struct iphdr *)fake_ack_buf;
	struct tcphdr *tcp_header2 = (struct tcphdr *)(fake_ack_buf+20);
	ip_header2->version = 4;
	ip_header2->ihl = 5;
	ip_header2->tot_len = htons(40);
	ip_header2->frag_off = 64; 
	ip_header2->ttl = 64; 
	ip_header2->protocol = 6;
	ip_header2->id= htons(client_ip_id+2);;
	ip_header2->saddr = fake_ip_addr;
	ip_header2->daddr = local_dest_ip_addr; 
	tcp_header2->doff= 5;
	tcp_header2->source = tcp_header->dest;
	tcp_header2->dest= local_port;
	tcp_header2->ack=1;
	tcp_header2->ack_seq = virtual_next_sequence;
	tcp_header2->seq = tcp_header->ack_seq;
	tcp_header2->window= 65535;
	unsigned char *data=copy_ip_packet(ip_header2);
	handshakePackets.push_back(data);
	outputPacketForDebug(LOG_NOTICE,BACKEND_FLAG,ip_header,tcp_header);
	outputPacketForDebug(LOG_NOTICE,CLIENT_FLAG,ip_header2,tcp_header2);
	send_ip_packet(chosenOutput,fake_ip_addr,fake_ack_buf,
			virtual_next_sequence,&nextSeq);
	totalSendPackets++;
}

/**
 * send faked ack packet to backend 
 */
void session_st::sendFakedAckToBackend(struct iphdr* ip_header,
		struct tcphdr* tcp_header)
{
	static unsigned char fake_ack_buf[40];
	memset(fake_ack_buf,40,0);
	struct iphdr *ip_header2 = (struct iphdr *)fake_ack_buf;
	struct tcphdr *tcp_header2 = (struct tcphdr *)(fake_ack_buf+20);
	ip_header2->version = 4;
	ip_header2->ihl = 5;
	ip_header2->tot_len = htons(40);
	ip_header2->frag_off = 64; 
	ip_header2->ttl = 64; 
	ip_header2->protocol = 6;
	ip_header2->id= htons(client_ip_id+2);;
	ip_header2->saddr = ip_header->daddr;
	tcp_header2->doff= 5;
	tcp_header2->source = tcp_header->dest;
	tcp_header2->ack=1;
	tcp_header2->ack_seq = virtual_next_sequence;
	tcp_header2->seq = tcp_header->ack_seq;
	tcp_header2->window= client_window;
	logInfo(LOG_INFO,"send faked syn ack to backend,client window:%u",
			tcp_header2->window);
	send_ip_packet(chosenOutput,fake_ip_addr,fake_ack_buf,
			virtual_next_sequence,&nextSeq);
	totalSendPackets++;
}

/**
 * send faked fin to backend
 */
void session_st::sendFakedFinToBackend(struct iphdr* ip_header,
		struct tcphdr* tcp_header)
{
	static unsigned char fake_fin_buf[40];
	memset(fake_fin_buf,40,0);
	struct iphdr *ip_header2 = (struct iphdr *)fake_fin_buf;
	struct tcphdr *tcp_header2 = (struct tcphdr *)(fake_fin_buf+20);
	ip_header2->version = 4;
	ip_header2->ihl = 5;
	ip_header2->tot_len = htons(40);
	ip_header2->frag_off = 64; 
	ip_header2->ttl = 64; 
	ip_header2->protocol = 6;
	ip_header2->id= htons(client_ip_id+2);;
	ip_header2->saddr = ip_header->daddr;
	tcp_header2->doff= 5;
	tcp_header2->source = tcp_header->dest;
	tcp_header2->fin =1;
	tcp_header2->ack=1;
	tcp_header2->ack_seq = virtual_next_sequence;
	tcp_header2->seq = tcp_header->ack_seq;
	tcp_header2->window= client_window;
	send_ip_packet(chosenOutput,fake_ip_addr,fake_fin_buf,
			virtual_next_sequence,&nextSeq);
	totalSendPackets++;
}

/**
 * processing backend packets
 */
void session_st::update_virtual_status(struct iphdr *ip_header,
		struct tcphdr* tcp_header)
{
	if(chosenOutput)
	{
		outputPacketForDebug(LOG_DEBUG,BACKEND_FLAG,ip_header,tcp_header);
	}
	if( tcp_header->rst)
	{
		reset_flag = true;
		logInfo(LOG_INFO,"reset from backend");
		return;
	}
	virtual_ack = tcp_header->ack_seq;
	if( tcp_header->syn)
	{
		virtual_next_sequence = plus_1(tcp_header->seq);
		virtual_status = SYN_CONFIRM;
		if(isHalfWayIntercepted)
		{
			sendFakedSynAckToBackend(ip_header,tcp_header);
			sendReservedPackets();
		}else
		{
			sendReservedPackets();
		}
		return;
	}
	else if(tcp_header->fin)
	{
		isTestConnClosed=true;
		isWaitBakendClosed=false;
		virtual_status  |= SERVER_FIN;
		virtual_next_sequence = plus_1(tcp_header->seq);
		int count=sendReservedPackets();
		if(count>0)
		{
			confirmed=true;
		}else
		{
			isFakedSendingFinToBackend=true;
			//send constructed server fin to the backend
			sendFakedAckToBackend(ip_header,tcp_header);
			sendFakedFinToBackend(ip_header,tcp_header);
		}
		return;
	}else if(tcp_header->ack)
	{
		if(isWaitResponse)
		{
			totalRequests++;
			isTrueWaitResponse=true;
		}
	}
	uint32_t tot_len = ntohs(ip_header->tot_len);
	uint32_t size_ip = ip_header->ihl<<2;
	uint32_t size_tcp = tcp_header->doff<<2;
	uint32_t contSize=tot_len-size_tcp-size_ip;
	uint32_t next_seq = htonl(ntohl(tcp_header->seq)+contSize);

	if(contSize>0)
	{
		if(chosenOutput)
		{
			logInfo(LOG_DEBUG,"receive from backend");
		}
		if(isWaitResponse)
		{
			if(chosenOutput)
			{
				logInfo(LOG_DEBUG,"receive backent server's response");
			}
			totalResponse++;
			isWaitResponse=false;
			isTrueWaitResponse=false;
			virtual_next_sequence =next_seq;
			virtual_status = SEND_RESPONSE_CONFIRM;
			sendReservedPackets();
			return;
		}
	}
	virtual_next_sequence= next_seq;
}

/**
 * copy the ip packet
 */
unsigned char * session_st::copy_ip_packet(struct iphdr *ip_header)
{
	uint16_t tot_len = ntohs(ip_header->tot_len);
	unsigned char *data = (unsigned char *)malloc(tot_len);
	if(data)
	{
		memcpy(data,ip_header,tot_len);
		size_t size_ip = ip_header->ihl<<2;
		struct tcphdr *tcp_header = (struct tcphdr*)((char *)data+size_ip);
		if(tcp_header->dest==remote_port)
		{
			logInfo(LOG_ERR,"dest is wrong");
		}
	}
	return data;
}

/**
 * establish a connection for intercepting already connected packets
 */
void session_st::establishConnectionForNoSynPackets(struct iphdr *ip_header,
		struct tcphdr *tcp_header)
{
	bool isOutput=false;
	if(chosenOutput)
	{
		isOutput=true;
		logInfo(LOG_INFO,"establish conn for already connected conn");
	}
	{
		int sock=address_find_sock(tcp_header->dest);
		if(-1 == sock)
		{
			logInfo(LOG_WARN,"sock is invalid in est Conn for NoSynPackets");
			outputPacketForDebug(LOG_WARN,CLIENT_FLAG,ip_header,tcp_header);
			return;
		}
		int result=msg_copyer_send(sock,ip_header->saddr,
				tcp_header->source,CLIENT_ADD);
		if(-1 == result)
		{
			logInfo(LOG_ERR,"msg copyer send error");
			return;
		}
		sendFakedSynToBackend(ip_header,tcp_header);
		isSynIntercepted=true;
		totalSendPackets++;
	}
	activeCount++;

}
/**
 * establish a connection for already closed connection
 */
void session_st::establishConnectionForClosedConn()
{
	bool isOutput=false;
	if(chosenOutput)
	{
		isOutput=true;
		logInfo(LOG_INFO,"reestablish connection for keepalive conn");
	}

	if(handshakePackets.size()!=2)
	{
		logInfo(LOG_WARN,"error:handshakePackets size is not two");
	}else
	{
		unsigned char *data = handshakePackets.front();
		struct iphdr *ip_header = (struct iphdr*)data;
		size_t size_ip = ip_header->ihl<<2;
		struct tcphdr *tcp_header = (struct tcphdr*)((char *)ip_header+size_ip);
		int sock=address_find_sock(tcp_header->dest);
		if(-1 == sock)
		{
			logInfo(LOG_WARN,"sock is invalid in establishConnForClosedConn");
			outputPacketForDebug(LOG_WARN,CLIENT_FLAG,ip_header,tcp_header);
			return;
		}
		if(0 == fake_ip_addr)
		{
			client_ip_addr=ip_header->saddr;
		}else
		{
			logInfo(LOG_DEBUG,"erase fake_ip_addr\n");
			trueIPContainer.erase(get_ip_port_value(fake_ip_addr,
						tcp_header->source));
		}
		fake_ip_addr=getRandomIP();
		logInfo(LOG_NOTICE,"change ip address");
		uint64_t key=get_ip_port_value(fake_ip_addr,tcp_header->source);
		trueIPContainer[key]=client_ip_addr;

		ip_header->saddr=fake_ip_addr;
		int result=msg_copyer_send(sock,ip_header->saddr,
				tcp_header->source,CLIENT_ADD);
		if(-1 == result)
		{
			logInfo(LOG_ERR,"msg copyer send error");
			return;
		}
		send_ip_packet(isOutput,fake_ip_addr,data,
				virtual_next_sequence,&nextSeq);
		isSynIntercepted=true;
		totalSendPackets++;
		data=handshakePackets.back();
		ip_header=(struct iphdr *)data;
		ip_header->saddr=fake_ip_addr;
		unsend.push_back(copy_ip_packet(ip_header));
	}

}

/**
 * processing client packets
 */
void session_st::process_recv(struct iphdr *ip_header,
		struct tcphdr *tcp_header)
{
	bool isOutput=false;
	if(chosenOutput)
	{
		isOutput=true;
		outputPacketForDebug(LOG_DEBUG,CLIENT_FLAG,ip_header,tcp_header);
	}
	local_dest_ip_addr=ip_header->daddr;

	save_header_info(ip_header,tcp_header);
	if(fake_ip_addr!=0)
	{
		logInfo(LOG_INFO,"set fake ip addr for client");
		ip_header->saddr=fake_ip_addr;
		tcp_header->seq=htonl(nextSeq);
	}
	//processing the reset packet
	if(tcp_header->rst)
	{
		send_ip_packet(isOutput,fake_ip_addr,(unsigned char *) ip_header,
				virtual_next_sequence,&nextSeq);
		totalSendPackets++;
		reset_flag = true;
		return;
	}
	//processing the syn packet
	if(tcp_header->syn)
	{
		isSynIntercepted=true;
		unsigned char *data=copy_ip_packet(ip_header);
		handshakePackets.push_back(data);
		send_ip_packet(isOutput,fake_ip_addr,(unsigned char *)ip_header,
				virtual_next_sequence,&nextSeq);
		totalSendPackets++;
		return;
	}
	//processing the fin packet
	if(tcp_header->fin)
	{
		if(isFakedSendingFinToBackend)
		{
			return;
		}
		//client sends fin ,and the server acks it
		if(virtual_ack == tcp_header->seq)
		{
			if(isWaitBakendClosed)
			{
				unsend.push_back(copy_ip_packet(ip_header));
			}else
			{
				while(! unsend.empty())
				{
					unsigned char *data = unsend.front();
					free(data);
					unsend.pop_front();
				}
				send_ip_packet(isOutput,fake_ip_addr,(unsigned char *)ip_header,
						virtual_next_sequence,&nextSeq);
				totalSendPackets++;
				virtual_status |= CLIENT_FIN;
				confirmed=true;
			}
		}
		else
		{
			if(chosenOutput)
			{
				logInfo(LOG_DEBUG,"push back packet");
			}
			unsend.push_back(copy_ip_packet(ip_header));
		}
		isClientClosed=true;
		return;
	}
	//processing the other type of packet
	uint16_t tot_len = ntohs(ip_header->tot_len);
	uint32_t size_ip = ip_header->ihl<<2;
	uint32_t size_tcp = tcp_header->doff<<2;
	uint32_t contSize=tot_len-size_tcp-size_ip;

	if(contSize>0)
	{
		totalContentPackets++;
	}
	//data packet or the third packet
	if(virtual_status ==SYN_SEND)
	{
		if(!isSynIntercepted)
		{
			establishConnectionForNoSynPackets(ip_header,tcp_header);
			unsend.push_back(copy_ip_packet(ip_header));
			return;
		}
		if(!isHalfWayIntercepted&&1==handshakePackets.size())
		{
			unsigned char *data=copy_ip_packet(ip_header);
			handshakePackets.push_back(data);
		}
		//when client send multiple packet more quickly than the local network
		unsend.push_back(copy_ip_packet(ip_header));
		if(chosenOutput)
		{
			logInfo(LOG_DEBUG,"SYN_SEND push back the packet from client");
		}
	}
	else
	{
		if(contSize>0)
		{
			if(chosenOutput)
			{
				logInfo(LOG_DEBUG,"check it is a http request");
			}
			if(isTestConnClosed)
			{
				//if the connection to the backend is closed,then we 
				//reestablish the connection and 
				//we reserve all comming packets for later disposure
				initSessionForKeepalive();
				establishConnectionForClosedConn();
				unsend.push_back(copy_ip_packet(ip_header));
				return;
			}
			if(!isSynIntercepted)
			{
				establishConnectionForNoSynPackets(ip_header,tcp_header);
				unsend.push_back(copy_ip_packet(ip_header));
				return;
			}
			if(checkRetransmission(tcp_header,lastReqContSeq))
			{
				if(chosenOutput)
				{
					logInfo(LOG_DEBUG,"it is a retransmission from client");
				}
				return;
			}else
			{
				if(isWaitResponse)
				{
					if(!checkTcpSeg(tcp_header,lastReqContSeq,lastAck))
					{
						logInfo(LOG_DEBUG,"push back the packet");
						unsend.push_back(copy_ip_packet(ip_header));
						return;
					}
				}
				if(!isTrueWaitResponse)
				{
					if(checkPacketLost(tcp_header,nextSeq))
					{
						lostPackets.push_back(copy_ip_packet(ip_header));
						if(chosenOutput)
						{
							logInfo(LOG_DEBUG,"lost and need prev packet");
						}
						isWaitPreviousPacket=true;
						return;
					}
					if(isWaitPreviousPacket)
					{
						//we do not support session when  two packets are 
						//lost and retransmitted
						send_ip_packet(isOutput,fake_ip_addr,
								(unsigned char *)ip_header,
								virtual_next_sequence,&nextSeq);
						totalSendPackets++;
						sendReservedLostPackets();
						isWaitResponse=true;
						return;
					}
				}
				virtual_status=SEND_REQUEST;
				if(isWaitResponse&&checkTcpSeg(tcp_header,
							lastReqContSeq,lastAck))
				{
					isSegContinue=true;
					send_ip_packet(isOutput,fake_ip_addr,
							(unsigned char *)ip_header,
							virtual_next_sequence,&nextSeq);
					totalSendPackets++;
					if(chosenOutput)
					{
						logInfo(LOG_DEBUG,"it is a continuous http request");
					}
				}
				lastReqContSeq=ntohl(tcp_header->seq);
				lastAck=ntohl(tcp_header->ack_seq);
				if(isSegContinue)
				{
					isSegContinue=false;
					return;
				}else
				{
					requestProcessed++;
					if(requestProcessed>30)
					{
						isKeepalive=true;
					}
					if(chosenOutput)
					{
						logInfo(LOG_DEBUG,"a new request from client");
					}
				}
			}
		}else
		{
			if(1==handshakePackets.size())
			{
				unsigned char *data=copy_ip_packet(ip_header);
				handshakePackets.push_back(data);
			}
		}
		if(isWaitBakendClosed)
		{
			//record the asyn close,then wait for backend server response
			unsend.push_back(copy_ip_packet(ip_header));
			if(chosenOutput)
			{
				logInfo(LOG_DEBUG,"push back fin ack for server active close ");
			}
		}else
		{
			if(isWaitResponse)
			{
				unsend.push_back(copy_ip_packet(ip_header));
				if(chosenOutput)
				{
					logInfo(LOG_DEBUG,"wait backent server's response");
				}
			}else
			{
				if(isClientClosed)
				{
					unsend.push_back(copy_ip_packet(ip_header));
					if(chosenOutput)
					{
						logInfo(LOG_DEBUG,"save ack for server fin");
					}
				}else
				{
					if(SEND_REQUEST==virtual_status)
					{
						isWaitResponse=true;
					}
					send_ip_packet(isOutput,fake_ip_addr,
							(unsigned char *)ip_header,
							virtual_next_sequence,&nextSeq);
					totalSendPackets++;
				}
			}
		}
	}
}


static double getusec()
{
	struct timeval tp;
	gettimeofday(&tp,NULL);
	double sec=tp.tv_sec*1000000+(1.0*tp.tv_usec);
	return sec ;
}

static bool checkPacketPadding(struct iphdr *ip_header,
		struct tcphdr *tcp_header)
{
	uint32_t size_ip = ip_header->ihl<<2;
	uint32_t size_tcp = tcp_header->doff<<2;
	uint32_t packSize=ntohs(ip_header->tot_len);
	uint32_t contSize=packSize-size_tcp-size_ip;
	if(contSize>0)
	{
		return true;
	}
	return false;

}

/**
 * filter packets 
 */
bool isPacketNeeded(const char *packet)
{
	bool isNeeded=false;
	struct tcphdr *tcp_header;
	struct iphdr *ip_header;
	uint32_t size_ip;
	uint32_t size_tcp;

	ip_header = (struct iphdr*)packet;
	//check if it is a tcp packet
	if(ip_header->protocol != IPPROTO_TCP)
	{
		return isNeeded;
	}

	size_ip = ip_header->ihl<<2;
	uint32_t packSize=ntohs(ip_header->tot_len);
	if (size_ip < 20) {
		logInfo(LOG_WARN,"Invalid IP header length: %d", size_ip);
		return isNeeded;
	}
	tcp_header = (struct tcphdr*)((char *)ip_header+size_ip);
	size_tcp = tcp_header->doff<<2;
	if (size_tcp < 20) {
		logInfo(LOG_WARN,"Invalid TCP header length: %d bytes,packet len:%d",
				size_tcp,packSize);
		return isNeeded;
	}
	//here we filter the packets we do care about
	{

		//because it may use several virtual ip addresses 
		//if you use ETH_P_ALL,then the following is wrong,
		//try to check ip and port instead
		if(checkLocalIPValid(ip_header->daddr) && 
				(tcp_header->dest==local_port))
		{
			//if(ip_header->saddr==company_ip)
			{
				isNeeded=true;
			}
		}
		else if(checkLocalIPValid(ip_header->saddr) && 
				(tcp_header->source==local_port))
		{
			//this is only valid when using ETH_P_ALL
			//printf("source for local ip and local port\n");
			if(tcp_header->fin)
			{
				isNeeded=true;
			}
		}
	}
	return isNeeded;
}

/**
 * the main procedure for processing the filtered packets
 */
void process(char *packet)
{
	double t1=getusec();
	struct tcphdr *tcp_header;
	struct iphdr *ip_header;
	uint32_t size_ip;
	bool reusePort=false;
	timeCount++;
	static double timeAdded=0;

	if(timeCount%2000000==0)
	{
		//this is for checking memory leak
		logInfo(LOG_INFO,"activeCount:%d,total reqs:%d,rel reqs:%d,obs del:%d",
				activeCount,enterCount,leaveCount,deleteObsoCount);
		clearTimeoutTcpSessions();
	}

	ip_header = (struct iphdr*)packet;
	size_ip = ip_header->ihl<<2;
	tcp_header = (struct tcphdr*)((char *)ip_header+size_ip);

	if((ip_header->saddr==remote_ip) && (tcp_header->source==remote_port) )
	{
		//when the packet comes from the targeted test machine
		uint32_t clientIP=ip_header->daddr;
		uint64_t key=get_ip_port_value(ip_header->daddr,tcp_header->dest);
		{
			//try to find session through fake ip
			IPIterator iter2=trueIPContainer.find(key);
			if(iter2!= trueIPContainer.end())
			{
				clientIP=iter2->second;
			}
		}
		SessIterator iter = sessions.find(get_ip_port_value(clientIP,
					tcp_header->dest));
		if(iter != sessions.end())
		{
			iter->second.confirmed=false;
			iter->second.lastUpdateTime=time(0);
			iter->second.update_virtual_status(ip_header,tcp_header);
			if( iter->second.is_over())
			{
				if(!iter->second.isStatClosed)
				{
					iter->second.isStatClosed=true;
					activeCount--;
				}
				leaveCount++;
				sessions.erase(iter);
			}
		}else
		{
			//it may happen when the last packet comes from backend
			//logInfo("no session for this packet from test machine");
		}
	}
	else if(checkLocalIPValid(ip_header->daddr) && 
			(tcp_header->dest==local_port))
	{
		//when the packet comes from client
		totalPackets++;
		uint64_t value=get_ip_port_value(ip_header->saddr,tcp_header->source);
		if(tcp_header->syn)
		{
			totalSynPackets++;
			activeCount++;
			enterCount++;
			SessIterator iter = sessions.find(value);
			if(iter != sessions.end())
			{
				//check if it is a duplicate syn
				time_t now=time(0);
				int diff=now-iter->second.createTime;
				//if less than 30 seconds,then we consider it is a dup syn 
				if(diff < 30)
				{
					if(iter->second.is_over())
					{
						logInfo(LOG_INFO,"dup syn,ses over,time diff:%d",diff);
					}else
					{
						logInfo(LOG_WARN,"duplicate syn,time diff:%d",diff);
						outputPacketForDebug(LOG_WARN,CLIENT_FLAG,ip_header,
								tcp_header);
						return;
					}
				}
				deleteObsoCount++;	
				activeCount--;
				//reuse port number
				iter->second.initSession();
				reusePort=true;
				logInfo(LOG_WARN,"reuse port number,key :%llu",value);
			}
			int sock=address_find_sock(tcp_header->dest);
			if(-1 == sock)
			{
				logInfo(LOG_WARN,"sock is invalid in process");
				outputPacketForDebug(LOG_WARN,CLIENT_FLAG,ip_header,tcp_header);
				return;
			}
			int result=msg_copyer_send(sock,ip_header->saddr,
					tcp_header->source,CLIENT_ADD);
			if(-1 == result)
			{
				logInfo(LOG_ERR,"msg coper send error");
				return;
			}else
			{
				if(reusePort)
				{
					iter->second.process_recv(ip_header,tcp_header);
				}else
				{
					sessions[value].process_recv(ip_header,tcp_header);
				}
			}
		}
		else
		{
			SessIterator iter = sessions.find(value);
			if(iter != sessions.end())
			{
				if(ip_header->saddr==company_ip)
				{
					iter->second.chosenOutput=true;
				}
				iter->second.confirmed=false;
				iter->second.process_recv(ip_header,tcp_header);
				iter->second.lastUpdateTime=time(0);
				if( (iter->second.is_over()))
				{
					if(!iter->second.isStatClosed)
					{
						iter->second.isStatClosed=true;
						activeCount--;
					}

					leaveCount++;
					sessions.erase(iter);
				}
			}else
			{
				//we check if we can pad tcp handshake for this request
				if(checkPacketPadding(ip_header,tcp_header))
				{
					logInfo(LOG_WARN,"it tries to initiate conn for data packets");
					sessions[value].process_recv(ip_header,tcp_header);
				}else
				{
					invalidPacketsFromClient++;

					if(invalidPacketsFromClient>1000)
					{
						logInfo(LOG_INFO,"this is a lost packet with no syn");
						outputPacketForDebug(LOG_INFO,CLIENT_FLAG,
								ip_header,tcp_header);
					}
				}
			}
		}
	}
	else if(checkLocalIPValid(ip_header->saddr) && 
			(tcp_header->source==local_port))
	{
		//when the packet comes from local server 
		//this is only valid when using ETH_P_ALL
		if(tcp_header->fin)
		{
			logInfo(LOG_DEBUG,"server fin from local ip and local port");
			SessIterator iter = sessions.find(get_ip_port_value(
						ip_header->daddr,tcp_header->dest));
			if(iter != sessions.end())
			{
				if(!iter->second.isClientClosed)
				{
					//server sends fin to the client
					iter->second.isWaitBakendClosed=true;
				}
			}
		}
	}else
	{
		//we don't know where the packet comes from
		logInfo(LOG_WARN,"unknown packet");
		outputPacketForDebug(LOG_WARN,UNKNOWN_FLAG,ip_header,tcp_header);
	}
	double t2=getusec();
	double usec=t2-t1;
	timeAdded+=usec;
	if(usec>500)
	{
		logInfo(LOG_INFO,"proc spent %f usec,avg spent:%f",usec,
				timeAdded/timeCount);
	}
}

