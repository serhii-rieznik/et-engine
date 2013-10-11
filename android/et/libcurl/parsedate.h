#ifndef HEADER_CURL_PARSEDATE_H
#define HEADER_CURL_PARSEDATE_H


extern const char * const Curl_wkday[7];
extern const char * const Curl_month[12];

CURLcode Curl_gmtime(time_t intime, struct tm *store);

#endif /* HEADER_CURL_PARSEDATE_H */

