#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include <editline/readline.h>
#include <histedit.h>

#include "rws_coli.h"

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
   /* char *name; */

   if (!state) {
      list_index = 0;
      len = strlen (text);
   }

   /* TODO: 
   while (name = commands[list_index].name) {
      list_index++;
      if (strncmp (name, text, len) == 0)
         return (dupstr(name));
   }
   */

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
   rl_readline_name = "DPSSH";

   rl_attempted_completion_function = fileman_completion;
}

int main(int argc, char **argv)
{
   char *line, *s;
   Tokenizer *tok;
   int   local_argc;
   char  **local_argv;

   (void)argc;
   (void)argv;

   initialize_readline();
   tok = tok_init(NULL);

   stifle_history(7);

   rwscoli_uds_init();

   while (1) {
      line = readline("$:");

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

            /* TODO: check if it is local or remote command */
            if (tok_str(tok, line, &local_argc, (const char ***)&local_argv) == 0) {
               result = rwscoli_send_cmd(local_argc, local_argv);
               if (result >= 0) {
                  rwscoli_wait_cmd_end();
               }
            }
         }

         free(expansion);
      }

      free(line);
      tok_reset(tok);
   }

   tok_end(tok);
   return 0;
}
