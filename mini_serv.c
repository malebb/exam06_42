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
typedef struct s_server
{
	struct sockaddr_in		addr;
	socklen_t 				addr_len;
	int						fd;
}			t_server;

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
	const int		buf_size = 5;
	int				nb_bytes = buf_size;
	char			buf[buf_size];
	char			*msg = NULL; 
	char			*msg_to_send;

	msg = malloc(sizeof(char) * (1));
	if (!msg)
		fatal_err();
	memset(msg, '\0', 1);
	do {
		nb_bytes = recv((*fd_set)[i][1], buf, buf_size - 1, 0);
		buf[nb_bytes] = '\0';
		msg = realloc(msg, sizeof(char) * (strlen(msg) + nb_bytes + 1));
		if (!msg)
			fatal_err();
		strcat(msg, buf);
	}
	while (nb_bytes == buf_size - 1 && buf[nb_bytes - 1] != '\n');
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
		int		size_msg;
		int		start_line_size = nb_size((*fd_set)[i][0]) + 9;
		int		j;

		j = 0;
		while (msg[j] != '\0')
		{
			size_msg = 0;
			for (int k = 0; msg[j + k] != '\n'; k++)
			{
				size_msg++;
			}
			size_msg += start_line_size + 1;
			msg_to_send = malloc(sizeof(char) * (size_msg + 1));
			if (!msg_to_send)
				fatal_err();
			sprintf(msg_to_send, "client %d: ", (*fd_set)[i][0]);
			for (int k = 0; msg[j + k] != '\n'; k++)
			{
				msg_to_send[start_line_size + k] = msg[j + k];
			}
			msg_to_send[size_msg - 1] = '\n';
			msg_to_send[size_msg] = '\0';
			j += (size_msg - start_line_size);
			send_msg_to_clients(msg_to_send, *fd_set, *nb_fd, (*fd_set)[i][1]);
			free(msg_to_send);
		}
		free(msg);
	}
}

void	init_server(int argc, char **argv, t_server *server)
{
	int						port;

	if (!check_arg(argc))
		arg_err();
	if ((port = get_port(argv[1])) == -1)
		fatal_err();
	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->fd == -1)
		fatal_err();
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(port);
	server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server->addr_len = sizeof(server->addr);
	if (bind(server->fd, (struct sockaddr *)&server->addr, server->addr_len) == -1)
		fatal_err();
	if (listen(server->fd, 1) == -1)
		fatal_err();
}

void	handle_new_connection(fd_set *fds_backup, t_server server, int ***fd_set, int *nb_fd, int *id, int *nb_client)
{
	char					*new_client_msg;
	int						client_fd;

	client_fd = accept(server.fd, (struct sockaddr *)&server.addr, &server.addr_len);
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
	fd_set					fds;
	fd_set					fds_backup;
	int						**fd_set = NULL;
	int						nb_fd = 0;
	int						nb_client;
	int						id = -1;
	t_server				server;

	init_server(argc, argv, &server);
	FD_ZERO(&fds);
	FD_SET(server.fd, &fds);
	if (!add_fd(&fd_set, server.fd, &nb_fd, &id))
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
				if (fd_set[i][1] == server.fd)
					handle_new_connection(&fds_backup, server, &fd_set, &nb_fd, &id, &nb_client);
				else
					manage_msg(&fds_backup, &fd_set, i, &nb_fd, &nb_client);
			}
		}
	}
	return (0);
}
