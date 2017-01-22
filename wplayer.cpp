#include <stdio.h>
#include "wplayer.h"
// #define DEBUG ;

wplayer :: wplayer()
{
	  player_init(&player);  
	  player.output.command = audio_output(0);  //设置音频解析函数
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
    audio_control_init(&control, AUDIO_COMMAND_INIT); //设置状态为初始化

    if (player.output.command(&control) == -1) {  //打开音频输出设备
 		   printf("audio error \n");
      goto fail;
    }
  }
  while( (result == 0) && (check_repeat == 1) )
  {
    if (result == 0)
    	result = play_one(&player);   //进入播放
    if(player.repeat == 0)
    	check_repeat--;
  }
  /* drain and close audio */

  if (player.output.command) {
    audio_control_init(&control, AUDIO_COMMAND_FINISH);  //设置状态为结束

    if (player.output.command(&control) == -1) {  //关闭音频输出设备
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
