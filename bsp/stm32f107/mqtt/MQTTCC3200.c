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

#include "MQTTCC3200.h"

char expired(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}


void countdown_ms(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}


void countdown(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}


int left_ms(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}


void InitTimer(Timer* timer) {
	timer->end_time = 0;
}


int rtt_lwip_read(Network* n, unsigned char* buffer, int len, int timeout_ms) {

	int bytes = 0;

	if(timeout_ms==0)
		timeout_ms = 100;
	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_ms, sizeof(int));

	while (bytes < len)
	{
		int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
		//rt_kprintf("rc=%d, errno=%d\r\n", rc, errno);
		if (rc == -1)
		{
			if (errno != ENOTCONN && errno != ECONNRESET)
			{
				bytes = -1;
				break;
			}
		}
		else
			bytes += rc;
	}
	return bytes;
}

int rtt_lwip_write(Network* n, unsigned char* buffer, int len, int timeout_ms) {
	int rc;

	if(timeout_ms==0)
		timeout_ms = 100;
	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_ms, sizeof(int));
	rc = send(n->my_socket, buffer, len, 0);
	return rc;
}

void rtt_lwip_disconnect(Network* n) {
	closesocket(n->my_socket);
}

void NewNetwork(Network* n) {
	n->my_socket = 0;
	n->mqttread = rtt_lwip_read;
	n->mqttwrite = rtt_lwip_write;
	n->disconnect = rtt_lwip_disconnect;
}

int ConnectNetwork(Network* n, char* addr, int port)
{
	int type = SOCK_STREAM;
	struct sockaddr_in address;
	int rc = -1;
	u8_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0)
	{
		struct addrinfo* res = result;

		/* prefer ip4 addresses */
		while (res)
		{
			if (res->ai_family == AF_INET)
			{
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET)
		{
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
			address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		}
		else
			rc = -1;

		freeaddrinfo(result);
	}

	if (rc == 0)
	{
		n->my_socket = socket(family, type, 0);
		if (n->my_socket != -1)
		{
			rc = connect(n->my_socket, (struct sockaddr*)&address, sizeof(address));
		}
	}

	return rc;
}


