/*
 * madplay - MPEG audio decoder and player
 * Copyright (C) 2000-2004 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: player.h,v 1.24 2004/02/23 21:34:53 rob Exp $
 */
 
 
 #ifndef PLAYER_H
#define PLAYER_H

#include "resample.h"
#include "audio.h"
#include "mad.h"

enum {
  PLAYER_OPTION_SHUFFLE      = 0x0001,
  PLAYER_OPTION_DOWNSAMPLE   = 0x0002,
  PLAYER_OPTION_IGNORECRC    = 0x0004,
  PLAYER_OPTION_IGNOREVOLADJ = 0x0008,

  PLAYER_OPTION_SKIP         = 0x0010,
  PLAYER_OPTION_TIMED        = 0x0020,
  PLAYER_OPTION_TTYCONTROL   = 0x0040,
  PLAYER_OPTION_STREAMID3    = 0x0080,

  PLAYER_OPTION_FADEIN       = 0x0100,
  PLAYER_OPTION_FADEOUT      = 0x0200,
  PLAYER_OPTION_GAP          = 0x0400,
  PLAYER_OPTION_CROSSFADE    = 0x0800,

# if defined(EXPERIMENTAL)
  PLAYER_OPTION_EXTERNALMIX  = 0x1000,
  PLAYER_OPTION_EXPERIMENTAL = 0x2000,
# endif

  PLAYER_OPTION_SHOWTAGSONLY = 0x4000
};

enum player_channel {
  PLAYER_CHANNEL_DEFAULT = 0,
  PLAYER_CHANNEL_LEFT    = 1,
  PLAYER_CHANNEL_RIGHT   = 2,
  PLAYER_CHANNEL_MONO    = 3,
  PLAYER_CHANNEL_STEREO  = 4
};

struct player {
  int stop; 
  int options;
  int repeat;
  char const *entries;
  mad_timer_t global_start;  //全局开始播放时间
    
  struct input {
    char const *path;  //歌曲文件路径
    int fd;  //文件标识符
    unsigned char *data; //缓冲池
    unsigned long length; //文件长度
    int eof;
  } input;

  struct output {
	 enum audio_mode mode;
    double voladj_db;
    double attamp_db;
	 audio_ctlfunc_t *command;
    int replay_gain;
    unsigned int channels_in;
    unsigned int channels_out;
    unsigned int speed_in;
    unsigned int speed_out;
    unsigned int speed_request;
	 enum player_channel select;
    unsigned int precision_in;
    unsigned int precision_out;
	 struct resample_state resample[2];
    mad_fixed_t (*resampled)[2][MAX_NSAMPLES];
  } output;

  struct stats {
 	 mad_timer_t play_timer;    //已经播放的时间
    unsigned long absolute_framecount; //总解码帧数
    unsigned long play_framecount;  //已解码帧数
    unsigned long error_frame;   //错误帧数
    unsigned long mute_frame;  //忽略帧数
    mad_timer_t global_timer;  //单曲播放时间
    mad_timer_t absolute_timer;  //总播放时间
    int vbr;
    unsigned int bitrate;    
    unsigned long vbr_frames;
    unsigned long vbr_rate;
    signed long nsecs;
	 struct audio_stats audio;
  } stats;
};

void player_init(struct player *);
int play_one(struct player *player);
void player_finish(struct player *player);

#endif
