#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

typedef struct		s_client
{
	int		id;
	int 	fd;
}					t_client;

typedef	struct		s_server
{
	t_client				*clients;
	int						id;
	int						nb_fd;
	int						port;
	int						fd;
	struct sockaddr_in		addr;
	socklen_t				addr_len;
	fd_set					set;
	fd_set					set_backup;
}					t_server;

void	ft_putstr_fd(char *str, int fd)
{
	write(fd, str, strlen(str));
}

void	arg_err()
{
	ft_putstr_fd("Wrong number of arguments\n", 2);
	exit(EXIT_FAILURE);
}

void	fatal_err(t_server *server)
{
	for (int i = 0; i < server->nb_fd; ++i)
	{
		close(server->clients[i].fd);
	}
	free(server->clients);
	ft_putstr_fd("Fatal error\n", 2);
	exit(EXIT_FAILURE);
}

void	add_client(int fd, t_server *server)
{

	server->clients = realloc(server->clients, sizeof(t_client) * (server->nb_fd + 1));
	if (!server->clients)
		fatal_err(server);
	server->clients[server->nb_fd] = (t_client){.id = server->id, .fd = fd};
	FD_SET(fd, &server->set_backup);
	server->nb_fd++;
	server->id++;
}

void	init_server(int argc, char **argv, t_server *server)
{
	if (argc != 2)
		arg_err();
	server->port = atoi(argv[argc - 1]);
	server->id = 0;
	server->nb_fd = 0;
	server->clients = NULL;
	server->addr_len = sizeof(server->addr);
	FD_ZERO(&server->set);
	FD_ZERO(&server->set_backup);
	server->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->fd == -1)
		fatal_err(server);
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(server->port);
	server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server->fd, (struct sockaddr*)&server->addr, server->addr_len) == -1)
		fatal_err(server);
	if (listen(server->fd, 1) == -1)
		fatal_err(server);
	FD_SET(server->fd, &server->set_backup);
}

int		greatest_inserted(t_server *server)
{
	int		greatest;

	greatest = server->fd;
	for (int i = 0; i < server->nb_fd; ++i)
	{
		if (server->clients[i].fd > greatest)
			greatest = server->clients[i].fd;
	}
	return (greatest);
}

void	send_msg_to_clients(char *msg, int size, int sender, t_server *server)
{
	for (int i = 0; i < server->nb_fd; ++i)
	{
		if (server->clients[i].fd == sender)
			continue ;
		send(server->clients[i].fd, msg, size, 0);
	}
}

void	handle_new_connection(t_server *server)
{
	int		client_fd;
	char	msg[1000];
	int		size_msg;

	client_fd = accept(server->fd, (struct sockaddr*)&server->addr, &server->addr_len);
	if (client_fd == -1)
		fatal_err(server);
	add_client(client_fd, server);
	size_msg = sprintf(msg, "server: client %d just arrived\n", server->clients[server->nb_fd - 1].id);
	send_msg_to_clients(msg, size_msg, client_fd, server);
}

void	remove_client(t_server *server, int i)
{
	FD_CLR(server->clients[i].fd, &server->set_backup);
	close(server->clients[i].fd);
	for (int j = i; j < server->nb_fd - 1; ++j)
	{
		server->clients[i] = server->clients[i + 1];
	}
	server->nb_fd--;
}

void	handle_disconnection(t_server *server, int i)
{
	char 	msg[1000];

	sprintf(msg, "server: client %d just left\n", server->clients[i].id);
	send_msg_to_clients(msg, strlen(msg), server->clients[i].fd, server);
	remove_client(server, i);
}

char	*extract_from_buffer(t_server *server, int i)
{
	char			*msg = NULL;
	const int		buf_size = 5;
	char			buf[buf_size];
	int				byte;

	msg = malloc(sizeof(char) * 1);
	memset(msg, '\0', 1);
	do
	{
		byte = recv(server->clients[i].fd, buf, buf_size - 1, 0);
		if (!byte)
			handle_disconnection(server, i);
		else
		{
			buf[byte] = '\0';
			msg = realloc(msg, sizeof(char) * (strlen(msg) + byte + 1));
			if (!msg)
				fatal_err(server);
			strcat(msg, buf);
		}
	}
	while (byte == buf_size - 1 && buf[buf_size - 2] != '\n');
	return (msg);
}

void	handle_new_msg(t_server *server, int i)
{
	char	*msg;
	char	*next_msg;
	char	*msg_to_send;
	int		j;
	char	start_line[1000];

	sprintf(start_line, "client %d: ", server->clients[i].id);
	msg = extract_from_buffer(server, i);
	j = 0;
	while (msg[j] != '\0')
	{
		next_msg = strstr(&msg[j], "\n");
		msg_to_send = malloc(sizeof(char) * ((next_msg - &msg[j]) + strlen(start_line) + 2));
		if (!msg_to_send)
		{
			free(msg);
			fatal_err(server);
		}
		next_msg[0] = '\0';
		sprintf(msg_to_send, "%s%s\n", start_line, (msg + j));
		send_msg_to_clients(msg_to_send, strlen(msg_to_send), server->clients[i].fd, server);
		free(msg_to_send);
		j += (next_msg - &msg[j]) + 1;
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
		for (int i = 0; i < server->nb_fd; ++i)
		{
			if (FD_ISSET(server->clients[i].fd, &server->set))
				handle_new_msg(server, i);
		}
	}
}

int	main(int argc, char **argv)
{
	t_server	server;

	init_server(argc, argv, &server);
	start_server(&server);
	return (0);
}
