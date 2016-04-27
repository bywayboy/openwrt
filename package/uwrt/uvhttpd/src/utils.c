#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <uv.h>
#include <ctype.h>
#include <time.h>
#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#define ltoa(a, b, c)  _ltoa(a, b, c)
#define open( a , b )		_open( a , b )
#define close( x )		_close( x )
#define read( a , b , c ) _read( a , b , c )
#define lseek( a , b , c )  _lseek( a , b , c )
#define mkgmtime( x ) _mkgmtime( x )
#define ltoa(a, b, c) _ltoa(a, b, c)
#else
#define O_BINARY	0
#include <unistd.h>
#endif

#include "automem.h"
#include "cfg.h"
#include "webserver.h"
#include "utils.h"

#include <uci.h>


char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

#if defined(_MSC_VER)
#define S_ISDIR(m) (((m) & 0170000) == (0040000))  
#endif

int mstrcmp(char * a, char * b) {
	char c;
	int r=0;
	while (c = *a++) {
		if (r = c - *b++)
			break;
	}
	return r;
}

char * analize_url(webrequest_t * request, uint32_t * code) {
	struct stat st;
	const config_t * cfg = config_get();
	uint32_t i=0, l = request->nurl;
	char * buf, *p,*bp,* ext = NULL;
	p = buf = malloc(request->nurl + cfg->lwwwroot + 12);

	memcpy(buf, cfg->wwwroot, cfg->lwwwroot);
	p += cfg->lwwwroot;
	bp = p;
	while (i < l) {
		unsigned char c;
		switch (c= request->_url[i]) {
		case '#':
		case '?':
		case '\0':
			goto parse_finish;
		case '%':
			*p++ = from_hex(request->_url[i + 1]) << 4 | from_hex(request->_url[i + 2]);
			i += 2;
			break;
		case '.':
			ext = p;
		default:
			*p++ = c;
			break;
		}
		i++;
	}
parse_finish:
	*p = '\0';
	if (0 == mstrcmp(cfg->cgi, bp)) {
		//ÊÇCGI½Å±¾.
		memmove(buf, bp + cfg->lcgi, l-cfg->lcgi);
		buf[l - cfg->lcgi] = '\0';
		*code = 600; // is script;
		request->is_cgi = 1;
		return request->file = buf;
	}
	*p = '\0';
	p--;
	puts(buf);
	if (p[0] == '\\' || p[0] == '/')p[0] = '\0';
	puts(buf);

	while (0 == stat(buf, &st))
	{
		if (S_ISDIR(st.st_mode)) {
			strcpy(p, "/index.html");
			ext = strchr(p, '.');
			continue;
		}
		*code = request->mt_mtime == st.st_mtime ? 304 : 200;

		if (ext) {
			webheader_t h;
			rbnode_t * n;
			h.key = ext + 1;
			if (n = rb_find(&cfg->mime, &h.n)) {
				request->mime = container_of(n, webheader_t, n)->val;
			}
		}
		request->mt_mtime = st.st_mtime;
		request->st_size = st.st_size;
		return request->file = buf;
	}
	*code = 404;
	return request->file = buf;
}
#if defined(__linux__)
char *ltoa(long N, char *str, int base)
{
#define BUFSIZE	128
	register int i = 2;
	long uarg;
	char *tail, *head = str, buf[BUFSIZE];

	if (36 < base || 2 > base)
		base = 10;                    /* can only use 0-9, A-Z        */
	tail = &buf[BUFSIZE - 1];           /* last character position      */
	*tail-- = '\0';

	if (10 == base && N < 0L)
	{
		*head++ = '-';
		uarg = -N;
	}
	else  uarg = N;

	if (uarg)
	{
		for (i = 1; uarg; ++i)
		{
			register ldiv_t r;

			r = ldiv(uarg, base);
			*tail-- = (char)(r.rem + ((9L < r.rem) ?
				('A' - 10L) : '0'));
			uarg = r.quot;
		}
	}
	else  *tail-- = '0';

	memcpy(head, ++tail, i);
	return str;
}
#endif
const char * http_statusstr(int code, unsigned int  * slen)
{
	char * sret = "HTTP/1.1 306 Unknown\r\n";
	*slen = sizeof("HTTP/1.1 306 Unknown\r\n") - 1;

	switch (code)
	{
	case 100:
		sret = ("HTTP/1.1 100 Continue\r\n");
		*slen = sizeof("HTTP/1.1 100 Continue\r\n") - 1;
		break;
	case 101:
		sret = ("HTTP/1.1 101 Switching Protocols\r\n");
		*slen = sizeof("HTTP/1.1 101 Switching Protocols\r\n") - 1;
		break;
	case 102:
		sret = ("HTTP/1.1 102 Processing\r\n");
		*slen = sizeof("HTTP/1.1 102 Processing\r\n") - 1;
		break;
	case 200:
		sret = ("HTTP/1.1 200 OK\r\n");
		*slen = sizeof("HTTP/1.1 200 OK\r\n") - 1;
		break;
	case 201:
		sret = ("HTTP/1.1 201 Created\r\n");
		*slen = sizeof("HTTP/1.1 201 Created\r\n") - 1;
		break;
	case 202:
		sret = ("HTTP/1.1 202 Accepted\r\n");
		*slen = sizeof("HTTP/1.1 202 Accepted\r\n") - 1;
		break;
	case 203:
		sret = ("HTTP/1.1 203 Non-Authoriative Information\r\n");
		*slen = sizeof("HTTP/1.1 203 Non-Authoriative Information\r\n") - 1;
		break;
	case 204:
		sret = ("HTTP/1.1 204 No Content\r\n");
		*slen = sizeof("HTTP/1.1 204 No Content\r\n") - 1;
		break;
	case 205:
		sret = ("HTTP/1.1 205 Reset Content\r\n");
		*slen = sizeof("HTTP/1.1 205 Reset Content\r\n") - 1;
		break;
	case 206:
		sret = ("HTTP/1.1 206 Partial Content\r\n");
		*slen = sizeof("HTTP/1.1 206 Partial Content\r\n") - 1;
		break;
	case 207:
		sret = ("HTTP/1.1 207 Multi-Status\r\n");
		*slen = sizeof("HTTP/1.1 207 Multi-Status\r\n") - 1;
		break;
	case 300:
		sret = ("HTTP/1.1 300 Multiple Choices\r\n");
		*slen = sizeof("HTTP/1.1 300 Multiple Choices\r\n") - 1;
		break;
	case 301:
		sret = ("HTTP/1.1 301 Moved Permanently\r\n");
		*slen = sizeof("HTTP/1.1 301 Moved Permanently\r\n") - 1;
		break;
	case 302:
		sret = ("HTTP/1.1 302 Found\r\n");
		*slen = sizeof("HTTP/1.1 302 Found\r\n") - 1;
		break;
	case 303:
		sret = ("HTTP/1.1 303 See Other\r\n");
		*slen = sizeof("HTTP/1.1 303 See Other\r\n") - 1;
		break;
	case 304:
		sret = ("HTTP/1.1 304 Not Modified\r\n");
		*slen = sizeof("HTTP/1.1 304 Not Modified\r\n") - 1;
		break;
	case 305:
		sret = ("HTTP/1.1 305 Use Proxy\r\n");
		*slen = sizeof("HTTP/1.1 305 Use Proxy\r\n") - 1;
		break;
	case 306:
		sret = ("HTTP/1.1 306 (Unused)\r\n");
		*slen = sizeof("HTTP/1.1 306 (Unused)\r\n") - 1;
		break;
	case 307:
		sret = ("HTTP/1.1 307 Temporary Redirect\r\n");
		*slen = sizeof("HTTP/1.1 307 Temporary Redirect\r\n") - 1;
		break;
	case 400:
		sret = ("HTTP/1.1 400 Bad Request\r\n");
		*slen = sizeof("HTTP/1.1 400 Bad Request\r\n") - 1;
		break;
	case 401:
		sret = ("HTTP/1.1 401 Unauthorized\r\n");
		*slen = sizeof("HTTP/1.1 401 Unauthorized\r\n") - 1;
		break;
	case 402:
		sret = ("HTTP/1.1 402 Payment Granted\r\n");
		*slen = sizeof("HTTP/1.1 402 Payment Granted\r\n") - 1;
		break;
	case 403:
		sret = ("HTTP/1.1 403 Forbidden\r\n");
		*slen = sizeof("HTTP/1.1 403 Forbidden\r\n") - 1;
		break;
	case 404:
		sret = ("HTTP/1.1 404 File Not Found\r\n");
		*slen = sizeof("HTTP/1.1 404 File Not Found\r\n") - 1;
		break;
	case 405:
		sret = ("HTTP/1.1 405 Method Not Allowed\r\n");
		*slen = sizeof("HTTP/1.1 405 Method Not Allowed\r\n") - 1;
		break;
	case 406:
		sret = ("HTTP/1.1 406 Not Acceptable\r\n");
		*slen = sizeof("HTTP/1.1 406 Not Acceptable\r\n") - 1;
		break;
	case 407:
		sret = ("HTTP/1.1 407 Proxy Authentication Required\r\n");
		*slen = sizeof("HTTP/1.1 407 Proxy Authentication Required\r\n") - 1;
		break;
	case 408:
		sret = ("HTTP/1.1 408 Request Time-out\r\n");
		*slen = sizeof("HTTP/1.1 408 Request Time-out\r\n") - 1;
		break;
	case 409:
		sret = ("HTTP/1.1 409 Conflict\r\n");
		*slen = sizeof("HTTP/1.1 409 Conflict\r\n") - 1;
		break;
	case 410:
		sret = ("HTTP/1.1 410 Gone\r\n");
		*slen = sizeof("HTTP/1.1 410 Gone\r\n") - 1;
		break;
	case 411:
		sret = ("HTTP/1.1 411 Length Required\r\n");
		*slen = sizeof("HTTP/1.1 411 Length Required\r\n") - 1;
		break;
	case 412:
		sret = ("HTTP/1.1 412 Precondition Failed\r\n");
		*slen = sizeof("HTTP/1.1 412 Precondition Failed\r\n") - 1;
		break;
	case 413:
		sret = ("HTTP/1.1 413 Request Entity Too Large\r\n");
		*slen = sizeof("HTTP/1.1 413 Request Entity Too Large\r\n") - 1;
		break;
	case 414:
		sret = ("HTTP/1.1 414 Request-URI Too Large\r\n");
		*slen = sizeof("HTTP/1.1 414 Request-URI Too Large\r\n") - 1;
		break;
	case 415:
		sret = ("HTTP/1.1 415 Unsupported Media Type\r\n");
		*slen = sizeof("HTTP/1.1 415 Unsupported Media Type\r\n") - 1;
		break;
	case 416:
		sret = ("HTTP/1.1 416 Requested range not satisfiable\r\n");
		*slen = sizeof("HTTP/1.1 416 Requested range not satisfiable\r\n") - 1;
		break;
	case 417:
		sret = ("HTTP/1.1 417 Expectation Failed\r\n");
		*slen = sizeof("HTTP/1.1 417 Expectation Failed\r\n") - 1;
		break;
	case 422:
		sret = ("HTTP/1.1 422 Unprocessable Entity");
		*slen = sizeof("HTTP/1.1 422 Unprocessable Entity") - 1;
		break;
	case 423:
		sret = ("HTTP/1.1 423 Locked\r\n");
		*slen = sizeof("HTTP/1.1 423 Locked\r\n") - 1;
		break;
	case 424:
		sret = ("HTTP/1.1 424 Failed Dependency\r\n");
		*slen = sizeof("HTTP/1.1 424 Failed Dependency\r\n") - 1;
		break;
	case 500:
		sret = ("HTTP/1.1 500 Internal Server Error\r\n");
		*slen = sizeof("HTTP/1.1 500 Internal Server Error\r\n") - 1;
		break;
	case 501:
		sret = ("HTTP/1.1 501 Not Implemented\r\n");
		*slen = sizeof("HTTP/1.1 501 Not Implemented\r\n") - 1;
		break;
	case 502:
		sret = ("HTTP/1.1 502 Bad Gateway\r\n");
		*slen = sizeof("HTTP/1.1 502 Bad Gateway\r\n") - 1;
		break;
	case 503:
		sret = ("HTTP/1.1 503 Service Unavailable\r\n");
		*slen = sizeof("HTTP/1.1 503 Service Unavailable\r\n") - 1;
		break;
	case 504:
		sret = ("HTTP/1.1 504 Gateway Timeout\r\n");
		*slen = sizeof("HTTP/1.1 504 Gateway Timeout\r\n") - 1;
		break;
	case 505:
		sret = ("HTTP/1.1 505 HTTP Version Not Supported\r\n");
		*slen = sizeof("HTTP/1.1 505 HTTP Version Not Supported\r\n") - 1;
		break;
	case 507:
		sret = ("HTTP/1.1 507 Insufficient Storage\r\n");
		*slen = sizeof("HTTP/1.1 507 Insufficient Storage\r\n") - 1;
		break;
	}
	return sret;
}

int automem_append_header_date(automem_t * mem,const char * fmt, time_t t) {
	struct tm tm;
	char str_tm[64];
	size_t lstr_tm;
	gmtime_s(&tm, &t);
	lstr_tm = strftime(str_tm, 60, fmt, &tm);
	return automem_append_voidp(mem, str_tm, lstr_tm);
}


int automem_init_headers(automem_t * mem, int code,int flags) {
	size_t slen;
	const char * status = http_statusstr(code, &slen);
	automem_append_voidp(mem, status, slen);
	automem_append_voidp(mem, "Server: uvhttpd/0.0.1\r\n", sizeof("Server: uvhttpd/0.0.1\r\n") - 1);
	if(flags & F_CONNECTION_KEEP_ALIVE)
		automem_append_voidp(mem, "Connection: keep-alive\r\n", sizeof("Connection: keep-alive\r\n") - 1);
	else
		automem_append_voidp(mem, "Connection: close\r\n", sizeof("Connection: close\r\n") - 1);

	automem_append_header_date(mem, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", time(NULL));

	return mem->size;
}

int automem_append_mime(automem_t * mem, const char * mime) {
	automem_append_voidp(mem, "Content-Type: ", sizeof("Content-Type: ") - 1);
	automem_append_voidp(mem, mime, strlen(mime));
	automem_append_voidp(mem, "\r\n", 2);
	return mem->size;
}
int automem_append_content_length(automem_t *mem, size_t sz) {
	size_t s;
	char buf[sizeof("Content-Length: ") + 32] = "Content-Length: ";
	ltoa(sz, buf + sizeof("Content-Length: ") - 1, 10);
	s = strlen(buf);
	memcpy(buf + s, "\r\n", 2);
	automem_append_voidp(mem, buf, s + 2);
	return mem->size;
}


int automem_append_contents(automem_t * mem, void * bytes, int sz) {
	size_t s;
	char buf[sizeof("Content-Length: ")+32]="Content-Length: ";
	ltoa(sz, buf+sizeof("Content-Length: ")-1, 10);
	s = strlen(buf);
	memcpy(buf + s, "\r\n\r\n", 4);
	automem_append_voidp(mem, buf,s+4);
	automem_append_voidp(mem,bytes, sz);
	mem->pdata[mem->size] = '\0';
	return mem->size;
}

typedef struct writefile writefile_t;
struct writefile {
	uv_write_t w;
	automem_t mem;
	webconn_t * conn;
	size_t st_size, st_offset;
	int fd;
	uv_buf_t buf;
};

static void webserver_after_writefile(uv_write_t * req, int status)
{
	writefile_t * wf = container_of(req, writefile_t, w);
	int nread = 0;
	if (0 == status) {
		if (wf->fd) {
			wf->mem.size = 0;
			while (wf->st_offset < wf->st_size && wf->mem.size < wf->mem.buffersize) {
				nread = read(wf->fd, wf->mem.pdata + wf->mem.size, wf->mem.buffersize - wf->mem.size);
				wf->st_offset += nread;
				lseek(wf->fd, wf->st_offset, SEEK_SET);
				wf->mem.size += nread;
			}
			if (wf->mem.size > 0) {
				wf->buf.len = wf->mem.size;
				if (0 == uv_write(&wf->w, (uv_stream_t *)wf->conn, &wf->buf, 1, webserver_after_writefile)) {
					return;
				}
			}
			close(wf->fd);
		}
	}
	/*else {

	}
	*/
	automem_uninit(&wf->mem);
	free(wf);
}

void webheader_free(webheader_t * header) {
	free(header->key);
	if(header->val) free(header->val);
	free(header);
}
int webcon_writefile(webconn_t * conn, const char * file, size_t st_size) {
	writefile_t * wf = calloc(1, sizeof(writefile_t));
	wf->st_size = st_size;
	automem_init(&wf->mem, 40960);// 40kb buffer.
	
	if (-1 != (wf->fd = open(file, O_RDONLY | O_BINARY))) {

		while (wf->st_offset < wf->st_size && wf->mem.size < wf->mem.buffersize)
		{
			wf->st_offset += read(wf->fd, wf->mem.pdata + wf->mem.size, wf->mem.buffersize - wf->mem.size);
			lseek(wf->fd, wf->st_offset, SEEK_SET);
			wf->mem.size += wf->st_offset;
		}

		wf->buf.base = wf->mem.pdata;
		wf->buf.len = wf->mem.size;
		wf->conn = conn;

		if (0 != uv_write(&wf->w, (uv_stream_t *)&conn->conn, &wf->buf, 1, webserver_after_writefile)) {
			automem_uninit(&wf->mem);
			close(wf->fd);
			free(wf);
		}
		return 0;
	}
	free(wf);
	return -1;
}

void lua_register_class(lua_State * L, const luaL_Reg * methods, const char * name, lua_CFunction has_index)
{
	luaL_newmetatable(L, name);
	luaL_setfuncs(L, methods, 0);

	lua_pushliteral(L, "__index");
	if (NULL == has_index)
		lua_pushvalue(L, -2);
	else
		lua_pushcfunction(L, has_index);

	lua_settable(L, -3);

	lua_pushliteral(L, "__metatable");
	lua_pushliteral(L, "you're not allowed to get this metatable");
	lua_settable(L, -3);

	lua_pop(L, 1);
}

unsigned char hex2byte(const unsigned char *ptr)
{
	int i;
	unsigned char result = 0;

	for (i = 0; i < 2; i++) {
		result <<= 4;
		if (ptr[i] >= '0' && ptr[i] <= '9')
			result |= (ptr[i] - '0');
		else if (ptr[i] >= 'A' && ptr[i] <= 'F')
			result |= (ptr[i] - 'A' + 10);
		else
			result |= (ptr[i] - 'a' + 10);
	}

	return result;
}

void lua_url_decode(lua_State *L, char *s, automem_t * mem)
{
	s++;
	char c, *ss = strchr(s, '=');
	unsigned int sz = 0;
	if (NULL == ss) return;

	mem->size = 0;

	while (s < ss) {
		c = *s++;
		automem_ensure_newspace(mem, 3);
		switch (c) {
		case '%':
			mem->pdata[mem->size++] = hex2byte(s);
			s += 2;
			break;
		case '+':
			mem->pdata[mem->size++] = ' ';
			break;
		default:
			mem->pdata[mem->size++] = c;
			break;
		}
	}
	mem->pdata[mem->size++] = '\0';
	sz = mem->size;

	ss++;
	while (c = *ss++) {
		automem_ensure_newspace(mem, 3);
		if (c == '&')break;
		switch (c) {
		case '%':
			mem->pdata[mem->size++] = hex2byte(ss);
			ss += 2;
			break;
		case '+':
			mem->pdata[mem->size++] = ' ';
			break;
		default:
			mem->pdata[mem->size++] = c;
			break;
		}
	}
	lua_pushlstring(L, mem->pdata + sz, mem->size - sz);
	lua_setfield(L, -2, mem->pdata);
}


int luadef_write_length(automem_t * pmem, int val)
{

	val &= 0x1FFFFFFF;
	if (val < 0x80)
	{
		automem_append_byte(pmem, (unsigned char)val);
		return 1;
	}
	else if (val < 0x4000)
	{
		automem_append_byte(pmem, (unsigned char)((val >> 7 & 0x7f) | 0x80));
		automem_append_byte(pmem, (unsigned char)(val & 0x7f));
		return 2;
	}
	else if (val < 0x200000)
	{
		automem_append_byte(pmem, (unsigned char)((val >> 14 & 0x7f) | 0x80));
		automem_append_byte(pmem, (unsigned char)((val >> 7 & 0x7f) | 0x80));
		automem_append_byte(pmem, (unsigned char)(val & 0x7f));
		return 3;
	}
	else
	{
		char tmp[4] = { (val >> 22 & 0x7f) | 0x80, (val >> 15 & 0x7f) | 0x80, (val >> 8 & 0x7f) | 0x80, val & 0xff };
		automem_append_voidp(pmem, tmp, 4);
	}

	return 4;
}

/**  reads an integer in FMT format */
int luadef_read_length(unsigned char * bytes, int * used)
{
	unsigned char * cp = (unsigned char *)bytes;
	int acc, mask, r, tmp;

	acc = *cp++;


	if (acc < 128)
	{
		*used = 1;
		return acc;
	}
	else
	{

		acc = (acc & 0x7f) << 7;
		tmp = *cp++;

		*used = 2;

		if (tmp < 128)
		{
			acc = acc | tmp;
		}
		else
		{
			acc = (acc | (tmp & 0x7f)) << 7;
			tmp = *cp++;

			if (tmp < 128)
			{
				*used = 3;
				acc = acc | tmp;
			}
			else
			{
				acc = (acc | (tmp & 0x7f)) << 8;
				tmp = *cp++;
				acc = acc | tmp;

				*used = 4;
			}
		}
	}
	/* To sign extend a value from some number of bits to a greater number of bits just copy the sign bit into all the additional bits in the new format */
	/* convert/sign extend the 29bit two's complement number to 32 bi */
	mask = 1 << 28;  /*  mas */
	r = -(acc & mask) | acc;

	return r;
}