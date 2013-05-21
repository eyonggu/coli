/**
 * @file rwscolish_main.c
 * @brief
 * @author  Yong Gu (yong.g.gu@ericsson.com)
 * @version 2.0
 * @date 2013-05-21
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include <editline/readline.h>
#include <histedit.h>

#include "rws_coli.h"
#include "rws_uds.h"
#include "rwscolish_cmd.h"

#define RWSCOLISH_DEFAULT_PROMPT "[rws] -> "

extern struct rwscolish_cmd rwscolish_cmds[];

static char *dupstr(char *s)
{
   char *r;

   if ( (r = malloc(strlen(s) + 1)) != NULL )
      strcpy(r, s);

   return (r);
}

static char *stripwhite (char *string)
{
   register char *s, *t;

   for (s = string; isspace (*s); s++)
      ;

   if (*s == 0)
      return (s);

   t = s + strlen (s) - 1;
   while (t > s && isspace (*t))
      t--;
   *++t = '\0';

   return s;
}

static char *command_generator(const char *text, int state)
{
   static int list_index, len;
   char *name;

   if (!state) {
      list_index = 0;
      len = strlen (text);
   }

   while (name = rwscolish_cmds[list_index].name) {
      list_index++;
      if (strncmp (name, text, len) == 0)
         return (dupstr(name));
   }

   return ((char *)NULL);
}

static char **fileman_completion(const char *text, int start, int end)
{
   char **matches;
   (void)end;

   matches = (char **)NULL;

   if (start == 0)
      matches = completion_matches(text, command_generator);

   return (matches);
}

void initialize_readline()
{
   rl_readline_name = "RWSCOLISH";

   rl_attempted_completion_function = fileman_completion;
}

int main(int argc, char **argv)
{
   char *line, *s;
   Tokenizer *tok;

   (void)argc;
   (void)argv;

   initialize_readline();
   tok = tok_init(NULL);

   stifle_history(7);

   int fd = rwscolish_init();
   if (fd < 0) {
      fprintf(stderr, "rwscolish init failed!");
      exit(1);
   }

   while (1) {
      line = readline(RWSCOLISH_DEFAULT_PROMPT);

      if (!line) {
         break;
      }

      s = stripwhite(line);

      if (s && (strlen(s) > 0)) {
         char *expansion;
         int result;

         result = history_expand(s, &expansion);

         if (result < 0 || result == 2) {
            fprintf(stderr, "%s\n", expansion);
         } else {
            add_history(expansion);

            int   local_argc;
            char  **local_argv;
            if (tok_str(tok, line, &local_argc, (const char ***)&local_argv) == 0) {
               struct rwscolish_cmd *cmd = rwscolish_find_cmd(local_argv[0]);

               if (cmd == NULL) {
                  /* unknown command, list known commands */
                  rwscolish_print_cmds();
               } else {
                  if (cmd->path != NULL) {
                     /* remote command */
                     result = rwscoli_send_cmd(fd, local_argc, local_argv, cmd->path);
                     if (result >= 0) {
                        rwscoli_wait_cmd_end(fd, 1, rwscolish_print_cmd_rsp);
                     }
                  } else {
                     /* local command */
                     rwscoli_exec_cmd(local_argc, local_argv);
                  }
               } /* else if cmd... */
            } /* if (tok_str... */
         } /* else if (result... */

         free(expansion);
      }

      free(line);
      tok_reset(tok);
   }

   tok_end(tok);
   return 0;
}
