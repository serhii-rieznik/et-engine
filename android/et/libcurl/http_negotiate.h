#ifndef HEADER_CURL_HTTP_NEGOTIATE_H
#define HEADER_CURL_HTTP_NEGOTIATE_H


#ifdef USE_HTTP_NEGOTIATE

/* this is for Negotiate header input */
int Curl_input_negotiate(struct connectdata *conn, bool proxy,
                         const char *header);

/* this is for creating Negotiate header output */
CURLcode Curl_output_negotiate(struct connectdata *conn, bool proxy);

void Curl_cleanup_negotiate(struct SessionHandle *data);

#ifdef USE_WINDOWS_SSPI
#define GSS_ERROR(status) (status & 0x80000000)
#endif

#endif /* USE_HTTP_NEGOTIATE */

#endif /* HEADER_CURL_HTTP_NEGOTIATE_H */
