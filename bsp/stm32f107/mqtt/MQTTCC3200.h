/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#ifndef __MQTT_CC3200_
#define __MQTT_CC3200_

#include <rtthread.h>
#include <lwip/netdb.h> 
#include <lwip/sockets.h>

#define	MilliTimer	(rt_tick_get()*1000/RT_TICK_PER_SECOND)

typedef struct __Timer Timer;

typedef struct __Timer {
	unsigned long systick_period;
	unsigned long end_time;
}Timer;

typedef struct __Network Network;

struct __Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);

void InitTimer(Timer*);

void NewNetwork(Network*);
int ConnectNetwork(Network* n, char* addr, int port);


#endif
