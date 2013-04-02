/**
 * @file rws_uds.h
 * @brief 
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 1.0
 * @date 2013-03-27
 */

#ifndef RWS_UDS_H 
#define RWS_UDS_H

extern int rwscoli_uds_publish(char *name);

extern void rwscoli_uds_print(char *buf, int size);

extern void rwscoli_uds_end_cmd();

#endif /* RWS_UDS_H */
