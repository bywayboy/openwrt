#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <uv.h>
#include "cfg.h"
#include "webserver.h"

static uv_loop_t * loop = NULL;
static webserver_t s;
int main(int argc, char * argv[])
{
	int ret;
	srand(time(NULL));
	config_init();
	loop = uv_default_loop();

	if(0 == (ret = webserver_init(loop,&s)))
		uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}
