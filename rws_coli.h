/**
 * @file rws_coli.h
 * @brief
 * @author eyonggu
 * @version 2.0
 * @date 2013-05-15
 */

#ifndef RWS_COLI_H
#define RWS_COLI_H

#ifdef __cplusplus
extern "C" {
#endif

/*********** DEFINES ***********/

#define RWSCOLI_MAX_PARAMS           16
#define RWSCOLI_MAX_LEVELS           5

/*********** TYPES ***********/

enum {
	RWSCOLI_UNDEFINED = 0,
	RWSCOLI_INTEGER,
	RWSCOLI_HEXINTEGER,
	RWSCOLI_STRING,
	RWSCOLI_ADDRESS,
	RWSCOLI_FLAG, /* no value */
	RWSCOLI_LONGINTEGER
};

struct rwscoli_param;

struct rwscoli_command {
	char *syntax;
	char *path[RWSCOLI_MAX_LEVELS];
	struct {char *tag; int type;} params[RWSCOLI_MAX_PARAMS];
	void (*cmd_cb) (struct rwscoli_param*);
};

enum {
        RWSCOLI_LOCAL = 0,
        RWSCOLI_UNIX,
        RWSCOLI_INET, /* not implemented yet */
        RWSCOLI_IPC_MAX
};

/*********** EXPORTS ***********/

void rwscoli_init(int ipcflag);

void rwscoli_register_cmd(struct rwscoli_command *command);
void rwscoli_exec_cmd(int argc, char* argv[]);

void rwscoli_printf(char *fmt, ...);
void rwscoli_printb(char *buf, int size);

struct sockaddr;
int                rwscoli_get_flag(struct rwscoli_param *params, char *tag);
int                rwscoli_get_int(struct rwscoli_param *params, char *tag, int def);
char              *rwscoli_get_str(struct rwscoli_param *params, char *tag, char *def);
struct sockaddr   *rwscoli_get_addr(struct rwscoli_param *params, char *tag, struct sockaddr *def);
unsigned long long rwscoli_get_llu(struct rwscoli_param *params, char *tag, unsigned long long def);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RWS_COLI_H */
