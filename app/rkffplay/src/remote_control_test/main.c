#include <stdio.h>
#include <string.h>

#include "ffplay_ipc.h"

int main()
{
	char cmd[128] = {0};
	char *path = NULL;

	while(1) {
		printf("#Please input your cmd:\n");
		memset(cmd, 0, sizeof(cmd));
		gets(cmd);
		if (strstr(cmd, "play")) {
			path = cmd + strlen("play");
			path += 1;
			printf("INFO: <play> path:%s\n", path);
			media_play(path, 0, 0);
		} else if (strstr(cmd, "pause")) {
			printf("INFO: <pause>\n");
			media_pause();
		} else if (strstr(cmd, "restore")) {
			printf("INFO: <restroe>\n");
			media_restore();
		} else if (strstr(cmd, "exit")) {
			printf("INFO: <exit>\n");
			media_exit();
		}
	}
	
	return 0;
}