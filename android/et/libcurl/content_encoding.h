#ifndef HEADER_CURL_CONTENT_ENCODING_H
#define HEADER_CURL_CONTENT_ENCODING_H

#include "curl_setup.h"

/*
 * Comma-separated list all supported Content-Encodings ('identity' is implied)
 */
#ifdef HAVE_LIBZ
#define ALL_CONTENT_ENCODINGS "deflate, gzip"
/* force a cleanup */
void Curl_unencode_cleanup(struct connectdata *conn);
#else
#define ALL_CONTENT_ENCODINGS "identity"
#define Curl_unencode_cleanup(x) Curl_nop_stmt
#endif

CURLcode Curl_unencode_deflate_write(struct connectdata *conn,
                                     struct SingleRequest *req,
                                     ssize_t nread);

CURLcode
Curl_unencode_gzip_write(struct connectdata *conn,
                         struct SingleRequest *k,
                         ssize_t nread);


#endif /* HEADER_CURL_CONTENT_ENCODING_H */
