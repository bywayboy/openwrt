#include <stdio.h>

#include "uv.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "webserver.h"
#include "webrequest.h"
#include "webctx.h"
#include "utils.h"

#define WEBCTX_REF		3

// ��ȡHTTPͷ����Ϣ
static int lua_webctx_server(lua_State * L) {
	int i, argc = lua_gettop(L);
	webqueuework_t * qwork = (lua_rawgeti(L, LUA_REGISTRYINDEX, WEBCTX_REF), lua_touserdata(L, -1));
	if (argc > 0) {
		for (i = 1; i <= argc; i++) {
			webheader_t n;
			rbnode_t *r;
			n.key = (char *)lua_tostring(L, i);
			if (r = rb_find(&qwork->request->headers, &n.n))
				lua_pushstring(L, container_of(r, webheader_t, n)->val);
			else
				lua_pushnil(L);
		}
	}
	return argc;
}

static int getBodyParams(lua_State * L, char * s) {
	automem_t mem;
	automem_init(&mem, 128);
	lua_newtable(L);
	do {
		lua_url_decode(L, s++, &mem);
		s = strchr(s, '&');
	} while (s);
	automem_uninit(&mem);
	return 1;
}
// ��ȡURL GET����
static int lua_webctx_get(lua_State * L) {
	webqueuework_t * qwork = (lua_rawgeti(L, LUA_REGISTRYINDEX, WEBCTX_REF), lua_touserdata(L, -1));
	if (NULL != qwork->request->params) {
		return getBodyParams(L, qwork->request->params);
	}
	return 0;
}

// ��ȡPOST����.
static int lua_webctx_post(lua_State * L) {
	webqueuework_t * qwork  = (lua_rawgeti(L, LUA_REGISTRYINDEX, WEBCTX_REF), lua_touserdata(L, -1));
	if (NULL != qwork->request->body) {
		return getBodyParams(L, qwork->request->body-1);
	}
	return 0;
}

// ���HTTPͷ����Ϣ
// �����һ����������Ϊ����, ������״̬��.
static int lua_webctx_header(lua_State * L) {
	int argc = lua_gettop(L);
	int t = lua_type(L, 1);
	webqueuework_t * qwork = (lua_rawgeti(L, LUA_REGISTRYINDEX, WEBCTX_REF), lua_touserdata(L, -1));

	switch (t) {
	case LUA_TNUMBER:
		qwork->status = (int)lua_tointeger(L, 1);
		break;
	case LUA_TSTRING:
		if (2 == argc) {
			webheader_set(&qwork->headers, lua_tostring(L, 1), lua_tostring(L, 2));
		}
		else {
			const char * key = (char *)lua_tostring(L, 1);
			webheader_set(&qwork->headers, key, NULL);
		}
		break;
	}
	
	return 0;
}

// ��ȡ�������ߵ��豸.
static int lua_webctx_getdevices(lua_State * L) {
	webqueuework_t * request = (lua_rawgeti(L, LUA_REGISTRYINDEX, WEBCTX_REF), lua_touserdata(L, -1));
}

// ��ͻ����������.
static int lua_webctx_write(lua_State * L) {
	int i,argc = lua_gettop(L),t;
	const char * s;
	webqueuework_t * qwork = (lua_rawgeti(L, LUA_REGISTRYINDEX, WEBCTX_REF), lua_touserdata(L, -1));

	for (i = 1; i <= argc; i++) {
		size_t len;
		t = lua_type(L, i);
		switch (t) {
		case LUA_TNIL:
		case LUA_TNONE:
			break;
		default:
			s = lua_tolstring(L, i, &len);
			automem_append_voidp(&qwork->mem, s, (unsigned int)len);
			break;
		}
	}
	return 0;
}

// ִ�� WebScoket ����. ֱ�ӷ��� 101 ����.

static const luaL_Reg lua_webctx[] = {
	{"server", lua_webctx_server},			// ��ȡHTTPͷ����Ϣ.
	{"get", lua_webctx_get},				// ��ȡURL GET����.
	{"post", lua_webctx_post},				// ��ȡPOST����.
	{"header",lua_webctx_header},			// ���HTTPͷ����Ϣ.
	{"write", lua_webctx_write},			// ��ͻ����������.
	{NULL,NULL},
};

static const luaL_Reg lua_wsctx[] = {
	{ "server", lua_webctx_server },			// ��ȡHTTPͷ����Ϣ.
	{ "get", lua_webctx_get },				// ��ȡURL GET����.
	{ "post", lua_webctx_post },				// ��ȡPOST����.
	{ "write", lua_webctx_write },			// ��ͻ����������.
	{ NULL,NULL },
};
int lua_openwebctx(lua_State * L) {luaL_newlib(L, lua_webctx);return 1;}
int lua_openwebsocketctx(lua_State * L) {luaL_newlib(L, lua_wsctx);return 1;}

//�����һ������.
int lua_pushwebctx(lua_State * L, webqueuework_t * qwork)
{
	lua_pushlightuserdata(L, qwork);
	int r = luaL_ref(L, LUA_REGISTRYINDEX);
	switch (qwork->conn->proto) {
	case WEB_PROTO_HTTP:
		luaL_requiref(L, "server", lua_openwebctx, 1);
		lua_pushstring(L, http_method_str(qwork->request->method));
		lua_setfield(L, -2, "METHOD");
		break;
	case WEB_PROTO_WEBSOCKET:
		luaL_requiref(L, "server", lua_openwebsocketctx, 1);
		if (NULL != qwork->frame) {
			lua_pushlstring(L, qwork->frame->pdata, qwork->frame->len);
			lua_setfield(L, -2, "BODY");
		}
		break;
	}
	lua_pop(L, 1);  /* remove lib */
	return r;
}
