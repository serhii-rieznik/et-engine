

#include "curl_setup.h"

#include "curl_memrchr.h"

#define _MPRINTF_REPLACE /* use our functions only */
#include <libcurl/mprintf.h>

#include "curl_memory.h"
/* The last #include file should be: */
#include "memdebug.h"

#ifndef HAVE_MEMRCHR

/*
 * Curl_memrchr()
 *
 * Our memrchr() function clone for systems which lack this function. The
 * memrchr() function is like the memchr() function, except that it searches
 * backwards from the end of the n bytes pointed to by s instead of forward
 * from the beginning.
 */

void *
Curl_memrchr(const void *s, int c, size_t n)
{
  const unsigned char *p = s;
  const unsigned char *q = s;

  p += n - 1;

  while(p >= q) {
    if(*p == (unsigned char)c)
      return (void *)p;
    p--;
  }

  return NULL;
}

#endif /* HAVE_MEMRCHR */
