#ifndef HEADER_CURL_HTTP_DIGEST_H
#define HEADER_CURL_HTTP_DIGEST_H

#include "curl_setup.h"

typedef enum {
  CURLDIGEST_NONE, /* not a digest */
  CURLDIGEST_BAD,  /* a digest, but one we don't like */
  CURLDIGEST_BADALGO, /* unsupported algorithm requested */
  CURLDIGEST_NOMEM,
  CURLDIGEST_FINE, /* a digest we act on */

  CURLDIGEST_LAST  /* last entry in this enum, don't use */
} CURLdigest;

enum {
  CURLDIGESTALGO_MD5,
  CURLDIGESTALGO_MD5SESS
};

/* this is for digest header input */
CURLdigest Curl_input_digest(struct connectdata *conn,
                             bool proxy, const char *header);

/* this is for creating digest header output */
CURLcode Curl_output_digest(struct connectdata *conn,
                            bool proxy,
                            const unsigned char *request,
                            const unsigned char *uripath);

#if !defined(CURL_DISABLE_HTTP) && !defined(CURL_DISABLE_CRYPTO_AUTH)
void Curl_digest_cleanup(struct SessionHandle *data);
#else
#define Curl_digest_cleanup(x) Curl_nop_stmt
#endif

#endif /* HEADER_CURL_HTTP_DIGEST_H */
