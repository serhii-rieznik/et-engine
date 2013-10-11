#ifndef HEADER_CURL_NTLM_H
#define HEADER_CURL_NTLM_H


#include "curl_setup.h"

#ifdef USE_NTLM

/* this is for ntlm header input */
CURLcode Curl_input_ntlm(struct connectdata *conn, bool proxy,
                         const char *header);

/* this is for creating ntlm header output */
CURLcode Curl_output_ntlm(struct connectdata *conn, bool proxy);

void Curl_http_ntlm_cleanup(struct connectdata *conn);

#else

#define Curl_http_ntlm_cleanup(a) Curl_nop_stmt

#endif

#endif /* HEADER_CURL_NTLM_H */
