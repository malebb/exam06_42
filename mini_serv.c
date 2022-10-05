#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

typedef int bool;

bool	check_arg(int argc)
{
	return (argc > 1);
}

void	putstr_fd(char *str, int fd)
{
	write(fd, str, strlen(str));
}

void	arg_err(void)
{
	putstr_fd("Wrong number of arguments\n", 2);
	exit(1);
}

void	fatal_err(void)
{
	putstr_fd("Fatal error\n", 2);
	exit(1);
}

int get_port(char *arg)
{
	int		port;

	port = atoi(arg);
	return (port);
}

int *add_clients(int *clients, int fd, int nb_clients)
{
	int *new_clients;

	new_clients = malloc(sizeof(int) * (nb_clients + 1));
	for (int i = 0; i < nb_clients; ++i)
	{
		new_clients[i] = clients[i];
	}
	new_clients[nb_clients] = fd;
	return (new_clients);
}

int	main(int argc, char **argv)
{
	int						port;
	int						server_fd;
	int						client_fd;
	socklen_t 				addr_len;
	struct sockaddr_in		addr;
	char					buf[1024];
	fd_set					fds;
	int						*clients;
	int						nb_clients = 0;

	if (!check_arg(argc))
		arg_err();
	if ((port = get_port(argv[1])) == -1)
		fatal_err();
	printf("port = %d\n", port);
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_len = sizeof(addr);
	if (bind(server_fd, (struct sockaddr *)&addr, addr_len)== -1)
		fatal_err();
	if (listen(server_fd, 3) == -1)
		fatal_err();
	while (1)
	{
		printf("loop\n");
		FD_ZERO(&fds);
		FD_SET(server_fd, &fds);
		select(server_fd + 1, &fds, NULL, NULL, NULL);
		if (FD_ISSET(server_fd, &fds))
		{
			printf("Waiting for connection ...\n");
			if ((client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len)) == -1)
				fatal_err();
			printf("Connection established\n");
			clients = add_clients(clients, client_fd, nb_clients);
			nb_clients++;
			for (int i = 0; i < nb_clients; ++i)
			{
				printf("fd = %d client %d \n", client_fd, clients[i]);
			}
		}
		for (int i = 0; i < nb_clients; ++i)
		{
			printf("CLIENT %d \n", clients[i]);
		}
		for (int i = 0; i < client_fd; ++i)
		{
			FD_ZERO(&fds);
			FD_SET(client_fd, &fds);
			select(client_fd + 1, &fds, NULL, NULL, NULL);
			if (FD_ISSET(client_fd, &fds))
			{
				recv(client_fd, buf, 1024, 0);
				printf("Recv: %s\n", buf);
			}
		}
	}
	return (0);
}
