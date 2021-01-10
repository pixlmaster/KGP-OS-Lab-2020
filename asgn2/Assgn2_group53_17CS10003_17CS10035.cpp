#include<iostream>
#include<bits/stdc++.h>
#include<sys/wait.h>
#include<dirent.h>
#include<sys/stat.h>
#include<time.h>
#include<grp.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<pwd.h>

#define MAX 100
#define STDIN 0
#define STDOUT 1
#define EXIT_FLAG 0
#define LEN 1000



using namespace std;

// Breaks strings according to the delimiter
int brstring(char **,char *,const char*);
void execio(char*);
void execext(char*);

void execpipe(char** argument, int n){
	//cout<<argument[0]<<endl; //for debugging
	// creating n-1 pipes
	int pip[n-1][2];
	//checking pipe creation
	for(int i=0;i<n-1;i++){
		if(pipe(pip[i])<0){
			cout<<"\ncould not create pipes"<<endl;
			return;
		}
	}
	pid_t fork_pd;
	// creating n child processes
	for (int i = 0; i < n; ++i)
	{
		fork_pd=fork();
		if(fork_pd==0){
			// enter chil process
			// take from previous pipe(except i==0)
			if(i!=0){
				// doesnt read from the next pipe
				close(pip[i-1][1]);
				// connect stdoin to read the end of the previous pipe
				dup2(pip[i-1][0], STDIN);
				// lose read pipe end
				close(pip[i-1][0]);
			}
			//write to next pipe (except i==n-1)
        	if(i!=n-1){
        		//doesn't read from the next pipe
				close(pip[i][0]);
				//connect stdout to write end of next pipe
        		dup2(pip[i][1], STDOUT);
        		// close write pipe end
				close(pip[i][1]);	
        	}
        	// closing all the pipes
        	for (int j = 0; j < n-1; ++j)
        	{
 				close(pip[j][0]);
				close(pip[j][1]);
        	}
        	//printf("here\n"); //for debugging
        	// executing i-th instruction
        	execio(argument[i]);
        	// exiting child
        	exit(0);
		} // else wait and close the pipes
		else{
			// suspend current thread for 10,000 micro-seconds
			usleep(10000);
			for (int k = 0; k < i; ++k)
			{
				close(pip[k][0]);
				close(pip[k][1]);
			}
		}
	}
	while(wait(NULL)>0);
	//printf("exiting\n"); // for de-bugging
	exit(0);
}

void execext(char *inp){
	char *argument[MAX];
	//If string is exit then exit
	if(strcmp(inp,"exit")==0){		
		cout<<"\nexiting from cs-shell"<<endl;
		exit(0);
	}
	brstring(argument,inp," \n");
	//execute the  other program
	execvp(argument[0],argument);
	cout<<"\ncould not execute/understand command"<<endl;
	kill(getpid(),SIGTERM);
}

void execio(char *inp){
	//cout<<inp<<endl; //for debugging
	// char array for argument
	char *argument[MAX];
	char *files[MAX];
	// character postions of "<" and ">" respectively, intitalise as false
	int posless=-1,posgreat=-1;
	// finding the postions of "<" and ">"
	int i=0;
	bool lessflag=false, greatflag=false;
	while(inp[i]!='\0'){
		if(inp[i]=='<' && lessflag==false){
			posless=i;
			lessflag=true;
		}
		if(inp[i]=='>' && greatflag==false){
			posgreat=i;
			greatflag=true;
		}
		i++;
	}
	//cout<<posless<<" "<<posgreat<<endl;// for debugging
	// break input arguments according &, <, > and \n
	i=brstring(argument,inp,"&<>\n");
	if(posless==-1 && posgreat==-1){
		// if both read from(<) and write to(>) are not there
		if(i<2){
			execext(argument[0]);
		}
		else{ 
			cout<<"\nError , More than req parameters entered"<<endl;
		}
	}
	else if(posless>0 && posgreat==-1){
		// if only read from(<) is present
		if(i>1 && i<3){
			int flag=brstring(files,argument[1]," \n");
			if(flag!=1){
				cout<<"\nAn error with the input file occured."<<endl;
				return;
			}
			// open the file in read only
			int input_file_descriptor = open(files[0], O_RDONLY);
			// cannot open the input file
			if(input_file_descriptor<0){
				cout<<"\nAn error occurred when trying to open the input file"<<endl;
				return;
			}
			//redirect stdin to the input file
			close(STDIN);
			dup(input_file_descriptor);
			close(input_file_descriptor);
		}
		else cout<<"\nError encountered!"<<endl;
	}
	else if(posless==-1 && posgreat>0){
		//cout<<"greater than"<<i<<endl; //for debugging
		// only write to(>) present
		if(i<3 && i>1){
			int flag=brstring(files,argument[1]," \n");
			if(flag!=1){
				cout<<"\nAn error with the output file occured!"<<endl;
				return;
			}
			// open the file, create it if necessary
			int output_file_descriptor = open(files[0], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			// cannot open/create the output file
			if(output_file_descriptor<0){
				cout<<"\nAn error occurred when trying to open/create the output file"<<endl;
				return;
			}
			//redirect stdout to the output file
			//cout<<"herefirst"<<endl;  // for debugging
			close(STDOUT);
			//cout<<"heresec"<<endl;	// for debugging
			dup(output_file_descriptor);
			//cout<<"here"<<endl;	// for debugging
			close(output_file_descriptor);
			//cout<<"here"<<endl;	// for debugging
		}
		else cout<<"Error ecountered!"<<endl;
	}
	else if(posless>0 && posgreat>0 && i>2 && i<4){
		// both are present
		int input_file_descriptor,output_file_descriptor,flag;
		if(posless>posgreat){
			flag=brstring(files,argument[1]," \n");
			if(flag!=1){
				cout<<"\nAn error with the output file occured!"<<endl;
				return;
			}
			// open/create the file to be written to
			output_file_descriptor = open(files[0], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			flag=brstring(files,argument[2]," \n");
			if(flag!=1){
				cout<<"\nAn error occurred when trying to open/create the output file"<<endl;
				return;
			}
			// open the file to read from
			input_file_descriptor = open(files[0], O_RDONLY);
		}
		else if(posless<posgreat){
			flag=brstring(files,argument[2]," \n");
			if(flag!=1){
				cout<<"\nAn error with the output file occured!"<<endl;
				return;
			}
			// open the file to write to
			output_file_descriptor = open(files[0], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			flag=brstring(files,argument[1]," \n");
			if(flag!=1){
				cout<<"\nAn error occurred when trying to open/create the output file"<<endl;
				return;
			}
			// open the file to read from
			input_file_descriptor = open(files[0], O_RDONLY);				
		}
		//redirect stdin and stdout to mentioned files
		close(STDIN);
		dup(input_file_descriptor);
		close(STDOUT);
		dup(output_file_descriptor);
		close(input_file_descriptor);
		close(output_file_descriptor);
	}
	else return;
	execext(argument[0]);
	return;
	exit(0);
}


int brstring(char **line,char *str,const char *delimiter){
	int i=0;
	line[i]=strtok(str,delimiter);
	while(line[i]!=NULL){
		i++;
		line[i]=strtok(NULL,delimiter);
	}
	return i;
}


int main(){
	cout<<"welcome to cs-shell\n";

	while(1){
		// 2-d array to store argument
		char *argument[MAX];
		// string for holding the input
		char inp[LEN] ="";
		// string for holding current directory
		char pwd[LEN] ="";
		// pointer to current directory
		char *ptr;
		// flag for "&"
		int amper = 0;
		// flag for fork
		int forkflag;
		// general iterators
		int i=0,j=0,k=0;

		// getcwd returns the address of working director and false if it's not able to
		ptr = getcwd(pwd, sizeof(pwd));

		if(ptr==NULL){
			perror("An error occured in getting the current directory");
			continue;
		}
		//print the current working directory
		cout<<pwd<<"$ ";

		// get input from the user
		fgets(inp,1000,stdin);

		// exit from shell
		if(strncmp(inp,"exit",4)==0){
			int k = 4;
			while(inp[k]==' '){
				k++;
			}
			if(inp[k]=='\n'){
				cout<<"\nExiting from cs-shell..."<<endl;	
				exit(0);
			}
		}
		// check for "&"(background process) and remove following spaces
		k=0;
		while(inp[k]!='\0'){

			if(inp[k]=='&'){
				int j = k+1;
				for (; inp[j]==' '; ++j){
					;
				}
				if(inp[j]=='\n'){
				amper = 1;
				inp[k] = '\n';
				inp[k+1]='\0';
				}
			}
			
			k++;
		}

		//split using "|" as delimiter
		i=0;
		argument[i]=strtok(inp,"|");
		//cout<<argument[0][0]<<endl;
		for (; argument[i]!=NULL; )
		{
			i++;
			argument[i]=strtok(NULL,"|");
		}

		// forking to create multiple processes
		forkflag=fork();
		if(forkflag==0){
			execpipe(argument, i);
		}
		else{
			// if process is not set to run in background, we wait for child to finish
			if (amper==0)
			{
				while(waitpid(-1,NULL,0)>0)
				{
					; //do nothing
				}
			}
		}

	}
}