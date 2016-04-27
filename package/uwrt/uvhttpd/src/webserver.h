#ifndef _UVHTTPD_WEBSERVER_H
#define _UVHTTPD_WEBSERVER_H
#include "http_parser.h"
#include "automem.h"
#include "rbtree.h"

#include "websocket.h"
typedef struct webserver webserver_t;
typedef struct webconn   webconn_t;

enum {
	WEB_PROTO_HTTP			= (1<<0),
	WEB_PROTO_HANDESHAKE	= (1 << 1),
	WEB_PROTO_WEBSOCKET		= ((1 << 2)),
};

typedef struct webrequest webrequest_t;
typedef struct webheader webheader_t;
typedef struct webqueuework webqueuework_t;

struct webqueuework {
	uv_work_t work;
	int status : 12;			//200 400 500??
	int flags : 6;
	rbroot_t headers;	// response headers
	automem_t mem;		// response body
	wsFrame * frame;
	webrequest_t * request; //当前请求上下文环境
	webconn_t * conn;
};

struct webheader {
	char * key, * val;
	rbnode_t n;
};

struct webrequest {
	time_t mt_mtime;					/* file timestamp */
	int _header_value_start, ref,is_cgi;
	automem_t buf;
	char * _url, * params;	//_url完整的URL String. params 参数起始位置指针
	const char *  mime;
	uint32_t nurl, nparams; //nurl 除掉参数后的长度 nparams 参数长度
	size_t st_size;

	char * file;	//文件路径,针对 脚本 它是模块名称.
	unsigned int method : 8;
	unsigned int upgrade : 1;
	rbroot_t headers;
	char * body;	// body 如果有值，则指向 buf.
};



struct webconn{
	uv_tcp_t conn;
	webserver_t * server;
	uv_loop_t * loop;
	int ref;
	unsigned int proto : 4;
	union {
		http_parser parser;
		wsparser_t ws_parser;
	};
	uv_shutdown_t shutdown;
	webrequest_t * request; //当前请求上下文环境
	int flags :6; // 来自 http parser 的 flags
};

struct webserver {
	uv_tcp_t s;
	uv_loop_t * loop;
	http_parser_settings parser_settings;
	uv_timer_t timer;
};

webconn_t * webconn_get(webconn_t * c);
webconn_t * webconn_put(webconn_t * c);
webrequest_t * webrequest_get(webrequest_t * r);
webrequest_t * webrequest_put(webrequest_t * r);
int webconn_sendmem(webconn_t * conn, automem_t * mem);

void webheader_free(webheader_t * header);

int webserver_init(uv_loop_t * loop, webserver_t * server);
void webserver_uninit(webserver_t * server);


webqueuework_t * webqueuework_create(void);

void webqueuework_free(webqueuework_t *);

int header_compare(rbnode_t * a, rbnode_t * b);
void webheader_node_free(rbnode_t * n);
const char * webheader_get(rbroot_t * root, const char * key);

void webheader_set(rbroot_t * root, const char * key, const char * val);
#endif