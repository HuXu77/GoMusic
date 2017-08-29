/*
 * HeadphoneListener.h
 *
 *  Created on: May 14, 2013
 *      Author: nmmrc18
 */

#ifndef HEADPHONELISTENER_H_
#define HEADPHONELISTENER_H_

#include <QObject>
#include <bps/bps.h>
#include <bps/audiodevice.h>
#include <bb/AbstractBpsEventHandler>
#include <bb/multimedia/MediaPlayer>

using namespace bb::multimedia;

class HeadphoneListener : public QObject, public bb::AbstractBpsEventHandler {
	Q_OBJECT
public:
	HeadphoneListener();
	virtual ~HeadphoneListener();

	MediaPlayer* player;

	virtual void event(bps_event_t *event);

Q_SIGNALS:
	void headphonesRemoved();

private:
	bool isConnected;
};

#endif /* HEADPHONELISTENER_H_ */
