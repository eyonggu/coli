/**
 * @file rws_uds.h
 * @brief
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 2.0
 * @date 2013-05-17
 */

#ifndef RWS_UDS_H
#define RWS_UDS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RWSCOLI_UDS_PATH         "/tmp/rwscoli/"

void rwscoli_uds_print(char *buf, int size);

void rwscoli_uds_cmd_end();

int rwscoli_publish(char *name);

int rwscoli_recv_cmd(int *argc, char ***argv);

int rwscoli_send_cmd(int fd, int argc, char **argv, char *to);

void rwscoli_wait_cmd_end(int fd, int timeout, void (*cb)(char *buf, int len));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RWS_UDS_H */
