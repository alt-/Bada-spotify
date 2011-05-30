#ifdef BADA

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>

#include "network.h"

#include <openssl/rand.h>
#include <openssl/err.h>

extern "C" {
#include "dns.h"
#include "keyexchange.h"
#include "session.h"
#include "packet.h"
#include "util.h"
}

#include <FSecAesSecureRandom.h>
#include <FSecKeyPairGenerator.h>
#include <FBaseUtilStringUtil.h>

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
	byte cache_rand;
	Osp::Security::AesSecureRandom prng;
	Osp::Base::ByteBuffer* bytes = prng.GenerateRandomBytesN(17);
	result ret = bytes->GetArray(session->client_random_16, 0, 16);
	AppAssert(ret == E_SUCCESS);
	bytes->GetByte(cache_rand);
	delete bytes;

	Osp::Security::KeyPairGenerator gen;
	result r = gen.Construct(1024);
	if(IsFailed(r)) {
		AppLog("KeyPairGenerator Construct failed with %s", GetErrorMessage(r));
		return NULL;
	}
	Osp::Security::KeyPair *pair = gen.GenerateKeyPairN();

	// 29B Asn1 header, 5B footer
	Osp::Base::ByteBuffer *asn1 = pair->GetPublicKey()->GetEncodedN();
	asn1->SetPosition(29);
	r = asn1->GetArray(session->rsa_pub_exp, 0, sizeof(session->rsa_pub_exp));
	if(IsFailed(r)) {
		AppLog("PublicKey GetArray failed with %s", GetErrorMessage(r));
		return NULL;
	}
	delete pair;


	/*
	 * Create a private and public key.
	 * This, along with key signing, is used to securely
	 * agree on a session key for the Shannon stream cipher.
	 *
	 */
	session->dh = DH_new ();
	session->dh->p = BN_bin2bn (DH_prime, 96, NULL);
	session->dh->g = BN_bin2bn (DH_generator, 1, NULL);
	AppAssert (DH_generate_key (session->dh) == 1);

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
#ifndef BADA // getpid() crashes bada target
	session->cache_hash[0] = (unsigned char) getpid ();
#else
	session->cache_hash[0] = (unsigned char) cache_rand;
#endif

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
