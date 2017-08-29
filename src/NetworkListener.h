/*
 * NetworkListener.h
 *
 *  Created on: Oct 8, 2013
 *      Author: nmmrc18
 */

#ifndef NETWORKLISTENER_H_
#define NETWORKLISTENER_H_

#include <QObject>
#include <bps/bps.h>
#include <bps/netstatus.h>
#include <bb/AbstractBpsEventHandler>

class NetworkListener : public QObject, public bb::AbstractBpsEventHandler {
	Q_OBJECT
public:
	NetworkListener();
	virtual ~NetworkListener();

	virtual void event(bps_event_t *event);
};

#endif /* NETWORKLISTENER_H_ */
