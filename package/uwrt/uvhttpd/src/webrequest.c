#include <stdio.h>
#include <stdint.h>

#include <uv.h>

#include "webserver.h"
#include "webrequest.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "webctx.h"
#include "utils.h"
#include "websocket.h"
#include "session.h"

static void webrequest_threadproc(uv_work_t * req)
{
	int ref = LUA_NOREF,sess_ref = LUA_NOREF;
	automem_t mem;
	webqueuework_t * qwork = container_of(req, webqueuework_t, work);
	
	lua_State * L = luaL_newstate();

	qwork->flags = qwork->conn->flags;
	luaL_openlibs(L);
	ref = lua_pushwebctx(L, qwork);
	sess_ref = luasession_init(L,qwork);
	automem_init(&mem,100);
	automem_append_voidp(&mem, "require(\"", sizeof("require(\"") - 1);
	automem_append_voidp(&mem, qwork->request->file, strlen(qwork->request->file));

	switch (qwork->conn->proto) {
	case WEB_PROTO_HTTP:
		automem_append_voidp(&mem, "\"):main()", sizeof("\"):main()"));
		break;
	case WEB_PROTO_WEBSOCKET:
		automem_append_voidp(&mem, "\"):websocket()", sizeof("\"):websocket()"));
		break;
	}
	luaL_dostring(L, mem.pdata);
	if (LUA_TSTRING == lua_type(L, -1)) {
		size_t len;
		const char * err = lua_tolstring(L, -1, &len);
		
		qwork->status = 500;
		automem_append_voidp(&qwork->mem, err, len);
	}

	automem_uninit(&mem);
	luaL_unref(L,LUA_REGISTRYINDEX, ref);
	lua_close(L);
}

static void after_queue_work(uv_work_t* req, int status)
{
	automem_t mem;
	webqueuework_t * qwork = container_of(req, webqueuework_t, work);
	rbnode_t * n = rb_first(&qwork->request->headers);

	automem_init(&mem, 256);
	if (qwork->conn->proto == WEB_PROTO_HTTP) {
		if (qwork->status == 101 && qwork->request->upgrade) {
			webheader_t k;
			rbnode_t * n;
			const char * ver = NULL, *key = NULL;
			k.key = "Sec-WebSocket-Version";
			if (n = rb_find(&qwork->request->headers, &k.n)) ver = container_of(n, webheader_t, n)->val;
			k.key = "Sec-WebSocket-Key";
			if (n = rb_find(&qwork->request->headers, &k.n)) key = container_of(n, webheader_t, n)->val;
			if (NULL != key && NULL != ver) {
				ws_do_handeshake(&mem, key, strlen(key));
				qwork->conn->proto = WEB_PROTO_WEBSOCKET;
				if (uv_is_active((uv_handle_t *)&qwork->conn->conn)) {
					wsparser_init(&qwork->conn->ws_parser, 13, 20480);
					qwork->conn->request = webrequest_get(qwork->request);//引用起来,不丢掉.
				}
				goto contents_prepare_final;
			}
		}
		rbnode_t * n = rb_first(&qwork->headers);
		automem_init_headers(&mem, qwork->status, qwork->flags);
		while (n) {
			webheader_t * h = container_of(n, webheader_t, n);
			automem_append_voidp(&mem, h->key, strlen(h->key));
			automem_append_voidp(&mem, ": ", 2);
			automem_append_voidp(&mem, h->val, strlen(h->val));
			automem_append_voidp(&mem, "\r\n", 2);
			n = rb_next(n);
		}
		automem_append_contents(&mem, qwork->mem.pdata, qwork->mem.size);

	}
	else if(qwork->conn->proto==WEB_PROTO_WEBSOCKET) {
		wsframe_make(&mem, WS_TEXT_FRAME, 0, qwork->mem.pdata, qwork->mem.size);
	}
contents_prepare_final:
	if (0 != webconn_sendmem(qwork->conn, &mem)) {
		automem_uninit(&mem);
	}
	webqueuework_free(qwork);
}

int webrequest_push(webrequest_t * request, webconn_t * conn, wsFrame * frame)
{
	webqueuework_t * qwork = webqueuework_create();
	qwork->request = webrequest_get(request);
	qwork->conn = webconn_get(conn);
	qwork->frame = frame;

	if (0 != uv_queue_work(conn->loop, &qwork->work, webrequest_threadproc, after_queue_work))
	{
		webqueuework_free(qwork);
		return -1;
	}
	return 0;
}