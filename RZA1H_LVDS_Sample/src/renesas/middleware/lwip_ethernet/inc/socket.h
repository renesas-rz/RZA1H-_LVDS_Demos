/*******************************************************************************
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
 * and to discontinue the availability of this software. By using this
 * software, you agree to the additional terms and conditions found by
 * accessing the following link:
 * http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
 *****************************************************************************/
/******************************************************************************
 * @headerfile     socket.h
 * @brief          Wrapper for the lwIP BSD socket interface
 * @version        1.00
 * @date           27.06.2018
 * H/W Platform    RZA1H
 *****************************************************************************/
 /*****************************************************************************
 * History      : DD.MM.YYYY Ver. Description
 *              : 04.02.2010 1.00 First Release
 *****************************************************************************/
/* Multiple inclusion prevention macro */
#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <stdio.h>
#include "r_devlink_wrapper.h"
#include "lwipopts.h"

/**************************************************************************//**
 * @ingroup  R_SW_PKG_93_POSIX_MIDDLEWARE
 * @defgroup R_SW_PKG_93_BSD_SOCKET lwIP BSD Socket
 * @brief Wrapper for the lwIP BSD socket interface
 * 
 * @anchor R_SW_PKG_93_SOCKET_SUMMARY
 * @par Summary
 *
 *   Wrapper for the lwIP BSD socket interface for the integration
 *   of functions which conflict with the POSIX style C run time
 *   library functions. NOTE: As of lwIP V1.3.2 the socket API is
 *   unfinished and the shutdown function has not been implemented.
 *   BEWARE: The lwIP BSD socket API is NOT thread safe. A crude
 *   modification has been made to lwIP to protect sockets from
 *   simultaneous access from multiple tasks. For best results
 *   only use one task / thread to access a socket & treat the API
 *   with kitten gloves. Remember, lwIP is worth what you paid for
 *   it.
 * 
 * @anchor R_SW_PKG_93_SOCKET_INSTANCES
 * @par Known Implementations:
 * This driver is used in the RZA1H Software Package.
 * @see RENESAS_APPLICATION_SOFTWARE_PACKAGE
 *
 * @see RENESAS_OS_ABSTRACTION  Renesas OS Abstraction interface
 * @{
 *****************************************************************************/
/******************************************************************************
Typedefs
******************************************************************************/
/* Similar to Microsoft Windows Extended data types */
typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN;
typedef struct sockaddr SOCKADDR, *PSOCKADDR;
typedef struct linger LINGER, *PLINGER;
typedef struct in_addr IN_ADDR, *PIN_ADDR;
typedef struct fd_set FD_SET, *PFD_SET;
typedef struct hostent HOSTENT, *PHOSTENT;
typedef struct servent SERVENT, *PSERVENT;

/******************************************************************************
Function Macros
******************************************************************************/

/* The File descriptor macros must be modified to turn the file descriptors
   into lwIP sockets for the public API */
#undef  FD_SETSIZE
/* Make FD_SETSIZE match NUM_SOCKETS in socket.c */
#define FD_SETSIZE    MEMP_NUM_NETCONN
#define FD_SET(n, p)  __FD_SET(n,p)
#define FD_CLR(n, p)  __FD_CLR(n,p)
#define FD_ISSET(n,p) __FD_ISSET(n,p)
#define FD_ZERO(p)    memset((void*)(p),0,sizeof(*(p)))

/*****************************************************************************
Typedefs
******************************************************************************/

typedef struct fd_set {
      unsigned char fd_bits [(FD_SETSIZE+7)/8];
    } fd_set;

/* Include the lwIP socket header now the FD macros have been defined */
#include "lwip\sockets.h"

/******************************************************************************
Public Functions
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief         Function to accept a TCP/IP connection and create a file 
 *                descriptor to represent the connection. 
 * 
 * @param[in]     s:       Valid socket identifer 
 * @param[out]    addr:    Pointer to sockaddr representing remote address
 * @param[out]    addrlen: Pointer to length of the remote address
 * 
 * @retval        n: on success, where n represents the file descriptor assigned 
 *                   to the connection
 * @retval       -1: on error
*/
extern  int accept(int s, struct sockaddr *addr, socklen_t *addrlen);

/**
 * @brief         Function to binds a socket s to a local port and/or IP address
 *                specified in the associated sockaddr structure.
 * 
 * @param[in]     s:       Valid socket identifier to perform bind operation
 * @param[in]     name:    Pointer to sockaddr object containing bind address 
 * @param[in]     namelen: socketlen_t specifying size of associated name
 * 
 * @retval        n: on success, where n represents the file descriptor assigned 
 *                   to the connection
 * @retval       -1: on error
*/
extern  int bind(int s, const struct sockaddr *name, socklen_t namelen);

/**
 * @brief         Function to close a specified socket connection and release 
 *                associated resources.
 *  
 * @param[in]     s:   Valid open socket identifier to perform shutdown on
 * @param[in]     how: not used
 * 
 * @retval        n: on success, where n represents the file descriptor assigned 
 *                   to the connection
 * @retval       -1: on error
*/
extern  int shutdown(int s, int how);

/**
 * @brief   Function to return address of the peer connected to socket s.
 * 
 * @param[in]     s:       Valid socket identifier to perform search 
 * @param[in]     name:    Pointer to sockaddr object representing peername
 * @param[in]     namelen: socketlen_t specifying size of associated name.
 * 
 * @retval        n: on success, where n represents the file descriptor assigned 
 *                   to the connection
 * @retval       -1: on error
*/
extern  int getpeername (int s, struct sockaddr *name, socklen_t *namelen);

/**
 * @brief        Function to return address for which the socket s is bound.
 * 
 * @param[in]    s:       Valid socket identifier to perform search 
 * @param[in]    name:    Pointer to sockaddr object representing socketname
 * @param[in]    namelen: socketlen_t specifying size of associated name
 * 
 * @retval        n: on success, where n represents the file descriptor assigned 
 *                   to the connection
 * @retval       -1: on error
*/

extern  int getsockname (int s, struct sockaddr *name, socklen_t *namelen);

/**
 * @brief         Function to read options for the socket referred to by the 
 *                socket s.   
 * 
 * @param[in]     s:       Valid socket identifier (refernece socket)
 * @param[in]     level:   Specified level at which the option resides
 * @param[in]     optname: Option to be modified
 * @param[in]     optval:  Data to be specified to optname
 * @param[in]     optlen:  Data size to be specified to optname in optval
 * 
 * @retval        n: on success, where n represents the file descriptor assigned 
 *                   to the connection
 * @retval       -1: on error
*/
extern  int getsockopt (int s, int level, int optname, void *optval,
    socklen_t *optlen);

/**
 * @brief         Function to to monitor multiple sockets (file descriptors), 
 *                waiting until one or more of the sockets become "ready" for 
 *                some class of I/O operation.
 * 
 * @param[in]        maxfdp1:   Maximum file descriptor across all the sets, +1
 * @param[in/out]    readset:   Input fd_set type, set of file descriptors to 
 *                              be checked for being ready to write.
 *                              Output indicates which file descriptors are 
 *                              ready to write.
 * @param[in/out]    writeset:  Input fd_set type, set of file descriptors to 
 *                              be checked for being ready to write.
 *                              Output indicates which file descriptors are 
 *                              ready to write.
 * @param[in/out]    exceptset: Input fd_set type,the file descriptors to be 
 *                              checked for error conditions pending.
 *                              Output indicates which file descriptors have 
 *                              error conditions pending.
 * @param[in]        timeout:   struct timeval that specifies a maximum interval 
 *                              to wait for the selection to complete. 
 *                              If the timeout argument points to an object of 
 *                              type struct timeval whose members are 0 do not 
 *                              block. 
 *                              If the timeout argument is NULL, block until an 
 *                              event causes one of the masks to be returned with 
 *                              a valid (non-zero) value.
 * 
 * @retval           0: on success, readset, writeset, exceptset may be be updated
 * @retval          -1: on error
*/
extern  int select(int maxfdp1, fd_set *readset, fd_set *writeset,
    fd_set *exceptset, struct timeval *timeout);

/** 
 * @brief         Function to manipulate options for the socket referred to by 
 *                the file descriptor s  
 * 
 * @param[in]     s:       Valid socket identifier (refernece socket)
 * @param[in]     level:   Specified level at which the option resides
 * @param[in]     optname: Option to be modified
 * @param[in]     optval:  Input Data to be specified to optname
 * @param[in]     optlen:  Data size to be specified to optname in optval
 * 
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int setsockopt (int s, int level, int optname, const void *optval,
    socklen_t optlen);

/**
 * @brief         Function to connect a socket, identified by its file descriptor
 *                to a remote host specified by that host's address in the 
 *                argument list
 * 
 * @param[in]     s:       Valid socket identifier
 * @param[out]    name:    Pointer to sockaddr representing remote address
 * @param[out]    namelen: Pointer to length of the remote address
 * 
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int connect(int s, const struct sockaddr *name, socklen_t namelen);

/**
 * @brief         Function to allow a bound TCP socket to enter a listening state
 * @param[in]     s:       Specifies a valid socket identifier 
 * @param[in]     backlog: Representation of the number of pending 
 *                         connections (currently limited to 255) that can 
 *                         be queued at any one time
 * @retval        n: on success with connection de-queued
 * @retval       -1: on error                        
*/
extern  int listen(int s, int backlog);

/**
 * @brief         Function to receive data from a socket s
 * 
 * @param[in]     s: Valid connected socket identifier to read from 
 * @param[out]    mem: Pointer to data to read from socket 
 * @param[in]     len: Size of mem buffer available
 * @param[in]     flags: Control flags for read operation NOTE options supported 
 *                for the flags parameter are as follows:<BR>
 *                MSG_PEEK - Peeks at an incoming message<BR>
 *                MSG_DONTWAIT - Nonblocking i/o for this operation only
 * 
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int recv(int s, void *mem, size_t len, int flags);

/**
 * @brief         Function to receive data from a connected socket s
 * @param[in]     s:     Valid socket identifier to read from 
 * @param[out]    mem:   Pointer to data to read from socket 
 * @param[in]     len:   Size of mem buffer available
 * @param[in]     flags: Control flags for read operation NOTE options supported 
 *                for the flags parameter are as follows:<BR>
 *                MSG_PEEK - Peeks at an incoming message<BR>
 *                MSG_DONTWAIT - Nonblocking i/o for this operation only<BR>
 * @param[in]     from:  Pointer to sockaddr representing remote address of 
 *                       data source 
 * @param[in]     fromlen: Pointer to length of the remote address
 *
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int recvfrom(int s, void *mem, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen);

/**
 * @brief         Function to send data to a connected socket s.
 * 
 * @param[in]     s:       Valid connected socket identifier to send to 
 * @param[in]     dataptr: Pointer to data to sent to socket 
 * @param[in]     size:    Size of mem buffer available
 * @param[in]     flags: Control flags for read operation NOTE options supported 
 *                for the flags parameter are as follows:<BR>
 *                MSG_PEEK - Peeks at an incoming message<BR>
 *                MSG_DONTWAIT - Nonblocking i/o for this operation only<BR>
 *                MSG_MORE - Sender will send more <BR>
 * 
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int send(int s, const void *dataptr, size_t size, int flags);

/**
 * @brief         Function to send data to a socket s
 * @param[in]     s:       Valid socket identifier to send to  
 * @param[out]    dataptr: Pointer to data to be sent to socket 
 * @param[in]     size:    Size of mem buffer available
 * @param[in]     flags: Control flags for read operation NOTE options supported 
 *                for the flags parameter are as follows:<BR>
 *                MSG_PEEK - Peeks at an incoming message<BR>
 *                MSG_DONTWAIT - Nonblocking i/o for this operation only<BR>
 *                MSG_MORE - Sender will send more <BR>
 * @param[in]     to:      Pointer to sockaddr representing remote address of 
 *                         data destination 
 * @param[in]     tolen:   Pointer to length of the remote address
 * 
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);

/**
 * @brief         Function to create a new socket of specified type and allocate 
 *                system resources for it 
 * 
 * @param[in]     domain:   Specifies the protocol family of the created 
 *                          socket(only PF_INET (IPv4) is supported). 
 * @param[in]     type:     Specifies connection type     <BR>
 *                          SOCK_RAW: (RAW protocol atop network layer)<BR>
 *                          SOCK_DGRAM: (UDP protocol atop network layer)<BR>
 *                          SOCK_STREAM: (TCP protocol atop network layer)<BR>
 * @param[in]     protocol: Specifies protocol used (supported protocols)<BR>
 *                          IPPROTO_IP      <BR> 
 *                          IPPROTO_TCP     <BR>      
 *                          IPPROTO_UDP     <BR>
 *                          IPPROTO_UDPLITE <BR>
 * @retval        n: on success (where n represents the newly-assigned descriptor)
 * @retval       -1: on error
*/
extern  int socket(int domain, int type, int protocol);

/**
 * @brief         Function to accept a TCP/IP connection and return a file
 *                pointer instead of a file descriptor. This is specifically for
 *                the command console code which uses file pointers for all IO
 *                
 * @param[in]     argp: Associated parameter with cmd
 * @param[in]     s:    Valid socket identifier 
 * @param[in]     cmd:  Command to operate on socket s NOTE options supported 
 *                      for the cmd parameter are as follows:<BR>
 *                      FIONREAD - read mode <BR>
 *                      FIONBIO - I/O (write) mode <BR>
 *                
 * @retval        0: on success
 * @retval       -1: on error
*/
extern  int ioctl(int s, long cmd, void *argp);

/* The prototypes of the FD_X macro replacements */
extern  void __FD_SET(int s, struct fd_set *p);
extern  void __FD_CLR(int s, struct fd_set *p);
extern  int __FD_ISSET(int s, struct fd_set *p);

/** @brief  Special version of the accept fuction that returns a file pointer
            instead of a file descriptor. This is specifically for the command
            console code which uses file pointers for all IO 
*/
extern  FILE *faccept(int s, struct sockaddr *addr, socklen_t *addrlen);

#ifdef __cplusplus
}
#endif

#endif /* SOCKET_H_INCLUDED */
/**************************************************************************//**
 * @} (end addtogroup)
 *****************************************************************************/
/******************************************************************************
End  Of File
******************************************************************************/
