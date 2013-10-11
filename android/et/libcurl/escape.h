#ifndef HEADER_CURL_ESCAPE_H
#define HEADER_CURL_ESCAPE_H

/* Escape and unescape URL encoding in strings. The functions return a new
 * allocated string or NULL if an error occurred.  */

CURLcode Curl_urldecode(struct SessionHandle *data,
                        const char *string, size_t length,
                        char **ostring, size_t *olen,
                        bool reject_crlf);

#endif /* HEADER_CURL_ESCAPE_H */

