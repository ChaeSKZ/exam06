#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

typedef struct s_client
{
	int					id;
	char				*buf;
}	t_client;

t_client *clients;
fd_set afd, rfd;
int maxfd = 0;
int sockfd;
int next_id = 0;
char wbuf[600000];
char rbuf[600000];

void	fatal()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void sendAll(int sender)
{
	for (int fd = 0; fd <= maxfd; fd++)
	{
		if (FD_ISSET(fd, &afd) && fd != sender && fd != sockfd)
			send(fd, wbuf, strlen(wbuf), 0);
	}
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


int main(int ac, char **av) {
	socklen_t len;
	struct sockaddr_in servaddr;

	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		fatal();
	bzero(&servaddr, sizeof(servaddr));
	clients = calloc(4000, sizeof(t_client));
	if (!clients)
		fatal();

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal();
	if (listen(sockfd, 10) != 0)
		fatal();
	FD_ZERO(&afd);
	FD_SET(sockfd, &afd);
	maxfd = sockfd;
	while (1)
	{
		rfd = afd;
		if (select(maxfd + 1, &rfd, NULL, NULL, NULL) < 0)
			continue ;
		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (!FD_ISSET(fd, &rfd))
				continue ;
			if (fd == sockfd)
			{
				len = sizeof(servaddr);
				int clientfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
				if (clientfd < 0)
					continue ;
				FD_SET(clientfd, &afd);
				if (clientfd > maxfd)
					maxfd = clientfd;
				clients[clientfd].id = next_id++;
				clients[clientfd].buf = NULL;
				sprintf(wbuf, "server: client %d just arrived\n", clients[clientfd].id);
				sendAll(clientfd);
			}
			else
			{
				int n = recv(fd, rbuf, 200000, 0);
				if (n <= 0)
				{
					FD_CLR(fd, &afd);
					free(clients[fd].buf);
					close(fd);
					sprintf(wbuf, "server: client %d just left\n", clients[fd].id);
					sendAll(fd);
					continue ;
				}
				rbuf[n] = 0;
				clients[fd].buf = str_join(clients[fd].buf, rbuf);
				char *line;
				while (extract_message(&clients[fd].buf, &line))
				{
					sprintf(wbuf, "client %d: %s", clients[fd].id, line);
					sendAll(fd);
					free(line);
				}
			}
		}
	}
}
