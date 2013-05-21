/**
 * @file rwscolish_cmd.h
 * @brief
 * @author  Yong Gu (yong.g.gu@ericsson.com)
 * @version 2.0
 * @date 2013-05-21
 */

#ifndef RWSCOLISH_CMD_H
#define RWSCOLISH_CMD_H

struct rwscolish_cmd
{
   char *name;
   char *path;
   char *descr;
};

int rwscolish_init();

struct rwscolish_cmd *rwscolish_find_cmd(char *cmd);

void rwscolish_print_cmd_rsp(char *buf, int len);

#endif /* RWSCOLISH_CMD_H */
