#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

int main( )
{
  pid_t pid = fork( );

  if( pid == 0 )
  {
    // Notice you can add as many NULLs on the end as you want
    char *env[] = {"PATH =/usr/local/bin","PATH=/usr/bin",
      "PATH=/bin",NULL};
    char * token[5] = {"man", "ls", NULL, NULL, NULL};
    int ret = execvpe("man", token, env);  
  
    if( ret == -1 )
    {
      perror("execl failed: ");
    }
  }
  else 
  {
    int status;
    wait( & status );
  }

  return 0;
}
