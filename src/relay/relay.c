# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include "login.h"

/*
 * NAME:	main()
 * DESCRIPTION:	Relay can be run as follows:
 *
 *		    relay hostname port program+arguments < input > /dev/null
 *
 *		which would connect to the server at hostname:port, read lines
 *		from the file input and forward them to the server, and read
 *		and discard any output from the server received before the
 *		last line of input was read.
 *		Once all lines from the input file are copied to the server,
 *		the given program will be executed with the socket as stdin
 *		and stdout.
 */
int main(int argc, char *argv[], char *envp[])
{
    int port, socket;

    /*
     * check arguments
     */
    if (argc < 4) {
	fprintf(stderr, "Usage: %s hostname port program [arguments...]\n",
		argv[0]);
	return 2;
    }
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
	fprintf(stderr, "%s: bad port\n", argv[0]);
	return 2;
    }

    /*
     * login on server
     */
    socket = login_dgd(argv[1], port, 0, 1);
    if (socket < 0) {
	return 1;
    }

    /*
     * execute program
     */
    dup2(socket, 0);
    dup2(socket, 1);
    close(socket);
    execvp(argv[3], argv + 3);
    perror("execvp");
    return 1;
}
