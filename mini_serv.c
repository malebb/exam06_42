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

int *add_fd(int *fds, int fd, int nb_fd, int *last_inserted)
{
	int *new_fds;

	new_fds= malloc(sizeof(int) * (nb_fd+ 1));
	for (int i = 0; i < nb_fd; ++i)
	{
		new_fds[i] = fds[i];
	}
	new_fds[nb_fd] = fd;
	*last_inserted = fd;
	return (new_fds);
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
	int						*fd_set = NULL;
	int						nb_fd = 0;
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
	fd_set = add_fd(fd_set, server_fd, nb_fd, &last_inserted);
	nb_fd++;
	while (1)
	{
		printf("loop, server = %d\n", server_fd);
		select(last_inserted + 1, &fds, NULL, NULL, NULL);
		printf("loop, server2 = %d\n", server_fd);
/*		printf("fd_set : ");
		for (int i = 0; i < nb_fd; ++i)
		{
			printf("|%d|", fd_set[i]);
		}
		printf("\n");
		*/
		for (int i = 0; i < nb_fd; ++i)
		{
			if (FD_ISSET(fd_set[i], &fds))
			{
				if (fd_set[i] == server_fd)
				{
					if (i == 0)
						printf("New connection\n");
					client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
					FD_SET(client_fd, &fds);
					fd_set = add_fd(fd_set, client_fd, nb_fd, &last_inserted);
					nb_fd++;
				}
				else
				{
					char			*msg;
					int				byte;

					bzero(buf, 1024);
					byte = recv(fd_set[i], buf, 1024, 0);
					msg = malloc(sizeof(char) * byte);
					strcpy(msg, buf);
					printf("client %d : %s", i, msg);
					free(msg);
				}
			}
		}
	}
	return (0);
}
