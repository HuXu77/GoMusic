/*
 * HeadphoneListener.cpp
 *
 *  Created on: May 14, 2013
 *      Author: nmmrc18
 */

#include "HeadphoneListener.h"

HeadphoneListener::HeadphoneListener() :
isConnected(false) {
	subscribe(audiodevice_get_domain());
	bps_initialize();
	audiodevice_request_events(0);
}

/*void HeadphoneListener::startListening() {
	while (listen) {
		audiodevice_get_info(headphones, &info);
		if (audiodevice_info_is_connected(info)) {
			isConnected = true;
		} else {
			if (isConnected) {
				emit headphonesRemoved();
			}
			isConnected = false;
		}
	}
	emit finished();
}*/

void HeadphoneListener::event(bps_event_t *event) {
	int domain = bps_event_get_domain(event);
	if (domain == audiodevice_get_domain()) {
		audiodevice_info_t *info;
		audiodevice_device_t headphones = AUDIODEVICE_DEVICE_HEADPHONE;
		audiodevice_get_info(headphones, &info);
		if (audiodevice_info_is_connected(info)) {
			isConnected = true;
		} else {
			if (isConnected) {
				//player->pause();
				emit headphonesRemoved();
			}
			isConnected = false;
		}
		audiodevice_free_info(&info);
	}
}

HeadphoneListener::~HeadphoneListener() {
	bps_shutdown();
}

