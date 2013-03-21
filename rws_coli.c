/**
 * @file rws_coli.c
 * @brief COLI command mechanism.
 * @author eyonggu
 * @version 1.0
 * @date 2013-02-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include "rws_uds.h"
#include "rws_coli.h"

#define RWSCOLI_MAX_CMD_ITEMS            64
#define RWSCOLI_MAX_PRINT_STR_LEN        512
#define RWSCOLI_PARAM_ITEM_TAG_LEN       8
#define RWSCOLI_MAX_LINE_LENGTH          4096
#define RWSCOLI_CMD_ENDMARK              0xDEADBEEF

union rwscoli_param_value {
   int integer;
   char *string;
   struct sockaddr_storage addr;
   unsigned long long long_integer;
};

struct rwscoli_param {
   struct rwscoli_param *next;
   char tag[RWSCOLI_PARAM_ITEM_TAG_LEN];
   int type;
   union rwscoli_param_value value;
};

struct rwscoli_cmd {
   char *name;
   struct rwscoli_cmd  *next;
   struct rwscoli_cmd  *children;
   int is_command;
   char *syntax;
   struct {char *tag; int type;} params[RWSCOLI_MAX_PARAMS];
   void (*cmd_cb) (struct rwscoli_param*);
};

static uint32_t rwscoli_next_free_cmd_node = 0;
static struct rwscoli_cmd free_cmd_nodes[RWSCOLI_MAX_CMD_ITEMS]; 
static struct rwscoli_cmd rwscoli_cmd_tree_root;

static void rwscoli_print_syntax(struct rwscoli_cmd *cmd)
{
   if (cmd->is_command)
      rwscoli_printf("%s\n", cmd->syntax);

   cmd = cmd->children;
   while (cmd) {
      rwscoli_print_syntax(cmd);
      cmd = cmd->next;
   }
}

static void rwscoli_free_params(struct rwscoli_param *params)
{
   struct rwscoli_param *param, *tmp;

   if (!params) {
      return;
   }

   for (param = params; param;) {
      if ((param->type == RWSCOLI_STRING) && (param->value.string)) {
         free(param->value.string);
      }

      tmp = param->next;
      free(param);
      param = tmp;
   }
}


static struct rwscoli_param *rwscoli_get_param(struct rwscoli_param *params, char* tag)
{
   struct rwscoli_param *param = params;

   while (param) {
      if (!strcmp(param->tag, tag)) {
         break;
      }
      param = param->next;
   }

   return param;
}

static int rwscoli_str2laddr(char *str, unsigned char *laddr)
{
   int c = 0;
   char buf[64];
   char *s, *t;

   if (!str || !laddr)
      return -1;

   strncpy(buf, str, 64);
   while ((t = strchr(buf, '-')) != NULL) 
      *t = ':';

   s = buf;
   while ((t = strchr(s, ':')) != NULL) {
      *t = 0;
      if (c > 5)
         return -1;
      laddr[c++] = strtoul(s, NULL, 16);
      s = t + 1;
   }

   if (c < 5)
      return -1;
   laddr[c] = strtoul(s, NULL, 16);

   return 0;
}

static int rwscoli_str2sockaddr(char *str, struct sockaddr_storage *ss)
{

/* TODO: haven't found equivilent structure in Linux */
#define	AF_LINK		18		/* Link layer interface */
struct sockaddr_dl {
	uint8_t	    sdl_len;	/* Total length of sockaddr */
	sa_family_t sdl_family;	/* AF_LINK */
	uint16_t   sdl_index;	/* if != 0, system given index for interface */
	uint8_t	    sdl_type;	/* interface type */
	uint8_t	    sdl_nlen;	/* interface name length, no trailing 0 reqd. */
	uint8_t	    sdl_alen;	/* link level address length */
	uint8_t	    sdl_slen;	/* link layer selector length */
	char	    sdl_data[12]; /* minimum work area, can be larger;
				     contains both if name and ll address */
};
#define LLADDR(s) ((uint8_t *)((s)->sdl_data + (s)->sdl_nlen))

   struct sockaddr_dl *sdl;
   struct sockaddr_in *sin;
   struct sockaddr_in6 *sin6;

   if (strstr(str, ":")) {
      sin6 = (struct sockaddr_in6 *)ss;
      memset(sin6, 0, sizeof(*sin6));

      /* sin6->sin6_len = sizeof(*sin6); */
      sin6->sin6_family = AF_INET6;

      return (inet_pton(AF_INET6, str, &sin6->sin6_addr) == 1) ? 0 : -1;
   } else if (strstr(str, "-")) {
      sdl = (struct sockaddr_dl *)ss;
      memset(sdl, 0, sizeof(*sdl));

      sdl->sdl_len = sizeof(*sdl);
      sdl->sdl_family = AF_LINK;
      sdl->sdl_alen = 6;

      return rwscoli_str2laddr(str, LLADDR(sdl));
   } else {
      sin = (struct sockaddr_in *)ss;
      memset(sin, 0, sizeof(*sin));
      /* sin->sin_len = sizeof(*sin); */
      sin->sin_family = AF_INET;

      return (inet_pton(AF_INET, str, &sin->sin_addr) == 1) ? 0 : -1;
   }

   return 0;
}

static struct rwscoli_param *rwscoli_parse_params(int argc, char* argv[], struct rwscoli_cmd *cmd)
{
   struct rwscoli_param *param, *head, **tail;
   int i, j, f;

   head = 0;
   tail = &head;

   i = 0;
   while (i < argc) {
      for (f = 0, j = 0; j < RWSCOLI_MAX_PARAMS; j++) {
         if (!cmd->params[j].tag)
            break;

         if (!strcmp(argv[i], cmd->params[j].tag)) {
            if (i + 1 == argc && cmd->params[j].type != RWSCOLI_FLAG) {
               rwscoli_printf("unexpected number of parameters\n");
               rwscoli_free_params(head);
               rwscoli_print_syntax(cmd);
               return NULL;
            }

            param = (struct rwscoli_param *)malloc(sizeof(*param));
            memset(param, 0, sizeof(*param));

            strcpy(param->tag, argv[i]);
            param->type = cmd->params[j].type;

            f = 1;
            switch (cmd->params[j].type) {
               case RWSCOLI_INTEGER:
                  param->value.integer = strtol(argv[i + 1], NULL, 10);
                  break;
               case RWSCOLI_HEXINTEGER:
                  param->value.integer = strtol(argv[i + 1], NULL, 16);
                  break;
               case RWSCOLI_STRING:
                  param->value.string = (char *)malloc(strlen(argv[i + 1]) + 1);

                  strcpy(param->value.string, argv[i + 1]);
                  break;
               case RWSCOLI_ADDRESS:
                  if (rwscoli_str2sockaddr(argv[i + 1], &param->value.addr) < 0) {
                     rwscoli_printf("unsupported address format (%s)\n", argv[i + 1]);
                     f = 3; /* error */						
                  }
                  break;
               case RWSCOLI_FLAG:
                  f = 2; /* no value */
                  break;
               case RWSCOLI_LONGINTEGER:
                  param->value.long_integer = strtoull(argv[i + 1], (char**)NULL, 10);
                  break;
               default:
                  rwscoli_printf("unexpected parameter type (%u)\n", cmd->params[j].type);
                  break;
            }

            *tail = param;
            tail = &(*tail)->next;
            break;
         }
      }

      switch (f) {
         case 0: /* tag is not found */
            rwscoli_printf("unexpected parameter tag (%s)\n", argv[i]);
            /* fall */
         case 3: /* error */			
            rwscoli_free_params(head);
            rwscoli_print_syntax(cmd);
            return NULL;
         case 1: /* tag + value */
            i++;
            break;
         case 2: /* tag without value */
            break;
         default:
            break;
      }		

      i++;
   }

   if (!head) {
      /* params must not be empty, add dummy */
      param = (struct rwscoli_param *)malloc(sizeof(*param));
      memset(param, 0, sizeof(*param));
      param->type = RWSCOLI_UNDEFINED;
      strcpy(param->tag, "_dummy_");
      *tail = param;
      tail = &(*tail)->next;
   }

   return head;
}

int rwscoli_get_flag(struct rwscoli_param *params, char *tag)
{
   if (rwscoli_get_param(params, tag))
      return 1;

   return 0;
}

int rwscoli_get_int(struct rwscoli_param *params, char *tag, int def)
{
   struct rwscoli_param *param;

   param = rwscoli_get_param(params, tag);
   if (!param || 
         (param->type != RWSCOLI_INTEGER && param->type != RWSCOLI_HEXINTEGER))
      return def;

   return param->value.integer;
}

char *rwscoli_get_str(struct rwscoli_param *params, char *tag, char *def)
{
   struct rwscoli_param *param;

   param = rwscoli_get_param(params, tag);
   if (!param || (param->type != RWSCOLI_STRING))
      return def;

   return param->value.string;
}

struct sockaddr *rwscoli_get_addr(struct rwscoli_param *params, char *tag, struct sockaddr *def)
{
   struct rwscoli_param *param;

   param = rwscoli_get_param(params, tag);
   if (!param || (param->type != RWSCOLI_ADDRESS)) 
      return def;

   return (struct sockaddr *) &param->value.addr;
}

unsigned long long rwscoli_get_llu(struct rwscoli_param *params, char *tag, unsigned long long def)
{
   struct rwscoli_param *param;

   param = rwscoli_get_param(params, tag);
   if (!param || (param->type != RWSCOLI_LONGINTEGER))
      return def;

   return param->value.long_integer;
} 

static void rwscoli_print_(char* buf, int size)
{
   rwscoli_uds_send_cmd_rsp(buf, size);
   return;
}

void rwscoli_printb(char *buf, int size)
{
   rwscoli_print_(buf, size);
   return;
}

void rwscoli_printf(char *fmt, ...)
{
   va_list ap;
   char buf[RWSCOLI_MAX_PRINT_STR_LEN];
   int  size;

   va_start(ap, fmt);
   size = vsnprintf(buf, RWSCOLI_MAX_PRINT_STR_LEN, fmt, ap);
   va_end(ap);	

   rwscoli_print_(buf, size);
   return;
}

static struct rwscoli_cmd *rwscoli_create_cmd(char *name, struct rwscoli_cmd *parent)
{
   struct rwscoli_cmd *cmd;

   if (rwscoli_next_free_cmd_node == RWSCOLI_MAX_CMD_ITEMS)
   {
      return NULL;
   }

   cmd = &free_cmd_nodes[rwscoli_next_free_cmd_node++];
   cmd->name = name;
   cmd->children = NULL;
   cmd->next = parent->children;
   parent->children = cmd;

   cmd->is_command = 0;

   return cmd;
}


static struct rwscoli_cmd *rwscoli_find_cmd(struct rwscoli_cmd *list, char *name)
{
   while (list) {
      if (!strcmp(list->name, name))
         break;
      list = list->next;
   }

   return list;
}

static void rwscoli_end_cmd()
{
   /* TODO: send a endmark ? */
   uint32_t endmark = RWSCOLI_CMD_ENDMARK;
   rwscoli_printb((char*)&endmark, sizeof(endmark));
}


void rwscoli_init(char *cmd_name)
{
   /* TODO: support multiple commands in one application */
   rwscoli_create_cmd(cmd_name, &rwscoli_cmd_tree_root);
}

void rwscoli_uninit()
{
   /* TODO: free memory */
}


void rwscoli_register_cmd(struct rwscoli_command *command)
{
   int cl = 0;
   int collision = 1;
   struct rwscoli_cmd *parent = &rwscoli_cmd_tree_root;
   struct rwscoli_cmd *cmd = NULL;

   for (cl = 0; cl < RWSCOLI_MAX_LEVELS; cl++) {
      if (command->path[cl] == NULL)
         break;

      cmd = rwscoli_find_cmd(parent->children, command->path[cl]);
      if (!cmd) {
         collision = 0;
         break;
      } 

      parent = cmd;
   }

   if (collision) {
      if (cmd && !cmd->is_command) {
         cmd->syntax = command->syntax;
         cmd->cmd_cb = command->cmd_cb;
         memcpy(cmd->params, command->params, sizeof(command->params));
         cmd->is_command = 1;
      }
      return;
   }

   for (; cl < RWSCOLI_MAX_LEVELS; cl++) {
      if (NULL == (cmd = rwscoli_create_cmd(command->path[cl], parent))) {
         return;
      }

      if ((RWSCOLI_MAX_LEVELS == cl + 1) || (command->path[cl + 1] == NULL)) {
         cmd->syntax = command->syntax;
         cmd->cmd_cb = command->cmd_cb;
         memcpy(cmd->params, command->params, sizeof(command->params));
         cmd->is_command = 1;
         break;
      }

      parent = cmd;
   }
}

int rwscoli_publish(char *name)
{
   /* TODO: support other IPC? */
   return rwscoli_uds_publish(name);
}

int rwscoli_recv_cmd(int *argc, char ***argv)
{
   static char buf[RWSCOLI_MAX_LINE_LENGTH];
   int size = sizeof(buf);
   int result;

   memset(buf, 0, sizeof(buf));

   result = rwscoli_uds_recv_cmd(buf, &size);
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

int rwscoli_send_cmd(int argc, char **argv)
{
   int result;
   char buf[RWSCOLI_MAX_LINE_LENGTH];
   int len = rwscoli_args_len(argc, argv);

   memset(buf, 0, sizeof(buf));
   rwscoli_pack_args(argc, argv, buf, &len);

   result = rwscoli_uds_send_cmd("/tmp/dpsd", buf, len);
   if (result < 0) {
      fprintf(stderr, "rwscoli_send_cmd() failed!\n");
      return -1;
   }

   return 0;
}

void rwscoli_wait_cmd_end()
{
   char buf[RWSCOLI_MAX_LINE_LENGTH];
   int  result = 0;
   do {
      memset(buf, 0, sizeof(buf));
      int len = sizeof(buf) - 1;
      result = rwscoli_uds_recv_cmd_rsp(buf, &len);
      if (result < 0 || *(uint32_t*)buf == RWSCOLI_CMD_ENDMARK) {
         break;
      }

      buf[len] = '\0';
      fprintf(stderr, "%s", buf);
      fflush(stderr);
   } while (1);

   return;
}

void rwscoli_exec_cmd(int argc, char* argv[])
{
   int cl = 0;
   struct rwscoli_cmd *cmd = &rwscoli_cmd_tree_root;
   struct rwscoli_cmd *tmp;
   struct rwscoli_param *params;

   for (cl = 0; cl < RWSCOLI_MAX_LEVELS; cl++) {
      if (argc == cl)
         break;

      tmp = rwscoli_find_cmd(cmd->children, argv[cl]);
      if (!tmp)
         break;
      cmd = tmp;
   }

   if (cmd->is_command) {
      params = rwscoli_parse_params(argc - cl, &argv[cl], cmd);
      if (!params) {
         goto exec_cmd_end;
      }

      cmd->cmd_cb(params);

      rwscoli_free_params(params);
   } else {
      rwscoli_printf("Supported commands:\n");
      rwscoli_print_syntax(cmd);
   }

exec_cmd_end:
   rwscoli_end_cmd();
   free(argv);
   return;
}


