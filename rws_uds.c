/**
 * @file rws_uds.c
 * @brief This file implements functions to support coli framework
 *        when unix domain socket is used to pass command from command
 *        interface.
 *
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
#include "rws_args.h"
#include "rws_uds.h"

#define RWSCOLI_UDS_CMD_ENDMARK  0xDEADBEEF
#define RWSCOLI_UDS_MAX_LINE_LEN 4096

static int rwscoli_uds_fd;
static struct sockaddr_un rwscolish_uds_addr;

static int rwscoli_uds_create_socket(char *path);
static int rwscoli_uds_close_socket(int fd);
static int rwscoli_uds_recv(int fd, char *buf, int *size, struct sockaddr_un *from);
static int rwscoli_uds_send(int fd, char *buf, int size, struct sockaddr_un *to);

int rwscoli_publish(char *name)
{
   char path[64];
   int  fd;

   /* attemp to create the directory if non-exist */
   mkdir(RWSCOLI_UDS_PATH, S_IRWXU|S_IRWXG|S_IRWXO);

   snprintf(path, sizeof(path), "%s%s", RWSCOLI_UDS_PATH, name);

   fd = rwscoli_uds_create_socket(path);

   rwscoli_uds_fd = fd;

   return fd;
}

int rwscoli_recv_cmd(int fd, int *argc, char ***argv)
{
   static char buf[RWSCOLI_UDS_MAX_LINE_LEN];
   int size = sizeof(buf);
   int result;

   memset(buf, 0, sizeof(buf));

   result = rwscoli_uds_recv(fd, buf, &size, &rwscolish_uds_addr);
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

int rwscoli_send_cmd(int fd, int argc, char **argv, char *to)
{
   char buf[RWSCOLI_UDS_MAX_LINE_LEN];
   int len    = 0;
   struct sockaddr_un sa_un;

   memset(buf, 0, sizeof(buf));
   len = rwscoli_args_len(argc, argv);
   rwscoli_pack_args(argc, argv, buf, &len);

   memset(&sa_un, 0, sizeof(sa_un));
   sa_un.sun_family = AF_UNIX;
   strcpy(sa_un.sun_path, to);

   return rwscoli_uds_send(fd, buf, len, &sa_un);
}

void rwscoli_wait_cmd_end(int fd, int timeout, void (*cb)(char *buf, int len))
{
   char buf[RWSCOLI_UDS_MAX_LINE_LEN];
   uint32_t endmark = RWSCOLI_UDS_CMD_ENDMARK;
   int  result = 0;

   do {
      memset(buf, 0, sizeof(buf));
      int len = sizeof(buf) - 1;
      result = rwscoli_uds_recv(fd, buf, &len, NULL);
      if (result < 0 || (memcmp(buf, &endmark, sizeof(endmark)) == 0)) {
         break;
      }

      cb(buf, len);
   } while (1);

   return;
}

void rwscoli_uds_print(char *buf, int size)
{
   rwscoli_uds_send(rwscoli_uds_fd, buf, size, &rwscolish_uds_addr);
   return;
}

void rwscoli_uds_cmd_end()
{
   uint32_t endmark = RWSCOLI_UDS_CMD_ENDMARK;
   rwscoli_uds_print((char*)&endmark, sizeof(endmark));
   return;
}

static int rwscoli_uds_create_socket(char *path)
{
   int fd;
   struct sockaddr_un sa_un;

   if (strlen(path) >= sizeof(sa_un.sun_path)) {
      return -1;
   }

   unlink(path);

   fd = socket(PF_UNIX, SOCK_DGRAM, 0);
   if (fd < 0) {
      return -1;
   }

   memset(&sa_un, 0, sizeof(sa_un));
   sa_un.sun_family = AF_UNIX;
   strcpy(sa_un.sun_path, path);

   if (bind(fd, (struct sockaddr *)&sa_un, sizeof(sa_un)) < 0) {
      close(fd);
      return -1;
   }

   if (chmod(path, S_IRWXU|S_IRWXG|S_IRWXO) < 0) {
      close(fd);
      unlink(path);
      return -1;
   }

   return fd;
}

static int rwscoli_uds_close_socket(int fd)
{
   close(fd);
   //unlink(RWSCOLI_UDS_SH_PATH);
   return 0;
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
      /* The error message is not preferred sometimes */
      /* perror("sendmsg"); */
   }

   return result;
}

