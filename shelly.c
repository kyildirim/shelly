#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <ifaddrs.h>
#include <readline/readline.h>
#include <readline/history.h>

//ANSI Graphics Mode
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_TEXT_BOLD     "\x1b[1m"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"

#define ANSI_COLOR_BLACK_BG   "\x1b[40m"
#define ANSI_COLOR_RED_BG     "\x1b[41m"
#define ANSI_COLOR_GREEN_BG   "\x1b[42m"
#define ANSI_COLOR_YELLOW_BG  "\x1b[43m"
#define ANSI_COLOR_BLUE_BG    "\x1b[44m"
#define ANSI_COLOR_MAGENTA_BG "\x1b[45m"
#define ANSI_COLOR_CYAN_BG    "\x1b[46m"
#define ANSI_COLOR_WHITE_BG   "\x1b[47m"
//

//Exit Codes
#define CHILD_EXIT_SUCCESS 1
#define EXIT_SUCCESS 0
#define FORK_ERROR -1
#define EXIT_ERROR_BUFFER -11
#define CHILD_EXEC_FAIL -21
#define INVALID_ARGUMENT_COUNT -31
#define ACTIVE_SCRIPT_MODE_ERR -41
//

//Shell Propmt Info
#define UNAME_MAX 32
#define HNAME_MAX 64
#define CWD_MAX 1024
#define MAX_PATH_DIR_NUM 32
#define MAX_FILES_IN_DIR 65536
//

//Command Line
#define BUFSIZE_MAX 1024
#define TOKEN_BUFSIZE_MAX 64
#define TOKEN_DELIMITER " \n\t\r\a"
//

//Program Descriptor
#define SHELL_NAME "Shelly"
//

//Username
char *uname;
//Hostname
char *hname;
//Current working directory
char cwd[CWD_MAX];
//User home
char *uhome;

//Script vars
int script_mode = 0;
char *script_file;
FILE *script_fd;

//Builtin command autocomplete functions
char **shelly_autocomplete(const char *, int, int);
char *shelly_autocomplete_generator(const char *, int);

//Builtin command declarations
int b_cd(char **args);
int b_help(char **args);
int b_exit(char **args);
int b_oldu_http(char **args);
int b_olmedi_http(char **args);
int b_wforecast(char **args);
int b_bookmark(char **args);
int b_script(char **args);

//Commands in paths
char **all_cmds;

//Builtin commands list
char *builtin_cmd[] = {
	"cd",
	"help",
	"exit",
	"öldühttp",
	"ölmedihttp",
	"wforecast",
	"bookmark",
	"script"
};

//Builtin command addresses
int (*builtin[]) (char **) ={
	&b_cd,
	&b_help,
	&b_exit,
	&b_oldu_http,
	&b_olmedi_http,
	&b_wforecast,
	&b_bookmark,
	&b_script
};

//Get number of builtin commands
int num_builtin(){
	return sizeof(builtin_cmd)/sizeof(char *);
}

//Builtin change directory
int b_cd(char **args){
	if(args[1] == NULL){
		fprintf(stderr, "%s: expected argument to \"cd\"\n", SHELL_NAME);
	}else{
		if(chdir(args[1]) != 0){
			perror(SHELL_NAME);
		}
	}
	//Load working directory data
	getcwd(cwd, sizeof(cwd));
	return 1;
}

//Builtin ölmedihttp
int b_olmedi_http(char **args){
    struct ifaddrs *addrs,*tmp;

    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
        char **netargs = malloc(sizeof(char *)*4);
        netargs[0]="ifconfig";    
        netargs[2]="up";
        netargs[3]=NULL;
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET){       
            netargs[1] = tmp->ifa_name;
            execute(netargs);
        }
        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    return 1;
}

//Builtin öldühttp
int b_oldu_http(char **args){
	struct ifaddrs *addrs,*tmp;

	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
		char **netargs = malloc(sizeof(char *)*4);
		netargs[0]="ifconfig";	
		netargs[2]="down";
	    netargs[3]=NULL;
	    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET){   	
	    	netargs[1] = tmp->ifa_name;
	        execute(netargs);
	    }
	    tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);
	return 1;
}

//Builtin weather forecast
int b_wforecast(char **args){

	char *out_file = malloc(strlen(args[1])*sizeof(char));
	strcpy(out_file, args[1]);
	int i;
	args = (char **)malloc(6*sizeof(char *));
	for(i = 0; i<6; i++)args[i]=malloc(CWD_MAX*sizeof(char));
	strcpy(args[0], "wttrin");
	strcpy(args[1], "-l");
	strcpy(args[2], "Istanbul");
	strcpy(args[3], "-p");
	strcpy(args[4], "-o");
	strcpy(args[5], out_file);
	args[6]=NULL;
	return launch(args);

}

//Builting bookmark
int b_bookmark(char **args){

	//Invalid Argument Count
	if(args[1] == NULL)return INVALID_ARGUMENT_COUNT;
	if(args[2] == NULL)return INVALID_ARGUMENT_COUNT;

	char *path = (char *)malloc(CWD_MAX*sizeof(char));
	strcpy(path, uhome);
	strcat(path, "/.shelly/bookmarks");

	if(strcmp(args[1], "-r")==0){
		FILE *bFile = fopen(path, "rb");
		
		//Get file size
		fseek(bFile, 0, SEEK_END);
		long fSize = ftell(bFile);
		fseek(bFile, 0, SEEK_SET); 

		char *bookmarks = malloc(fSize + 1);
		fread(bookmarks, fSize, 1, bFile);

		bookmarks[fSize] = '\0';

		fclose(bFile);

		strcat(args[2], " "); //Match exact key
		char *new_bookmarks = strstr(bookmarks, args[2]);
		if(new_bookmarks && (bookmarks==new_bookmarks || new_bookmarks[-1]=='\n')){
			int i = 0;
			while(new_bookmarks[i]!='\n')i++;
			i++;
			int j = strlen(bookmarks)-strlen(new_bookmarks);
			for(j; j<strlen(bookmarks)-i; j++){
				bookmarks[j] = bookmarks[j+i];
			}
			bookmarks[j]='\0';
			if(bookmarks[j-1]=='\n')bookmarks[j-1]='\0';


		}

		bFile = fopen(path, "w");

		fprintf(bFile, "%s\n", bookmarks);
		fclose(bFile);

	}else{
		FILE *bFile = fopen(path, "a+");

		char *cmd = malloc(strlen(args[2])*sizeof(char));
		int i = 3;
		strcpy(cmd, args[2]);
		while(args[i]!=NULL){
			strcat(cmd, " ");
			strcat(cmd, args[i]);
			i++;
		}
		fprintf(bFile, "%s %s\n", args[1], cmd);
		fclose(bFile);
	}
	

	return 1;
}

//Builtin script
int b_script(char **args){

	if(args[1] == NULL)return INVALID_ARGUMENT_COUNT;
	if(script_mode == 1)return ACTIVE_SCRIPT_MODE_ERR;

	script_file = malloc(strlen(args[1])*sizeof(char));
	strcpy(script_file, args[1]);
	script_mode=1;

	printf("Writing script to: %s\n", script_file);

	script_fd = fopen(script_file, "a+");

	return 1;

}

//Builtin help
int b_help(char **args){
	printf("****************\n");
	printf("* Shelly v0.1b *\n");
	printf("****************\n");
	printf("Loaded builtin functions:\n");
	int i;
	for(i = 0; i<num_builtin(); i++){
		printf("%s\t", builtin_cmd[i]);
		if(i%3==2)printf("\n");
	}
	printf("\n");
}

//Builtin exit
int b_exit(char **args){
	if(script_mode==1){
		script_mode=0;
		script_file = NULL;
	}else{
		return 0;
	}
}

//Get all files from path
void load_path(){
	char *path_var = strdup(getenv("PATH"));
	char **paths = malloc(MAX_PATH_DIR_NUM*sizeof(char *));
	all_cmds = malloc(MAX_FILES_IN_DIR*sizeof(char *));

	//Get all paths separated
	int i = 0;
	char *token, *temp;

	token = strtok_r(path_var, ":;", &temp);
	while(1){
		token = strtok_r(NULL, ":;", &temp);
		if(token == NULL)break;
		paths[i]=(char *)malloc(CWD_MAX*sizeof(char));
		strcpy(paths[i], token);
		i++;
	}
	i--;
	
	//Executable file completion from PATH of environment
	DIR *dir;
	struct dirent *ent;
	int ctr = 0;
	char *fPath = malloc(CWD_MAX*2*sizeof(char));
	for(i; i>-1; i--){
		if ((dir = opendir (paths[i])) != NULL) {
	 		while ((ent = readdir (dir)) != NULL) {
	 			strcpy(fPath, paths[i]);
	 			strcat(fPath, "/");
	 			strcat(fPath, ent->d_name);
	 			if (access(fPath, X_OK) != 0)continue;
	 			all_cmds[ctr] = malloc(CWD_MAX*sizeof(char));
	 			strcpy(all_cmds[ctr], ent->d_name);
	 			ctr++;
		 	}
	  		closedir (dir);
		} else {
			perror ("");
	  		return -1;
		}
	}

	//Full file completion for current working directory
	if ((dir = opendir (cwd)) != NULL) {
 		while ((ent = readdir (dir)) != NULL) {
 			all_cmds[ctr] = malloc(CWD_MAX*sizeof(char));
 			strcpy(all_cmds[ctr], ent->d_name);
 			ctr++;
	 	}
  		closedir (dir);
	} else {
		perror ("");
  		return -1;
	}
	
	//Builtin command completion
	int k = 0;
	for(k;k<num_builtin();k++){
		all_cmds[ctr] = malloc(CWD_MAX*sizeof(char));
	 	strcpy(all_cmds[ctr], builtin_cmd[k]);
		ctr++;
	}
	

}

//Autocomplete generator for shelly commands
char *shelly_autocomplete_generator(const char *str, int s){
    static int i, l;
    char *cmd;

    //State
    if (!s) {
        i = 0;
        l = strlen(str);
    }

    while ((cmd = all_cmds[i++])) {
        if (strncmp(cmd, str, l) == 0) {
            return strdup(cmd);
        }
    }



    return NULL;
}

//Autocomplete for shelly commands
char **shelly_autocomplete(const char *str, int s, int e){
    rl_attempted_completion_over = 1;
    return rl_completion_matches(str, shelly_autocomplete_generator);
}

//Tokenize input
char **tokenize(char *cmd){
	int bufsize = TOKEN_BUFSIZE_MAX;
	int pos = 0;
	char **tokens = malloc(bufsize * sizeof(char *));
	char *token;

	//Buffer check
	if(!tokens){
		fprintf(stderr, "Buffer allocation error! Error code: %d\n", EXIT_ERROR_BUFFER);
		exit(EXIT_ERROR_BUFFER);
	}

	//Get a token
	token = strtok(cmd, TOKEN_DELIMITER);

	//While there are tokens
	while(token != NULL){
		//Handle environment variables
		if(strlen(token)>1&&token[0]=='$'){
			tokens[pos] = getenv(++token);
		}else{
			tokens[pos] = token;
		}
		pos++;

		//If required extend buffer size
		if(pos>=bufsize){
			bufsize += TOKEN_BUFSIZE_MAX;
			tokens = realloc(tokens, bufsize * sizeof(char *));
			//Buffer check
			if(!tokens){
				fprintf(stderr, "Buffer allocation error! Error code: %d\n", EXIT_ERROR_BUFFER);
				exit(EXIT_ERROR_BUFFER);
			}
		}

		token = strtok(NULL, TOKEN_DELIMITER);
	}

	tokens[pos] = NULL;
	return tokens;

}

//Handle input
int execute(char **args){

	//Empty command
	if(args[0] == NULL){
		return 1;
	}

	//Check bookmarks TO-D0 maybe move to memory
	char *path = (char *)malloc(CWD_MAX*sizeof(char));
	strcpy(path, uhome);
	strcat(path, "/.shelly/bookmarks");

	FILE *bFile = fopen(path, "r");

	//If no bookmarks file exists
	if(bFile==NULL){
		strcpy(path, uhome);
		strcat(path, "/.shelly");
		mkdir(path, 0777-umask(0));
		strcpy(path, uhome);
		strcat(path, "/.shelly/bookmarks");
		bFile = fopen(path, "w+");
	}

	//Get file size
	fseek(bFile, 0, SEEK_END);
	long fSize = ftell(bFile);
	fseek(bFile, 0, SEEK_SET); 

	char *bookmarks = malloc(fSize + 1);
	fread(bookmarks, fSize, 1, bFile);

	bookmarks[fSize] = '\0';

	fclose(bFile);

	char *arg0 = malloc(strlen(args[0])*sizeof(char));
	strcpy(arg0, args[0]);
	strcat(arg0, " "); //Match exact key
	char *new_bookmarks = strstr(bookmarks, arg0);
	if(new_bookmarks && (bookmarks==new_bookmarks || new_bookmarks[-1]=='\n')){
		int i = 0;
		while(new_bookmarks[i]!=' ')i++;
		i++;
		int j = 0;
		while(new_bookmarks[i+j]!='\n')j++;
		char *cmd=malloc(j*sizeof(char));
		memcpy(cmd, new_bookmarks+i, j*sizeof(char));
		cmd[j] = '\0';
		return execute(tokenize(cmd));
	}

	//Check builtin commands
	int i;
	for(i = 0; i<num_builtin(); i++){
		if(strcmp(args[0], builtin_cmd[i]) == 0){
			//Call builtin
			return (*builtin[i])(args);
		}
	}

	//Not builtin, call launch
	return launch(args);

}

//Launch programs
int launch(char **args){
	pid_t pid, cpid;
	int status;

	//Count arguments
	int i;
	for(i=0;;i++){
		if(args[i]==NULL)break;
	}
	i--;

	//Background
	int amp = 0;
	if(strcmp("&", args[i])==0){
		amp = 1;
		args[i] = NULL;
	}
	
	//Redirect
	int redir = 0;
	if(args[i-1] != NULL && i>0){
		if(strcmp(">", args[i-1])==0){
			redir = 1;
			args[i-1] = NULL;
		}else if(strcmp(">>", args[i-1])==0){
			redir = 2;
			args[i-1] = NULL;
		}
	}

	pid = fork();

	if(pid == 0){ //Child Process
		if(script_mode==1){
			freopen(script_file, "a+", stdout);
			freopen(script_file, "a+", stderr);
		}
		if(redir == 1){
   			freopen(args[i], "w", stdout);
    		freopen(args[i], "w", stderr);
		}else if(redir == 2){
			freopen(args[i], "a", stdout);
			freopen(args[i], "a", stderr);
		}
		if(amp == 1){ //Background execution
			setpgid(pid,0);
			freopen("/dev/null", "r", stdin);
   			freopen("/dev/null", "w", stdout);
    		freopen("/dev/null", "w", stderr);
    		umask(0);
		}
		//Check exec
		if(execvp(args[0], args) == -1){
			perror(SHELL_NAME);
			exit(CHILD_EXEC_FAIL);
		}
		exit(CHILD_EXIT_SUCCESS);
	}else if(pid<0){ //Fork Error
		perror(SHELL_NAME);
		exit(FORK_ERROR);
	}else{ //Parent Process
		//Wait for the child to finish or signal
		if(amp == 1){
			return 1;
		}else{
			do{
				cpid = waitpid(pid, &status, WUNTRACED);
			}while(!WIFEXITED(status) && !WIFSIGNALED(status));
		}
	}

	return 1;
}

//Read line from user
char *read_line(void){

	char *input;
	input = readline("\\-> ");
	return input;

}

void prompt(void){
	printf("\n");
	printf("/");
	printf(ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, uname);
	printf("@");
	printf(ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET, hname);
	printf("|");
	printf(ANSI_COLOR_CYAN_BG "%s>" ANSI_COLOR_RESET, cwd);
	printf("\n");
}

//Main loop
void loop(void){

	char *cmd;
	char **args;
	int stat = 0;

	do{
		load_path();
		//Script mode
		if(script_mode==1){
			int stdfd = dup(STDOUT_FILENO);
			dup2(fileno(script_fd), STDOUT_FILENO);
			prompt();
			dup2(stdfd, STDOUT_FILENO);
		}
		prompt();
		cmd = read_line();
		add_history(cmd);
		if(script_mode==1){
			fprintf(script_fd, "\\-> %s\n", cmd);
			fflush(script_fd);
		}
		args = tokenize(cmd);
		stat = execute(args);

		//Reset
		free(cmd);
		free(args);
	}while(stat);

}

//Handle interrupt signal to exit child process first
void sigint_handler(){

	char *path = malloc(CWD_MAX*sizeof(char));
	sprintf(path, "/proc/%d/task/%d/children", getpid(), getpid());
	FILE *cfd = fopen(path, "r");
	if(cfd){
		pid_t cpid = -1;
		fscanf(cfd, "%d", &cpid);
		(cpid>0)?kill(cpid, SIGTERM):kill(getpid(), SIGTERM);
	}else{
		kill(getpid(), SIGTERM);
	}
	fclose(cfd);

}

int main(int argc, char **argv){

	signal(SIGINT, sigint_handler);

	//Load user data
	uname = (char *)malloc(UNAME_MAX*sizeof(char));
	uname = getpwuid(getuid())->pw_name;
	//Load host data
	hname = (char *)malloc(HNAME_MAX*sizeof(char));
	gethostname(hname, HNAME_MAX);
	//Load working directory data
	getcwd(cwd, sizeof(cwd));
	//Get user home
	uhome = (char *)malloc(CWD_MAX*sizeof(char));
	uhome = getpwuid(getuid())->pw_dir;

	//Autocomplete
	rl_bind_key('\t', rl_menu_complete);
	rl_attempted_completion_function = shelly_autocomplete;

	int i;
	if(argc>1){
		for(i=1;i<argc;i++){
			argv[i-1] = malloc(strlen(argv[i])*sizeof(char));
			strcpy(argv[i-1], argv[i]);
		}
		argv[i] = NULL;
		return execute(argv);
	}
	loop();

	return EXIT_SUCCESS;	

}