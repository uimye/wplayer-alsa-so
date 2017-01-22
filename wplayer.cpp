#include <stdio.h>
#include "wplayer.h"
// #define DEBUG ;

wplayer :: wplayer()
{
	  player_init(&player);  
	  player.output.command = audio_output(0);  //������Ƶ��������
}

wplayer :: ~wplayer()
{
	  player_finish(&player);
}

int wplayer::play(char const *filepath)
{
   int result = 0;   
   int check_repeat = 1;
   union audio_control control;  
   player.entries = filepath;
  

//  set_gain(player, 0, 0);
     if (player.output.command) {    
    audio_control_init(&control, AUDIO_COMMAND_INIT); //����״̬Ϊ��ʼ��

    if (player.output.command(&control) == -1) {  //����Ƶ����豸
 		   printf("audio error \n");
      goto fail;
    }
  }
  while( (result == 0) && (check_repeat == 1) )
  {
    if (result == 0)
    	result = play_one(&player);   //���벥��
    if(player.repeat == 0)
    	check_repeat--;
  }
  /* drain and close audio */

  if (player.output.command) {
    audio_control_init(&control, AUDIO_COMMAND_FINISH);  //����״̬Ϊ����

    if (player.output.command(&control) == -1) {  //�ر���Ƶ����豸
 		   printf("audio error \n");
      goto fail;
    }
  }

  if (0) {
  fail:
    result = -1;
  }
  
  return result;
}



int wplayer :: checkrunning()
{
	return running;
}

void wplayer :: setrunning()
{
	running = 1;
}

void wplayer :: closerunning()
{
	stop();
	running = 0;
}

void wplayer :: stop()
{
  player.stop = 1;
  player.global_start  = mad_timer_zero;
}

void wplayer :: pause()
{
	player.stop = 1;
	player.global_start = player.stats.global_timer;
}

int wplayer :: checkstop()
{
	return player.stop;
}

void wplayer :: repeat()
{
#ifdef DEBUG
	printf("DEBUG : player.repeat : %d \n" , player.repeat);
#endif
	player.repeat = !player.repeat;
}
