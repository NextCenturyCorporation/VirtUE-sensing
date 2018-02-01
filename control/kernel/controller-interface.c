/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/

#include "controller.h"
#include "jsmn/jsmn.h"

/* hold socket JSON probe interface */


/* A probe routine is a kthread  worker, called from kernel thread.
 * it needs to execute quickly and can't hold any locks or
 * blocking objects. Once it runs once, it must be re-initialized
 * and re-queued to run again...see /include/linux/kthread.h

 * The probe is the low-level struct that we want to get into, it has
 * the keys to the other state information. We want to get:
 * 1) struct kthread_work *work.
 * 2) struct kthread_worker work->worker
 * 3) struct probe_s probe_struct *probe_work  using container_of()
 */

void  k_probe(struct kthread_work *work)
{
#ifdef NOTHING
    struct kernel_sensor *ks =
		(struct kernel_sensor *)container_of(&work, struct kernel_sensor, kwork);
	struct probe_s *k_probe =
		(struct probe_s *)container_of(&work, struct probe_s, probe_work);

	do
	{
		if (l->sock == accept_controller_sock(    );
			if (l->sock < 0)
			{
				printk (KERN_ALERT "accept on %p failed with code %d\n",
						l->sock, l_probe->cli);
			}
			/*
			 * sched_yield is needed in the inner loop to prevent a DOS by
			 * a client flooding the server with request messages; the
			 * loop will keep reading messages until the client closes the
			 * socket. A bad client can send msg after msg after msg, ad
			 * infinutum. In addition to a yield, I think there should
			 * also be a counter to limit how many times through the inner
			 * loop the server will allow for a client.
			 */

			while (l_probe->client_fd > 0 &&
				   l_probe->number_of_client_messages < CONTROLLER_MSG_SESSION_LIMIT)
			{
				uint16_t version, id;
				uint32_t len;

				l_probe->quit = read_controller_message_header (l_probe->client_fd,
																&version,
																&id, &len,
																(void **)&l_probe->listen_buf);
				if (l_probe->quit == CONTROLLER_ERR_CLOSED)
				{
					printk (KERN_ALERT "client closed socket %d\n", l_probe->client_fd);
					close (l_probe->client_fd);
					l_probe->client_fd = -1;
				}
				else if (l_probe->quit < 0)
				{
					printk (KERN_ALERT "error reading header %d\n", l_probe->quit);
					return;
				}\

				l_probe->number_of_client_messages++;
				sched_yield ();
			}
			}
		else
		{
			printk (KERN_ALERT "bad socket value in listen thread\n");
			return;
		}
	} while (! l_probe->quit );
	if (l_probe->client_fd >= 0)
		close (l_probe->client_fd);
#endif
	return;
}

#ifdef NOTHING
/* Returns a linux kernel soket strukture that is listening and capable
 * of reproducing
 * it may create a structure from stratch, or re-incarnate an existing
 * linux listen soket struktur, depending upon the need of the
 * probe/controller
 * the point of the double-pointer business is to stuff all the
 * linux-isms into the pointer-abstracted-metaphysics
 */
// create and listen on a unix domain socket.
// connect, peek at the incoming data.
// sock_name: full path of the socket e.g.
// we need to sock name, we need to return the actual sock, so:

int listen_controller_sock(struct listen **l, uint8_t *name)
{

	/* 1) already listening,
	 *    1a) already listening, already reproduced
	 * 2) not already listening
	 *    2a) already existing
	 * 3) not already existing
	 */

	struct listen_linux *ll = NULL;
	int ccode = CONTROLLER_OK;

	if (!l || ! name) {
		return CONTROLLER_ERR_BAD_FD;
	}

	if (!*l) {
		/* allocate a new linux listener */
		ll = kzalloc(sizeof(struct listen_linux), GFP_KERNEL);
		if (!ll) {
			return ERR_PTR(-ENOMEM);
		}
		*l = (struct listen *)ll;
	} else {
		/* zero-out an existing linux listener that has
		 * been allocated here in a previous call
		*/
		ll = (struct listen_linux *)*l;
		memset(ll, 0x00, sizeof(struct listen_linux));
	}

	/* now that we have a linux-specific socket struct, open an AF_UNIX file
	 */
	ccode  = sock_create(AF_UNIX, SOCK_STREAM, 0, &ll->sock);
/* check the return code, add some linux-specific error handling
 */  ccode = listen(




	return ccode;
}


/*******************
 *   Error handling (accept man page)
 *   Linux accept() (and accept4()) passes already-pending network errors on
 *   the new socket as an  error code  from  accept().   This  behavior
 *   differs from other BSD socket implementations.  For reliable operation
 *   the application should detect the network errors defined for the protocol
 *   after  accept() and treat them like EAGAIN by retrying.  In the case of
 *   TCP/IP, these are ENETDOWN, EPROTO, ENOPRO‐TOOPT, EHOSTDOWN, ENONET,
 *  EHOSTUNREACH, EOPNOTSUPP, and ENETUNREACH.
 ****************/

int
accept_controller_sock(struct listen *listen)
{

	/* need to have already listening successfullly on the socket
     * got the sock, open it
     * kernel does choose the protocol if you select the family and type
	 * we select AF_UNIX, SOCK_STREAM, which is a basiK domain socket
	 */
    int listen_controller_sock(struct listen **l, uint8_t *name)

	int ccode = CONTROLLER_OK;
	struct sockaddr_un un = {
		.AF_UNIX,
		."/var/run/kernel/control"
	};
	struct listen_linux l_lx = {
		.NULL,
		.un.sun_path
	};



    len = sizeof (struct sockaddr_un);
	do
    {
		clifd = accept (listenfd, (struct sockaddr *) &un, &len);

    }
	while ((clifd == -1)
		   && (errno == EINTR || errno == EAGAIN || EPERM || EACCES
			   || EALREADY || EINPROGRESS || ETIMEDOUT));

	DMSG ("accept returning clifd %d\n", clifd);
	return (clifd);
}

/*
 *  a UNIX domain socket path is 108 bytes
 */
#define SUN_PATH_SIZE 108
int
client_func (void *p)
{
	DMSG ("client %d\n", getpid ());

	char *spath = (char *) p;
	int s, len;
	int should_unlink = 0;
	struct sockaddr_un un, sun;
	char cpath[SUN_PATH_SIZE];
	memset (cpath, 0, sizeof (cpath));
	sprintf (cpath, "%d", getpid ());

	if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
		DMSG ("unable to get a socket\n");
		return CONTROLLER_ERR_BAD_FD;
    }
	should_unlink = 1;
	memset (&un, 0, sizeof (un));
	un.sun_family = AF_UNIX;
	snprintf (un.sun_path, SUN_PATH_SIZE, "%s", cpath);

	len = offsetof (struct sockaddr_un, sun_path) + strlen (un.sun_path);
	unlink (un.sun_path);
	if (bind (s, (struct sockaddr *) &un, len) < 0)
    {
		DMSG ("bind failed (client) %s\n", cpath);
		perror (NULL);
		goto errout;
    }

	if (chmod (un.sun_path, S_IRWXU) < 0)
    {
		DMSG ("failed to set permissions %s\n", un.sun_path);
		perror (NULL);
		goto errout;
    }
	memset (&sun, 0, sizeof (sun));
	sun.sun_family = AF_UNIX;
	strncpy (sun.sun_path, spath, sizeof (sun.sun_path));
	len = offsetof (struct sockaddr_un, sun_path) + strlen (spath);
	DMSG ("client connecting to %s\n", spath);
	if (connect (s, (struct sockaddr *) &sun, len) < 0)
    {
		unlink (un.sun_path);
		DMSG ("connect from %s failed\n", un.sun_path);
		perror (NULL);
		goto errout;
    }
	unlink (cpath);
	DMSG ("connected\n");
	return s;
errout:
	if (should_unlink && (strlen (cpath) > 0))
    {
		unlink (cpath);
    }
	return CONTROLLER_ERR_BAD_FD;
}

/*
 * WRS, with check for Linux EAGAIN
 */
ssize_t
readn (int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;
	ptr = vptr;
	nleft = n;
	while (nleft > 0)
    {
		if ((nread = read (fd, ptr, nleft)) < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
				nread = 0;		/* and call read() again */
			else
				return (-1);
		}
		else if (nread == 0)
			break;			/* EOF */

		nleft -= nread;
		ptr += nread;
    }
	return (n - nleft);		/* return >= 0 */
}

/*
 * WRS, with check for Linux EAGAIN
 */
ssize_t
writen (int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
    {
		if ((nwritten = write (fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && (errno == EINTR || errno == EAGAIN))
				nwritten = 0;	/* and call write() again */
			else
			{
				DMSG ("errno: %d\n", errno);
				perror (NULL);
				return (-1);	/* error */
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
    }
	return (n);
}

int
write_controller_message_header (int fd, uint16_t version, uint16_t id)
{
	uint8_t magic[] = CONTROLLER_MSG_MAGIC;
	uint32_t len = CONTROLLER_MSG_HDRLEN;

	if (writen (fd, magic, sizeof (magic)) != sizeof (magic))
		goto errout;
	if (writen (fd, &version, sizeof (uint16_t)) != sizeof (uint16_t))
		goto errout;
	if (writen (fd, &id, sizeof (uint16_t)) != sizeof (uint16_t))
		goto errout;
	if (writen (fd, &len, sizeof (len)) != sizeof (len))
		goto errout;
	return CONTROLLER_OK;
errout:
	return CONTROLLER_ERR_RW;
}

/*
 * header is 3 * 32 bytes - 'CONT' overall message length
 * if this function returns ERR, ptr parameters are undefined
 * if it returns 0, ptr parameters will have correct values
 *
 * void **buf is for the dispatch function to place data for the caller.
 * buf points to a pointer to null (*(void **)buf == NULL)
 */

int
read_controller_message_header (int fd, uint16_t * version,
								uint16_t * id, uint32_t * len, void **buf)
{
	uint8_t hbuf[CONTROLLER__HBUFLEN];
	uint32_t ccode = CONTROLLER_OK;
	int i = 0;

	DMSG ("reading messsge header...\n");
	DMSG ("reading %d bytes from %d into %p\n", CONTROLLER_MSG_HDRLEN, fd, hbuf);

	for (; i < CONTROLLER_MSG_HDRLEN; i++)
	{
		if ((ccode = readn (fd, &hbuf[i], 1)) != 1)
		{
			if (ccode == 0)
			{
				DMSG ("read_controller_message_header: other party"
					  " closed the socket\n");
				return CONTROLLR_ERR_CLOSED;
			}
			goto errout;
		}
	}
	dump_controller (hbuf, CONTROLLER_H_MSG_HDRLEN);
	DMSG ("checking magic ...\n");
	if (check_magic (hbuf))
	{
		ccode = CONTROLLER_ERR_BAD_HDR;
		DMSG ("bad magic on message header\n");
		goto errout;
	}
	DMSG ("checking protocol version...%d\n", CONTROLLER_MSG_GET_VER (hbuf));
	if (CONTROLLER_MSG_VERSION != (*version = CONTROLLER_MSG_GET_VER (hbuf)))
	{
		ccode = CONTROLLER_ERR_BAD_VER;
		goto errout;
	}
	*id = CONTROLLER_MSG_GET_ID (hbuf);
	DMSG ("reading message type: %d\n", *id);

	if (*id < CONTROLLER_MSG_FIRST || *id > CONTROLLER_MSG_LAST)
	{
		ccode = CONTROLLER_ERR_BAD_MSGID;
		goto errout;
	}

	DMSG ("read header: msglen %d\n", CONTROLLER_MSG_GET_LEN (hbuf));
	dump_controller (hbuf + 8, 4);
	if (CONTROLLER_ALLOC_SIZE < (*len = CONTROLLER_MSG_GET_LEN (hbuf)))
	{
		DMSG ("max length: %d; this length:%d\n", CONTROLLER_ALLOC_SIZE, *len);
		ccode = CONTROLLER_ERR_BAD_LEN;
		goto errout;
	}

	if (ccode == CONTROLLER_OK)
		return ccode;
errout:
	DMSG ("read a bad or incomplete controller header\n");
	return ccode;
}

#endif /* NOTHING */
