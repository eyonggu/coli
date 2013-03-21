#ifndef RWS_UDS_H 
#define RWS_UDS_H

int rwscoli_uds_publish(char *path);
int rwscoli_uds_recv_cmd(int fd, char *buf, int *size);

#endif /* RWS_UDS_H */
