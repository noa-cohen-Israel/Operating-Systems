 #define _GNU_SOURCE
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <assert.h>
 #include <sys/wait.h>
 #include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
     
     

 int prepare(void){
     signal(SIGINT,SIG_IGN);
      printf("Welcome to MyShell ^_^\n");
  return 0;
  }
   int finalize(void){
          return 0;  
 }
 void ctrl_c(int signum){
     kill(getppid(), SIGTERM);
 }
 
  void zombie(int signum){
      //int stut;
     //wait(&stut);
       while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
 }
 
 void  func1(char **arglist)
 {
          int stat_val;

 		pid_t pid1;
 		pid1=fork();
 		if(pid1==-1) {   
         perror("Failed to fork");
         return;
      }  
 		if (pid1 == 0){
 		signal(SIGINT, ctrl_c);
 				execvp(arglist[0],arglist);
 				perror("execvp failed");
 				return;
 		}
        waitpid(pid1,&stat_val,0); 
 }
 
 
 void  func2(char **arglist)
 {
  		pid_t pid1;
 		pid1=fork();
 		if(pid1==-1) {   
         perror("Failed to fork");
         return;
      }  
 		if (pid1 == 0){
 		
 				execvp(arglist[0],arglist);
 				perror("execvp failed");
 				return;
 		}
 		signal(SIGCHLD, zombie);
 }
  
void  func3(char **arglist,int idx_pipe)
 {  
     int stat_val;
 		int pipe_fd[2]; 
 		pid_t pid1,pid2;
 		arglist[idx_pipe]=NULL;
 	if(pipe(pipe_fd)!=0){
 	perror("ERROR");
 	return;
 	}

      
 		pid1=fork();
 		if(pid1==-1) {   
         perror("Failed to fork");
         return;
      } 
     		if (pid1 == 0){
     		signal(SIGINT, ctrl_c);
 				close(1);
 				dup2(pipe_fd[1],1);
 				execvp(arglist[0],arglist);
 				perror("execvp failed");
 				return;
 		}else
 		{	
 				close(pipe_fd[1]);
 				 		pid2=fork();
 		if(pid2==-1) {   
         perror("Failed to fork");
         return;
      } 
 				if  (pid2==0){
 				signal(SIGINT, ctrl_c);
 						close(0);
 						dup2(pipe_fd[0],0);
 						execvp(arglist[idx_pipe+1],&arglist[idx_pipe+1]);
 						perror("execvp failed");
 						return;
 				}
 		} 
        wait(&stat_val); 
         wait(&stat_val); 
 }
 
 int process_arglist(int count, char **arglist){

  if(!strcmp(arglist[count-1],"&")){
  arglist[count-1]=NULL;
 func2(arglist);
 }
 else{
 for(int i=0 ; i<count;i++){
   if(!strcmp(arglist[i],"|")){
 func3(arglist,i);
 return 1;       
 }
 }
 func1(arglist);
 }
return 1;       
 }


 
 
 
 
 
 
 
 
 
 
 
 


