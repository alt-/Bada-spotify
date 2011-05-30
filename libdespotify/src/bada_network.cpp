#ifdef BADA

#include <stdlib.h>
#include <unistd.h>

#include "network.h"
 
#include <FNetSockSocket.h>
#include <FNetSockSocketUtility.h>
#include <FNetDNS.h>
#include <FNetIDnsEventListener.h>
#include <FNetIPHostEntry.h>
#include <FNetIP4Address.h>
#include <FBaseRtThreadThread.h>

#include <map>

extern "C" {
#include "dns.h"
}

using namespace Osp::Base;
using namespace Osp::Net;
using namespace Osp::Net::Sockets;

Sockets::SocketUtility socketutility;
hostent *ent;
std::map<int, Socket*> sockets;

void free_hostent(hostent *trees) {
	if(trees->h_addr_list == 0) return;
	int i = 0;
	char* addr;
	do {
		if(i!=0) delete addr;
		addr = trees->h_addr_list[i++];
	} while(addr != 0);
	delete trees->h_addr_list;
	trees->h_addr_list = 0;
}

class DnsThread : public virtual Osp::Base::Runtime::Thread
{
private :
	String host;

	class DnsListener :	public Object, public virtual IDnsEventListener
	{
	public:
		Runtime::Monitor monitor;

		IpHostEntry* asyncDnsResult;

		DnsListener(void){
			monitor.Construct();
			asyncDnsResult = 0;
		}

		void OnDnsResolutionCompletedN(IpHostEntry* ipHostEntry, result r)
		{
			AppLog("Async DNS result received by listener");
			asyncDnsResult = ipHostEntry;
			monitor.Notify();
		}
	}dnsListener;

	Dns::Dns dnsutility;
public:
	DnsThread(const char *host) {
		this->host.Append(host);
	}

	IpHostEntry* result() {
		return dnsListener.asyncDnsResult;
	}

	Runtime::Monitor *monitor() {
		return &dnsListener.monitor;
	}

	bool OnStart(void) {
		dnsutility.Construct(dnsListener);
		dnsutility.GetHostByName(host);
		return true;
	}
	void OnStop(void) {
	}
};

int network_init (void)
{
	socketutility.Construct();
	AppLog("Network utilities constructed");
	ent = new hostent();
	ent->h_addr_list = 0;
	return 0;
}
int network_cleanup (void)
{
	free_hostent(ent);
	delete ent;
	return 0;
}
int network_connect(char *host, int port) {
	unsigned long ip = dns_resolve_name(host);
	if(ip == 0) return -1;

	Socket *socket = new Socket();
	result r = socket->Construct(NET_SOCKET_AF_IPV4, NET_SOCKET_TYPE_STREAM, NET_SOCKET_PROTOCOL_TCP);
	if(IsFailed(r)) {
		AppLog("Socket construct failed with %s", GetErrorMessage(r));
		return -1;
	}

	Ip4Address addr(ip);
	NetEndPoint endpoint(addr, port);

	unsigned long value = 0;
	r = socket->Ioctl(NET_SOCKET_FIONBIO, value);
	if(IsFailed(r)) {
		AppLog("Socket ioctl failed with %s", GetErrorMessage(r));
		return -1;
	}

	r = socket->Connect(endpoint);
	if(IsFailed(r)) {
		AppLog("Socket connect failed with %s", GetErrorMessage(r));
		return -1;
	}

	sockets[socket->GetSocketFd()] = socket;

	return socket->GetSocketFd();
}

int sock_close(int fd) {
	Socket *socket = sockets[fd];

	result r = socket->Close();
	if(IsFailed(r)) {
		AppLog("Socket close failed with %s", GetErrorMessage(r));
		return -1;
	}

	return 0;
}
int send(int s, void *buf, int len, int flags)
{
	Socket *socket = sockets[s];
	int sent;
	if(IsFailed(socket->Send(buf, len, sent))) return -1;

	return sent;
}
int recv(int s, void *buf, int len, int flags) {
	Socket *socket = sockets[s];
	int received;
	if(IsFailed(socket->Receive(buf, len, received))) return -1;

	return received;
}
unsigned short ntohs(unsigned short netshort)
{
	return socketutility.NtoHS(netshort);
}
unsigned short htons(unsigned short netshort) {
	return socketutility.HtoNS(netshort);
}
unsigned int ntohl(unsigned int netlong) {
	return socketutility.NtoHL(netlong);
}
unsigned int htonl(unsigned int netlong) {
	return socketutility.HtoNL(netlong);
}
in_addr_t inet_addr(const char *cp) {
	String string(cp);
	Ip4Address ip(string);
	if(IsFailed(GetLastResult())) return INADDR_NONE;

	in_addr_t addr;
	ByteBuffer buf;
	buf.Construct(4);
	ip.GetAddress(buf);
	buf.Flip();
	buf.GetLong((long int &)addr);

	return htonl(addr);
}
struct hostent *gethostbyname(const char *name) {
	DnsThread *thread = new DnsThread(name);
	thread->Construct(Runtime::THREAD_TYPE_EVENT_DRIVEN);
	thread->Start();

	AppLog("Waiting for async gethostbyname");
	Runtime::Monitor *monitor = thread->monitor();
	monitor->Enter();
	monitor->Wait();
	monitor->Exit();

	IpHostEntry* result = thread->result();

	thread->Stop();
	delete thread;

	free_hostent(ent);
	ent->h_length = 4;
	// TODO: All records and fields
	ent->h_addr_list = new char*[2];
	ent->h_addr_list[0] = new char[4];
	ent->h_addr_list[1] = 0;

	Osp::Base::Collection::IList* addressList = result->GetAddressList();
	if (addressList != null)
	{
		int count = addressList->GetCount();

		for (int i=0; i < count; i++)
		{
			IpAddress* pIpAddress = (IpAddress*)(addressList->GetAt(i));
			ByteBuffer buf;
			buf.Construct(4);
			pIpAddress->GetAddress(buf);
			buf.Flip();
			long addr;
			buf.GetLong(addr);
			addr = htonl(addr);
			buf.SetLong(0, addr);
			buf.Flip();
			buf.GetArray((byte*)ent->h_addr_list[0], 0, 4);
			break;
		}
	}

	delete result;

	return ent;
}

#endif
