# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <sys/time.h> 
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <errno.h>


/*
 * NAME:	login_dgd()
 * DESCRIPTION:	connect to DGD server, login, and return a socket
 */
int login_dgd(char *hostname, int port, int in, int out)
{
    char input[10240], output[10240];
    struct addrinfo hint, *res;
    struct sockaddr_in6 sin;
    int sock, i, n;
    fd_set fds;

    /*
     * get server address
     */
    memset(&hint, '\0', sizeof(struct addrinfo));
    hint.ai_family = PF_INET6;
    if (getaddrinfo(hostname, NULL, &hint, &res) != 0) {
	memset(&hint, '\0', sizeof(struct addrinfo));
	hint.ai_family = PF_INET;
	if (getaddrinfo(hostname, NULL, &hint, &res) != 0) {
	    fprintf(stderr, "unknown host: %s\n", hostname);
	    return -1;
	}
    }
    memcpy(&sin, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    sin.sin6_port = htons(port);

    /*
     * establish connection
     */
    sock = socket(sin.sin6_family, SOCK_STREAM, 0);
    if (sock < 0) {
	perror("socket");
	return -1;
    }
    if (connect(sock, (struct sockaddr *) &sin,
		(sin.sin6_family == AF_INET6) ?
		 sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)) < 0)
    {
	perror("connect");
	close(sock);
	return -1;
    }

    sleep(1);	/* give server time to send banner */
    memset(&fds, '\0', sizeof(fd_set));
    i = 0;

    for (;;) {
	/*
	 * wait for input
	 */
	FD_SET(in, &fds);
	FD_SET(sock, &fds);
	if (select(sock + 1, &fds, NULL, NULL, NULL) <= 0) {
	    perror("select");
	    close(sock);
	    return -1;
	}

	if (FD_ISSET(in, &fds)) {
	    /*
	     * input from stdin: read one character
	     */
	    n = read(in, input + i, 1);
	    if (n < 0) {
		perror("read");
		close(sock);
		return -1;
	    }
	    if (n == 0) {
		/*
		 * no more input; flush the input buffer if needed, and
		 * return the socket
		 */
		if (i != 0 && write(sock, input, i) != i) {
		    perror("socket write");
		    close(sock);
		    return -1;
		}
		return sock;
	    }
	}

	if (FD_ISSET(sock, &fds)) {
	    /*
	     * input from socket: copy directly to stdout
	     */
	    n = read(sock, output, sizeof(output));
	    if (n <= 0) {
		perror("socket read");
		close(sock);
		return -1;
	    }
	    if (write(out, output, n) != n) {
		perror("write");
		close(sock);
		return -1;
	    }
	}

	if (FD_ISSET(in, &fds) && (input[i++] == '\n' || i == sizeof(input))) {
	    /*
	     * forward a line of input to the server
	     */
	    if (write(sock, input, i) != i) {
		perror("socket write");
		close(sock);
		return -1;
	    }
	    i = 0;
	    sleep(1);	/* give server time to respond */
	}
    }
}
