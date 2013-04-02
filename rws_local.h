/**
 * @file rws_local.h
 * @brief 
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 1.0
 * @date 2013-04-02
 */

#ifndef RWS_LOCAL_H
#define RWS_LOCAL_H

extern int rwscoli_local_publish(char *name);

extern void rwscoli_local_print(char* buf, int size);

extern int rwscoli_local_recv(char *buf, int *size);

extern void rwscoli_local_end_cmd();

#endif /* RWS_LOCAL_H */
