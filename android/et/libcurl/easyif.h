#ifndef HEADER_CURL_EASYIF_H
#define HEADER_CURL_EASYIF_H


/*
 * Prototypes for library-wide functions provided by easy.c
 */
void Curl_easy_addmulti(struct SessionHandle *data, void *multi);

void Curl_easy_initHandleData(struct SessionHandle *data);

#endif /* HEADER_CURL_EASYIF_H */

