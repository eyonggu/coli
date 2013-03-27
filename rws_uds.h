/**
 * @file rws_uds.h
 * @brief 
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 1.0
 * @date 2013-03-27
 */

#ifndef RWS_UDS_H 
#define RWS_UDS_H

extern int rwscoli_uds_publish(char *path);
extern int rwscoli_uds_recv_cmd(char *buf, int *size);
extern int rwscoli_uds_send_cmd_rsp(char *buf, int size);

extern int rwscoli_uds_send_cmd(char *path, char *buf, int size);
extern int rwscoli_uds_recv_cmd_rsp(char *buf, int *size);
#endif /* RWS_UDS_H */
