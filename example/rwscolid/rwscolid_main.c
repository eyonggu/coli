#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rws_coli.h"

void hello_world_cc(struct rwscoli_param *params)
{	
   (void)params;
   rwscoli_printf("hello world\n");
}

void hej_varld_cc(struct rwscoli_param *params)
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
	struct rwscoli_command cc_hello_world = { 
		"rws hello world",
		{"rws", "hello", "world", 0, 0},
		{
                   {0, 0}
		},
		hello_world_cc
	};

        /* coli command with one parameter of type string */
	struct rwscoli_command cc_hej_varld = {
		"rws hej varld -i <name>",
		{"rws", "hej", "varld", 0, 0},
		{
                   {"-i", RWSCOLI_STRING},
                   {0, 0}
		},
		hej_varld_cc
	};
	
	rwscoli_register_cmd(&cc_hej_varld);
	rwscoli_register_cmd(&cc_hello_world);
}


int main()
{
   int    argc = 0;
   char **argv = NULL;
   int    result;

   rwscoli_init("rws", "");

   setup_colis();

   rwscoli_publish(RWSCOLI_UNIX);

   while (1) {
      result = rwscoli_recv_cmd(&argc, &argv);
      if (result < 0) {
         break;
      }

      rwscoli_exec_cmd(argc, argv);
   }
   return 0;
}
