#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rws_coli.h"

void cc_hello_world(struct rwscoli_param *params)
{	
   (void)params;
   rwscoli_printf("hello world\n");
}

void cc_hej_varld(struct rwscoli_param *params)
{
   char *name;

   name = rwscoli_get_str(params, "-i", NULL);
   if (name == NULL) {
      rwscoli_printf("name is not specified\n");
      return;
   }

   rwscoli_printf("hej värld: %s\n", name);
}

void setup_colis(void)
{
        /* coli command without any parameter */
	struct rwscoli_command hello_world_cc = { 
		"dps hello world",
		{"dps", "hello", "world", 0, 0},
		{
                   {0, 0}
		},
		cc_hello_world
	};

        /* coli command with one parameter of type string */
	struct rwscoli_command hej_varld_cc = {
		"dps hej varld -i <name>",
		{"dps", "hej", "varld", 0, 0},
		{
                   {"-i", RWSCOLI_STRING},
                   {0, 0}
		},
		cc_hej_varld
	};
	
	rwscoli_register_cmd(&hej_varld_cc);
	rwscoli_register_cmd(&hello_world_cc);
}


int main()
{
   int    argc = 0;
   char **argv = NULL;
   int    result;

   rwscoli_init("dps");

   setup_colis();

   rwscoli_publish("/tmp/dpsd");

   while (1) {
      result = rwscoli_recv_cmd(&argc, &argv);
      if (result < 0) {
         break;
      }

      rwscoli_exec_cmd(argc, argv);
   }
   return 0;
}
