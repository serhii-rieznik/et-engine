#ifndef HEADER_CURL_NON_ASCII_H
#define HEADER_CURL_NON_ASCII_H

#include "curl_setup.h"

#ifdef CURL_DOES_CONVERSIONS

#include "urldata.h"

/*
 * Curl_convert_clone() returns a malloced copy of the source string (if
 * returning CURLE_OK), with the data converted to network format.
 *
 * If no conversion was needed *outbuf may be NULL.
 */
CURLcode Curl_convert_clone(struct SessionHandle *data,
                            const char *indata,
                            size_t insize,
                            char **outbuf);

void Curl_convert_init(struct SessionHandle *data);
void Curl_convert_setup(struct SessionHandle *data);
void Curl_convert_close(struct SessionHandle *data);

CURLcode Curl_convert_to_network(struct SessionHandle *data,
                                 char *buffer, size_t length);
CURLcode Curl_convert_from_network(struct SessionHandle *data,
                                 char *buffer, size_t length);
CURLcode Curl_convert_from_utf8(struct SessionHandle *data,
                                 char *buffer, size_t length);
CURLcode Curl_convert_form(struct SessionHandle *data, struct FormData *form);
#else
#define Curl_convert_clone(a,b,c,d) ((void)a, CURLE_OK)
#define Curl_convert_init(x) Curl_nop_stmt
#define Curl_convert_setup(x) Curl_nop_stmt
#define Curl_convert_close(x) Curl_nop_stmt
#define Curl_convert_to_network(a,b,c) ((void)a, CURLE_OK)
#define Curl_convert_from_network(a,b,c) ((void)a, CURLE_OK)
#define Curl_convert_from_utf8(a,b,c) ((void)a, CURLE_OK)
#define Curl_convert_form(a,b) CURLE_OK
#endif

#endif /* HEADER_CURL_NON_ASCII_H */
