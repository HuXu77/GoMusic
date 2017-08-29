/*
 * NetworkListener.cpp
 *
 *  Created on: Oct 8, 2013
 *      Author: nmmrc18
 */

#include "NetworkListener.h"
#include <QDebug>

NetworkListener::NetworkListener() {
	subscribe(netstatus_get_domain());
	bps_initialize();
	netstatus_request_events(0);
}

void NetworkListener::event(bps_event_t *event) {
	int domain = bps_event_get_domain(event);
	if (domain == netstatus_get_domain()) {
		const char *interface = netstatus_event_get_default_interface(event);
		netstatus_interface_details_t *details;
		netstatus_get_interface_details(interface, &details);
		if (netstatus_interface_is_connected(details)) {
			netstatus_interface_type_t type = netstatus_interface_get_type(details);
			if (type == NETSTATUS_INTERFACE_TYPE_CELLULAR) {
				qDebug() << "GMA: Interface connected: " << "4G";
			} else if (type == NETSTATUS_INTERFACE_TYPE_WIFI) {
				qDebug() << "GMA: Interface connected: " << "Wifi";
			}
		}
		netstatus_free_interface_details(&details);
	}
}

NetworkListener::~NetworkListener() {
	bps_shutdown();
}

