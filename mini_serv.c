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

int ***add_fd(int ***fds, int fd, int *nb_fd, int *id)
{
	*fds = realloc(*fds, sizeof(int *) * (*nb_fd + 1));
	if (!(*fds))
		return (NULL);
	(*fds)[*nb_fd] = malloc(sizeof(int) * 2);
	if (!(*fds)[*nb_fd])
		return (NULL);
	(*fds)[*nb_fd][0] = *id;
	(*fds)[*nb_fd][1] = fd;
	*nb_fd = *nb_fd + 1;
	*id = *id + 1;
	return (fds);
}

int ***remove_fd(int ***fds, int fd, int *nb_fd)
{
	int		pos;

	for (int i = 0; i < *nb_fd; ++i)
	{
		if ((*fds)[i][1] == fd)
		{
			close((*fds)[i][1]);
			free((*fds)[i]);
			pos = i;
		}
	}
	if (pos != *nb_fd - 1)
	{
		for (int i = pos; i < *nb_fd - 1; ++i)
		{
			(*fds)[i] = (*fds)[i + 1];
		}
	}
	*fds = realloc(*fds, sizeof(int *) * (*nb_fd - 1));
	if (!(*fds))
		return (NULL);
	*nb_fd = *nb_fd - 1;
	return (fds);
}

char	*extract_msg_from_buffer(int	nb_bytes, char buf[1024])
{
	char	*msg;

	msg = malloc(sizeof(char) * (nb_bytes + 1));
	if (!msg)
		return (NULL);
	for (int i = 0; i < nb_bytes; i++)
	{
		msg[i] = buf[i];
	}
	msg[nb_bytes] = '\0';
	return (msg);
}

void	send_msg_to_clients(char *msg, int **fd_set, int nb_fd, int sender)
{
	for (int i = 1; i < nb_fd; ++i)
	{
		if (fd_set[i][1] != sender)
		{
			if (send(fd_set[i][1], msg, strlen(msg), 0) == -1)
				fatal_err();
		}
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

int	greatest_inserted(int **fd_set, int nb_fd)
{
	int		greatest = 2;

	for (int i = 0; i < nb_fd; ++i)
	{
		if (fd_set[i][1] > greatest)
		{
			greatest = fd_set[i][1];
		}
	}
	return (greatest);
}

void	manage_msg(fd_set *fds_backup, int ***fd_set, int i, int *nb_fd, int *nb_client)
{
	int		nb_bytes;
	char	*msg_to_send;
	char	buf[1024];

	nb_bytes = recv((*fd_set)[i][1], buf, 1024, 0);
	if (nb_bytes <= 0)
	{
		msg_to_send = malloc(sizeof(char) * (27 + nb_size((*fd_set)[i][0])));
		if (!msg_to_send)
			fatal_err();
		sprintf(msg_to_send, "server: client %d just left\n", (*fd_set)[i][0]);
		send_msg_to_clients(msg_to_send, *fd_set, *nb_fd, (*fd_set)[i][1]);
		FD_CLR((*fd_set)[i][1], fds_backup);
		if (!remove_fd(fd_set, (*fd_set)[i][1], nb_fd))
			fatal_err();
		*nb_client = *nb_client - 1;
	}
	else
	{
		char	*msg;

		msg = extract_msg_from_buffer(nb_bytes, buf);
		if (!msg)
			fatal_err();
		msg_to_send = malloc(sizeof(char) * (10 + nb_size((*fd_set)[i][1]) + strlen(msg)));
		if (!msg_to_send)
			fatal_err();
		sprintf(msg_to_send, "client %d: ", (*fd_set)[i][0]);
		strcat(msg_to_send, msg);
		send_msg_to_clients(msg_to_send, *fd_set, *nb_fd, (*fd_set)[i][1]);
		free(msg);
	}
	free(msg_to_send);
}

int	init_server(int argc, char **argv)
{
	int						server_fd;
	int						port;
	struct sockaddr_in		addr;
	socklen_t 				addr_len;

	if (!check_arg(argc))
		arg_err();
	if ((port = get_port(argv[1])) == -1)
		fatal_err();
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		fatal_err();
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_len = sizeof(addr);
	if (bind(server_fd, (struct sockaddr *)&addr, addr_len) == -1)
		fatal_err();
	if (listen(server_fd, 1) == -1)
		fatal_err();
	return (server_fd);
}

void	handle_new_connection(fd_set *fds_backup, int server_fd, int ***fd_set, int *nb_fd, int *id, int *nb_client)
{
	char					*new_client_msg;
	struct sockaddr_in		addr;
	socklen_t 				addr_len;
	int						client_fd;

	client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
	if (client_fd == -1)
		fatal_err();
	FD_SET(client_fd, fds_backup);
	if (!add_fd(fd_set, client_fd, nb_fd, id))
		fatal_err();
	new_client_msg = malloc(sizeof(char) * (30 + nb_size((*fd_set)[*nb_fd - 1][0])));
	if (!new_client_msg)
		fatal_err();
	sprintf(new_client_msg, "server: client %d just arrived\n", (*fd_set)[*nb_fd - 1][0]);
	send_msg_to_clients(new_client_msg, *fd_set, *nb_fd, (*fd_set)[*nb_fd - 1][1]);
	*nb_client = *nb_client + 1;
	free(new_client_msg);
}

int	main(int argc, char **argv)
{
	int						server_fd;
	fd_set					fds;
	fd_set					fds_backup;
	int						**fd_set = NULL;
	int						nb_fd = 0;
	int						nb_client;
	int						id = -1;

	server_fd = init_server(argc, argv);
	FD_ZERO(&fds);
	FD_SET(server_fd, &fds);
	if (!add_fd(&fd_set, server_fd, &nb_fd, &id))
		fatal_err();
	fds_backup = fds;
	while (1)
	{
		fds = fds_backup;
		if (select(greatest_inserted(fd_set, nb_fd) + 1, &fds, NULL, NULL, NULL) <= 0)
			continue ;
		nb_client = nb_fd;
		for (int i = 0; i < nb_client; ++i)
		{
			if (FD_ISSET(fd_set[i][1], &fds))
			{
				if (fd_set[i][1] == server_fd)
					handle_new_connection(&fds_backup, server_fd, &fd_set, &nb_fd, &id, &nb_client);
				else
					manage_msg(&fds_backup, &fd_set, i, &nb_fd, &nb_client);
			}
		}
	}
	return (0);
}
