#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <strings.h>

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

int *add_fd(int *fds, int fd, int nb_fd)
{
	int *new_fds;

	new_fds = malloc(sizeof(int) * (nb_fd + 1));
	for (int i = 0; i < nb_fd; ++i)
	{
		new_fds[i] = fds[i];
	}
	new_fds[nb_fd] = fd;
	return (new_fds);
}

int *remove_fd(int *fds, int fd, int nb_fd)
{
	int *new_fds;
	int	j;

	new_fds = malloc(sizeof(int) * (nb_fd - 1));
	j = 0;
	for (int i = 0; i < nb_fd - 1; ++i)
	{
		if (fds[i] != fd)
		{
			new_fds[j] = fds[i];
			j++;
		}
	}
	return (new_fds);
}

char	*extract_msg_from_buffer(int	nb_bytes, char buf[1024])
{
	char	*msg;

	msg = malloc(sizeof(char) * (nb_bytes));
	for (int i = 0; i < nb_bytes; i++)
	{
		msg[i] = buf[i];
	}
	return (msg);
}

void	send_msg_to_clients(char *msg, int *fd_set, int nb_fd, int sender)
{
	for (int i = 1; i < nb_fd; ++i)
	{
		if (fd_set[i] != sender)
			send(fd_set[i], msg, strlen(msg), 0);
	}
}

int		nb_size(int nb)
{
	int		size;

	size = 1;
	while (nb >= 10)
	{
		nb /= 10;
		size++;
	}
	return (size);
}

int	greatest_inserted(int *fd_set, int nb_fd)
{
	int		greatest = 2;

	for (int i = 0; i < nb_fd; ++i)
	{
		if (fd_set[i] > greatest)
		{
			greatest = fd_set[i];
		}
	}
	return (greatest);
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
	fd_set					fds_backup;
	int						*fd_set = NULL;
	int						nb_fd = 0;
	int						nb_client;
	int						last_inserted = 2;

	if (!check_arg(argc))
		arg_err();
	if ((port = get_port(argv[1])) == -1)
		fatal_err();
//	printf("port = %d\n", port);
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_len = sizeof(addr);
	if (bind(server_fd, (struct sockaddr *)&addr, addr_len)== -1)
		fatal_err();
	if (listen(server_fd, 3) == -1)
		fatal_err();
	FD_ZERO(&fds);
	FD_SET(server_fd, &fds);
	fd_set = add_fd(fd_set, server_fd, nb_fd);
	nb_fd++;
	fds_backup = fds;
	while (1)
	{
//		printf("loop, server = %d\n", server_fd);
		fds = fds_backup;
		select(greatest_inserted(fd_set, nb_fd) + 1, &fds, NULL, NULL, NULL);
/*		printf("fd_set : ");
		for (int i = 0; i < nb_fd; ++i)
		{
			printf("|%d|", fd_set[i]);
		}
		printf("\n");
		*/
		nb_client = nb_fd;
		for (int i = 0; i < nb_client; ++i)
		{
			if (FD_ISSET(fd_set[i], &fds))
			{
				if (fd_set[i] == server_fd)
				{
					char	*new_client_msg;

					if (i == 0)
						printf("New connection\n");
					client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
					
					FD_SET(client_fd, &fds_backup);
					fd_set = add_fd(fd_set, client_fd, nb_fd);
					nb_fd++;
					new_client_msg = malloc(sizeof(char) * (30 + nb_size(nb_fd - 2)));
					sprintf(new_client_msg, "server: client %d just arrived\n", nb_fd - 2);
					send_msg_to_clients(new_client_msg, fd_set, nb_fd, fd_set[nb_fd - 1]);
				}
				else
				{
					int		nb_bytes;
					char	*msg;
					char	*msg_to_send;

					nb_bytes = recv(fd_set[i], buf, 1024, 0);
					if (nb_bytes <= 0)
					{
						msg_to_send = malloc(sizeof(char) * (27 + nb_size(i - 1) + strlen(msg)));
						sprintf(msg_to_send, "server: client %d just left\n", i - 1);
						send_msg_to_clients(msg_to_send, fd_set, nb_fd, fd_set[i]);
						printf("client %d just left\n", i - 1);
						FD_CLR(fd_set[i], &fds_backup);
						fd_set = remove_fd(fd_set, fd_set[i], nb_fd);
						nb_fd--;
					}
					else
					{
						msg = extract_msg_from_buffer(nb_bytes, buf);
						msg_to_send = malloc(sizeof(char) * (9 + nb_size(i - 1) + strlen(msg)));
						sprintf(msg_to_send, "client %d: ", i - 1);
						strcat(msg_to_send, msg);
						send_msg_to_clients(msg_to_send, fd_set, nb_fd, fd_set[i]);
						printf("client %d: %s", i - 1, msg);
					}
				}
			}
		}
	}
	return (0);
}
