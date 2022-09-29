#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include  <string.h>

int	main(int argc, char **argv)
{
	int		client_fd;
	int		server_fd;
	struct  sockaddr_in	addr;
	char buf[1000];

	if (argc < 2)
	{
		printf("Error: Wrong number of arguments");
		exit(1);
	}
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error: socket creation");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	if ((server_fd = connect(client_fd, (struct sockaddr*)&addr,sizeof(addr))) == -1)
	{
		printf("Error: connect to server");
		exit(1);
	}
	while (1)
	{
		printf("Enter new message: \n");
		scanf("%s", buf);
		printf("Your message: %s\n", buf);
		if (send(client_fd, buf, 1000, 0) == -1)
			printf("Error: send failed\n");
	}
	return (0);
}
