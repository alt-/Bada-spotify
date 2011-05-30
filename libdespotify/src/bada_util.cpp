#ifdef BADA

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "network.h"

#include <stdarg.h>
#include <stdlib.h>
#include "bada_util.h"
#include "util.h"

#include <FBaseInteger.h>
#include <FBaseRtThread.h>

unsigned char *hex_ascii_to_bytes (char *ascii, unsigned char *bytes, int len)
{
	int i;
	int value;
	Osp::Base::String hex("0x00");

	for (i = 0; i < len; i++) {
		hex.SetCharAt(*(ascii + 2*i), 2);
		hex.SetCharAt(*(ascii + 2*i + 1), 3);
		if (Osp::Base::Integer::Decode(hex, value) != E_SUCCESS) {
			return NULL;
		}

		bytes[i] = value & 0xff;
	}

	return bytes;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
	Osp::Base::Runtime::Thread::Sleep(rqtp->tv_sec * 1000 + rqtp->tv_nsec/1000/1000);
	return 0;
}

#endif
