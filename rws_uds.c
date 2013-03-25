#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>

static int rwscoli_uds_fd = -1;
static struct sockaddr_un rwscoli_sh_sa_un;

/*TODO List:
 * 1) heartbeat to rwscolid to get cmd/syntax
 */

/*======================rwscolid part===================*/

#define RWSCOLI_UDS_PATH "/tmp/rwscoli/"

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

int rwscoli_uds_recv_cmd(char *buf, int *size)
{
   struct msghdr msg;
   struct iovec iov;
   int result;
   
   iov.iov_base = buf;
   iov.iov_len = *size;
   
   msg.msg_name = &rwscoli_sh_sa_un;
   msg.msg_namelen = sizeof(rwscoli_sh_sa_un);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = NULL;
   msg.msg_controllen = 0; 
   
   result = recvmsg(rwscoli_uds_fd, &msg, 0);

   if (result == -1) {
      perror("rwscoli_uds_recv_cmd");
      return -1;
   }

   *size = result;
   return result;
}

int rwscoli_uds_send_cmd_rsp(char *buf, int size)
{
   struct msghdr msg;
   struct iovec iov;
   int result;
   
   iov.iov_base = buf;
   iov.iov_len = size;
   
   msg.msg_name = &rwscoli_sh_sa_un;
   msg.msg_namelen = sizeof(rwscoli_sh_sa_un);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = NULL;
   msg.msg_controllen = 0; 

   result = sendmsg(rwscoli_uds_fd, &msg, 0);
   if (result == -1) {
      perror("rwscoli_uds_send_cmd_rsp");
      return -1;
   }

   return result;
}

/*=========================rwscolish part=================*/
#define RWSCOLI_UDS_SH_PATH "/tmp/.rwscolish"
#define RWSCOLI_UDPS_CMD_MAX  16

struct rwscoli_uds_cmd
{
   char *name;
   char *path;
   char *short_descr;
   char *descr;
};
static struct rwscoli_uds_cmd rwscoli_uds_cmds[RWSCOLI_UDPS_CMD_MAX];

void rwscoli_uds_print_cmd_list()
{
   printf("%-16s%s\n", "Command", "Description");
   for (int i = 0; i < RWSCOLI_UDPS_CMD_MAX; i++) {
      if (rwscoli_uds_cmds[i].name != NULL) {
         printf("%-16s%s\n", rwscoli_uds_cmds[i].name, rwscoli_uds_cmds[i].descr);
      }
   }

   return;
}

static int rwscoli_uds_init_cmd_list()
{
   struct dirent *file;
   DIR *dir = opendir(RWSCOLI_UDS_PATH);
   if (dir == NULL) {
      fprintf(stderr, "error opendir %s!\n", RWSCOLI_UDS_PATH);
      return -1;
   }
   while((file=readdir(dir)) != NULL) {
      if (file->d_type == DT_SOCK) {
         for (int i = 0; i < RWSCOLI_UDPS_CMD_MAX; i++) {
            if (rwscoli_uds_cmds[i].name == NULL) {
               rwscoli_uds_cmds[i].name = strdup(file->d_name);
               char path[64];
               snprintf(path, 64, "%s%s", RWSCOLI_UDS_PATH, file->d_name);
               rwscoli_uds_cmds[i].path = strdup(path);
               break;
            }
         }
      }
   }

   return 0;
}

int rwscoli_uds_init()
{
   int fd;
   struct sockaddr_un sa_un;

   rwscoli_uds_init_cmd_list();

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

   rwscoli_uds_fd = fd;
   return fd;
}

static char *rwscoli_uds_find_cmd_path(char *cmd)
{
   for (int i = 0; i < RWSCOLI_UDPS_CMD_MAX; i++) {
      if (rwscoli_uds_cmds[i].name && 
           strcmp(cmd, rwscoli_uds_cmds[i].name) == 0 ) {
         return rwscoli_uds_cmds[i].path;
      }
   }
   
   rwscoli_uds_print_cmd_list();
   return NULL;
}

int rwscoli_uds_send_cmd(char *cmd, char *buf, int size)
{
   struct msghdr msg;
   struct iovec vec; 
   struct sockaddr_un sa_un;
   char *path;
   int result;

   path = rwscoli_uds_find_cmd_path(cmd);
   if (path == NULL) {
      return -1;
   }

   memset(&sa_un, 0, sizeof(sa_un));
   sa_un.sun_family = AF_UNIX;
   strcpy(sa_un.sun_path, path);

   vec.iov_base = buf;
   vec.iov_len = size;

   msg.msg_name = (struct sockaddr*)&sa_un;
   msg.msg_namelen = sizeof(sa_un);
   msg.msg_iov = &vec;
   msg.msg_iovlen = 1;
   msg.msg_control = NULL;
   msg.msg_controllen = 0;
   msg.msg_flags = 0;
   
   result = sendmsg(rwscoli_uds_fd, &msg, 0);
   if (result < 0) {
      perror("rwscoli_uds_send_cmd");
   }

   return result;
}

int rwscoli_uds_recv_cmd_rsp(char *buf, int *size)
{
   struct msghdr msg;
   struct iovec iov;
   int result;
   
   iov.iov_base = buf;
   iov.iov_len = *size;
   
   msg.msg_name = 0;
   msg.msg_namelen = 0;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control = NULL;
   msg.msg_controllen = 0; 
   
   result = recvmsg(rwscoli_uds_fd, &msg, 0);
   if (result < -1) {
      perror("rwscoli_uds_recv_cmd_rsp");
      return -1;
   }

   *size = result;
   return result;

}

