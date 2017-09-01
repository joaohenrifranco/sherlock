#ifndef CHANNEL_SWITCHER_H
#define CHANNEL_SWITCHER_H

extern "C" { 
	#include <user_interface.h>
}

#include "../config.h"

void SetChannelHop(void *pauseFlag);
void ChannelChangerCallback(void *pauseFlag);

int myokay();


#endif