/**
 * @file rws_coli.h
 * @brief
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 2.0
 * @date 2013-05-15
 */

#ifndef RWS_COLI_H
#define RWS_COLI_H

#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RWSCOLI_MAX_PARAMS               16
#define RWSCOLI_MAX_LEVELS               5
#define RWSCOLI_MAX_CMD_ITEMS            512
#define RWSCOLI_MAX_PRINT_STR_LEN        4096
#define RWSCOLI_PARAM_ITEM_TAG_LEN       16

enum {
	RWSCOLI_UNDEFINED = 0,
	RWSCOLI_INTEGER,
	RWSCOLI_HEXINTEGER,
	RWSCOLI_STRING,
	RWSCOLI_ADDRESS,
	RWSCOLI_FLAG, /* no value */
	RWSCOLI_LONGINTEGER
};

enum {
        RWSCOLI_LOCAL = 0,
        RWSCOLI_UNIX,
        RWSCOLI_INET, /* not implemented yet */
        RWSCOLI_IPC_MAX
};

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

struct rwscoli_command {
	char *syntax;
	char *path[RWSCOLI_MAX_LEVELS];
	struct {char *tag; int type;} params[RWSCOLI_MAX_PARAMS];
	void (*cmd_cb) (struct rwscoli_param*);
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

struct rwscoli {
   struct rwscoli_cmd cmd_tree_root;
   int    next_free_cmd_node;
   struct rwscoli_cmd free_cmd_nodes[RWSCOLI_MAX_CMD_ITEMS];
   int    uds_fd;
   char   uds_path[64];
   void (*print)(char* buf, int size);
   void (*cmd_end)();
};

void rwscoli_init(int ipcflag);

void rwscoli_register_cmd(struct rwscoli_command *command);
void rwscoli_exec_cmd(int argc, char* argv[]);

void rwscoli_printf(char *fmt, ...);
void rwscoli_printb(char *buf, int size);

int                rwscoli_get_flag(struct rwscoli_param *params, char *tag);
int                rwscoli_get_int(struct rwscoli_param *params, char *tag, int def);
char              *rwscoli_get_str(struct rwscoli_param *params, char *tag, char *def);
struct sockaddr   *rwscoli_get_addr(struct rwscoli_param *params, char *tag, struct sockaddr *def);
unsigned long long rwscoli_get_llu(struct rwscoli_param *params, char *tag, unsigned long long def);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RWS_COLI_H */
