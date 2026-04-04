/* NOTE: it is assumed that "util.c" and thus "win32_util.c" are #included before this file. As a result, #include <windows.h> is not required */

/* includes needed for socketing code */
#include <ws2tcpip.h>

/* 
 * Winsock Error codes list (for WSAGetLastError)
 *
 * https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2 
 */

SOCKET socket_connect(const char *hostname, const char *port)
{
	WSADATA wsa_data;
	struct addrinfo hints, *server_info, *address_info_pointer;
	SOCKET socket_out = INVALID_SOCKET;

	WSAStartup(MAKEWORD(2,2), &wsa_data);

	hints.ai_flags	   = 0;
	hints.ai_family    = AF_INET;
	hints.ai_socktype  = SOCK_STREAM;
	hints.ai_protocol  = 0;
	hints.ai_addrlen   = 0;
	hints.ai_canonname = 0;
	hints.ai_addr	   = 0;
	hints.ai_next	   = 0;

	if(getaddrinfo(hostname, port, &hints, &server_info) != 0)
	{
		log_error("Failed to connect to '%s' on port %s. Function 'getaddrinfo()' returned non-zero value. WSA Error Code: %d", 
			hostname, port, WSAGetLastError());
		WSACleanup();
		return(INVALID_SOCKET);
	}

	for(address_info_pointer = server_info; address_info_pointer != NULL; address_info_pointer = address_info_pointer->ai_next)
	{
		socket_out = socket(address_info_pointer->ai_family, address_info_pointer->ai_socktype, address_info_pointer->ai_protocol);
		if(socket_out == INVALID_SOCKET)
		{
			log_error("Failed to connect to '%s' on port %s. Function 'socket()' returned INVALID_SOCKET. WSA Error Code: %d", 
				hostname, port, WSAGetLastError());
			WSACleanup();
			return(INVALID_SOCKET);
		}

		if(connect(socket_out, address_info_pointer->ai_addr, address_info_pointer->ai_addrlen) == SOCKET_ERROR)
		{
			log_warn("Failed to connect to '%s' on port %s Connect returned SOCKET_ERROR. WSA Error Code: %d. Trying next address info...", 
				hostname, port, WSAGetLastError());
			closesocket(socket_out);
			socket_out = INVALID_SOCKET;
			continue;
		}

		/* NOTE: if we make it here, we have successfully connected OR ran out of address info's to try */
		break;
	}

	if(address_info_pointer == NULL)
	{
		log_error("Failed to connect to '%s' on port %s. Function 'connect()' returned SOCKET_ERROR for all address info's. WSA Error Code: %d", hostname, port, WSAGetLastError());
		return(INVALID_SOCKET);
	}

	freeaddrinfo(server_info);

	if(socket_out == INVALID_SOCKET)
	{
		log_error("Failed to connect to '%s' on port %s. INVALID_SOCKET. WSA Error Code: %d", hostname, port, WSAGetLastError());
		WSACleanup();
		return(INVALID_SOCKET);
	}

	log_trace("Successfully connected to '%s' on port %s", hostname, port);
	return(socket_out);
}

/* NOTE: socket_send and socket_recv DO NOT assume that the data they are getting are strings, 
 * callers of these functions must handle any string-specific requirements
 */
b32 socket_send(SOCKET socket_id, void *message, u64 size)
{
	if(send(socket_id, message, size, 0) == -1)
	{
		return(false);
	}
	return(true);
}

/* NOTE: socket_send and socket_recv DO NOT assume that the data they are getting are strings, 
 * callers of these functions must handle any string-specific requirements
 */
b32 socket_receive(SOCKET socket_id, void *message_buffer, u64 buffer_size)
{
	i32 bytes_count = recv(socket_id, message_buffer, buffer_size, 0);
	if(bytes_count == -1)
	{
		return(false);
	}
	return(true);
}

/* XXX: handle hangup or whatever. see linux implementation of socket_poll() (in server side code) + obviously, test this function*/
i32 socket_poll(SOCKET socket_id)
{
	/* non-blocking by setting timeout to 0 */
	TIMEVAL timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	fd_set read;
	FD_ZERO(&read);
	FD_SET(socket_id, &read);
	i32 result = select(0, &read, NULL, NULL, &timeout);
	if(result == SOCKET_ERROR)
	{
		return(-1);
	}
	if(result == 0)
	{
		return(false);
	}
	/* TODO: other cases before assuming 'true'? */
	return(true);
}
