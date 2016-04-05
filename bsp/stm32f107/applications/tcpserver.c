#include <rtthread.h>
#include <lwip/sockets.h>

static const char send_data[] = "This is TCP Server from RT-Thread.";

void tcpserv(void *parameter)
{
	char *recv_data; 
	rt_uint32_t sin_size;
	int sock, connected, bytes_received;
	struct sockaddr_in server_addr, client_addr;
	rt_bool_t stop = RT_FALSE;

	recv_data = rt_malloc(1024);
	if (recv_data == RT_NULL)
	{
	    rt_kprintf("No memory\n");
	    return;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
	    rt_kprintf("Socket error\n");
	    rt_free(recv_data);
	    return;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5000);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	rt_memset(&(server_addr.sin_zero), 8, sizeof(server_addr.sin_zero));

	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
	    rt_kprintf("Unable to bind\n");

	    /* release recv buffer */
	    rt_free(recv_data);
	    return;
	}

	if (listen(sock, 5) == -1)
	{
	    rt_kprintf("Listen error\n");

	    /* release recv buffer */
	    rt_free(recv_data);
	    return;
	}

	rt_kprintf("\nTCPServer Waiting for client on port 5000...\n");
	while (stop != RT_TRUE)
	{
		sin_size = sizeof(struct sockaddr_in);

		connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);

		rt_kprintf("I got a connection from (%s , %d)\n",
		           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		send(connected, send_data, strlen(send_data), 0);

		while (1)
		{
			bytes_received = recv(connected, recv_data, 1024, 0);
			if (bytes_received <= 0)
			{
			    lwip_close(connected);
			    break;
			}
			
			recv_data[bytes_received] = '\0';
			if (strcmp(recv_data , "q") == 0 || strcmp(recv_data , "Q") == 0)
			{
			    lwip_close(connected);
			    break;
			}
			else if (strcmp(recv_data, "exit") == 0)
			{
			    lwip_close(connected);
			    stop = RT_TRUE;
			    break;
			}
			else
			{
				//TODO
				switch (recv_data[0])
				{
					case '1':
						send(connected, "zhangzhen", strlen("zhangzhen"), 0);
						break;
					case '2':
						send(connected, "chendaxia", strlen("chendaxia"), 0);
						break;
					default:
						send(connected, "error", strlen("error"), 0);
						break;
				}
			}
		}
	}

	lwip_close(sock);
	rt_free(recv_data);

	return ;
}

#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(tcpserv, startup tcp server);
#endif
