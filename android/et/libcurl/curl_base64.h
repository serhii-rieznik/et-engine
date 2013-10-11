#ifndef HEADER_CURL_BASE64_H
#define HEADER_CURL_BASE64_H


CURLcode Curl_base64_encode(struct SessionHandle *data,
                            const char *inputbuff, size_t insize,
                            char **outptr, size_t *outlen);

CURLcode Curl_base64_decode(const char *src,
                            unsigned char **outptr, size_t *outlen);

#endif /* HEADER_CURL_BASE64_H */
