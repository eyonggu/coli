/**
 * @file rws_args.c
 * @brief Helper functions for passing command 
 *        from rwscolish to rwscolid
 * @author Yong Gu (yong.g.gu@ericsson.com)
 * @version 1.0
 * @date 2013-02-15
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>

int rwscoli_args_len(int argc, char **argv)
{
   int   i;
   int   len;
   
   /*
   **  Add size of c string end marks and a trailing endmark
   */
   len = argc + 1;
   
   /*
   **  Add size of c strings
   */
   for (i = 0; i < argc; i++)
      len += strlen(argv[i]);
   
   return len;
}

int rwscoli_pack_args(int argc, char **argv, char *buf, int *size)
{
   int   i;
   char  *ptr;
   int   len;
   
   len = rwscoli_args_len(argc, argv);

   *size = len <= *size ? len : 0;

   if ( *size == 0 )
      return -1;
   
   ptr = buf;

   for (i = 0; i < argc; i++) {
      strcpy(ptr, argv[i]);
      ptr = &ptr[strlen(ptr)];
      *ptr++ = '\0';
   }
   
   *ptr++ = '\0';
  
   return 0;
}


int rwscoli_unpack_args(char *buf, int size, int *argc, char ***argv)
{
   int   i;
   int   num = 0;
   char *ptr = buf;

   if ((size == 0) || (buf[size-1] != '\0') ||
         ((size > 1) && (buf[size-2] != '\0'))) {
      return -1;
   }

   while (strlen(ptr) > 0) {
      ptr = &ptr[strlen(ptr) + 1];
      num++;
   }

   *argc = num;
   *argv = malloc(*argc * sizeof(char *));

   ptr   = buf;
   for (i = 0; i < *argc; i++) {
      (*argv)[i] = ptr;
      ptr = &ptr[strlen(ptr) + 1];
   }

   return 0;
}
