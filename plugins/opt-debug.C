#include <iostream>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include "damaris/data/VariableManager.hpp"
#include "ross.h"

using namespace damaris;

extern "C" {

// rename the signals we're using so they're more descriptive
#define SIG_SEQ_STOP SIGUSR1
#define SIG_SEQ_RESUME SIGUSR2
#define MAX_CMD_LENGTH 8192

static void parent_handler(int child_pid);
static void child_handler(char *path, char **args);
static void opt_parse_mod(int argc_p, char *argv_p, char *path, char **args);

/* pulled from ROSS, so we can modify a bit */
static const tw_optdef *opt_groups[10];
static unsigned int opt_index = 0;

void opt_add(const tw_optdef *options)
{
	if(!options || !options->type)
		return;

	opt_groups[opt_index++] = options;
	opt_groups[opt_index] = NULL;
}

static int match_opt_mod(const char *arg)
{
	const char *eq = strchr(arg + 2, '=');
	const tw_optdef **group = opt_groups;

	for (; *group; group++)
	{
		const tw_optdef *def = *group;
		for (; def->type; def++)
		{
			if (!def->name || def->type == TWOPTTYPE_GROUP)
				continue;
			if (!eq && !strcmp(def->name, arg + 2))
			{
				return 1;
			}
			else if (eq && !strncmp(def->name, arg + 2, eq - arg - 2))
			{
				return 1;
			}
		}
	}
	return 0;
}

void opt_parse_mod(int argc_p, char *argv_p, char *path, char **args)
{
	unsigned i;
	int found, str_idx = 0;

	const char s[2] = " ";
    char *token;
	
    /* get the first token */
    token = strtok(argv_p, s);
    
    /* walk through other tokens */
    while(token != NULL) {
		// TODO will need to double check with CODES commands
		if (!strncmp(token, "--", 2) == 0)
		{
			strcpy(path, token);
			path[strlen(token)] = '\0';
		}
		found = match_opt_mod(token);
		if (!found)
		{
			strcpy(&args[str_idx][0], token);
			args[str_idx][strlen(token)] = '\0';
			str_idx++;
		}
        token = strtok(NULL, s);
    }
	args[str_idx] = NULL;
}
/* end of parsing functions */

/* damaris event for setting up sequential simulation
 * only needs to be called once
 * note: User args doesn't seem to be implemented in Damaris, 
 * so pull in args as regular Damaris variables
 */
void seq_init_event(const std::string& event, int32_t src, int32_t step, const char* args)
{
	int num_args;
	char *arg_str;

	// grab all the args from Damaris
    VariableManager::iterator it = VariableManager::Begin();
    VariableManager::iterator end = VariableManager::End();

    while (it != end)
    {
		if (it->get()->GetName().compare("opt_debug/num_sim_args") == 0)
		{
			//cout << "variable.GetName: " << it->get()->GetName() << endl;
			num_args = *(int*)it->get()->GetBlock(0, 0, 0)->GetDataSpace().GetData();
			//cout << "nbr items " << num_args << endl;

		}
		else if (it->get()->GetName().compare("opt_debug/sim_args") == 0)
		{
			//cout << "variable.GetName: " << it->get()->GetName() << endl;
			it->get()->GetBlock(0, 0, 0)->NbrOfItems();
			arg_str = (char*)(it->get()->GetBlock(0, 0, 0)->GetDataSpace().GetData());
			//cout << "arg str: ";
			//for (int i = 0; i < it->get()->GetBlock(0, 0, 0)->NbrOfItems(); i++)
			//	cout << arg_str[i];
			//cout << endl;
		}
        it++;
    }

	// now do some processing to get in the right format for exec
	// as well as removing damaris and instrumentation options from command
	char path[1024];
	char **cmd;
	cmd = (char**) calloc(25, sizeof(char*));
	for (int i = 0; i < 25; i++)
	{
		cmd[i] = (char*)calloc(1024, sizeof(char));
	}
	// grab these options, so we can remove them from our arg list
	// we don't want these options turned on in our sequential run
    opt_add(st_inst_opts());
    opt_add(st_damaris_opts(NULL, NULL));
    opt_add(st_special_lp_opts());
	opt_parse_mod(num_args, arg_str, &path[0], cmd);
 
    // using fork/exec to create child process that will handle the seq simulation context
    int child_pid = fork();
    switch( child_pid )
    {
        case -1:
            // how to handle? 
            // make damaris send error to regular ROSS ranks to exit?
            break;
        case 0:
            child_handler(path, cmd);
            // shouldn't return
            perror( "[opt-debug-fork-exec] exec failed" );
            exit( EXIT_FAILURE );
            break;
        default:
            // no errors
            // parent needs to wait on a signal from the child that it has reached tw_sched_sequential()
            parent_handler(child_pid);
            break;
    }


}

void parent_handler(int child_pid)
{
    int status;
    wait(&status);
	long orig_eax = ptrace(PTRACE_PEEKUSER, child_pid, 4 * ORIG_RAX, NULL);
	printf("The child made a system call %ld\n", orig_eax);
	ptrace(PTRACE_CONT, child_pid, NULL, NULL);

}

void child_handler(char *path, char **args)
{
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
    execv(path, args);
}

//void handle_signal_child(int sig)
//{
//    // child receives signal
//    if (sig == SIG_SEQ_STOP)
//    {
//
//    }
//}

// damaris event for optimistic debug analysis
//void optimistic_debug(const std::string& event, int32_t src, int32_t step, const char* args)
//{
//    // have GVT value passed in through args
//    // run sequential up to that point in virtual time
//}
//


}
