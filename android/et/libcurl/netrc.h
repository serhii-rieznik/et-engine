#ifndef HEADER_CURL_NETRC_H
#define HEADER_CURL_NETRC_H


/* Make sure we have room for at least this size: */
#define LOGINSIZE 64
#define PASSWORDSIZE 64

/* returns -1 on failure, 0 if the host is found, 1 is the host isn't found */
int Curl_parsenetrc(const char *host,
                    char *login,
                    char *password,
                    char *filename);
  /* Assume: password[0]=0, host[0] != 0.
   * If login[0] = 0, search for login and password within a machine section
   * in the netrc.
   * If login[0] != 0, search for password within machine and login.
   */

#endif /* HEADER_CURL_NETRC_H */
