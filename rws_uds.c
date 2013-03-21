#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

static int rwscoli_uds_fd = -1;

static struct sockaddr_un rwscoli_sh_sa_un;

/*======================sshd part===================*/

int rwscoli_uds_publish(char *path)
{
   struct sockaddr_un sa_un;
   int fd;
  
   if (strlen(path) >= sizeof(sa_un.sun_path)) {
      return -1;
   }

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

/*=========================ssh part=================*/

int rwscoli_uds_init()
{
   int fd;
   struct sockaddr_un sa_un;

   unlink("/tmp/dpssh");

   fd = socket(PF_UNIX, SOCK_DGRAM, 0);
   if (fd < 0) {
      return -1;
   }

   memset(&sa_un, 0, sizeof(sa_un));
   sa_un.sun_family = AF_UNIX;
   strcpy(sa_un.sun_path, "/tmp/dpssh"); 

   if (bind(fd, (struct sockaddr *)&sa_un, sizeof(sa_un)) < 0) {
      close(fd);
      return -1;
   }

   rwscoli_uds_fd = fd;
   return fd;
}

int rwscoli_uds_send_cmd(char *path, char *buf, int size)
{
   struct msghdr msg;
   struct iovec vec; 
   int result;
   struct sockaddr_un sa_un;

   if (strlen(path) >= (sizeof(sa_un.sun_path) - 1)) {
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

