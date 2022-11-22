#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct 		s_server
{
	struct sockaddr_in		addr;
	socklen_t				addr_len;
	int						port;
	int						fd;
	int						**fds;
	int						nb_fd;
	int						id;
	fd_set					fd_set;
	fd_set					fd_set_backup;
}					t_server;

typedef int			bool;

void	ft_putstr_fd(char *str, int fd)
{
	write(fd, str, strlen(str));
}

void	arg_err(void)
{
	ft_putstr_fd("Wrong number of arguments\n", 2);
	exit(EXIT_FAILURE);
}

void	fatal_err(t_server *server)
{
	for (int i = 0; i < server->nb_fd; ++i)
	{
		close(server->fds[i][1]);
		free(server->fds[i]);
	}
	free(server->fds);
	ft_putstr_fd("Fatal error\n", 2);
	exit(EXIT_FAILURE);
}

int		greatest_inserted(int **fds, int nb_fd)
{
	int		greatest = 2;

	for (int i = 0; i < nb_fd; i++)
	{
		if (fds[i][1] > greatest)
			greatest = fds[i][1];
	}
	return (greatest);
}

void	add_fd(t_server *server, int fd)
{
	server->fds = realloc(server->fds, sizeof(int *) * (server->nb_fd + 1));
	if (!server->fds)
		fatal_err(server);
	server->fds[server->nb_fd] = malloc(sizeof(int) * 2);
	if (!server->fds[server->nb_fd])
		fatal_err(server);
	server->fds[server->nb_fd][0] = server->id;
	server->fds[server->nb_fd][1] = fd;
	server->nb_fd++;
	server->id++;
}

void	init_server(t_server *server, int argc, char **argv)
{
	server->fds = NULL;
	server->id = -1;
	server->nb_fd = 0;
	server->port = atoi(argv[argc - 1]);
	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->fd == -1)
		fatal_err(server);
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(server->port);
	server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server->addr_len = sizeof(server->addr);
	if (bind(server->fd, (struct sockaddr *)&server->addr, server->addr_len) == -1)
		fatal_err(server);
	if (listen(server->fd, 1) == -1)
		fatal_err(server);
	FD_ZERO(&server->fd_set_backup);
	FD_SET(server->fd, &server->fd_set_backup);
	add_fd(server, server->fd);
}

bool	send_msg_to_clients(t_server *server, int sender, char *msg)
{
	for (int i = 1; i < server->nb_fd; ++i)
	{
		if (server->fds[i][1] == sender)
			continue ;
		if (send(server->fds[i][1], msg, strlen(msg), 0) == -1)
			return (0);
	}
	return (1);
}

int		size_nb(int nb)
{
	int		size;

	size = 1;
	while (nb >= 10)
	{
		size++;
		nb /= 10;
	}
	return (size);
}

void	handle_new_connection(t_server *server)
{
	int		client_fd;
	char	*msg;

	client_fd = accept(server->fd, (struct sockaddr *)&server->addr, &server->addr_len);
	if (client_fd == -1)
		fatal_err(server);
	FD_SET(client_fd, &server->fd_set_backup);
	add_fd(server, client_fd);
	msg = malloc(sizeof(char) * (29 + size_nb(server->fds[server->nb_fd - 1][0]) + 1));
	if (!msg)
		fatal_err(server);
	sprintf(msg, "server: client %d just arrived\n", server->fds[server->nb_fd - 1][0]);
	if (!send_msg_to_clients(server, client_fd, msg))
	{
		free(msg);
		fatal_err(server);
	}
	free(msg);
}

void	remove_fd(t_server *server, int pos)
{
	for (int i = pos; i < server->nb_fd - 1; ++i)
	{
		server->fds[i] = server->fds[i + 1];
	}
	server->fds = realloc(server->fds, sizeof(int *) * (server->nb_fd - 1));
	if (!server->fds)
		fatal_err(server);
	server->nb_fd--;
}

void	handle_disconnection(t_server *server, int i)
{
	char	*msg;

	msg = malloc(sizeof(char) * (26 + size_nb(server->fds[i][0]) + 1));
	if (!msg)
		fatal_err(server);
	sprintf(msg, "server: client %d just left\n", server->fds[i][0]);
	if (!send_msg_to_clients(server, server->fds[i][1], msg))
	{
		free(msg);
		fatal_err(server);
	}
	free(msg);
	FD_CLR(server->fds[i][1], &server->fd_set_backup);
	close(server->fds[i][1]);
	free(server->fds[i]);
	remove_fd(server, i);
}

char	*extract_msg(t_server *server, int i)
{
	char			*msg = NULL;
	const int		buf_size = 5;
	char			buf[buf_size];
	int				byte;

	msg = malloc(sizeof(char) * (1));
	if (!msg)
		fatal_err(server);
	memset(msg, '\0', 1);
	do
	{
		byte = recv(server->fds[i][1], buf, buf_size - 1, 0);
		if (byte == -1)
		{
			free(msg);
			fatal_err(server);
		}
		else if (!byte)
		{
			free(msg);
			handle_disconnection(server, i);
			return (NULL);
		}
		else
		{
			buf[byte] = '\0';
			msg = realloc(msg, sizeof(char) * (strlen(msg) + byte + 1));
			if (!msg)
				fatal_err(server);
			strcat(msg, buf);
		}
	}
	while ((byte == buf_size - 1) && buf[byte - 1] != '\n');
	return (msg);
}

void	handle_new_message(t_server *server, int i)
{
	char	*msg;
	int		msg_size;
	char	*msg_to_send;
	int		start_line_size = 9 + size_nb(server->fds[i][0]);

	msg = extract_msg(server, i);
	if (msg)
	{
		int j = 0;
		while (msg[j] != '\0')
		{
			msg_size = 1;
			for (int k = 0; msg[j + k] != '\n'; ++k)
			{
				msg_size++;
			}
			msg_to_send = malloc(sizeof(char) * (msg_size + start_line_size + 1));
			if (!msg_to_send)
			{
				free(msg);
				fatal_err(server);
			}
			sprintf(msg_to_send, "client %d: ", server->fds[i][0]);
			for (int k = 0; msg[j + k] != '\n'; ++k)
			{
				msg_to_send[start_line_size + k] = msg[j + k];
			}
			msg_to_send[start_line_size + msg_size - 1] = '\n';
			msg_to_send[start_line_size + msg_size] = '\0';
			if (!send_msg_to_clients(server, server->fds[i][1], msg_to_send))
			{
				free(msg_to_send);
				free(msg);
				fatal_err(server);
			}
			free(msg_to_send);
			j += msg_size;
		}
		free(msg);
	}
}

void	start_server(t_server *server)
{
	while (1)
	{
		server->fd_set = server->fd_set_backup;
		if (select(greatest_inserted(server->fds, server->nb_fd) + 1, &server->fd_set, NULL, NULL, 0) <= 0)
			continue ;
		for (int i = 0; i < server->nb_fd; ++i)
		{
			if (FD_ISSET(server->fds[i][1], &server->fd_set) == 1)
			{
				if (i == 0)
					handle_new_connection(server);
				else
					handle_new_message(server, i);
			}
		}
	}
}

int	main(int argc, char **argv)
{
	t_server	server;

	if (argc != 2)
		arg_err();
	init_server(&server, argc, argv);
	start_server(&server);
	return (0);
}
