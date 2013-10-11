#ifndef HEADER_CURL_RAWSTR_H
#define HEADER_CURL_RAWSTR_H


#include <libcurl/curl.h>

/*
 * Curl_raw_equal() is for doing "raw" case insensitive strings. This is meant
 * to be locale independent and only compare strings we know are safe for
 * this.
 *
 * The function is capable of comparing a-z case insensitively even for
 * non-ascii.
 */
int Curl_raw_equal(const char *first, const char *second);
int Curl_raw_nequal(const char *first, const char *second, size_t max);

char Curl_raw_toupper(char in);

/* checkprefix() is a shorter version of the above, used when the first
   argument is zero-byte terminated */
#define checkprefix(a,b)    Curl_raw_nequal(a,b,strlen(a))

void Curl_strntoupper(char *dest, const char *src, size_t n);

#endif /* HEADER_CURL_RAWSTR_H */

