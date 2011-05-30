#ifndef BADA_NETWORK_H
#define BADA_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

int network_init (void);
int network_connect(char *host, int port);
int network_cleanup (void);

#ifndef _IN_ADDR_T_DEFINED
#define _IN_ADDR_T_DEFINED
typedef unsigned long in_addr_t;
#endif

#ifndef INADDR_NONE
# define INADDR_NONE	0xffffffff
#endif /* INADDR_NONE */

struct  hostent {
     char    *h_name;
     char    **h_aliases;
     int     h_addrtype;
     int     h_length;
     char    **h_addr_list;
};

#define h_addr  h_addr_list[0]

struct addrinfo {
	int     ai_flags;
	int     ai_family;
	int	    ai_socktype;
	int     ai_protocol;
	size_t  ai_addrlen;
	struct	sockaddr *ai_addr;
	char    *ai_canonname;
	struct  addrinfo *ai_next;
};

#ifdef BADA
int sock_close(int fd);
int send(int s, void *buf, int len, int flags);
int recv(int s, void *buf, int len, int flags);
unsigned short ntohs(unsigned short netshort);
unsigned short htons(unsigned short netshort);
unsigned int ntohl(unsigned int netlong);
unsigned int htonl(unsigned int netlong);
in_addr_t inet_addr(const char *cp);
struct hostent *gethostbyname(const char *name);
#endif

#ifdef __cplusplus
}
#endif

#endif
