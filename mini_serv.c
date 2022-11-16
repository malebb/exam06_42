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

	new_fds = malloc(sizeof(int) * (nb_fd + 1));
	for (int i = 0; i < nb_fd; ++i)
	{
		new_fds[i] = fds[i];
	}
	new_fds[nb_fd] = fd;
	*last_inserted = fd;
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
	fd_set = add_fd(fd_set, server_fd, nb_fd, &last_inserted);
	nb_fd++;
	fds_backup = fds;
	while (1)
	{
//		printf("loop, server = %d\n", server_fd);
		fds = fds_backup;
		select(last_inserted + 1, &fds, NULL, NULL, NULL);
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
					if (i == 0)
						printf("New connection\n");
					client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
					
					FD_SET(client_fd, &fds_backup);
					fd_set = add_fd(fd_set, client_fd, nb_fd, &last_inserted);
					nb_fd++;
				}
				else
				{
					int		nb_bytes;
					char	*msg;
					/*printf("%ld bytes received", */nb_bytes = recv(fd_set[i], buf, 1024, 0);

					msg = extract_msg_from_buffer(nb_bytes, buf);
					
					printf("user %d: %s\n", i, msg);
				}
			}
		}
	}
	return (0);
}
