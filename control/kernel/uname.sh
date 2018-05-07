#!/bin/bash

FILENAME=uname.h

LINUX_KERNEL_BUILD=$(uname -o | grep Linux)

FULL_VERSION=$(uname -r)
OLDIFS=$IFS
IFS='-'
SHORT_VERSION=$(echo $FULL_VERSION | awk '{print $1}')

echo $FULL_VERSION $SHORT_VERSION

IFS='.'
VERSION=$(echo $SHORT_VERSION | awk '{print $1}')
PATCHLEVEL=$(echo $SHORT_VERSION | awk '{print $2}')
SUBLEVEL=$(echo $SHORT_VERSION | awk '{print $3}')

IFS=$OLDIFS

echo "#ifndef _UNAME_CONTROLLER_H" > $FILENAME
echo "#define _UNAME_CONTROLLER_H" >> $FILENAME
echo "/*this file is generated automatically in the makefile */" >> $FILENAME
echo "/*NB: I always assume you are running the same kernel you are building!*/">>$FILENAME
echo "#define VERSION_STRING \"$FULL_VERSION\0\"" >> $FILENAME

if (( $VERSION >= 4 )) ; then
    if (( $PATCHLEVEL < 9 )); then
	API=old
	echo "#define OLD_API 1" >> $FILENAME
	echo "#define CONT_INIT_WORK init_kthread_work"  >> $FILENAME
	echo "#define CONT_INIT_WORKER init_kthread_worker"  >> $FILENAME
	echo "#define CONT_FLUSH_WORK flush_kthread_work" >> $FILENAME
	echo "#define CONT_FLUSH_WORKER flush_kthread_worker" >> $FILENAME
	echo "#define CONT_QUEUE_WORK queue_kthread_work"  >> $FILENAME
    else
	API=new
	echo "#define NEW_API 1" >> $FILENAME
	echo "#define CONT_INIT_WORK kthread_init_work"  >> $FILENAME
        echo "#define CONT_INIT_WORKER kthread_init_worker"  >> $FILENAME
	echo "#define CONT_FLUSH_WORK kthread_flush_work" >> $FILENAME
	echo "#define CONT_FLUSH_WORKER kthread_flush_worker" >> $FILENAME
	echo "#define CONT_QUEUE_WORK kthread_queue_work"  >> $FILENAME
    fi
fi


if (( $VERSION >= 4 )) ; then
    if (( $PATCHLEVEL < 7 )); then
	SOCKAPI=old
	echo "#define OLD_SOCK_API 1" >> $FILENAME
	echo "#define SOCK_RECVMSG(s, m, z, f) sock_recvmsg((s), (m), (z), (f))" >> $FILENAME
	echo "#define SOCK_CREATE_KERN(i, f, t, p, r) sock_create_kern((f), (t), (p), (r))" >> $FILENAME
    else
	SOCKAPI=new
	echo "#define NEW_SOCK_API 1" >> $FILENAME
	echo "#define SOCK_RECVMSG(s, m, z, f) sock_recvmsg((s), (m), (f))" >> $FILENAME
	echo "#define SOCK_CREATE_KERN(i, f, t, p, r) sock_create_kern((i), (f), (t), (p), (r))" >> $FILENAME
    fi
fi

if (( $VERSION >= 4 )) ; then
    if (( $PATCHLEVEL < 11 )); then
	ACCEPT_API=old
	echo "#define OLD_ACCEPT_API 1" >> $FILENAME
	echo "#define SOCK_ACCEPT(s, ns, i) accept((s), (ns), (i))" >> $FILENAME
    else
	ACCEPT_API=new
	echo "#define NEW_ACCEPT_API 1" >> $FILENAME
	echo "#define SOCK_ACCEPT(s, ns, i) accept((s), (ns), (i), 1)" >> $FILENAME
    fi
fi

if (( $VERSION >= 4 )) ; then
    if (( $PATCHLEVEL < 6 )); then
	FRAME_CHECKING=no
	echo "#define STACK_FRAME_NON_STANDARD(a)" >> $FILENAME
    else
	FRAME_CHECKING=yes
	echo "#define FRAME_CHECKING 1" >> $FILENAME
	echo "#include <linux/frame.h>" >> $FILENAME

    fi
fi

if (( $VERSION >= 4 )) ; then
    if (( $PATCHLEVEL < 14 )); then
	echo "#define ANCIENT_FILE_API 1" >> $FILENAME
    else
	echo "#define MODERN_FILE_API 1" >> $FILENAME
    fi
fi





#socket interface changes:
# version 4.1.13:

#int sock_recvmsg(struct socket *sock, struct msghdr *msg, size_t size,
#		 int flags);

# version 4.7:
#int sock_recvmsg(struct socket *sock, struct msghdr *msg, int flags);

echo "static const char *cont_api __attribute__((used)) = \"$API\";" >> $FILENAME
echo "#endif /* _UNAME_CONTROLLER_H */" >> $FILENAME

echo "kthreads api: $API"
echo "sock api: $SOCKAPI"
echo "accept api: $ACCEPT_API"
