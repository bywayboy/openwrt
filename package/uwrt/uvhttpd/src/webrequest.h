#ifndef _UVHTTPD_WEBREQUEST_H
#define _UVHTTPD_WEBREQUEST_H


int webrequest_push(webrequest_t * request, webconn_t * conn, wsFrame * frame);
#endif