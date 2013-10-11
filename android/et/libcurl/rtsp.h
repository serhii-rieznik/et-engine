#ifndef HEADER_CURL_RTSP_H
#define HEADER_CURL_RTSP_H

#ifndef CURL_DISABLE_RTSP

extern const struct Curl_handler Curl_handler_rtsp;

bool Curl_rtsp_connisdead(struct connectdata *check);
CURLcode Curl_rtsp_parseheader(struct connectdata *conn, char *header);

#else
/* disabled */
#define Curl_rtsp_parseheader(x,y) CURLE_NOT_BUILT_IN
#define Curl_rtsp_connisdead(x) TRUE

#endif /* CURL_DISABLE_RTSP */

/*
 * RTSP Connection data
 *
 * Currently, only used for tracking incomplete RTP data reads
 */
struct rtsp_conn {
  char *rtp_buf;
  ssize_t rtp_bufsize;
  int rtp_channel;
};

/****************************************************************************
 * RTSP unique setup
 ***************************************************************************/
struct RTSP {
  /*
   * http_wrapper MUST be the first element of this structure for the wrap
   * logic to work. In this way, we get a cheap polymorphism because
   * &(data->state.proto.rtsp) == &(data->state.proto.http) per the C spec
   *
   * HTTP functions can safely treat this as an HTTP struct, but RTSP aware
   * functions can also index into the later elements.
   */
  struct HTTP http_wrapper; /*wrap HTTP to do the heavy lifting */

  long CSeq_sent; /* CSeq of this request */
  long CSeq_recv; /* CSeq received */
};


#endif /* HEADER_CURL_RTSP_H */

