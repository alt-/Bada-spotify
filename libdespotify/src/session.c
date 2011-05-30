/*
 * $Id: session.c 508 2010-04-29 19:08:47Z dstien $
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>

#include "network.h"

#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include "dns.h"
#include "keyexchange.h"
#include "session.h"
#include "packet.h"
#include "util.h"

#ifndef BADA

static unsigned char DH_generator[1] = { 2 };

static unsigned char DH_prime[] = {
	/* Well-known Group 1, 768-bit prime */
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9,
	0x0f, 0xda, 0xa2, 0x21, 0x68, 0xc2, 0x34, 0xc4, 0xc6,
	0x62, 0x8b, 0x80, 0xdc, 0x1c, 0xd1, 0x29, 0x02, 0x4e,
	0x08, 0x8a, 0x67, 0xcc, 0x74, 0x02, 0x0b, 0xbe, 0xa6,
	0x3b, 0x13, 0x9b, 0x22, 0x51, 0x4a, 0x08, 0x79, 0x8e,
	0x34, 0x04, 0xdd, 0xef, 0x95, 0x19, 0xb3, 0xcd, 0x3a,
	0x43, 0x1b, 0x30, 0x2b, 0x0a, 0x6d, 0xf2, 0x5f, 0x14,
	0x37, 0x4f, 0xe1, 0x35, 0x6d, 0x6d, 0x51, 0xc2, 0x45,
	0xe4, 0x85, 0xb5, 0x76, 0x62, 0x5e, 0x7e, 0xc6, 0xf4,
	0x4c, 0x42, 0xe9, 0xa6, 0x3a, 0x36, 0x20, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

SESSION *session_init_client (void)
{
	SESSION *session;

	if ((session = (SESSION *) calloc (1, sizeof (SESSION))) == NULL)
		return NULL;

	session->client_OS = 0x00;	/* 0x00 == Windows, 0x01 == Mac OS X */
	memcpy(session->client_id, "\x01\x04\x01\x01", 4);
	session->client_revision = 99999;
	
	/*
	 * Client and server generate 16 random bytes each.
	 */
	RAND_bytes (session->client_random_16, 16);

	if ((session->rsa =
	     RSA_generate_key (1024, 65537, NULL, NULL)) == NULL) {
		DSFYDEBUG ("RSA key generation failed with error %lu\n",
			   ERR_get_error ());
	}
	assert (session->rsa != NULL);

	/*
	 * Create a private and public key.
	 * This, along with key signing, is used to securely
	 * agree on a session key for the Shannon stream cipher.
	 *
	 */
	session->dh = DH_new ();
	session->dh->p = BN_bin2bn (DH_prime, 96, NULL);
	session->dh->g = BN_bin2bn (DH_generator, 1, NULL);
	assert (DH_generate_key (session->dh) == 1);

	BN_bn2bin (session->dh->priv_key, session->my_priv_key);
	BN_bn2bin (session->dh->pub_key, session->my_pub_key);

	/*
	 * Found in Storage.dat (cache) at offset 16.
	 * Automatically generated, but we're lazy.
	 *
	 */
	memcpy (session->cache_hash,
		"\xf4\xc2\xaa\x05\xe8\x25\xa7\xb5\xe4\xe6\x59\x0f\x3d\xd0\xbe\x0a\xef\x20\x51\x95",
		20);
	session->cache_hash[0] = (unsigned char) getpid ();

	session->ap_sock = -1;
	session->username[0] = 0;
	session->server_host[0] = 0;
	session->server_port = 0;

	session->key_recv_IV = 0;
	session->key_send_IV = 0;

	session->user_info.username[0] = 0;
	session->user_info.country[0] = 0;
	session->user_info.server_host[0] = 0;
	session->user_info.server_port = 0;

	pthread_mutex_init(&session->login_mutex, NULL);
	pthread_cond_init(&session->login_cond, NULL);

	return session;
}
#endif

void session_auth_set (SESSION * session, const char *username, const char *password)
{
	DSFYstrncpy (session->user_info.username, username,
		     sizeof session->user_info.username);
	DSFYstrncpy (session->username, username, sizeof session->username);
	session->username[sizeof (session->username) - 1] = 0;
	session->username_len = strlen (session->username);

	DSFYstrncpy (session->password, password, sizeof session->password);
	session->password[sizeof (session->password) - 1] = 0;
}


int session_connect (SESSION * session)
{
	struct srv_list service_list;

	/* Lookup service hosts in DNS */
    service_list = dns_srv_list ("_spotify-client._tcp.spotify.com");
	if (service_list.len == 0) {
            DSFYDEBUG ("Service lookup failed. falling back to ap.spotify.com\n");
            struct srv fallback;
            fallback.host = strdup("ap.spotify.com");
            fallback.port = 4070;
            service_list.len = 1;
            service_list.arr = (struct srv*)malloc(sizeof(struct srv));
            service_list.arr[0] = fallback;
        }

	unsigned int i;
	for (i = 0; i < service_list.len; i++) {
		struct srv service = service_list.arr[i];
		DSFYDEBUG ("Connecting to %s:%d\n", service.host, service.port);

		session->ap_sock = network_connect(service.host, service.port);

		if (session->ap_sock != -1) {
			/*
			 * Save for later use in ConnectionInfo message
			 * (too lazy to do getpeername() later ;)
			 */
			DSFYstrncpy (session->server_host, service.host, sizeof session->server_host);
			session->server_port = service.port;

			DSFYstrncpy (session->user_info.server_host, service.host,
				     sizeof session->user_info.server_host);
			session->user_info.server_port = service.port;

			break;
		}
	}

	service_list.len = 0;
	for (i = 0; i < service_list.len; i++) {
		free(service_list.arr[i].host);
	}
	free(service_list.arr);

	if (session->ap_sock == -1)
		return -1;

	return 0;
}

void session_disconnect (SESSION * session)
{
	if (session->ap_sock != -1) {
		sock_close (session->ap_sock);
		session->ap_sock = -1;
	}

	session->key_recv_IV = 0;
	session->key_send_IV = 0;

	session->user_info.username[0] = 0;
	session->user_info.country[0] = 0;
	session->user_info.server_host[0] = 0;
	session->user_info.server_port = 0;
}

void session_free (SESSION * session)
{
	session_disconnect (session);

	if (session->dh)
		DH_free (session->dh);

//	if (session->rsa)
//		RSA_free (session->rsa);

	pthread_cond_destroy(&session->login_cond);
	pthread_mutex_destroy(&session->login_mutex);

	free (session);
}
