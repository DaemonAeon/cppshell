#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <iostream>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <fstream>
#include <dirent.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <sstream>
#include <vector>
#define LINELEN 256

using namespace std;

void copy_file(FILE *fin, FILE* fout, int writeLineNumbers);
void send_to_stdout(FILE*);
void numberLines();

const char *words[] = {"mkdir", "cd", "chmod", "rmdir", "rm", "cat", "ln", "ps", "uname", "kill","exit"};
inline bool isInteger(const string &);
inline vector<string> readStats(string);
vector<string> split(string,char);
const char *DIR_PROC = "/proc";
void checkCommands(string StringOutPut);
struct command{
  const char **argv2;
};

void *xmalloc (int size){
    void *buf;
    buf = malloc (size);
    if (!buf){
        fprintf (stderr, "Error: Out of memory. Exiting.\n");
        exit (1);
    }
    return buf;
}

char *dupstr (const char *str){
    char *temp;
    temp = (char *) xmalloc (strlen (str) + 1);
    strcpy (temp, str);
    return (temp);
}

char *my_generator (const char *text, int state){
    static int list_index, len;
    const char *name;

    if (!state){
        list_index = 0;
        len = strlen (text);
    }

    while (name = words[list_index]){
        list_index++;
        if (strncmp (name, text, len) == 0)
        	return dupstr (name);
    }

    // If no names matched, then return NULL.
    return ((char *) NULL);
}

static char **my_completion (const char *text, int start, int end){
    char **matches = (char **) NULL;
    if (start == 0){
        matches = rl_completion_matches ((char *) text, &my_generator);
    }else {
    	rl_bind_key ('\t', rl_abort);
    }
    return matches;
}

void parsear(char* c, char** argv){
	while (*c != '\0') {       /* if not the end of line ....... */
        while (*c == ' ' || *c == '\t' || *c == '\n')
            *c++ = '\0';     /* replace white spaces with 0    */
          	*argv++ = c;          /* save the argument position     */
        	while (*c != '\0' && *c != ' ' && *c != '\t' && *c != '\n')
               	c++;             /* skip the argument until ...    */
    }
    *argv = '\0';                 /* mark the end of argument list  */
}

int spawn_proc (int in, int out, struct command *cmd){
  	pid_t pid;

  	if ((pid = fork ()) == 0){
      	if (in != 0){
          	dup2 (in, 0);
          	close (in);
        }

      	if (out != 1){
          	dup2 (out, 1);
          	close (out);
        }
      	return execvp (cmd->argv2 [0], (char * const *)cmd->argv2);
    }
  	return pid;
}

void fork_pipes (int n, struct command *cmd){
  	int i;
  	pid_t pid;
  	int in, fd [2];
  	in = 0;

  	for (i = 0; i < n - 1; ++i){
      	pipe (fd);
      	spawn_proc (in, fd [1], cmd + i);
      	close (fd [1]);
      	in = fd [0];
    }

  	if (in != 0)
    	dup2 (in, 0);
	if(execvp (cmd [i].argv2 [0], (char * const *)cmd [i].argv2) < 0){
		printf("*** ERROR: exec failed\n");
	    return;
	}

}

void redirectionOutput(char** argv){
	pid_t  pid = fork();
    int    status;
	if (pid == 0){          /* for the child process:         */
    	// function for redirection ( '<' , '>' )
	    int fd0,fd1,i = 0;
		while(argv[i]!=NULL){
			if(strcmp(argv[i], ">")==0){
				argv[i] = NULL;
				break;
			}
			i++;
		}

        if ((fd1 = creat(argv[i+1] , 0644)) < 0) {
            perror("Couldn't open the output file");
            exit(0);
        }

        dup2(fd1, STDOUT_FILENO);
        close(fd1);
		cout << "hey"<<argv[0];
	    execvp(*argv, argv);
	    perror("execvp");
	    _exit(1);

	    // another syntax
	    /*      if (!(execvp(*argv, argv) >= 0)) {     // execute the command
	            printf("*** ERROR: exec failed\n");
	            exit(1);
	     */
	}else if((pid) < 0){
        printf("fork() failed!\n");
        exit(1);
    }else {                                  /* for the parent:      */

        while (!(wait(&status) == pid)) ; // good coding to avoid race_conditions(errors)
    }

}

void redirectionInput(char** argv){
	pid_t  pid = fork();
    int    status;
	if (pid == 0){          /* for the child process:         */

    // function for redirection ( '<' , '>' )

	    int fd0,fd1,i = 0;
		while(argv[i]!=NULL){
			if(strcmp(argv[i], "<")==0){
				argv[i] = NULL;
				break;
			}
			i++;
		}

        if ((fd0 = open(argv[i+1], O_RDONLY, 0)) < 0) {
            perror("Couldn't open input file");
            exit(0);
        }
	        // dup2() copies content of fdo in input of preceeding file
        dup2(fd0, 0); // STDIN_FILENO here can be replaced by 0
        close(fd0); // necessary
	    execvp(*argv, argv);
	    perror("execvp");
	    _exit(1);

	    // another syntax
	    /*      if (!(execvp(*argv, argv) >= 0)) {     // execute the command
	            printf("*** ERROR: exec failed\n");
	            exit(1);
	     */
	}else if((pid) < 0){
        printf("fork() failed!\n");
        exit(1);
    }else {                                  /* for the parent:      */
        while (!(wait(&status) == pid)) ; // good coding to avoid race_conditions(errors)
    }

}

void redirectionOutputAndError(char** argv){
	pid_t  pid = fork();
    int    status;
	if (pid == 0){          /* for the child process:         */
	    // function for redirection ( '<' , '>' )
	    int fd0,fd1,i = 0;
		while(argv[i]!=NULL){
			if(strcmp(argv[i], "&>")==0){
				argv[i] = NULL;
				break;
			}
			i++;
		}

	    if ((fd1 = creat(argv[i+1] , 0644)) < 0) {
	        perror("Couldn't open the output file");
	        exit(0);
	    }

	    dup2(fd1, STDOUT_FILENO);
		dup2(fd1, STDERR_FILENO);
	    close(fd1);
		cout << "hey"<<argv[0];
	    execvp(*argv, argv);
	    perror("execvp");
	    _exit(1);

	    // another syntax
	    /*      if (!(execvp(*argv, argv) >= 0)) {     // execute the command
	            printf("*** ERROR: exec failed\n");
	            exit(1);
	     */
	}else if((pid) < 0){
        printf("fork() failed!\n");
        exit(1);
    }else {                                  /* for the parent:      */
        while (!(wait(&status) == pid)) ; // good coding to avoid race_conditions(errors)
    }
}

void redirectionAppendError(char** argv){
	pid_t  pid = fork();
    int    status;
	if (pid == 0){          /* for the child process:         */

	    // function for redirection ( '<' , '>' )

	    int fd0,fd1,i = 0;
		while(argv[i]!=NULL){
			if(strcmp(argv[i], ">>")==0){
				argv[i] = NULL;
				break;
			}
			i++;
		}


		int fd = open(argv[i+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR, O_APPEND);

	    dup2(fd, STDOUT_FILENO);   // make stdout go to file
	    dup2(fd, STDERR_FILENO);   // make stderr go to file - you may choose to not do this
	                   // or perhaps send stderr to another file

	    close(fd);     // fd no longer needed - the dup'ed handles are sufficient

		cout << "hey"<<argv[0];
	    execvp(*argv, argv);
	    perror("execvp");
	    _exit(1);

	    // another syntax
	    /*      if (!(execvp(*argv, argv) >= 0)) {     // execute the command
	            printf("*** ERROR: exec failed\n");
	            exit(1);
	     */
	}else if((pid) < 0){
        printf("fork() failed!\n");
        exit(1);
    }else {                                  /* for the parent:      */
        while (!(wait(&status) == pid)) ; // good coding to avoid race_conditions(errors)
    }

}

void redirectionErrorToOutput(char** argv){
	/*const char* PROGRAM_NAME = "./child";
	char arg1[] = "arg1";
	char arg2[] = "arg2";
	char *args[] = { arg1, arg2, NULL };*/
	int i = 0;

	while(argv[i]!=NULL){
		if(strcmp(argv[i], "2>&!")==0){
			argv[i] = NULL;
		}
		i++;
	}

	int pipeForStdOut[2], pipeForStdErr[2];
	std::string cntStdErr;

	char buf[32] = {0};
	ssize_t bytesRead;
	pid_t childPid;

	pipe(pipeForStdErr);

	childPid = fork();
	if(childPid == -1){
	    perror("fork");
	    exit(-1);
	}else if(childPid == 0){
		close(pipeForStdErr[0]); // parent keeps the input
		if(dup2(pipeForStdErr[1], 2) < 0){
		    perror("dup2.2");
		    exit(-1);
		}

		if(execv(*argv, argv) == -1){
	      perror("execv");
	      exit(-1);
	    }
	    exit(0);
	}
	wait(NULL);

	fcntl(pipeForStdErr[0], F_SETFL, O_NONBLOCK  | O_ASYNC);
	while(1) {
	    bytesRead = read(pipeForStdErr[0], buf, sizeof(buf)-1);
	    if (bytesRead <= 0) break;
	    buf[bytesRead] = 0; // append null terminator
	    cntStdErr += buf; // append what wuz read to the string
	}
	const char * c = cntStdErr.c_str();
	printf("%s",c);

	close(pipeForStdErr[0]);
}

bool exists_archivo (const char* name) {
  struct stat buffer;
  return (stat (name, &buffer) == 0);
}

void cd(char** argv){
	struct stat s;
	if( stat(argv[1],&s) == 0 ){
	    if( s.st_mode & S_IFDIR ){ //directorio
	        chdir(argv[1]);
	    }else if( s.st_mode & S_IFREG ){//archivo
	        cout<<"No es un archivo"<<endl;
	    }
	}else{
		struct passwd *pw = getpwuid(getuid());
		const char* homedir = pw->pw_dir;
	    chdir(homedir);
	}

}

void makedir(char** argv){
	if (argv[1] && strcmp(argv[1], " ")!=0){
		if (exists_archivo(argv[1])){
			cout<<"Ya existe un archivo con ese nombre"<<endl;
		}else{
			mkdir(argv[1], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		}
	}else{
		cout<<"Aun falta operandos"<<endl;
	}
}

void chmod(char** argv){
    int permisos=strtol(argv[1], 0, 8);
    if (permisos){
    	if (exists_archivo(argv[2])){
    		chmod(argv[2],permisos);
    	}else{
    		cout<<"No existe tal archivo."<<endl;
    	}
    }else{
    	cout<<"Permisos equivocado"<<endl;
    }
	/*owner/group/others
	4 read (r)
	2 write (w)
	1 execute (x)
	*/
}

int rmdir_R(const char *path){
   	DIR* directorio = opendir(path);
   	size_t path_len = strlen(path);
   	int r = -1;
   	if (directorio){
      	struct dirent *p;
      	r = 0;
      	while (!r && (p=readdir(directorio))){
          	int r2 = -1;
          	char *buf;
          	size_t len;
          	if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")){
             	continue;
          	}
          	len = path_len + strlen(p->d_name) + 2;
          	buf = (char*)malloc(len);
          	if (buf){
             	struct stat statbuf;
             	snprintf(buf, len, "%s/%s", path, p->d_name);
	            if (!stat(buf, &statbuf)){
	                if (S_ISDIR(statbuf.st_mode)){
	                   	r2 = rmdir_R(buf);
	                }else{
	                   	r2 = unlink(buf);
	                }
	            }
	            free(buf);
	        }
          	r = r2;
      	}
   	   	closedir(directorio);
   	}
   	if (!r){
      	r = rmdir(path);
   	}
   	return r;
}

void rmdir(char** argv){
	if (argv[1]){
		if (strcmp(argv[1], "-R")==0){
			if (exists_archivo(argv[2])){
				rmdir_R(argv[2]);
			}else{
				cout<<"No existe tal directorio."<<endl;
			}
		}else{
			if (exists_archivo(argv[1])){
				std::ifstream file(argv[1]);
				file.seekg(0, ios::end);
				if (file.tellg()!=0){
					rmdir(argv[1]);
				}else{
					cout<<"El directorio no esta vacio."<<endl;
				}
			}else{
				cout<<"No existe tal directorio."<<endl;
			}
		}
	}else{
		cout<<"Falta ingresar el directorio."<<endl;
	}
}

void rm(char** argv){
	//elimina un archivo
	if (argv[1])
	{
		if (exists_archivo(argv[1])){
				cout << argv[1]<< endl;
				remove(argv[1]);
				cout << "Archivo eliminado" << endl;
		}else{
			cout<<"No existe tal archivo."<<endl;
		}
	}
}

void cat(char** argv)
{
	/*
	string line;
	ifstream archivo(argv[1], ios::in);
	//archivo.open();
	if (archivo.is_open()){
		while(getline(archivo,line)){
			cout<<line<<endl;
		}
		archivo.close();
	}
	*/
char cpr[]= "-n";
    char cpr2[]= ">>";
    FILE *fp;
    /*if the input does not contain -n*/

    if(sizeof(argv) >1)
    if(strcmp(cpr,argv[1])!=0)
    {
//        printf("Trace ... about to open the file\n");
          if((fp= fopen(argv[1], "r"))!=NULL)
          {
             copy_file(fp, stdout, 0);
             fclose(fp);
             //exit(1);
          }
          /*else it's nothing.*/
          else
          {
//              printf("Trace ... about to perror\n");
              perror("could not open the file.");
              //exit(1);
          }
    }
    /*if the user only type in one command then echo input*/
//    printf("Trace ... about to test argc\n");
    if(sizeof(argv)==1)
    {
//      printf("Trace ... about to send_to_stdout(stdin)\n");
     // send_to_stdout(stdin);
        copy_file(stdin, stdout, 0);
        //exit(1);
    }
   /*else input must contains -n*/
    else if(strcmp(cpr2,argv[1])==0)
    {
      cout<<"tiene dos cositos"<<endl;
    }else{
        /*if input only has -n then just echo and number the output lines*/
        if(sizeof(argv)==2)
        {
//             printf("Trace ... about to do numberFileLines(stdin)\n");
        //     numberFileLines(stdin);
               copy_file(stdin, stdout, 1);
               //sexit(1);
        }
        /*else just output files and number all the lines*/
        else
        {
             if((fp= fopen(argv[2], "r"))!=NULL)
             {
//               printf("Trace ... about to do numberFileLines(fp)\n");
                 copy_file(fp, stdout, 1);
                 fclose(fp);
                 //exit(1);
             }
             else
             {
                //perror("1could not open the file.");
                //exit(1);
             }
        }
    }
}

void copy_file(FILE *fin, FILE* fout, int writeLineNumbers)
{
    char line[LINELEN];
    int lineno = 1;

    while (fgets(line, LINELEN, fin))
    {
        if (writeLineNumbers)
            printf("%d ", lineno);

        if (fputs(line, fout) == EOF)
        {
            perror("Write to stdout failed.");
            return;
        }

        ++lineno;
    }
}
               
/***********************************************/

void ln(char** argv){
  string argumento = argv[1];
  vector<string> ruta;
    vector<string> archivos;

    char* duplicado2 = strdup(argumento.c_str());
    char* token2 = strtok(duplicado2, " ");
    while(token2 != NULL){
        archivos.push_back(string(token2));
        // the call is treated as a subsequent calls to strtok:
        // the function continues from where it left in previous invocation
        token2 = strtok(NULL, " ");
    }

    string archivo1 = archivos[0];
    string archivo2 = archivos[1];

    if (symlink(archivo1.c_str(), archivo2.c_str()) != -1 )
    {
      
    }
    else{
      cout << "Error al ejecutar ln" << endl;
    }
}

//***************+++++++++++PS KILL AND UNAME++++++***********************************************************
inline vector<string> readStats(string stats){
    vector<string> parseStats = split(stats,' ');
    vector<string> retVal;
    long TIME;
    retVal.push_back(parseStats[0]);
    retVal.push_back(parseStats[6]);
    retVal.push_back(parseStats[13]);
    retVal.push_back(parseStats[1]);
    return retVal;
}

inline bool isInteger(const string &s)
{
  if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;
  char * p ;
  strtol(s.c_str(), &p, 10) ;
  return (*p == 0) ;
}

vector<string> split(string str, char delimiter) {
    vector<string> split;
    stringstream ss(str);
    string tok;
    while(getline(ss, tok, delimiter)) {
        split.push_back(tok);
      }
    return split;
}


  void PsGeneral(int argc ,char** argv,string  StringOutPut){
    int cont = 0;
    vector< vector<string> > ALL_STATS;
  	if (argc != 1){
  		exit(1);
  	}

  	string RUN_DIR(argv[0]);
  	RUN_DIR+="/cat";
  	stringstream SS;
  	int fd_p2c[2], fd_c2p[2], bytes_read;
      pid_t childpid;
      char readbuffer[256];
      string receive = "";
  	//vector<string> PID;
  	DIR *dir;
  	struct dirent *ent;

  	if ((dir = opendir (DIR_PROC)) != NULL&& cont ==1) {

  	  /* print all the files and directories within directory */
  	  while ((ent = readdir (dir)) != NULL) {

  	  	receive="";
  	  	if (isInteger(ent->d_name)){
  			 if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)  {
  			        cerr << "Failed to pipe\n";
  			        exit(1);
  			}
  			childpid = fork();

  			if (childpid < 0){
  			    cout << "Fork failed" << endl;
  			    exit(-1);
  			}else if (childpid == 0){
  			    if (dup2(fd_p2c[0], 0) != 0 ||
  			        close(fd_p2c[0]) != 0 ||
  			        close(fd_p2c[1]) != 0){

  			            cerr << "Child: failed to set up standard input\n";
  			            exit(1);
  			    }
  			    if (dup2(fd_c2p[1], 1) != 1 ||
  			       close(fd_c2p[1]) != 0 ||
  			       close(fd_c2p[0]) != 0){

  			      		cerr << "Child: failed to set up standard output\n";
  			            exit(1);
  			    }

  			    string DIR(DIR_PROC);
  			    char *arg[] = {(char *)(DIR+"/"+ent->d_name+"/stat").c_str(), (char *)0};
  		        execv(RUN_DIR.c_str(), arg);
  		        exit(1);
  		    }else{
  		    	close(fd_p2c[0]);
          		close(fd_c2p[1]);
  				while (1){
              		bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

              		if (bytes_read <= 0)
                  		break;

              		readbuffer[bytes_read] = '\0';
              		receive += readbuffer;
          		}
         			close(fd_c2p[0]);
         			ALL_STATS.push_back(readStats(receive));
         			//cout << "PID" << '\t' << "TTY" << '\t' << "TIME" << '\t' << "CMD" << endl;
         			 //cout << "From child: <<" << receive << ">>" << endl;
  		    }

  	  	}

  	  }
  	  closedir (dir);
  	} else {
  	  /* could not open directory */
  	  //perror ("");
  	}
  	/* code */

//  	cout << "PID" << '\t' << "TTY" << '\t' << "TIME" << '\t' << "CMD" << endl;
  /*	for (int i = 0; i < ALL_STATS.size() ; i++){
  		cout << ALL_STATS[i][0] << '\t' << ALL_STATS[i][1] << '\t' << ALL_STATS[i][2] << '\t' << ALL_STATS[i][3] << endl;
  }*/

    checkCommands(StringOutPut);

  }

  void Ps(char** argv){

    stringstream StringOutPut;
    StringOutPut.str("");
    int i = 0;
    bool checkCommand = true;
    int control = 0;
    while(argv[i]!=NULL){

      if(strcmp(argv[i], "ps")==0 ||strcmp(argv[i], "-a")==0){
        if(control==0)
          StringOutPut<<"ps -a";

        control++;
      } else if(strcmp(argv[i], "-u")==0){
        StringOutPut<<" -u";
      }else if(strcmp(argv[i], "-x")==0){
        StringOutPut<<" -x";
      }
      else if(strcmp(argv[i], "-e")==0){
          StringOutPut<<" -e";
      }else{
        checkCommand = false;
      }

      i++;
      }
      if(checkCommand){
        PsGeneral(1,argv,StringOutPut.str());
      }else{
          cout<<"Comando no encontrado, error en sintaxis"<<endl;
      }
  }

  void Uname(char** argv){
    struct utsname sysinfo;

    if (uname(&sysinfo) != 0) {
      perror("uname");
      exit(EXIT_FAILURE);
   }
   stringstream StringOutPut;
   StringOutPut.str("");
   int i = 0;
   bool checkCommand = true;
	while(argv[i]!=NULL){

    if(strcmp(argv[i], "uname")==0 || strcmp(argv[i], "-s")==0){
        StringOutPut<<sysinfo.sysname<<" ";
    } else if(strcmp(argv[i], "-n")==0){
        StringOutPut<<sysinfo.nodename<<" ";
    }else if(strcmp(argv[i], "-v")==0){
        StringOutPut<<sysinfo.release<<" ";
    }
    else if(strcmp(argv[i], "-m")==0){
        StringOutPut<<sysinfo.machine<<" ";
    }else if(strcmp(argv[i], "-a")==0){
        StringOutPut<<sysinfo.sysname<<" "<<sysinfo.nodename<<" "<<sysinfo.release<<" "<<sysinfo.machine<<" ";
    }else{
      checkCommand = false;
    }

    i++;
    }
    if(checkCommand){
      cout<<StringOutPut.str()<<endl;
    }else{
        cout<<"Comando no encontrado, error en sintaxis"<<endl;
    }
  }

  void Kill(char** argv){
      if(strcmp(argv[1], "-9")==0){
        if(!argv[2]){
            if (kill( atoi(argv[2]), SIGKILL ) != 0){
            cout<<"Ocurrio un error en kill"<<endl;
          }
        }else{
          cout<<"No se encontró el proceso"<<endl;
        }
      }else{
        cout<<"Comando no encontrado"<<endl;
      }

  }
void checkCommands(string StringOutPut){system(StringOutPut.c_str());}
//***************++++++FIN PS KILL AND UNAME++++++++**********************************************************
void ejecutar(char **argv){
    pid_t  pid;
    int    status;

    if ((pid = fork()) < 0) {     /* fork a child process           */
        printf("*** ERROR: forking child process failed\n");
        return;
    }else if (pid == 0) {          /* for the child process:         */
        if (execvp(*argv, argv) < 0) {     /* execute the command  */
            printf("*** ERROR: exec failed\n");
            return;
        }
    }else {                                  /* for the parent:      */
        while (wait(&status) != pid);       /* wait for completion  */
    }
}

void ejecutarbackground(char **argv){
    char** argv2 = argv;
	for(int i = 0; i < 1024; i++ ){
		if(argv2[0][i]=='&'){
			argv2[0][i] = '\0';
			break;
		}
	}
	cout << endl;
	for(int i = 0; i < 1024; i++ ){
		cout<<argv2[0][i];
		if(argv2[0][i]=='\0')
			break;
	}
    pid_t  pid;
    int    status;

    if ((pid = fork()) < 0) {     /* fork a child process           */
        printf("*** ERROR: forking child process failed\n");
        return;
    }else if (pid == 0) {          /* for the child process:         */
        if (execvp(*argv, argv2) < 0) {     /* execute the command  */
            printf("*** ERROR: exec failed\n");
            return;
        }
    }
}


int main (){
    char *buf;
    rl_attempted_completion_function = my_completion;

    while((buf = readline("\nmi_sh>> "))!=NULL) {
		char* argv[10];
		int i = 0, j = 0;
		bool pipeMe = false, redirout = false, redirinput = false, redirouterr = false, redirappenderr = false, redirerrout = false;// | > < &> >> 2>&1
        //enable auto-complete
        rl_bind_key('\t',rl_complete);

        /*printf("cmd [%s]\n",buf);
        if (strcmp(buf,"quit")==0)
            break;*/

        if (buf[0]!=0){
            add_history(buf);
        }
		parsear(buf, argv);

		cout << endl;

		while(argv[i]!=NULL){
			if(strcmp(argv[i], "|") ==0 ){
				pipeMe = true;
				j++;
			}
			if(strcmp(argv[i], ">") ==0 ){
				redirout = true;
			}
			if(strcmp(argv[i], "<") ==0 ){
				redirinput = true;
			}
			if(strcmp(argv[i], ">&") ==0 ){
				redirouterr = true;
			}
			if(strcmp(argv[i], ">>") ==0 ){
				redirappenderr = true;
			}
			if(strcmp(argv[i], "2>&1") ==0 ){
				redirerrout = true;
			}
			i++;
		}
		if (pipeMe){
			int h = 0;

			int f = 0;
			i = 0;
			char* coms[5][5];

			for(int w = 0; w < 5; w++){
				for(int z = 0; z < 5; z++){
					coms[w][z] = 0;
				}
			}

			while(argv[i]!=NULL){
				if(strcmp(argv[i], "|")!=0){
					coms[h][f]=argv[i];
					f++;
				}else{
					h++;
					f=0;
				}
				i++;
			}
			/*command cmd[] = {{(const char**)coms[0]}, {(const char**)coms[1]}, {(const char**)coms[2]}, {(const char**)coms[3]}, {(const char**)coms[4]}};
			fork_pipes (j+1, cmd);*/
			command cmd[] = {{(const char**)coms[0]},{(const char**)coms[1]}, {(const char**)coms[2]} ,{(const char**)coms[3]},{(const char**)coms[4]}};
			fork_pipes(j, cmd);
		}else if(redirout){
			redirectionOutput(argv);
		}else if(redirinput){
			redirectionInput(argv);
		}else if(redirouterr){
			redirectionOutputAndError(argv);
		}else if(redirappenderr){
			redirectionAppendError(argv);
		}else if(redirerrout){
			redirectionErrorToOutput(argv);
		}else if (strcmp(argv[0], "bye") == 0){    // exit if the user enters bye
	        return 0;
        }else if (strcmp(argv[0], "exit") == 0){    // exit if the user enters bye
        return 0;
		}else if (strcmp(argv[0], "cd") == 0){
			cd(argv);
		}else if (strcmp(argv[0], "mkdir") == 0){
			makedir(argv);
		}else if (strcmp(argv[0], "chmod") == 0){
			chmod(argv);
		}else if (strcmp(argv[0], "rmdir") == 0){
			rmdir(argv);
		}else if (strcmp(argv[0], "rm") == 0){
			rm(argv);
		}
		else if (strcmp(argv[0], "cat") == 0){
			cat(argv);
		}else if(strcmp(argv[0], "ps") == 0){
      	Ps(argv);
    }else if(strcmp(argv[0], "uname")== 0){
        	Uname(argv);
    }else if(strcmp(argv[0], "kill")== 0){
      	Kill(argv);
    }else{
			signal(SIGINT, SIG_IGN);       	        //The instructions said to ignore the SIGINT signal
		    signal(SIGTERM, SIG_DFL);               //SIGTERM signal must be caught.
			if(strrchr(buf,'&')==NULL){
		       	ejecutar(argv);
			}else{
				ejecutarbackground(argv);
			}

		}
	}
    free(buf);
    return 0;
}