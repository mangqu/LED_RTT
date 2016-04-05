#include <rtthread.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

#define BUFSZ   1024

void tcpclient(const char *url, int port)
{
	int sock;
	char *recv_data;
	int recv_bytes;
	struct hostent* host;
	struct sockaddr_in server_addr;

	recv_data = rt_malloc(BUFSZ);
	if (recv_data == RT_NULL)
	{
	    rt_kprintf("No memory\n");
	    return;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		rt_kprintf("create sock error!\n");
		rt_free(recv_data);
		return;
	}

	host = gethostbyname(url);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *(struct in_addr *)host->h_addr;
	rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
		rt_kprintf("connect host error!\n");
		rt_free(recv_data);
		return;
	}

	while (1)
	{
		recv_bytes = recv(sock, recv_data, BUFSZ, 0);
		if (recv_bytes <= 0)
		{
			closesocket(sock);
			rt_free(recv_data);
			return;
		}
		
		recv_data[recv_bytes] = '\0';
		if ((strcmp(recv_data, "q")==0) || (strcmp(recv_data, "Q")==0))
		{
			closesocket(sock);
			rt_free(recv_data);
			break;
		}
		else
		{
			send(sock, recv_data, recv_bytes, 0);
		}
	}
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(tcpclient, startup tcp client);
#endif
