#ifndef BADA
/*
 * $Id: network.c 399 2009-07-29 11:50:46Z noah-w $
 *
 * Cross platform networking for despotify
 *
 */
 
#include <stdlib.h>
#include <unistd.h>

#include "network.h"
 
int network_init (void)
{
	#ifdef __use_winsock__
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
		fprintf (stderr, "Winsock failed. \n");
		return -1;
	}
	#endif
	return 0;
}
int network_cleanup (void)
{
	#ifdef __use_winsock__
	return WSACleanup();
	#endif
	return 0;
}
int network_connect(char *host, int port) {
	int sp_sock = -1;
	struct addrinfo h, *airoot, *ai;

	memset(&h, 0, sizeof(h));
	h.ai_family = PF_UNSPEC;
	h.ai_socktype = SOCK_STREAM;
	h.ai_protocol = IPPROTO_TCP;
	if (getaddrinfo (host, port, &h, &airoot)) {
		DSFYDEBUG ("getaddrinfo(%s,%s) failed with error %d\n",
				host, port, errno);
		continue;
	}

	for(ai = airoot; ai; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET
			&& ai->ai_family != AF_INET6)
			continue;

		ap_sock = socket (ai->ai_family,
				ai->ai_socktype, ai->ai_protocol);
		if (ap_sock < 0)
			continue;

		if (connect (ap_sock,
			(struct sockaddr *) ai->ai_addr,
			ai->ai_addrlen) != -1)
			break;

		sock_close (ap_sock);
		ap_sock = -1;
	}

	freeaddrinfo (airoot);
	return ap_sock;
}
#endif
