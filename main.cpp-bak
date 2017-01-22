#include "wplayer.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// #define DEBUG ;

wplayer wplayer;
char *path;
int result;
pthread_t playthread;


void *playthread_fun(void *p)
{
	result = wplayer.play(path);
}

void play()
{
   if(!wplayer.checkstop())
   {
	wplayer.stop();
	pthread_join(playthread , NULL);
#ifdef DEBUG
	printf("In play thread \n");
#endif
	pthread_create(&playthread , NULL , playthread_fun , NULL);
   }
   else
   {
#ifdef DEBUG
	printf("In play thread \n");
#endif
   	pthread_create(&playthread , NULL , playthread_fun , NULL);
   }
}

int main(int argc, char *argv[])
{
  char choose;
  path = argv[1];
  wplayer.setrunning();
  while(wplayer.checkrunning())
  {
  	  printf("Please input your choose \n 1.Stop \n 2.Pause \n 3.Play\n 4.Repeat \n 5.Quit\ninput:");
  	  choose = getchar();
  	  
  	  switch(choose)
  	  {
  	  		case '1':
  	  			printf("           Stop \n\n");
  	  			wplayer.stop();
  	  			break;
  	  		case '2':
  	  			printf("           Pause \n\n");
  	  			wplayer.pause();
  	  			break;
  	  		case '3':
  	  			printf("           Play \n\n");
  	  			play();
  	  			break;
  	  		case '4':
  	  			printf("           Repeat \n\n");
  	  			wplayer.repeat();
  	  			break;
  	  		case '5':
  	  			printf("           Quit \n\n");
  	  			wplayer.closerunning();
  	  			break;
  	  		default:
  	  			printf("error : please input a number betreen 1 to 3 \n");
  	  			break;
 	  }
	  
	  while(getchar() != '\n')
	  	continue;
 	 
  }
  	pthread_join(playthread , NULL);
  printf("Wplayer end\n");

  return result;
}


