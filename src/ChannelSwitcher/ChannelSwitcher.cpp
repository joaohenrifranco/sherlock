#include "ChannelSwitcher.h"

os_timer_t channelSwitcherTimer;

void SetChannelHop(void *pauseFlag) {
	os_timer_disarm(&channelSwitcherTimer);
	os_timer_setfn(&channelSwitcherTimer, (os_timer_func_t *) ChannelChangerCallback, pauseFlag);
	os_timer_arm(&channelSwitcherTimer, CHANNEL_HOP_INTERVAL_MS, 1);
}

// Callback for channel hoping
void ChannelChangerCallback(void *pauseFlag)
{
	if (*(bool*)pauseFlag == true)
		return;
	uint8 new_channel = wifi_get_channel() + 1;
	if (new_channel > 14)
		new_channel = 1;
	wifi_set_channel(new_channel);
}