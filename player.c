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
 * $Id: player.c,v 1.69 2004/02/23 21:34:53 rob Exp $
 */
 
 #include "player.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


#define O_BINARY  0
#define STDIN_FILENO	0
#define MPEG_BUFSZ	40000	/* 2.5 s at 128 kbps; 1 s at 320 kbps */
#define FREQ_TOLERANCE	2	/* percent sampling frequency tolerance */



/*
 * NAME:	player_finish()
 * DESCRIPTION:	destroy a player structure
 */
void player_finish(struct player *player)
{
  if (player->output.resampled) {
    resample_finish(&player->output.resample[0]);
    resample_finish(&player->output.resample[1]);

    free(player->output.resampled);
    player->output.resampled = 0;
  }
}





/*
 * NAME:	decode->output()
 * DESCRIPTION: configure audio module and output decoded samples
 */
static
enum mad_flow decode_output(void *data, struct mad_header const *header,
			    struct mad_pcm *pcm)
{
  //printf("begin to decode_output...\n");
  struct player *player = data;
  struct output *output = &player->output;
  mad_fixed_t const *ch1, *ch2;
  unsigned int nchannels;
  union audio_control control;

  ch1 = pcm->samples[0];
  ch2 = pcm->samples[1];

  switch (nchannels = pcm->channels) {
  case 1:
    ch2 = 0;
    if (output->select == PLAYER_CHANNEL_STEREO) {
      ch2 = ch1;
      nchannels = 2;
    }
    break;

  case 2:
    switch (output->select) {
    case PLAYER_CHANNEL_RIGHT:
      ch1 = ch2;
      /* fall through */

    case PLAYER_CHANNEL_LEFT:
      ch2 = 0;
      nchannels = 1;
      /* fall through */

    case PLAYER_CHANNEL_STEREO:
      break;

    default:
      if (header->mode == MAD_MODE_DUAL_CHANNEL) {
	if (output->select == PLAYER_CHANNEL_DEFAULT) {

	  output->select = -PLAYER_CHANNEL_LEFT;
	}

	ch2 = 0;
	nchannels = 1;
      }
    }
  }

  if (output->channels_in != nchannels ||
      output->speed_in != pcm->samplerate) {
    unsigned int speed_request;


    speed_request = output->speed_request ?
      output->speed_request : pcm->samplerate;

    audio_control_init(&control, AUDIO_COMMAND_CONFIG);

    control.config.channels  = nchannels;
    control.config.speed     = speed_request;
    control.config.precision = output->precision_in;

    if (output->command(&control) == -1) {
//      error("output", audio_error);
      return MAD_FLOW_BREAK;
    }

    output->channels_in  = nchannels;
    output->speed_in     = pcm->samplerate;

    output->channels_out  = control.config.channels;
    output->speed_out     = control.config.speed;
    output->precision_out = control.config.precision;



    /* check whether resampling is necessary */
    if (abs(output->speed_out - output->speed_in) <
	(long) FREQ_TOLERANCE * output->speed_in / 100) {
      if (output->resampled) {
	resample_finish(&output->resample[0]);
	resample_finish(&output->resample[1]);

	free(output->resampled);
	output->resampled = 0;
      }
    }
    else {
      if (output->resampled) {
	resample_finish(&output->resample[0]);
	resample_finish(&output->resample[1]);
      }
      else {
	output->resampled = malloc(sizeof(*output->resampled));
	if (output->resampled == 0) {
//	  error("output",
//		_("not enough memory to allocate resampling buffer"));

	  output->speed_in = 0;
	  return MAD_FLOW_BREAK;
	}
      }

      if (resample_init(&output->resample[0],
			output->speed_in, output->speed_out) == -1 ||
	  resample_init(&output->resample[1],
			output->speed_in, output->speed_out) == -1) {
//	error("output", _("cannot resample %u Hz to %u Hz"),
	//      output->speed_in, output->speed_out);

	free(output->resampled);
	output->resampled = 0;

	output->speed_in = 0;
	return MAD_FLOW_BREAK;
      }
    }
  }

  audio_control_init(&control, AUDIO_COMMAND_PLAY);

  if (output->channels_in != output->channels_out)
    ch2 = (output->channels_out == 2) ? ch1 : 0;

  if (output->resampled) {
    control.play.nsamples = resample_block(&output->resample[0],
					   pcm->length, ch1,
					   (*output->resampled)[0]);
    control.play.samples[0] = (*output->resampled)[0];

    if (ch2 == ch1)
      control.play.samples[1] = control.play.samples[0];
    else if (ch2) {
      resample_block(&output->resample[1], pcm->length, ch2,
		     (*output->resampled)[1]);
      control.play.samples[1] = (*output->resampled)[1];
    }
    else
      control.play.samples[1] = 0;
  }
  else {
    control.play.nsamples   = pcm->length;
    control.play.samples[0] = ch1;
    control.play.samples[1] = ch2;
  }

  control.play.mode  = output->mode;
  control.play.stats = &player->stats.audio;

  if (output->command(&control) == -1) {
//    error("output", audio_error);
    return MAD_FLOW_BREAK;
  }

  ++player->stats.play_framecount;
  mad_timer_add(&player->stats.play_timer, header->duration);

  return MAD_FLOW_CONTINUE;
}



/*
 * NAME:	decode->header()
 * DESCRIPTION:	decide whether to continue decoding based on header
 */
static
enum mad_flow decode_header(void *data, struct mad_header const *header)
{
  struct player *player = data;
  
  if(player->stop == 0)
  {


    /* accounting (except first frame) */

    if (player->stats.absolute_framecount) {
    ++player->stats.absolute_framecount;
    mad_timer_add(&player->stats.absolute_timer, header->duration);

    mad_timer_add(&player->stats.global_timer, header->duration);

    if (mad_timer_compare(player->stats.global_timer,player->global_start) < 0)
      return MAD_FLOW_IGNORE;
      
      
    }
    ++player->stats.absolute_framecount;
    return MAD_FLOW_CONTINUE;
  }
  else
  {
    return MAD_FLOW_STOP;
  }
}



/*
 * NAME:	decode->input_read()
 * DESCRIPTION:	(re)fill decoder input buffer by reading a file descriptor
 */
static
enum mad_flow decode_input_read(void *data, struct mad_stream *stream)
{
  printf("begin to decode_input_read...\n");
  struct player *player = data;
  struct input *input = &player->input;
  int len;

  if (input->eof)
    return MAD_FLOW_STOP;

  printf("input->length = %d\n", input->length);
  if (stream->next_frame) { 
      printf("stream->next_frame...\n");
      memmove(input->data, stream->next_frame,
	                 input->length = &input->data[input->length] - stream->next_frame);
  }

  do {
    printf("read input->length = %d\n", input->length);
    len = read(input->fd, input->data + input->length,
	       MPEG_BUFSZ - input->length);
  }
  while (len == -1);

  if (len == -1) {
    error("input", ":read");
    return MAD_FLOW_BREAK;
  }
  else if (len == 0) {
    input->eof = 1;

    assert(MPEG_BUFSZ - input->length >= MAD_BUFFER_GUARD);

    while (len < MAD_BUFFER_GUARD)
      input->data[input->length + len++] = 0;
  }

  printf("begin to mad_stream_buffer...\n");
  mad_stream_buffer(stream, input->data, input->length += len);

  return MAD_FLOW_CONTINUE;
}



static
int decode(struct player *player)
{
  struct mad_decoder decoder;
  int options, result;


    player->input.data = malloc(MPEG_BUFSZ); //分配缓冲池
    if (player->input.data == 0) {
      error("decode", "not enough memory to allocate input buffer");
      return -1;
    }

  player->input.length = 0;
  player->stop = 0;
  player->input.eof = 0;
  player->output.channels_in = 0;
  /* reset statistics */
  player->stats.absolute_framecount   = 0;
  player->stats.play_framecount       = 0;
  player->stats.error_frame           = -1;
  player->stats.vbr                   = 0;
  player->stats.bitrate               = 0;
  player->stats.vbr_frames            = 0;
  player->stats.vbr_rate              = 0;
  player->stats.global_timer          = mad_timer_zero;


  printf("begin to mad_decoder_init...\n");
  mad_decoder_init(&decoder, player,
		   decode_input_read,
		   decode_header, 0 ,
		   player->output.command ? decode_output : 0,
		   0, 0);  //设置回调函数
		   
		   /*
		   decoder -> 数据结构
		   player -> 用户自定义的回调函数
		   decode_input_read -> 读取歌曲文件
		   decode_header -> 用于决定是否解码此帧的判断函数
		   filter_func - 0 ->  用于在播放中进行控制的函数(详细可以参考原版的madplay)
		   decode_output  -> 输出解码好的音频信息
		   error_func - 0 -> 用于处理解码失败的帧(详细可以参考原版的madplay)
		   message_func - 0 -> madplay没有定义这个函数 我也不知道这个干嘛的.....汗
		   */

  options = 0;
  if (player->options & PLAYER_OPTION_IGNORECRC)  //是否忽略每帧的CRC效验
    options |= MAD_OPTION_IGNORECRC;

  printf("begin to mad_decoder_options...\n");
  mad_decoder_options(&decoder, options);  //初始化decoder的控制信息

  printf("begin to mad_decoder_run...\n");
  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC); //运行解码器

  printf("begin to mad_decoder_finish...\n");
  mad_decoder_finish(&decoder);


  if (player->input.data) {
    free(player->input.data);
    player->input.data = 0;
  }

 

  return result;
}


/*
 * NAME:	play_one()
 * DESCRIPTION:	open and play a single file
 */
int play_one(struct player *player)
{
  char const *file = &player->entries[0];
  int result;

    player->input.path = file;
    player->input.fd   = open(file, O_RDONLY | O_BINARY);  //打开歌曲文件
    if (player->input.fd == -1) {
      error(0, ":", file);
      return -1;
    }

  /* reset file information */
  result = decode(player);

  if (player->input.fd != STDIN_FILENO &&
      close(player->input.fd) == -1 && result == 0) {   //关闭歌曲文件
    error(0, ":", player->input.path);
    result = -1;
  }

  return result;
}


/*
 * NAME:	player_init()
 * DESCRIPTION:	initialize player structure
 */
void player_init(struct player *player)
{
  player->stop = 1;       
  player->options = 0;
  player->repeat  = 0;
  player->global_start = mad_timer_zero;
  player->entries = 0;



  player->input.path   = 0;
  player->input.fd     = -1;
  player->input.data   = 0;
  player->input.length = 0;
  player->input.eof    = 0;


  player->output.mode          = AUDIO_MODE_DITHER;
  player->output.voladj_db     = 0;
  player->output.attamp_db     = 0;
  player->output.replay_gain   = 0;
 // player->output.filters       = 0;
  player->output.channels_in   = 0;
  player->output.channels_out  = 0;
  player->output.select        = PLAYER_CHANNEL_DEFAULT;
  player->output.speed_in      = 0;
  player->output.speed_out     = 0;
  player->output.speed_request = 0;
  player->output.precision_in  = 0;
  player->output.precision_out = 0;
  player->output.command       = 0;
  /* player->output.resample */
  player->output.resampled     = 0;

  player->stats.play_timer            = mad_timer_zero;
  player->stats.global_timer          = mad_timer_zero;
  player->stats.absolute_timer        = mad_timer_zero;
  player->stats.absolute_framecount   = 0;
  player->stats.play_framecount       = 0;
  player->stats.error_frame           = -1;
  player->stats.mute_frame            = 0;
  player->stats.vbr                   = 0;
  player->stats.bitrate               = 0;
  player->stats.vbr_frames            = 0;
  player->stats.vbr_rate              = 0;
  player->stats.nsecs                 = 0;
  player->stats.audio.clipped_samples = 0;
  player->stats.audio.peak_clipping   = 0;
  player->stats.audio.peak_sample     = 0;
}

