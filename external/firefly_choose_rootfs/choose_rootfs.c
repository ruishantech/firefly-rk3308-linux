#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/stat.h>

struct android_bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];

	/* The 'recovery' field used to be 1024 bytes.	It has only ever
	 * been used to store the recovery command line, so 768 bytes
	 * should be plenty.  We carve off the last 256 bytes to store the
	 * stage string (for multistage packages) and possible future
	 * expansion. */
	char stage[32];

	/* The 'reserved' field used to be 224 bytes when it was initially
	 * carved off from the 1024-byte recovery field. Bump it up to
	 * 1184-byte so that the entire bootloader_message struct rounds up
	 * to 2048-byte. */
	char reserved[1184];
};


#define MISC_FILE_PATH "/dev/block/by-name/misc"
#define UBUNTU_FILE_PATH "/dev/block/by-name/ubuntu"
#define BUILDROOT_FILE_PATH "/dev/block/by-name/buildroot"

#define MISC_MSG_OFFSET 16 * 1024

//文件存在返回1，否则返回0
static int file_exist(const char *file)
{
    struct stat stat0;
    return (stat(file, &stat0)==0) ? 1 : 0;
}

int main(int argc, char **argv)
{
    if(NULL==argv[1]){
        printf("%s buildroot/ubuntu\n",argv[0]);
        return -1;
    }

    if( !( strcmp(argv[1], "buildroot")==0 || strcmp(argv[1], "ubuntu")==0 )){
        printf("%s buildroot/ubuntu\n",argv[0]);
        return -1;
    }

    if( !file_exist(UBUNTU_FILE_PATH)  && !file_exist(BUILDROOT_FILE_PATH) ){
        printf("Error: not exist multiple systems\n");
        return -1;
    }

    FILE *misc_file;

	struct android_bootloader_message msg;
	memset(&msg, 0, sizeof(msg));
	strcpy(msg.command, "boot-from-choose-rootfs");
	strcpy(msg.recovery, argv[1]);

	printf("update: write command to misc file: ");
	if((misc_file = fopen(MISC_FILE_PATH,"wb")) == NULL){
		printf("Open misc file error.\n");
		return -1;
	}
	fseek(misc_file, MISC_MSG_OFFSET, SEEK_SET);
	fwrite(&msg, sizeof(msg), 1, misc_file);
    
	fclose(misc_file);
	printf("done\n");

    sync();
    reboot(RB_AUTOBOOT);
    return 0;
}
