#ifndef HEADER_CURL_NTLM_WB_H
#define HEADER_CURL_NTLM_WB_H


#include "curl_setup.h"

#if defined(USE_NTLM) && defined(NTLM_WB_ENABLED)

/* this is for creating ntlm header output by delegating challenge/response
   to Samba's winbind daemon helper ntlm_auth */
CURLcode Curl_output_ntlm_wb(struct connectdata *conn, bool proxy);

void Curl_ntlm_wb_cleanup(struct connectdata *conn);

#endif /* USE_NTLM && NTLM_WB_ENABLED */

#endif /* HEADER_CURL_NTLM_WB_H */
