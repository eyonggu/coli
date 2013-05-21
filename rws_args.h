/**
 * @file rws_args.h
 * @brief
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 2.0
 * @date 2013-05-21
 */

#ifndef RWS_ARGS_H
#define RWS_ARGS_H

#ifdef __cplusplus
extern "C" {
#endif

int rwscoli_args_len(int argc, char **argv);

int rwscoli_pack_args(int argc, char **argv, char *buf, int *size);

int rwscoli_unpack_args(char *buf, int size, int *argc, char ***argv);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RWS_ARGS_H */
