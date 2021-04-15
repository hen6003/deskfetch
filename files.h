#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char* get_uptime(char* char_buf);
char* get_mem(char* buf);
void get_cpu(char* buf);
void get_device(char* buf);
void get_kernel(char* buf);
void get_distro(char* buf);
int get_image_path(char* path);
