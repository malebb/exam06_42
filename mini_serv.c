#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

typedef struct		s_client
{
	int							id;
	int							fd;
}					t_client;

typedef struct		s_server
{
	t_client					*clients;
	int							fd;
	int							nb_clients;
	int							id;
	struct sockaddr_in 			addr;
	socklen_t					addrlen;
	fd_set						set;
	fd_set						set_backup;
}					t_server;

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
	close(server->fd);
	for (int i = 0; i < server->nb_clients; ++i)
	{
		close(server->clients[i].fd);
	}
	free(server->clients);
	ft_putstr_fd("Fatal error\n", 2);
	exit(EXIT_FAILURE);
}

void	init_server(int argc, char **argv, t_server *server)
{
	int		port;

	if (argc != 2)
		arg_err();
	port = atoi(argv[argc - 1]);
	server->id = 0;
	server->nb_clients = 0;
	server->clients = NULL;
	FD_ZERO(&server->set);
	FD_ZERO(&server->set_backup);
	server->addrlen = sizeof(server->addr);
	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->fd == -1)
		fatal_err(server);
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(port);
	server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server->fd, (struct sockaddr *)&server->addr, server->addrlen) == -1)
		fatal_err(server);
	if (listen(server->fd, 1) == -1)
		fatal_err(server);
	FD_SET(server->fd, &server->set_backup);
}

int		greatest_inserted(t_server *server)
{
	int		greatest;

	greatest = server->fd;
	for (int i = 0; i < server->nb_clients; ++i)
	{
		if (server->clients[i].fd > greatest)
			greatest = server->clients[i].fd;
	}
	return (greatest);
}

void	add_client(int fd, t_server *server)
{
	server->clients = realloc(server->clients, sizeof(t_client) * (server->nb_clients + 1));
	if (!server->clients)
		fatal_err(server);
	server->clients[server->nb_clients] = (t_client){.id = server->id, .fd = fd};
	server->id++;
	server->nb_clients++;
	FD_SET(fd, &server->set_backup);
}

void	send_msg_to_clients(char *msg, int len, int sender, t_server *server)
{
	for (int i = 0; i < server->nb_clients; ++i)
	{
		if (server->clients[i].fd == sender)
			continue ;
		send(server->clients[i].fd, msg, len, 0);
	}
}

void	handle_new_connection(t_server *server)
{
	int 	client_fd;
	char	msg[1000];

	client_fd = accept(server->fd, (struct sockaddr *)&server->addr, &server->addrlen);
	if (client_fd == -1)
		fatal_err(server);
	add_client(client_fd, server);
	sprintf(msg, "server: client %d just arrived\n", server->clients[server->nb_clients - 1].id);
	send_msg_to_clients(msg, strlen(msg), client_fd, server);
}

void	remove_client(t_server *server, int pos)
{
	close(server->clients[pos].fd);
	for (int i = pos; i < server->nb_clients - 1; ++i)
	{
		server->clients[i] = server->clients[i + 1];
	}
	server->nb_clients--;
}

void	handle_disconnection(t_server *server, int pos)
{
	char		msg[1000];

	sprintf(msg, "server: client %d just left\n", server->clients[pos].id);
	send_msg_to_clients(msg, strlen(msg), server->clients[pos].fd, server);
	FD_CLR(server->clients[pos].fd, &server->set_backup);
	FD_CLR(server->clients[pos].fd, &server->set);
	remove_client(server, pos);
}

char	*extract_from_buf(t_server *server, int pos)
{
	const int			buf_size = 5;
	char				buf[buf_size];
	char				*msg;
	int					byte_read;

	msg = calloc(1, sizeof(char));
	if (!msg)
		fatal_err(server);
	do
	{
		byte_read = recv(server->clients[pos].fd, buf, buf_size - 1, 0);
		if (!byte_read)
			handle_disconnection(server, pos);
		else
		{
			buf[byte_read] = '\0';
			msg = realloc(msg, sizeof(char) * (strlen(msg) + byte_read + 1));
			if (!msg)
				fatal_err(server);
			strcat(msg, buf);
		}
	}
	while (byte_read == buf_size - 1 && buf[byte_read - 1] != '\n');
	return (msg);
}

void	handle_new_msg(t_server *server, int pos)
{
	char			*msg;
	int				i;
	char			*next_msg;
	char			start_line[1000];
	char			*msg_to_send;

	msg = extract_from_buf(server, pos);
	sprintf(start_line, "client %d: ", server->clients[pos].id);
	i = 0;
	while (msg[i] != '\0')
	{
		next_msg = strstr(&msg[i], "\n");
		next_msg[0] = '\0';
		msg_to_send = malloc(sizeof(char) * (strlen(start_line) + (next_msg - &msg[i]) + 2));
		if (!msg_to_send)
		{
			free(msg);
			fatal_err(server);
		}
		sprintf(msg_to_send, "%s%s\n", start_line, &msg[i]);
		send_msg_to_clients(msg_to_send, strlen(msg_to_send), server->clients[pos].fd, server);
		free(msg_to_send);
		i += (next_msg - &msg[i]) + 1;
	}
	free(msg);
}

void	start_server(t_server *server)
{
	while (1)
	{
		server->set = server->set_backup;
		if (select(greatest_inserted(server) + 1, &server->set, NULL, NULL, 0) <= 0)
			continue;
		if (FD_ISSET(server->fd, &server->set))
			handle_new_connection(server);
		for (int i = 0; i < server->nb_clients; ++i)
		{
			if (FD_ISSET(server->clients[i].fd, &server->set))
				handle_new_msg(server, i);
		}
	}
}

int		main(int argc, char ** argv)
{
	t_server		server;

	init_server(argc, argv, &server);
	start_server(&server);
	return (EXIT_SUCCESS);
}
