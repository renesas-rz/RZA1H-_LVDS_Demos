/******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only 
* intended for use with Renesas products. No other uses are authorized. This 
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS 
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE 
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2011 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************
* File Name    : socket.c
* Version      : 1.0
* Device(s)    : Renesas
* Tool-Chain   : GNUARM-NONE-EABI v14.02
* OS           : None
* H/W Platform : RZA1
* Description  : Wrapper for the lwIP BSD socket interface for the integration
*                of functions which conflict with the POSIX style C run time
*                library functions. NOTE: As of lwIP V1.3.2 the socket API is
*                unfinished and the shutdown function has not been implemented.
*                BEWARE: The lwIP BSD socket API is NOT thread safe. A crude
*                modification has been made to lwIP to protect sockets from
*                simultaneous access from multiple tasks. 
*                 For best results only use single task.
******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : 01.08.2009 1.00 First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <fcntl.h>
#include "compiler_settings.h"
#include "socket.h"

/******************************************************************************
Function Macros
******************************************************************************/

/* File descriptor to lwIP socket macro */
#define F2S(fd)                     getSocket(fd)

/******************************************************************************
Function Prototypes
******************************************************************************/

static int getSocket(int iFileDescriptor);

/*****************************************************************************
Public Functions
******************************************************************************/

/*****************************************************************************
Function Name: accept 
Description:   Function to accept a TCP/IP connection and create a file 
               descriptor to represent the connection. 
Arguments:     IN  s - Valid socket identifer 
               OUT addr - Pointer to sockaddr representing remote address
               OUT addrlen - Pointer to length of the remote address
Return value: n on success, where n represents the file descriptor assigned 
              to the connection
              -1 on error
*****************************************************************************/

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int iSocket = lwip_accept(F2S(s), addr, addrlen);
    if (iSocket >= 0)
    {
        /* Try to open the socket wrapper */
        int iFileDescriptor = open(DEVICE_INDENTIFIER "lwip socket", O_RDWR, 0x1000 /* FNBIO */);
        if (iFileDescriptor >= 0)
        {
            /* Set the file descriptor */
            if (control(iFileDescriptor,
                        CTL_SET_LWIP_SOCKET_INDEX,
                        &iSocket) == 0)
            {
                return iFileDescriptor;
            }
            else
            {
                close(iFileDescriptor);
            }
        }
        lwip_close(iSocket);
    }
    return  -1;
}
/*****************************************************************************
End of function  accept
*****************************************************************************/

/*****************************************************************************
Function Name: bind 
Description:   Function to binds a socket s to a local port and/or IP address
               specified in the associated sockaddr structure.
Arguments:     IN  s - Valid socket identifier to perform bind operation
               IN name - Pointer to sockaddr object containing bind address 
               IN namelen - socketlen_t specifying size of associated name
Return value: n on success, -1 on error
*****************************************************************************/

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_bind(F2S(s), name, namelen);
}
/*****************************************************************************
End of function  bind
*****************************************************************************/

/*****************************************************************************
Function Name: shutdown 
Description:   Function to close a specified socket connection and release 
               associated resources. 
Arguments:     IN  s - Valid open socket identifier to perform shutdown on
               IN how - not used
Return value: n on success, -1 on error
*****************************************************************************/

int shutdown(int s, int how)
{
    return lwip_shutdown(F2S(s), how);
}
/*****************************************************************************
End of function  shutdown
*****************************************************************************/

/*****************************************************************************
Function Name: getpeername 
Description:   Function to return address of the peer connected to socket s.
Arguments:     IN  s - Valid socket identifier to perform search 
               IN name - Pointer to sockaddr object representing peername
               IN namelen - socketlen_t specifying size of associated name
Return value: n on success, -1 on error
*****************************************************************************/

int getpeername (int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getpeername (F2S(s), name, namelen);
}
/*****************************************************************************
End of function  getpeername
*****************************************************************************/

/*****************************************************************************
Function Name: getsockname 
Description:   Function to return address for which the socket s is bound.
Arguments:     IN  s - Valid socket identifier to perform search 
               IN name - Pointer to sockaddr object representing socketname
               IN namelen - socketlen_t specifying size of associated name
Return value: n on success, -1 on error
*****************************************************************************/

int getsockname (int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getsockname(F2S(s), name, namelen);
}
/*****************************************************************************
End of function  getsockname
*****************************************************************************/

/*****************************************************************************
Function Name: getsockopt 
Description:   Function to read options for the socket referred to by the 
               socket s.   
Arguments:     IN  s - Valid socket identifier (refernece socket)
               IN level - Specified level at which the option resides
               IN optname - Option to be modified
               IN optval - Data to be specified to optname
               IN optlen - Data size to be specified to optname in optval
Return value: 0 on success, -1 on error
*****************************************************************************/

int getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen)
{
    return lwip_getsockopt(F2S(s), level, optname, optval, optlen);
}
/*****************************************************************************
End of function getsockopt
*****************************************************************************/

/*****************************************************************************
Function Name: select 
Description:   Function to to monitor multiple sockets (file descriptors), 
               waiting until one or more of the sockets become "ready" for 
               some class of I/O operation.
Arguments:     IN  maxfdp1s - Maximum file descriptor across all the sets, +1
               IN/OUT readset - Input fd_set type, set of file descriptors to 
                                   be checked for being ready to write.
                                Output indicates which file descriptors are 
                                ready to write.
               IN/OUT writeset - Input fd_set type, set of file descriptors to 
                                    be checked for being ready to write.
                                 Output indicates which file descriptors are 
                                 ready to write.
               IN/OUT exceptset - Input fd_set type,the file descriptors to be 
                                     checked for error conditions pending.
                                  Output indicates which file descriptors have 
                                  error conditions pending.
               IN timeout - struct timeval that specifies a maximum interval 
                               to wait for the selection to complete. 
                            If the timeout argument points to an object of 
                            type struct timeval whose members are 0 do not 
                            block. 
                            If the timeout argument is NULL, block until an 
                            event causes one of the masks to be returned with 
                            a valid (non-zero) value.
Return value: 0 on success, readset, writeset, exceptset may be be updated
              -1 on error
*****************************************************************************/

int select(int maxfdp1, fd_set *readset, fd_set *writeset,
    fd_set *exceptset, struct timeval *timeout)
{
    return lwip_select(maxfdp1,
                       readset,
                       writeset,
                       exceptset,
                       timeout);
}
/*****************************************************************************
End of function select
*****************************************************************************/

/*****************************************************************************
Function Name: setsockopt 
Description:   Function to manipulate options for the socket referred to by 
               the file descriptor s  
Arguments:     IN  s - Valid socket identifier (refernece socket)
               IN level - Specified level at which the option resides
               IN optname - Option to be modified
               IN optval - Input Data to be specified to optname
               IN optlen - Data size to be specified to optname in optval
Return value: 0 on success, -1 on error
*****************************************************************************/

int setsockopt (int s, int level, int optname, const void *optval,
                                                             socklen_t optlen)
{
    return lwip_setsockopt(F2S(s), level, optname, optval, optlen);
}
/*****************************************************************************
End of function setsockopt
*****************************************************************************/

/*****************************************************************************
Function Name: connect 
Description:   Function to connect a socket, identified by its file descriptor
               to a remote host specified by that host's address in the 
               argument list
Arguments:     IN  s - Valid socket identifier
               OUT addr - Pointer to sockaddr representing remote address
               OUT addrlen - Pointer to length of the remote address
Return value: 0 on success, -1 on error
*****************************************************************************/

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_connect(F2S(s), name, namelen);
}
/*****************************************************************************
End of function connect
*****************************************************************************/

/*****************************************************************************
Function Name: listen 
Description:   Function to allow a bound TCP socket to enter a listening state
Arguments:     IN  s - Specifies a valid socket identifier 
               IN  backlog - Representation of the number of pending 
                             connections (currently limited to 255) that can 
                             be queued at any one time
Return value: n on success with connection de-queued, -1 on error
*****************************************************************************/

int listen(int s, int backlog)
{
    return lwip_listen(F2S(s), backlog);
}
/*****************************************************************************
End of function listen
*****************************************************************************/

/*****************************************************************************
Function Name: recv 
Description:   Function to receive data from a socket s
Arguments:     IN  s - Valid connected socket identifier to read from 
               OUT addr - Pointer to data to read from socket 
               IN len - Size of mem buffer available
               IN flags - Control flags for read operation
               NOTE options supported for the flags parameter are as follows:
               MSG_PEEK - Peeks at an incoming message
               MSG_DONTWAIT - Nonblocking i/o for this operation only
Return value: 0 on success, -1 on error
*****************************************************************************/

int recv(int s, void *mem, size_t len, int flags)
{
    return lwip_recv(F2S(s), mem, len, flags);
}
/*****************************************************************************
End of function recv
*****************************************************************************/

/*****************************************************************************
Function Name: recvfrom 
Description:   Function to receive data from a connected socket s
Arguments:     IN  s - Valid socket identifier to read from 
               OUT addr - Pointer to data to read from socket 
               IN len - Size of mem buffer available
               IN flags - Control flags for read operation
               IN from - Pointer to sockaddr representing remote address of 
                            data source 
               IN fromlen - Pointer to length of the remote address
               NOTE options supported for the flags parameter are as follows:
               MSG_PEEK - Peeks at an incoming message
               MSG_DONTWAIT - Nonblocking i/o for this operation only
Return value: 0 on success, -1 on error
*****************************************************************************/

int recvfrom(int s, void *mem, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen)
{
    return lwip_recvfrom(F2S(s), mem, len, flags, from, fromlen);
}
/*****************************************************************************
End of function recvfrom
*****************************************************************************/

/*****************************************************************************
Function Name: send 
Description:   Function to send data to a connected socket s
Arguments:     IN  s - Valid connected socket identifier to send to 
               IN addr - Pointer to data to sent to socket 
               IN len - Size of mem buffer available
               IN flags - Control flags for send operation
               NOTE options supported for the flags parameter are as follows:
               MSG_PEEK - Peeks at an incoming message
               MSG_DONTWAIT - Nonblocking i/o for this operation only
               MSG_MORE - Sender will send more
Return value: 0 on success, -1 on error
*****************************************************************************/

int send(int s, const void *dataptr, size_t size, int flags)
{
    return lwip_send(F2S(s), dataptr, size, flags);
}
/*****************************************************************************
End of function send
*****************************************************************************/

/*****************************************************************************
Function Name: sendto 
Description:   Function to send data to a socket s
Arguments:     IN  s - Valid socket identifier to send to  
               OUT addr - Pointer to data to be sent to socket 
               IN len - Size of mem buffer available
               IN flags - Control flags for send operation
                  IN to - Pointer to sockaddr representing remote address of 
                          data destination 
               IN tolen - Pointer to length of the remote address
               NOTE options supported for the flags parameter are as follows:
               MSG_PEEK - Peeks at an incoming message
               MSG_DONTWAIT - Nonblocking i/o for this operation only
               MSG_MORE - Sender will send more

Return value: 0 on success, -1 on error
*****************************************************************************/

int sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen)
{
    return lwip_sendto(F2S(s), dataptr, size, flags, to, tolen);
}
/*****************************************************************************
End of function sendto
*****************************************************************************/

/*****************************************************************************
Function Name: socket 
Description:   Function to create a new socket of specified type and allocate 
               system resources for it 
Arguments:     IN  domain - Specifies the protocol family of the created 
                            socket(only PF_INET (IPv4) is supported). 
               IN  type -     Specifies connection type     
                            SOCK_RAW: (RAW protocol atop network layer)
                            SOCK_DGRAM: (UDP protocol atop network layer)
                            SOCK_STREAM: (TCP protocol atop network layer)
               IN  protocol - Specifies protocol used (supported protocols)
                            IPPROTO_IP    
                            IPPROTO_TCP        
                            IPPROTO_UDP    
                            IPPROTO_UDPLITE
Return value: n on success (where n represents the newly-assigned descriptor)
              -1 on error
*****************************************************************************/

int socket(int domain, int type, int protocol)
{
    /* Call the lwIP socket funtion */
    int iSocket = lwip_socket(domain, type, protocol);
    if (iSocket >= 0)
    {
        /* Try to open the socket wrapper */
        int iFileDescriptor = open(DEVICE_INDENTIFIER "lwip socket", O_RDWR, 0x1000 /* FNBIO */);
        if (iFileDescriptor >= 0)
        {
            /* Set the file descriptor */
            if (control(iFileDescriptor,
                        CTL_SET_LWIP_SOCKET_INDEX,
                        &iSocket) == 0)
            {
                return iFileDescriptor;
            }
            else
            {
                close(iFileDescriptor);
            }
        }
        lwip_close(iSocket);
    }
    return -1;
}
/*****************************************************************************
End of function socket
*****************************************************************************/

/*****************************************************************************
Function Name: ioctl
Description:   Function to accept a TCP/IP connection and return a file
               pointer instead of a file descriptor. This is specifically for
               the command console code which uses file pointers for all IO
Arguments:     IN  s - Valid socket identifier 
               IN cmd - Command to operate on socket s
               IN argp - Associated parameter with cmd
               NOTE options supported for the cmd parameter are as follows:
               FIONREAD - read mode 
               FIONBIO - I/O (write) mode
               
Return value:  0 on success, -1 on error
*****************************************************************************/

int ioctl(int s, long cmd, void *argp)
{
    return lwip_ioctl(F2S(s), cmd, argp);
}
/*****************************************************************************
End of function ioctl
*****************************************************************************/

/*****************************************************************************
Function Name: faccept
Description:   Function to accept a TCP/IP connection and return a file
               pointer instead of a file descriptor. This is specifically for
               the command console code which uses file pointers for all IO
Arguments:     IN  s - The file descriptor (not lwIP socket)
               OUT addr - Pointer to the client's address
               OUT addrlen - Pointer to the length of the client's address
Return value:  Pointer to a file or NULL on error
*****************************************************************************/

FILE *faccept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    /* Call the accept function */
    int iSocket = lwip_accept(F2S(s), addr, addrlen);
    if (iSocket >= 0)
    {
        /* Try to open the socket file wrapper */
        FILE    *pFile = fopen(DEVICE_INDENTIFIER "lwip fsocket", "r+");
        if (pFile)
        {
            /* Set the file descriptor */
            if (control(R_DEVLINK_FilePtrDescriptor(pFile),
                        CTL_SET_LWIP_SOCKET_INDEX,
                        &iSocket) == 0)
            {
                return pFile;
            }
            fclose(pFile);
        }
        lwip_close(iSocket);
    }    
    return NULL;
}
/*****************************************************************************
End of function faccept
*****************************************************************************/

/*****************************************************************************
End of API
*****************************************************************************/

/* Function macro replacements which translate file descriptors into lwIP
   sockets before performing the macro function */
void __FD_SET(int s, struct fd_set *p)
{
    int n = F2S(s);
    ((p)->fd_bits[(n)/8] |=  (unsigned char)(1 << ((n) & 7)));
}

void __FD_CLR(int s, struct fd_set *p)
{
    int n = F2S(s);
    ((p)->fd_bits[(n)/8] &= (unsigned char)(~(1 << ((n) & 7))));
}

int __FD_ISSET(int s, struct fd_set *p)
{
    int n = F2S(s);
    return ((p)->fd_bits[(n)/8] & (1 << ((n) & 7)));
}


/******************************************************************************
Private Functions
******************************************************************************/

/*****************************************************************************
Function Name: getSocket
Description:   Function to get an lwIP socket index from a file descriptor
Arguments:     IN  iFileDescriptor - The file descriptor
Return value:  The lwIP socket 
*****************************************************************************/
static int getSocket(int iFileDescriptor)
{
    int iSocket;
    if (control(iFileDescriptor, CTL_GET_LWIP_SOCKET_INDEX, &iSocket) == 0)
    {
        return iSocket;
    }
    else
    {
        return -1;
    }
}
/*****************************************************************************
End of function getSocket
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
