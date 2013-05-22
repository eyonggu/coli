/**
 * @file rwscolish_cmd.c
 * @brief
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 2.0
 * @date 2013-05-21
 */

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rws_coli.h"
#include "rws_uds.h"
#include "rwscolish_cmd.h"

#define RWSCOLISH_CMD_MAX      64
#define RWSCOLISH_UDS_PATH     "/tmp/.rwscolish"

struct rwscolish_cmd rwscolish_cmds[RWSCOLISH_CMD_MAX+1];
static int rwscolish_uds_fd;

static char *rwscolish_fetch_cmd_path;

static int rwscolish_create_socket(char *path)
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

static void rwscolish_filter_remote_cmds(char *buf, int len)
{
   int i;
   char *s;

   /* skip Usage line */
   if (strncmp(buf, "Usage", strlen("Usage")) == 0) {
      return;
   }

   /* extract cmd name */
   s = strchr(buf, ' ');
   if (s != NULL) {
      *s = '\0';
   } else {
      buf[len] = '\0';
   }

   if (rwscolish_find_cmd(buf) == NULL) {
      for (i = 0; i < RWSCOLISH_CMD_MAX; i++) {
         if (rwscolish_cmds[i].name != NULL) {
            continue;
         }

         rwscolish_cmds[i].name = strdup(buf);
         rwscolish_cmds[i].path = strdup(rwscolish_fetch_cmd_path);
         break;
      }
   }
}

static int rwscolish_fetch_remote_cmds(char *path)
{
   int result;
   char *cmd_hello[1];
   cmd_hello[0] = "rwscolish_hello";

   result = rwscoli_send_cmd(rwscolish_uds_fd, 1, cmd_hello, path);
   if (result >= 0) {
      rwscolish_fetch_cmd_path = path;
      rwscoli_wait_cmd_end(rwscolish_uds_fd, 1, rwscolish_filter_remote_cmds);
   }

   return 0;
}


static int rwscolish_remote_cmd_init()
{
   DIR           *dir;
   struct dirent *file;

   /* find commands and path */
   dir = opendir(RWSCOLI_UDS_PATH);
   if (dir == NULL) {
      fprintf(stderr, "error opendir %s!\n", RWSCOLI_UDS_PATH);
      return -1;
   }

   while((file=readdir(dir)) != NULL) {
      if (file->d_type == DT_SOCK) {
         char path[64];
         snprintf(path, 64, "%s%s", RWSCOLI_UDS_PATH, file->d_name);
         rwscolish_fetch_remote_cmds(path);
      }
   }

   closedir(dir);
   return 0;
}

void rwscolish_print_cmd_rsp(char *buf, int len)
{
   assert(buf != NULL);

   buf[len] = '\0';
   fprintf(stderr, "%s", buf);
   fflush(stderr);
   return;
}


struct rwscolish_cmd *rwscolish_find_cmd(char *cmd)
{
   int i;
   for (i = 0; i < RWSCOLISH_CMD_MAX; i++) {
      if (rwscolish_cmds[i].name &&
           strcmp(cmd, rwscolish_cmds[i].name) == 0 ) {
         return &rwscolish_cmds[i];
      }
   }

   return NULL;
}

void rwscolish_print_cmds()
{
   int i;

   fprintf(stderr, "Supported commands:\n");
   for (i = 0; i < RWSCOLISH_CMD_MAX; i++) {
      if (rwscolish_cmds[i].name != NULL) {
         fprintf(stderr, "%s\n", rwscolish_cmds[i].name);
      }
   }
   return;
}

int rwscolish_init()
{
   int result = 0;

   result = rwscolish_create_socket(RWSCOLISH_UDS_PATH);
   if (result < 0) {
      return -1;
   }

   rwscolish_uds_fd = result;

   rwscolish_register_local_cmds();

   rwscolish_remote_cmd_init();

   return result;
}


void rwscolish_exit_cc(struct rwscoli_param *params)
{
   (void)params;
   exit(0);
}

void rwscolish_help_cc(struct rwscoli_param *params)
{
   (void)params;
   rwscolish_print_cmds();
}

void rwscolish_register_local_cmds()
{
   rwscoli_init(RWSCOLI_LOCAL);

   struct rwscoli_command cc_exit = {
      "exit",
      {"exit", 0, 0, 0, 0},
      {
         {0, 0}
      },
      rwscolish_exit_cc
   };

   struct rwscoli_command cc_help = {
      "help",
      {"help", 0, 0, 0, 0},
      {
         {0, 0}
      },
      rwscolish_help_cc
   };

   rwscoli_register_cmd(&cc_exit);
   rwscoli_register_cmd(&cc_help);

   /* add to cmd list */
   rwscolish_cmds[0].name = "exit";
   rwscolish_cmds[1].name = "help";
}

