# ifdef WIN32
# include <Windows.h>
# define DLLEXPORT		__declspec(dllexport)
# else
# define DLLEXPORT		/* nothing */
# endif

# define LPCEXT			/* declare */
# include "lpc_ext.h"
# include <stdarg.h>
# ifndef WIN32
# include <unistd.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <fcntl.h>
# else
# include <process.h>
# endif


/*
 * NAME:	ext->cb()
 * DESCRIPTION:	set up callbacks
 */
static int ext_cb(void *ftab[], int size, int n, ...)
{
    va_list args;
    void **func;

    if (size < n) {
	return 0;
    }

    va_start(args, n);
    while (n > 0) {
	func = va_arg(args, void**);
	*func = *ftab++;
	--n;
    }
    va_end(args);

    return 1;
}

static void (*ext_spawn)(void (*)(int*, int), void (*)(void));
static void (*ext_fdclose)(int*, int);

# ifndef WIN32
# define INVALID_HANDLE_VALUE	-1
static int in, out, back;
# else
static HANDLE in, out, back;

/*
 * NAME:	read()
 * DESCRIPTION:	Windows compatibility wrapper
 */
static int read(HANDLE h, void *buffer, int len)
{
    int size;

    if (!ReadFile(h, buffer, len, &size, NULL)) {
	return -1;
    }
    return size;
}

/*
 * NAME:	write()
 * DESCRIPTION:	Windows compatibility wrapper
 */
static int write(HANDLE h, const void *buffer, int len)
{
    int size;

    if (!WriteFile(h, buffer, len, &size, NULL)) {
	return -1;
    }
    return size;
}

/*
 * NAME:	close()
 * DESCRIPTION:	Windows compatibility wrapper
 */
static void close(HANDLE h)
{
    CloseHandle(h);
}
# endif

/*
 * NAME:	ext->init()
 * DESCRIPTION:	initialize extension handling
 */
DLLEXPORT int ext_init(int major, int minor, void **ftabs[], int sizes[],
		       const char *config)
{
    in = out = back = INVALID_HANDLE_VALUE;

    return (major == LPC_EXT_VERSION_MAJOR && minor >= LPC_EXT_VERSION_MINOR &&
           ext_cb(ftabs[0], sizes[0], 5,
		   &lpc_ext_kfun,
		   &lpc_ext_dbase,
		   &ext_spawn,
		   &ext_fdclose,
		   &lpc_ext_jit) &&
	    ext_cb(ftabs[1], sizes[1], 4,
		   &lpc_frame_object,
		   &lpc_frame_dataspace,
		   &lpc_frame_arg,
		   &lpc_frame_atomic) &&
	    ext_cb(ftabs[2], sizes[2], 2,
		   &lpc_data_get_val,
		   &lpc_data_set_val) &&
	    ext_cb(ftabs[3], sizes[3], 4,
		   &lpc_value_type,
		   &lpc_value_nil,
		   &lpc_value_temp,
		   &lpc_value_temp2) &&
	    ext_cb(ftabs[4], sizes[4], 2,
		   &lpc_int_getval,
		   &lpc_int_putval) &&
# ifndef NOFLOAT
	    ext_cb(ftabs[5], sizes[5], 2,
		   &lpc_float_getval,
		   &lpc_float_putval) &&
# endif
	    ext_cb(ftabs[6], sizes[6], 5,
		   &lpc_string_getval,
		   &lpc_string_putval,
		   &lpc_string_new,
		   &lpc_string_text,
		   &lpc_string_length) &&
	    ext_cb(ftabs[7], sizes[7], 6,
		   &lpc_object_putval,
		   &lpc_object_name,
		   &lpc_object_isspecial,
		   &lpc_object_ismarked,
		   &lpc_object_mark,
		   &lpc_object_unmark) &&
	    ext_cb(ftabs[8], sizes[8], 6,
		   &lpc_array_getval,
		   &lpc_array_putval,
		   &lpc_array_new,
		   &lpc_array_index,
		   &lpc_array_assign,
		   &lpc_array_size) &&
	    ext_cb(ftabs[9], sizes[9], 7,
		   &lpc_mapping_getval,
		   &lpc_mapping_putval,
		   &lpc_mapping_new,
		   &lpc_mapping_index,
		   &lpc_mapping_assign,
		   &lpc_mapping_enum,
		   &lpc_mapping_size) &&
	    ext_cb(ftabs[10], sizes[10], 6,
		   &lpc_runtime_error,
		   &lpc_md5_start,
		   &lpc_md5_block,
		   &lpc_md5_end,
		   &lpc_runtime_ticks,
		   &lpc_runtime_check) &&
	    lpc_ext_init(major, minor, config));
}


# define FD_CHUNK	500	/* # file descriptors to close in one batch */

/*
 * NAME:	ext->fdlist()
 * DESCRIPTION:	pass file descriptors to child process
 */
static void ext_fdlist(int *fdlist, int size)
{
    int num;

    while (size != 0) {
	num = (size > FD_CHUNK) ? FD_CHUNK : size;
	write(out, &num, sizeof(num));
	write(out, fdlist, num * sizeof(int));
	fdlist += num;
	size -= num;
    }

    num = 0;
    write(out, &num, sizeof(num));
}

/*
 * NAME:	ext->finish()
 * DESCRIPTION:	clean up before exiting
 */
static void ext_finish(void)
{
    close(in);
    close(out);
    close(back);
}

/*
 * NAME:	lpc_ext->spawn()
 * DESCRIPTION:	spawn a child process and execute the given program
 */
int lpc_ext_spawn(const char *program)
{
# ifndef WIN32
    int input[2], output[2];
    int pid;
    int status;

    /*
     * Fork a child and let it exit after forking a grandchild, so that
     * DGD never has to deal with any zombie processes.
     */
    pipe(input);
    pipe(output);
    pid = fork();
    if (pid > 0) {
	in = input[0];
	out = output[1];
	back = input[1];
	fcntl(in, F_SETFD, FD_CLOEXEC);
	fcntl(out, F_SETFD, FD_CLOEXEC);
	fcntl(back, F_SETFD, FD_CLOEXEC);
	close(output[0]);
	do {
	    wait(&status);
	} while (!WIFEXITED(status));

	(*ext_spawn)(&ext_fdlist, &ext_finish);
	return 1;
    } else if (pid == 0) {
	dup2(output[0], 0);
	dup2(input[1], 1);
	close(input[0]);
	close(input[1]);
	close(output[0]);
	close(output[1]);

	if (fork() == 0) {
	    int fds[FD_CHUNK], num;

	    /*
	     * receive file descriptors to close, until there are none left
	     */
	    for (;;) {
		if (read(0, &num, sizeof(num)) != sizeof(num)) {
		    break;
		}
		if (num == 0) {
		    /* execute the program */
		    execl("/bin/sh", "sh", "-c", program, (char *) NULL);
		    break;
		}
		if (read(0, fds, num * sizeof(int)) != num * sizeof(int)) {
		    break;
		}
		(*ext_fdclose)(fds, num);
	    }
	}

	/* child, grandchild if exec fails */
	_exit(0);
    } else {
	close(input[0]);
	close(input[1]);
	close(output[0]);
	close(output[1]);
    }

    return 0;	/* failure of some sort */
# else
    SECURITY_ATTRIBUTES sattr;
    HANDLE output;
    STARTUPINFO sinfo;
    PROCESS_INFORMATION pinfo;

    /* create pipes */
    sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
    sattr.bInheritHandle = TRUE;
    sattr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&in, &back, &sattr, 0)) {
	return 0;
    }
    if (!SetHandleInformation(in, HANDLE_FLAG_INHERIT, 0) ||
	!CreatePipe(&output, &out, &sattr, 0)) {
	CloseHandle(in);
	CloseHandle(back);
	return 0;
    }
    if (!SetHandleInformation(out, HANDLE_FLAG_INHERIT, 0)) {
	CloseHandle(in);
	CloseHandle(back);
	CloseHandle(output);
	CloseHandle(out);
	return 0;
    }

    /* spawn process */
    ZeroMemory(&sinfo, sizeof(STARTUPINFO));
    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.hStdInput = output;
    sinfo.hStdOutput = back;
    sinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    sinfo.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pinfo, sizeof(PROCESS_INFORMATION));
    if (!CreateProcess(NULL, (LPTSTR) program, NULL, NULL, TRUE, 0, NULL, NULL,
		       &sinfo, &pinfo)) {
	CloseHandle(in);
	CloseHandle(back);
	CloseHandle(output);
	CloseHandle(out);
	return 0;
    }

    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);
    CloseHandle(output);

    _setmode(out, O_BINARY);
    _setmode(back, O_BINARY);
    return 1;
# endif
}

/*
 * NAME:	lpc_ext->read()
 * DESCRIPTION:	read input from the child process
 */
int lpc_ext_read(void *buffer, int len)
{
    return read(in, buffer, len);
}

/*
 * NAME:	lpc_ext->write()
 * DESCRIPTION:	write output to the child process
 */
int lpc_ext_write(const void *buffer, int len)
{
    return write(out, buffer, len);
}

/*
 * NAME:	lpc_ext->writeback()
 * DESCRIPTION:	write back to oneself
 */
int lpc_ext_writeback(const void *buffer, int len)
{
    return write(back, buffer, len);
}
