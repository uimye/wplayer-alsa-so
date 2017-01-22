#ifndef WPLAYER_H
#define WPLAYER_H



extern "C"
{
	#include "player.h"
}
class wplayer
{
private:
	struct player player;
	int running;
	

	
public:
	
	wplayer();
	~wplayer();
	int next();
	int previous();
	void stop();
	void pause();
	int play(char const *);
	int checkrunning();
	void setrunning();
	void closerunning();
	int checkstop();
	void repeat();
};


#endif

