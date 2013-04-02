/**
 * @file rws_uds.c
 * @brief This file implements functions to support coli framework 
 *        when unix domain socket is used.
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 1.0
 * @date 2013-02-15
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rws_coli.h"

static int rwscoli_uds_fd = -1;
static struct sockaddr_un rwscoli_uds_sh_sa_un;

/*======================rwscolid part===================*/

#define RWSCOLI_UDS_PATH         "/tmp/rwscoli/"
#define RWSCOLI_UDS_CMD_ENDMARK  0xDEADBEEF
#define RWSCOLI_UDS_MAX_LINE_LEN          4096

static int rwscoli_uds_recv(int fd, char *buf, int *size, struct sockaddr_un *from);
static int rwscoli_uds_send(int fd, char *buf, int size, struct sockaddr_un *to);

int rwscoli_recv_cmd(int *argc, char ***argv)
{
   static char buf[RWSCOLI_UDS_MAX_LINE_LEN];
   int size = sizeof(buf);
   int result;

   memset(buf, 0, sizeof(buf));

   result = rwscoli_uds_recv(rwscoli_uds_fd, buf, &size, &rwscoli_uds_sh_sa_un);
   if (result < 0) {
      fprintf(stderr, "rwscoli_recv_cmd failed!\n");
      return -1;
   }

   if (rwscoli_unpack_args(buf, size, argc, argv) < 0) {
      fprintf(stderr, "rwscoli_unpack_args failed!\n");
      return -1;
   }

   return 0;
}

int rwscoli_uds_publish(char *name)
{
   struct sockaddr_un sa_un;
   int fd;
   char path[64];
   snprintf(path, sizeof(path), "%s%s", RWSCOLI_UDS_PATH, name);
  
   if (strlen(path) >= sizeof(sa_un.sun_path)) {
      return -1;
   }

   /* attemp to create the directory if non-exist */
   mkdir(RWSCOLI_UDS_PATH, S_IRWXU|S_IRWXG|S_IRWXO);

   if (unlink(path) < 0) {
      if (errno != ENOENT) {
	 fprintf(stderr, "%s: ", path);
	 perror("unlink");
	 return -1;
      }
   }
  
   memset(&sa_un, 0, sizeof(sa_un));
   sa_un.sun_family = AF_UNIX;
   strcpy(sa_un.sun_path, path);

   fd = socket(PF_UNIX, SOCK_DGRAM, 0);
   if (fd < 0) {
      return -1;
   }

   if (bind(fd, (struct sockaddr *)&sa_un, sizeof(sa_un)) < 0) {
      close(fd);
      return -1;
   }

   /*
   **  Allow everyone to use the socket
   */
   if (chmod(path, S_IRWXU|S_IRWXG|S_IRWXO) < 0) {
      close(fd);
      unlink(path);
      return -1;
   }

   rwscoli_uds_fd = fd;
   return fd;
}

void rwscoli_uds_print(char *buf, int size)
{
   rwscoli_uds_send(rwscoli_uds_fd, buf, size, &rwscoli_uds_sh_sa_un);
   return;
}

void rwscoli_uds_end_cmd()
{
   uint32_t endmark = RWSCOLI_UDS_CMD_ENDMARK;
   rwscoli_uds_print((char*)&endmark, sizeof(endmark));
   return;
}

static int rwscoli_uds_recv(int fd, char *buf, int *size, struct sockaddr_un *from)
{
   struct msghdr msg;
   struct iovec iov;
   int result;
   
   iov.iov_base = buf;
   iov.iov_len = *size;
   
   msg.msg_name = from;
   msg.msg_namelen = (from ? sizeof(*from) : 0);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = NULL;
   msg.msg_controllen = 0; 
   
   result = recvmsg(fd, &msg, 0);

   if (result == -1) {
      perror("recvmsg");
      return -1;
   }

   *size = result;
   return result;
}

static int rwscoli_uds_send(int fd, char *buf, int size, struct sockaddr_un *to)
{
   struct msghdr msg;
   struct iovec vec; 
   int result;

   vec.iov_base = buf;
   vec.iov_len = size;

   msg.msg_name = to;
   msg.msg_namelen = sizeof(*to);
   msg.msg_iov = &vec;
   msg.msg_iovlen = 1;
   msg.msg_control = NULL;
   msg.msg_controllen = 0;
   msg.msg_flags = 0;
   
   result = sendmsg(fd, &msg, 0);
   if (result < 0) {
      perror("sendmsg");
   }

   return result;
}

/* Below are the utility functions to implement a sh program that
 * provides command interface for other programs with coli support
 * by using unix domain socket as IPC to pass command/response.
 *
 * Please see example/rwscolish.
 */

static int rwscoli_uds_sh_fd = -1;

#define RWSCOLI_UDS_SH_PATH "/tmp/.rwscolish"
#define RWSCOLI_UDS_SH_CMD_MAX  16

struct rwscoli_uds_sh_cmd
{
   char *name;
   char *path;
   char *descr;
};
static struct rwscoli_uds_sh_cmd rwscoli_uds_sh_cmds[RWSCOLI_UDS_SH_CMD_MAX];

static int  rwscoli_uds_sh_init_cmd_list();
static void rwscoli_uds_sh_print_cmd_list();
static struct sockaddr_un *rwscoli_uds_sh_find_cmd(char *cmd);

int rwscoli_uds_sh_init()
{
   int fd;
   struct sockaddr_un sa_un;

   rwscoli_uds_sh_init_cmd_list();

   unlink(RWSCOLI_UDS_SH_PATH);

   fd = socket(PF_UNIX, SOCK_DGRAM, 0);
   if (fd < 0) {
      return -1;
   }

   memset(&sa_un, 0, sizeof(sa_un));
   sa_un.sun_family = AF_UNIX;
   strcpy(sa_un.sun_path, RWSCOLI_UDS_SH_PATH); 

   if (bind(fd, (struct sockaddr *)&sa_un, sizeof(sa_un)) < 0) {
      close(fd);
      return -1;
   }

   rwscoli_uds_sh_fd = fd;
   return fd;
}

int rwscoli_uds_sh_send_cmd(int argc, char **argv)
{
   int result;
   struct sockaddr_un *sa_un = NULL;
   char buf[RWSCOLI_UDS_MAX_LINE_LEN];
   int len = rwscoli_args_len(argc, argv);

   sa_un = rwscoli_uds_sh_find_cmd(argv[0]);
   if (sa_un == NULL) {
      rwscoli_uds_sh_print_cmd_list();
      return -1;
   }

   memset(buf, 0, sizeof(buf));
   rwscoli_pack_args(argc, argv, buf, &len);

   result = rwscoli_uds_send(rwscoli_uds_sh_fd, buf, len, sa_un);
   if (result < 0) {
      return -1;
   }

   return 0;
}

void rwscoli_uds_sh_wait_cmd_end()
{
   char buf[RWSCOLI_UDS_MAX_LINE_LEN];
   int  result = 0;
   do {
      memset(buf, 0, sizeof(buf));
      int len = sizeof(buf) - 1;
      result = rwscoli_uds_recv(rwscoli_uds_sh_fd, buf, &len, NULL);
      if (result < 0 || *(uint32_t*)buf == RWSCOLI_UDS_CMD_ENDMARK) {
         break;
      }

      buf[len] = '\0';
      fprintf(stderr, "%s", buf);
      fflush(stderr);
   } while (1);

   return;
}

static struct sockaddr_un *rwscoli_uds_sh_find_cmd(char *cmd)
{
   static struct sockaddr_un sa_un;
   bool found = false;
   int  i;

   for (i = 0; i < RWSCOLI_UDS_SH_CMD_MAX; i++) {
      if (rwscoli_uds_sh_cmds[i].name && 
           strcmp(cmd, rwscoli_uds_sh_cmds[i].name) == 0 ) {
         found = true;
         break;
      }
   }

   if (found) {
      memset(&sa_un, 0, sizeof(sa_un));
      sa_un.sun_family = AF_UNIX;
      strcpy(sa_un.sun_path, rwscoli_uds_sh_cmds[i].path);
      return &sa_un;
   }
   
   return NULL;
}

static int rwscoli_uds_sh_init_cmd_list()
{
   int i;
   struct dirent *file;

   DIR *dir = opendir(RWSCOLI_UDS_PATH);
   if (dir == NULL) {
      fprintf(stderr, "error opendir %s!\n", RWSCOLI_UDS_PATH);
      return -1;
   }

   while((file=readdir(dir)) != NULL) {
      if (file->d_type == DT_SOCK) {
         for (i = 0; i < RWSCOLI_UDS_SH_CMD_MAX; i++) {
            if (rwscoli_uds_sh_cmds[i].name == NULL) {
               rwscoli_uds_sh_cmds[i].name = strdup(file->d_name);
               char path[64];
               snprintf(path, 64, "%s%s", RWSCOLI_UDS_PATH, file->d_name);
               rwscoli_uds_sh_cmds[i].path = strdup(path);
               break;
            }
         }
      }
   }

   return 0;
}

static void rwscoli_uds_sh_print_cmd_list()
{
   int i;
   printf("%-16s%s\n", "Command", "Description");
   for (i = 0; i < RWSCOLI_UDS_SH_CMD_MAX; i++) {
      if (rwscoli_uds_sh_cmds[i].name != NULL) {
         printf("%-16s%s\n", rwscoli_uds_sh_cmds[i].name, rwscoli_uds_sh_cmds[i].descr);
      }
   }

   return;
}
