/*
	File:		MoreUNIX.c

	Contains:	Generic UNIX utilities.

	Written by:	Quinn

	Copyright:	Copyright (c) 2002 by Apple Computer, Inc., All Rights Reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Change History (most recent first):

$Log: MoreUNIX.c,v $
Revision 1.7  2003/05/25 11:47:41  eskimo1
Yet more fixes to the descriptor passing code. Handle generation of EPIPE on EOF. Only pass one byte of data, which makes for fewer cases on the receive side. Funnily enough, the code has evolved to be very close to the canonical code from Stevens.

Revision 1.6  2003/05/24 15:18:00  eskimo1
Fixed a bug in descriptor passing. We have to pass some data along with the descriptor, otherwise recvmsg does not block properly.  Also, fixed a bug where I incorrectly initialised msg_controllen in MoreUNIXReadDescriptor.

Revision 1.5  2003/05/23 21:55:48  eskimo1
Added MoreUNIXReadDescriptor and MoreUNIXWriteDescriptor.

Revision 1.4  2002/12/13 00:30:18  eskimo1
10.1 compatibility. futimes isn't available in 10.1. Drat! Also, in NSGetExecutablePathOnTenOneAndEarlierOnly, have to skip over multiple NULLs between the environment and the relative path point, in case someone (like MoreSecDestroyInheritedEnvironment perhaps) has called unsetenv.

Revision 1.3  2002/12/12 23:14:29  eskimo1
C++ compatibility.

Revision 1.2  2002/11/14 20:15:20  eskimo1
In MoreUNIXCopyFile, correctly set the modified and accessed times of the resulting file.

Revision 1.1  2002/11/09 00:15:21  eskimo1
A collection of useful UNIX-level routines.


*/

/////////////////////////////////////////////////////////////////

// Our prototypes

#include "MoreUNIX.h"

// System interfaces

#include <sys/param.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <stdbool.h>

// "crt_externs.h" has no C++ guards [3126393], so we have to provide them 
// ourself otherwise we get a link error.

#ifdef __cplusplus
extern "C" {
#endif

#include <crt_externs.h>

#ifdef __cplusplus
}
#endif

// MIB Interfaces

/////////////////////////////////////////////////////////////////

#if MORE_DEBUG

	// There's a macro version of this in the header.
	
	extern int MoreUNIXErrno(int result)
	{
		int err;
		
		err = 0;
		if (result < 0) {
			err = errno;
			assert(err != 0);
		}
		return err;
	}

#endif

/////////////////////////////////////////////////////////////////

static int NSGetExecutablePathOnTenOneAndEarlierOnly(char *execPath, size_t *execPathSize)
{
    int  	err = 0;
    char 	**cursor;
    char 	*possiblyRelativePath;
    char 	absolutePath[MAXPATHLEN];
    size_t 	absolutePathSize;
    
    assert(execPath != NULL);
    assert(execPathSize != NULL);
    
    cursor = (char **) (*(_NSGetArgv()) + *(_NSGetArgc()));
    
    // There should be a NULL after the argv array.
    // If not, error out.
    
    if (*cursor != 0) {
        err = -1;
	}
    
    if (err == 0) {
        // Skip past the NULL after the argv array.
        
        cursor += 1;
        
        // Now, skip over the entire kernel-supplied environment, 
        // which is an array of char * terminated by a NULL.
        
        while (*cursor != 0) {
            cursor += 1;
        }
        
        // Skip over the NULL terminating the environment.  Actually, 
        // there can be multiple NULLs if the program has called 
        // unsetenv, so we skip along until we find a the next non-NULL.
        
        while (*cursor == 0) {
            cursor += 1;
        }

        // Now we have the path that was passed to exec 
        // (not the argv[0] path, but the path that the kernel 
        // actually executed).
        
        possiblyRelativePath = *cursor;

        // Convert the possibly relative path to an absolute 
        // path.  We use realpath for expedience, although 
        // the real implementation of _NSGetExecutablePath
        // uses getcwd and can return a path with symbolic links 
        // etc in it.
        
        if (realpath(possiblyRelativePath, absolutePath) == NULL) {
            err = errno;
		}
    }
    
    // Now copy the path out into the client's buffer, returning 
    // an error if the buffer isn't big enough.
    
    if (err == 0) {
        absolutePathSize = (strlen(absolutePath) + 1);
        
        if (absolutePathSize <= *execPathSize) {
            strcpy(execPath, absolutePath);
        } else {
            err = -1;
        }
        
        *execPathSize = absolutePathSize;
    }

    return err;
}

typedef int (*NSGetExecutablePathProcPtr)(char *buf, uint32_t *bufsize);

extern int MoreGetExecutablePath(char *execPath, uint32_t *execPathSize)
{
	return _NSGetExecutablePath(execPath, execPathSize);
	// It looks like the code above is legacy anyway when deploying to 10.2 and above.
	// I got rid of it to avoid deprecation warnings under Leopard
	
/*	
    if (NSIsSymbolNameDefined("__NSGetExecutablePath")) {
        return ((NSGetExecutablePathProcPtr) NSAddressOfSymbol(NSLookupAndBindSymbol("__NSGetExecutablePath")))(execPath, execPathSize);
    } else {
        // The function _NSGetExecutablePath() is new in Mac OS X 10.2, 
        // so use this custom version when running on 10.1.x and earlier.
        return NSGetExecutablePathOnTenOneAndEarlierOnly(execPath, execPathSize);
    }
*/
}

/////////////////////////////////////////////////////////////////

extern int MoreUNIXRead( int fd,       void *buf, size_t bufSize, size_t *bytesRead   )
	// See comment in header.
{
	int 	err;
	char *	cursor;
	size_t	bytesLeft;
	ssize_t bytesThisTime;

	assert(fd >= 0);
	assert(buf != NULL);
	
	err = 0;
	bytesLeft = bufSize;
	cursor = (char *) buf;
	while ( (err == 0) && (bytesLeft != 0) ) {
		bytesThisTime = read(fd, cursor, bytesLeft);
		if (bytesThisTime > 0) {
			cursor    += bytesThisTime;
			bytesLeft -= bytesThisTime;
		} else if (bytesThisTime == 0) {
			err = EPIPE;
		} else {
			assert(bytesThisTime == -1);
			
			err = errno;
			assert(err != 0);
			if (err == EINTR) {
				err = 0;		// let's loop again
			}
		}
	}
	if (bytesRead != NULL) {
		*bytesRead = bufSize - bytesLeft;
	}
	
	return err;
}

extern int MoreUNIXWrite(int fd, const void *buf, size_t bufSize, size_t *bytesWritten)
	// See comment in header.
{
	int 	err;
	char *	cursor;
	size_t	bytesLeft;
	ssize_t bytesThisTime;
	
	assert(fd >= 0);
	assert(buf != NULL);
	
	// SIGPIPE occurs when you write to pipe or socket 
	// whose other end has been closed.  The default action 
	// for SIGPIPE is to terminate the process.  That's 
	// probably not what you wanted.  So, in the debug build, 
	// we check that you've set the signal action to SIG_IGN 
	// (ignore).  Of course, you could be building a program 
	// that needs SIGPIPE to work in some special way, in 
	// which case you should define MORE_UNIX_WRITE_CHECK_SIGPIPE 
	// to 0 to bypass this check.
	//
	// MoreUNIXIgnoreSIGPIPE, a helper routine defined below, 
	// lets you disable SIGPIPE easily.
	
	#if !defined(MORE_UNIX_WRITE_CHECK_SIGPIPE)
		#define MORE_UNIX_WRITE_CHECK_SIGPIPE 1
	#endif
	#if MORE_DEBUG && MORE_UNIX_WRITE_CHECK_SIGPIPE
		{
			int junk;
			struct sigaction currentSignalState;
			
			junk = sigaction(SIGPIPE, NULL, &currentSignalState);
			assert(junk == 0);
			
			assert( currentSignalState.sa_handler == SIG_IGN );
		}
	#endif

	err = 0;
	bytesLeft = bufSize;
	cursor = (char *) buf;
	while ( (err == 0) && (bytesLeft != 0) ) {
		bytesThisTime = write(fd, cursor, bytesLeft);
		if (bytesThisTime > 0) {
			cursor    += bytesThisTime;
			bytesLeft -= bytesThisTime;
		} else if (bytesThisTime == 0) {
			assert(false);
			err = EPIPE;
		} else {
			assert(bytesThisTime == -1);
			
			err = errno;
			assert(err != 0);
			if (err == EINTR) {
				err = 0;		// let's loop again
			}
		}
	}
	if (bytesWritten != NULL) {
		*bytesWritten = bufSize - bytesLeft;
	}
	
	return err;
}

extern int MoreUNIXIgnoreSIGPIPE(void)
	// See comment in header.
{
	int err;
	struct sigaction signalState;
	
	err = sigaction(SIGPIPE, NULL, &signalState);
	err = MoreUNIXErrno(err);
	if (err == 0) {
		signalState.sa_handler = SIG_IGN;
		
		err = sigaction(SIGPIPE, &signalState, NULL);
		err = MoreUNIXErrno(err);
	}
	
	return err;
}

// When we pass a descriptor, we have to pass at least one byte 
// of data along with it, otherwise the recvmsg call will not 
// block if the descriptor hasn't been written to the other end 
// of the socket yet.

static const char kDummyData = 'D';

extern int MoreUNIXReadDescriptor(int fd, int *fdRead)
	// See comment in header.
{
	int 				err;
	struct msghdr 		msg;
	struct iovec		iov;
	struct {
		struct cmsghdr 	hdr;
		int            	fd;
	} 					control;
	char				dummyData;
	ssize_t				bytesReceived;

	assert(fd >= 0);
	assert( fdRead != NULL);
	assert(*fdRead == -1);

	iov.iov_base = (char *) &dummyData;
	iov.iov_len  = sizeof(dummyData);
	
    msg.msg_name       = NULL;
    msg.msg_namelen    = 0;
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = (caddr_t) &control;
    msg.msg_controllen = sizeof(control);
    msg.msg_flags	   = MSG_WAITALL;
    
    do {
	    bytesReceived = recvmsg(fd, &msg, 0);
	    if (bytesReceived == sizeof(dummyData)) {
	    	if (   (dummyData != kDummyData)
	    		|| (msg.msg_flags != 0) 
	    		|| (msg.msg_control == NULL) 
	    		|| (msg.msg_controllen != sizeof(control)) 
	    		|| (control.hdr.cmsg_len != sizeof(control)) 
	    		|| (control.hdr.cmsg_level != SOL_SOCKET)
				|| (control.hdr.cmsg_type  != SCM_RIGHTS) 
				|| (control.fd < 0) ) {
	    		err = EINVAL;
	    	} else {
	    		*fdRead = control.fd;
		    	err = 0;
	    	}
	    } else if (bytesReceived == 0) {
	    	err = EPIPE;
	    } else {
	    	assert(bytesReceived == -1);

	    	err = errno;
	    	assert(err != 0);
	    }
	} while (err == EINTR);

	assert( (err == 0) == (*fdRead >= 0) );
	
	return err;
}

extern int MoreUNIXWriteDescriptor(int fd, int fdToWrite)
	// See comment in header.
{
	int 				err;
	struct msghdr 		msg;
	struct iovec		iov;
	struct {
		struct cmsghdr 	hdr;
		int            	fd;
	} 					control;
	ssize_t 			bytesSent;

	assert(fd >= 0);
	assert(fdToWrite >= 0);

    control.hdr.cmsg_len   = sizeof(control);
    control.hdr.cmsg_level = SOL_SOCKET;
    control.hdr.cmsg_type  = SCM_RIGHTS;
    control.fd             = fdToWrite;

	iov.iov_base = (char *) &kDummyData;
	iov.iov_len  = sizeof(kDummyData);
	
    msg.msg_name       = NULL;
    msg.msg_namelen    = 0;
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = (caddr_t) &control;
    msg.msg_controllen = control.hdr.cmsg_len;
    msg.msg_flags	   = 0;
    do {
	    bytesSent = sendmsg(fd, &msg, 0);
	    if (bytesSent == sizeof(kDummyData)) {
	    	err = 0;
	    } else {
	    	assert(bytesSent == -1);

	    	err = errno;
	    	assert(err != 0);
	    }
	} while (err == EINTR);

    return err;
}

extern int MoreUNIXCopyDescriptorToDescriptor(int source, int dest)
	// See comment in header.
{
	int		err;
	bool    done;
	char *  buffer;
	static const size_t kBufferSize = 64 * 1024;
	
	assert(source >= 0);
	assert(dest   >= 0);
	
	err = 0;
	buffer = (char *) malloc(kBufferSize);
	if (buffer == NULL) {
		err = ENOMEM;
	}

	if (err == 0) {	
		done = false;
		do {
			size_t bytesRead;
			
			err = MoreUNIXRead(source, buffer, kBufferSize, &bytesRead);
			if (err == EPIPE) {
				done = true;
				err = 0;
			}
			if (err == 0) {
				err = MoreUNIXWrite(dest, buffer, bytesRead, NULL);
			}
		} while (err == 0 && !done);
	}
	
	free(buffer);
	
	return err;
}

extern int MoreUNIXCopyFile(const char *source, const char *dest)
	// See comment in header.
{
	int 		err;
	int 		junk;
	int 		sourceFD;
	int 		destFD;
	struct stat sb;
	
	assert(source != NULL);
	assert(dest   != NULL);
	
	sourceFD = -1;
	destFD   = -1;

	err = stat(source, &sb);
	err = MoreUNIXErrno(err);
	
	if (err == 0) {
		sourceFD = open(source, O_RDONLY, 0);
		err = MoreUNIXErrno(sourceFD);
	}
	
	if (err == 0) {
		destFD = open(dest, O_WRONLY | O_CREAT | O_TRUNC, sb.st_mode & ~(S_ISUID | S_ISGID | S_ISVTX));
		err = MoreUNIXErrno(destFD);
	}
	
	if (err == 0) {
		err = MoreUNIXCopyDescriptorToDescriptor(sourceFD, destFD);
	}

	// Close the descriptors.
	
	if (sourceFD != -1) {
		junk = close(sourceFD);
		assert(junk == 0);
	}
	if (destFD != -1) {
		junk = close(destFD);
		assert(junk == 0);
	}

	// Set the access and modification times of the destination 
	// to match the source.  I originally did this using futimes, 
	// but hey, that's not available on 10.1, so now I do it 
	// using utimes.  *sigh*
	
	if (err == 0) {
		struct timeval times[2];
		
		TIMESPEC_TO_TIMEVAL(&times[0], &sb.st_atimespec);
		TIMESPEC_TO_TIMEVAL(&times[1], &sb.st_mtimespec);
		
		err = utimes(dest, times);
		err = MoreUNIXErrno(err);
	}

	return err;
}
