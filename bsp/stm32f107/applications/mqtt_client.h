#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#define	MAX_MQTT_MSG_LEN	18

typedef struct 
{
	unsigned int peerAddr;
	unsigned char msg_len;
	unsigned char type;
	unsigned char msg[MAX_MQTT_MSG_LEN];
} mqtt_cmd_t;

extern rt_mq_t mqtt_mq;


#endif

