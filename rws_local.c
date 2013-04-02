/**
 * @file rws_local.c
 * @brief 
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 1.0
 * @date 2013-04-02
 */

#include <stdio.h>

int rwscoli_local_publish(char *name)
{
   (void)name;
   return 0;
}

void rwscoli_local_print(char* buf, int size)
{
   (void)size;
   fprintf(stderr, "%s", buf);
   return;
}

int rwscoli_local_recv(char *buf, int *size)
{
   (void)buf;
   (void)size;
   return 0;
}

void rwscoli_local_end_cmd()
{
   return;
}


