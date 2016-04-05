#include <rtthread.h>
#include <lwip/netdb.h> 
#include <lwip/sockets.h>
#include "lwip/netif.h"
#include "mqtt_client.h"

#include "MQTTPacket.h"

#include <board.h>


//#define		JJST_SRV
#define		LOCALHOST

#ifdef JJST_SRV
#define     HOSTNAME    	"www.jianjist.com"
#define     TOPIC_SUB   	"sensor"
#define     TOPIC_PUB		"sensor_pub"
#endif

#ifdef LOCALHOST
#define     HOSTNAME    	"172.24.120.200"
#define     TOPIC_SUB   	"sensor"
#define     TOPIC_PUB		"sensor_pub"
#endif

rt_mq_t mqtt_mq;

static int mqtt_sockID = -1;     // 全局变量
char mqtt_connected = 0;

int getdata(unsigned char* buf, int count) 
{
    int rc = recv(mqtt_sockID, buf, count, 0);
	//rt_kprintf("rc = %d, errno = %d\r\n", rc, errno);
	return rc;
}

static rt_timer_t ping_timer = RT_NULL;
static volatile int ping_resp_cnt = 0;
static unsigned char send_ping = 0;

static void mqtt_ping(void* parameter)
{
	static int last_ping_cnt = -1;

	if ((last_ping_cnt+1)!=ping_resp_cnt)
	{
		rt_kprintf("ping response lost!\r\n");
		mqtt_connected = 0;
		last_ping_cnt = -1;
		ping_resp_cnt = 0;
		return;
	}
	last_ping_cnt = ping_resp_cnt;
	send_ping = 1;
}

int mqtt_connect(char* hostname, int port, int *psock)
{
    struct hostent *mqtt_host;
    struct in_addr mqtt_ipaddr;              // mqtt服务器IP地址，二进制形式
    struct sockaddr_in mqtt_sockaddr;        // mqtt服务器IP信息 包括端口号、IP版本和IP版本

    // 第一步 DNS地址解析
    mqtt_host = gethostbyname(hostname);
	if(mqtt_host==NULL){
		rt_kprintf("gethostbyname fail!\r\n");
		return -1;
	}
    mqtt_ipaddr.s_addr = *(unsigned long *) mqtt_host->h_addr_list[0]; 
    rt_kprintf("HOST:%s\r\nIP Address:%s\r\n" , HOSTNAME, inet_ntoa(mqtt_ipaddr));

    // 第二步 创建套接字 
    if ((*psock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        rt_kprintf("Socket error = %d\r\n", *psock); 
        return -2;
    }
	rt_kprintf("Socket=%d\r\n", *psock);

    mqtt_sockaddr.sin_family = AF_INET;
    mqtt_sockaddr.sin_port = htons(port);
    mqtt_sockaddr.sin_addr = mqtt_ipaddr;
    rt_memset(&(mqtt_sockaddr.sin_zero), 0, sizeof(mqtt_sockaddr.sin_zero));
    // 第三步 连接mqtt 
    if (connect(*psock, (struct sockaddr *)&mqtt_sockaddr, 
                sizeof(struct sockaddr)) == -1) {
        rt_kprintf("Connect Fail!\n");
        return -3;
    }

    return 0;   // 返回0代表成功
}

int mqtt_pub(unsigned char* payload, int buflen)
{
	MQTTString topicStringPub = MQTTString_initializer;
	char buf[64];
	int len = 0, rc;

	if(!mqtt_connected){
		return -1;
	}

	topicStringPub.cstring = TOPIC_PUB;
	len = MQTTSerialize_publish(buf, sizeof(buf), 0, 0, 0, 0, topicStringPub, payload, buflen);
	rc = send(mqtt_sockID, buf, len, 0);
	return rc;
}

static int mqtt_client_connect(void)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc = 0;
	char client_id[64];
    unsigned char buf[128];
    int buflen = sizeof(buf);
    int msgid = 1;
    MQTTString topicString = MQTTString_initializer;
    int req_qos = 0;
    char payload[128];
    int payloadlen = strlen(payload);
    int len = 0;
    int tv = 1000;
    int ret = -1;

	if (ping_timer)
	{
		rt_timer_stop(ping_timer);
	}

	if (mqtt_sockID>=0)
	{
		closesocket(mqtt_sockID);
	}

    if ((rc = mqtt_connect(HOSTNAME, 1883, &mqtt_sockID)) < 0)
        goto exit;

    // 设置接收超时时间
    setsockopt(mqtt_sockID, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(int));

	rt_sprintf(client_id,"%02x%02x%02x", *(rt_uint8_t*)(0x1FFFF7E8+7), *(rt_uint8_t*)(0x1FFFF7E8+8), *(rt_uint8_t*)(0x1FFFF7E8+9));
	data.clientID.cstring = client_id;
	data.keepAliveInterval = 60;
	data.cleansession = 1;
	data.username.cstring = client_id;
	data.password.cstring = client_id;

	len = MQTTSerialize_connect(buf, buflen, &data);
	rc = send(mqtt_sockID, buf, len, 0);

    /* wait for connack */
    if (MQTTPacket_read(buf, buflen, getdata) == CONNACK) 
	{
		unsigned char sessionPresent, connack_rc;

		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0) 
		{
            rt_kprintf("Unable to connect, return code %d\n", connack_rc);
            goto exit;
       } 
		else 
		{
			rt_kprintf("Connect OK\n", connack_rc);
			// 建立心跳任务
			if (ping_timer==RT_NULL)
			{
				ping_timer = rt_timer_create("mqtt_ping", mqtt_ping, RT_NULL, RT_TICK_PER_SECOND*5, RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
			}
			if (ping_timer)
			{
				rt_timer_start(ping_timer);
				rt_kprintf("ping timer start!\r\n");
			}
			else
			{
				rt_kprintf("Create ping timer failed!\r\n");
			}
		}
	}
	else
		goto exit;

	// 订阅主题
	topicString.cstring = TOPIC_SUB;
	len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);
	rc = send(mqtt_sockID, buf, len, 0);
		
	if (MQTTPacket_read(buf, buflen, getdata) == SUBACK) 
	{
		unsigned short submsgid;
		int subcount;
		int granted_qos;

		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
		if (granted_qos != 0) 
		{
			rt_kprintf("granted qos != 0, %d\n", granted_qos);
			goto exit;
		} 
		else 
		{
			rt_kprintf("Subscribe OK\n");
			mqtt_connected = 1;
			return 0;
        }
    }

exit:
	rt_kprintf("connect fail, close socket %d!\r\n", mqtt_sockID);
	if(mqtt_sockID>=0)
		closesocket(mqtt_sockID);

	return ret;
}

static int mqtt_parse_response(char *buf, char len, mqtt_cmd_t *cmd)
{
	int ret = 0;

	if (len<10)
	{
		rt_kprintf("Invalid cmd\r\n");
		return 0;
	}

	ret += sscanf(buf, "%08x", &(cmd->peerAddr));
	ret += sscanf(buf+8, "%02x", (char*)&(cmd->type));
	memset(cmd->msg, 0, MAX_MQTT_MSG_LEN);
	memcpy(cmd->msg, buf+10, len-10);
	cmd->msg_len = len-10;
	ret += len-10;
	return ret;
}

static void mqtt_client_thread_entry(void* param)
{
    int rc = 0;
    unsigned char buf[128];
	static int sub_cnt = 0;
    int buflen = sizeof(buf);
	MQTTString topicStringPub = MQTTString_initializer;
    char payload[128];
    int len = 0;
    int ret = 0;

	unsigned char dup;
	int qos;
	unsigned char retained;
	unsigned short msgid;
	int payloadlen_in;
	unsigned char* payload_in;
	MQTTString receivedTopic;

	mqtt_cmd_t cmd;

	// Waiting till NETIF UP
	rt_kprintf("\r\nWaiting for net up...\r\n");
	rt_thread_delay(RT_TICK_PER_SECOND/2);
	while (1)
	{
		if ((netif_default->flags)&NETIF_FLAG_UP)
		{
		#ifdef RT_USING_FINSH
			extern void list_if(void);
			list_if();
		#endif
			break;
		}
		GPIO_SetBits(GPIOA, GPIO_Pin_5);
		rt_thread_delay(RT_TICK_PER_SECOND/10);
		GPIO_ResetBits(GPIOA, GPIO_Pin_5);	// To indicate NET is UP
		rt_thread_delay(RT_TICK_PER_SECOND/50);
	}

	topicStringPub.cstring = TOPIC_PUB;
	
	mqtt_mq = rt_mq_create("mqtt_mq", sizeof(mqtt_cmd_t), 32, RT_IPC_FLAG_FIFO);

    while (1)
	{
		// try connect if lost connetion
		while (!mqtt_connected)
		{
			mqtt_client_connect();
			rt_thread_delay(RT_TICK_PER_SECOND/2);
		}
		GPIO_SetBits(GPIOA, GPIO_Pin_5);	// To indicate connect is OK
		ret = MQTTPacket_read(buf, buflen, getdata);
		if (ret == PUBLISH) 
		{
			GPIO_ResetBits(GPIOA, GPIO_Pin_5);

			rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                    &payload_in, &payloadlen_in, buf, buflen);
			rt_kprintf("msg: %.*s\n", payloadlen_in, payload_in);
			if (mqtt_parse_response((char*)payload_in, payloadlen_in, &cmd)>=2)
			{
				// Send msg to MRFI thread for RF control
				rt_mq_send(mqtt_mq, &cmd, sizeof(cmd));
			}
			
			GPIO_SetBits(GPIOA, GPIO_Pin_5);
        }
		else if (ret == PINGRESP)
		{
            rt_kprintf("PINGRESP: %d\n", ++ping_resp_cnt);
        }

		if (send_ping) 
		{
			unsigned char buf[16];
			int buflen = sizeof(buf);
			int len;
			send_ping = 0;
			GPIO_ResetBits(GPIOA, GPIO_Pin_5);

			rt_sprintf(buf,"%02x%02x%02x", *(rt_uint8_t*)(0x1FFFF7E8+7), *(rt_uint8_t*)(0x1FFFF7E8+8), *(rt_uint8_t*)(0x1FFFF7E8+9));
			len = MQTTSerialize_pingreq(buf, buflen);
			send(mqtt_sockID, buf, len, 0);
			rt_kprintf("ping...\t");

			GPIO_SetBits(GPIOA, GPIO_Pin_5);
		}
    }
}


int mqtt_client(void)
{
    rt_thread_t tid;

	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    tid = rt_thread_create("MQTT",
        mqtt_client_thread_entry, RT_NULL,
        2048, 15, 20);
    if(tid != RT_NULL){
		rt_thread_startup(tid);
    }

    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(mqtt_client, Get Actutor Information From MQTT server Using MQTT Protocol);
#endif

