#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <uci.h>
#include <time.h>
#include "uv.h"
#include "webserver.h"
#include "cfg.h"

static struct uci_context * ctx = NULL;
static config_t cfg = {
#if defined(__linux__)
	.bind="0.0.0.0",
	.port=80,
	.wwwroot=NULL
#else
	"0.0.0.0",
	80,
	NULL
#endif
};

#define UCI_CONFIG_FILE	"uvhttpd"

const config_t * config_get(void) {
	return &cfg;
}
static void server_config(struct uci_section * s)
{
	struct uci_element *n = NULL;
	uci_foreach_element(&s->options, n) {
		struct uci_option *o = uci_to_option(n);
		switch (n->name[0]) {
		case 'b':// bind.
			strcpy(cfg.bind, o->v.string);
			break;
		case 'p':
			cfg.port = strtoul(o->v.string, NULL, 10);
			break;
#if defined(__linux__)
		case 'w':
			cfg.wwwroot = strdup(o->v.string);
			break;
#endif
		case 'c':
			cfg.cgi = strdup(o->v.string);
			cfg.lcgi = strlen(cfg.cgi);
			break;
		}
	}
}

static void mime_config(struct uci_section * s) {
	struct uci_element *n = NULL;
	uci_foreach_element(&s->options, n) {
		struct uci_option *o = uci_to_option(n);
		webheader_set(&cfg.mime, n->name, o->v.string);
	}
}


int config_init(void)
{
	struct uci_package * pkg = NULL;
	if (NULL == ctx)
		ctx = uci_alloc_context();
#if defined(_WIN32) || defined(_WIN64)
	uci_set_confdir(ctx, "D:/VC/uwrt/uvhttpd/files/etc/config");
#endif
	rb_root_init(&cfg.mime, header_compare);
	if (UCI_OK == uci_load(ctx, UCI_CONFIG_FILE, &pkg))
	{
		struct uci_element * e;
		uci_foreach_element(&pkg->sections, e)
		{
			struct uci_section * s = uci_to_section(e);
			switch (s->type[0])
			{
			case 's':
				server_config(s);
				break;
			case 'm':
				mime_config(s);
				break;
			}
		}
	}
	if (NULL == cfg.wwwroot) {
#if defined(__linux__)
		cfg.wwwroot = strdup("/wwwroot/");
#else
		cfg.wwwroot = strdup("D:\\VC\\uwrt\\wwwroot\\");
#endif
	}
	if (cfg.wwwroot)
	{
		cfg.lwwwroot = strlen(cfg.wwwroot);
		if (cfg.wwwroot[cfg.lwwwroot-1] == '\\' || cfg.wwwroot[cfg.lwwwroot-1] == '/') {
			cfg.wwwroot[cfg.lwwwroot--] = '\0';
		}
	}
	return 0L;
}

void config_uninit(void) {

	rbtree_destroy(&cfg.mime, webheader_node_free);
	if (NULL != cfg.wwwroot)
		free(cfg.wwwroot);
	if (NULL != cfg.cgi)
		free(cfg.cgi);
	cfg.cgi = cfg.wwwroot = NULL;
}