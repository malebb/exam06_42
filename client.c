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
	size_t	buf_size = 1000;
	char *buf = malloc(sizeof(char) * 1000);
	int		bytes;
	int		size_line;

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
		size_line = getline(&buf, &buf_size, stdin);
//		printf("Your message: %s | %d\n", buf, size_line);
		if ((bytes = send(client_fd, buf, size_line, 0)) == -1)
			printf("Error: send failed\n");
		else
			printf("%d bytes have been sent\n", bytes);
	}
	return (0);
}
