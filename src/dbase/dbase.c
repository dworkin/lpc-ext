/*
 * This code is released into the public domain.
 *
 * Minimal example implementation of database module: replace snapshots with
 * database.  This is incomplete; not all errors are checked for, locking is
 * not implemented, pwrite/pread/aio/io_uring could be used, file descriptors
 * are kept open for all database objects, internal database snapshots are
 * not cleaned up and the database can grow very large, etc.
 *
 * Functions may simultaneously be called from different threads, but each
 * database object will only be accessed by one thread at a time.  Snapshot
 * functions are always called sequentially.
 *
 * The database module support code in Hydra is not as well-tested as the
 * swap/snapshot code.  Note that Hydra can restore snapshots into a database
 * but not the other way around; once you switch to using the database
 * interface, you are committed!
 *
 * Hydra's snapshot code is not an implementation of the database interface,
 * and there is a difference in what data is saved.  That difference is likely
 * to increase in the future.
 *
 * For use in production, this code should be completely rewritten, perhaps to
 * forward calls to an external database.
 */

# include <string.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <stdlib.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdio.h>
# include "lpc_ext.h"


# define TRUE	1
# define FALSE	0

struct _lpc_database_ {
    char path[1024];	/* path to database directory */
    int generation;	/* start at 1, up to 65534, wraparound */
};
struct _lpc_db_object_ {
    uint64_t index;	/* unique object index */
    int generation;	/* database generation */
    int fd;		/* database object file descriptor */
    LPC_db *db;		/* database this object is in */
};

static char dir[1024];	/* directory containing database */

/*
 * determine, based on the file name, whether something is a database
 */
static int db_valid(const char *file)
{
    ssize_t len;

    len = strlen(file);
    return (len >= 8 && strcmp(file + len - 7, ".testdb") == 0);
}

/*
 * open a database
 */
static LPC_db *db_open(const char *file, int *created)
{
    char buffer[2048];
    struct stat statbuf;
    LPC_db *db;

    if (!db_valid(file)) {
	return NULL;
    }
    sprintf(buffer, "%s/%s", dir, file);
    if (stat(buffer, &statbuf) != 0) {
	/*
	 * create new database
	 */
	if (mkdir(buffer, 0700) != 0) {
	    return NULL;
	}

	db = malloc(sizeof(LPC_db));
	strcpy(db->path, buffer);
	db->generation = 1;

	/* first generation */
	strcat(buffer, "/1");
	if (mkdir(buffer, 0700) != 0) {
	    free(db);
	    return NULL;
	}

	*created = TRUE;
    } else {
	int fd;

	/*
	 * open existing databse
	 */
	if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
	    return NULL;
	}
	db = malloc(sizeof(LPC_db));
	strcpy(db->path, buffer);

	strcat(buffer, "/snapshot");
	fd = open(buffer, O_RDONLY);
	if (fd < 0) {
	    free(db);
	    return NULL;
	}
	if (read(fd, buffer, sizeof(buffer)) < 0) {
	    close(fd);
	    free(db);
	    return NULL;
	}
	close(fd);

	/* start new generation */
	db->generation = atoi(buffer) + 1;
	if (db->generation == 65535) {
	    db->generation = 1;
	}

	*created = FALSE;
    }

    return db;
}

/*
 * close a database
 */
static void db_close(LPC_db *db)
{
    /* expect that all dbase objects will be deleted first */
    free(db);
}

/*
 * create a new object in a database
 */
static LPC_db_object *db_new_obj(LPC_db *db, LPC_db_index index)
{
    char buffer[2048];
    int fd;
    LPC_db_object *obj;

    sprintf(buffer, "%s/%d/%lld", db->path, db->generation, (long long) index);
    fd = open(buffer, O_CREAT | O_TRUNC | O_RDWR, 0600);
    if (fd < 0) {
	return NULL;
    }

    obj = malloc(sizeof(LPC_db_object));
    obj->index = index;
    obj->generation = db->generation;
    obj->fd = fd;
    obj->db = db;
    return obj;
}

/*
 * load an object from a database
 */
static LPC_db_object *db_load_obj(LPC_db *db, LPC_db_index index,
				  LPC_db_handle handle)
{
    char buffer[2048];
    int fd;
    LPC_db_object *obj;

    /* assume that the handle passed by the caller is the object generation */
    sprintf(buffer, "%s/%d/%lld", db->path, (int) handle, (long long) index);
    fd = open(buffer, O_RDWR);
    if (fd < 0) {
	return NULL;
    }

    obj = malloc(sizeof(LPC_db_object));
    obj->index = index;
    obj->generation = (int) handle;
    obj->fd = fd;
    obj->db = db;
    return obj;
}

/*
 * delete an object reference
 */
static int db_del_obj(LPC_db_object *obj)
{
    if (obj->fd >= 0) {
	close(obj->fd);
    }
    free(obj);

    return TRUE;
}

/*
 * if the object is not of the current generation, it should be refreshed
 * (recreated in the current generation) when modified
 */
static int db_refresh_obj(LPC_db_object *obj)
{
    return (obj->generation != obj->db->generation);
}

/*
 * Resize an object in a database (first time, resize from 0). Objects that
 * change in size are always resized before they are written to.
 */
static int db_resize_obj(LPC_db_object *obj, uint64_t size,
			 LPC_db_handle *handle)
{
    if (ftruncate(obj->fd, (size_t) size) == 0) {
	/* pass the object generation to the caller */
	*handle = obj->generation;
	return TRUE;
    } else {
	return FALSE;
    }
}

/*
 * read from an object in a database
 */
static int db_read_obj(LPC_db_object *obj, LPC_db_request *request,
		       int nrequest)
{
    int i;

    for (i = 0; i < nrequest; i++) {
	if (lseek(obj->fd, (off_t) request[i].offset, SEEK_SET) < 0 ||
	    read(obj->fd, request[i].data, request[i].size) != request[i].size)
	{
	    return FALSE;
	}
    }

    return TRUE;
}

/*
 * write to an object in a database
 */
static int db_write_obj(LPC_db_object *obj, LPC_db_request *request,
			int nrequest)
{
    int i;

    for (i = 0; i < nrequest; i++) {
	if (lseek(obj->fd, (off_t) request[i].offset, SEEK_SET) < 0 ||
	    write(obj->fd, request[i].data, request[i].size) != request[i].size)
	{
	    return FALSE;
	}
    }

    return TRUE;
}

/*
 * remove an object from the database
 */
static int db_remove_obj(LPC_db_object *obj)
{
    char buffer[2048];

    if (obj->fd < 0) {
	return FALSE;	/* already removed */
    }

    close(obj->fd);
    obj->fd = -1;
    sprintf(buffer, "%s/%d/%lld", obj->db->path, obj->generation,
	    (long long) obj->index);
    return (unlink(buffer) == 0);
}

/*
 * save snapshot info in a database
 */
static int db_save(LPC_db *db, LPC_db_request *request)
{
    char buffer[2048];
    int fd;

    /* save snapshot tables for current generation */
    sprintf(buffer, "%s/snapshot-%d", db->path, db->generation);
    fd = open(buffer, O_WRONLY | O_CREAT, 0600);
    if (fd < 0) {
	return FALSE;
    }

    if (lseek(fd, (off_t) (request->offset + 1024), SEEK_SET) < 0 ||
	write(fd, request->data, request->size) != request->size) {
	close(fd);
	return FALSE;
    }
    close(fd);

    return TRUE;
}

/*
 * restore snapshot info from a database
 */
static int db_restore(LPC_db *db, LPC_db_request *request)
{
    char buffer[2048];
    int snapshot;
    int fd;

    /* restore snapshot tables from previous generation */
    snapshot = db->generation - 1;
    if (snapshot == 0) {
	snapshot = 65534;
    }
    sprintf(buffer, "%s/snapshot-%d", db->path, snapshot);
    fd = open(buffer, O_RDONLY);
    if (fd < 0) {
	return FALSE;
    }

    if (lseek(fd, (off_t) (request->offset + 1024), SEEK_SET) < 0 ||
	read(fd, request->data, request->size) != request->size) {
	close(fd);
	return FALSE;
    }
    close(fd);

    return TRUE;
}

/*
 * finish snapshot in a database
 */
static int db_save_snapshot(LPC_db *db, LPC_db_request *request)
{
    char buffer[2048];
    int fd;

    /* save snapshot header for current generation */
    sprintf(buffer, "%s/snapshot-%d", db->path, db->generation);
    fd = open(buffer, O_WRONLY);
    if (fd < 0) {
	return FALSE;
    }
    if (write(fd, request->data, request->size) != request->size) {
	close(fd);
	return FALSE;
    }
    close(fd);

    /* save snapshot generation number */
    sprintf(buffer, "%s/snapshot", db->path);
    fd = open(buffer, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
	return FALSE;
    }
    sprintf(buffer, "%d", db->generation);
    if (write(fd, buffer, strlen(buffer)) != strlen(buffer)) {
	close(fd);
	return FALSE;
    }
    close(fd);

    /* increase generation */
    db->generation++;
    if (db->generation == 65535) {
	db->generation = 1;
    }
    sprintf(buffer, "%s/%d", db->path, db->generation);
    mkdir(buffer, 0700);	/* allowed to fail (generation wraparound) */

    return TRUE;
}

/*
 * restore snapshot from a database
 */
static int db_restore_snapshot(LPC_db *db, LPC_db_request *request)
{
    char buffer[2048];
    int snapshot;
    int fd;

    /* restore previous generation */
    snapshot = db->generation - 1;
    if (snapshot == 0) {
	snapshot = 65534;
    }
    sprintf(buffer, "%s/snapshot-%d", db->path, snapshot);
    fd = open(buffer, O_RDONLY);
    if (fd < 0) {
	return FALSE;
    }
    if (read(fd, request->data, request->size) != request->size) {
	close(fd);
	return FALSE;
    }
    close(fd);

    return TRUE;
}

static const LPC_ext_dbase db_ext = {
    &db_valid,
    &db_open,
    &db_close,
    &db_new_obj,
    &db_load_obj,
    &db_del_obj,
    &db_refresh_obj,
    &db_resize_obj,
    &db_read_obj,
    &db_write_obj,
    &db_remove_obj,
    &db_save,
    &db_restore,
    &db_save_snapshot,
    &db_restore_snapshot
};

/*
 * initialize database interface
 */
int lpc_ext_init(int major, int minor, const char *config)
{
    strcpy(dir, config);
    (*lpc_ext_dbase)(&db_ext);

    return TRUE;
}
