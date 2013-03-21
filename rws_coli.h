/**
 * @file rws_coli.h
 * @brief 
 * @author eyonggu
 * @version 1.0
 * @date 2013-02-15
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

/*********** EXPORTS ***********/

extern void rwscoli_init(char *cmd_name);
extern int  rwscoli_publish(char *name);

extern void rwscoli_register_cmd(struct rwscoli_command *command);
extern void rwscoli_cmd_handler(char *buf, int size);

extern void rwscoli_print(char *, ...);
extern void rwscoli_print_buf(char *);

struct sockaddr;
extern int rwscoli_get_flag(struct rwscoli_param *params, char *tag);
extern int rwscoli_get_int(struct rwscoli_param *params, char *tag, int def);
extern char *rwscoli_get_str(struct rwscoli_param *params, char *tag, char *def);
extern struct sockaddr *rwscoli_get_addr(struct rwscoli_param *params, char *tag, struct sockaddr *def);
extern unsigned long long rwscoli_get_llu(struct rwscoli_param *params, char *tag, unsigned long long def);

extern int rwscoli_args_len(int argc, char **argv);
extern int rwscoli_pack_args(int argc, char **argv, char *buf, int *size);
extern int rwscoli_unpack_args(char *buf, int size, int *argc, char ***argv);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RWS_COLI_H */
