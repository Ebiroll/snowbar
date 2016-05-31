#ifndef ICSOCKET_HPP
#define ICSOCKET_HPP
#include <errno.h>
#include <string.h>
#include "ipv4addr.hpp"
#include <stdexcept>
#include <iostream>

#ifdef WIN32
#define ssize_t signed int
#define timeout_t time_t
#endif

#ifndef	MSG_NOSIGNAL
#define	MSG_NOSIGNAL	0
#endif


namespace ic
{

enum Pending {
        pendingInput,
        pendingOutput,
        pendingError
};
typedef enum Pending Pending;

enum Error {
        errSuccess = 0,
        errCreateFailed,
        errCopyFailed,
        errInput,
        errInputInterrupt,
        errResourceFailure,
        errOutput,
        errOutputInterrupt,
        errNotConnected,
        errConnectRefused,
        errConnectRejected,
        errConnectTimeout,
        errConnectFailed,
        errConnectInvalid,
        errConnectBusy,
        errConnectNoRoute,
        errBindingFailed,
        errBroadcastDenied,
        errRoutingDenied,
        errKeepaliveDenied,
        errServiceDenied,
        errServiceUnavailable,
        errMulticastDisabled,
        errTimeout,
        errNoDelay,
        errExtended,
        errLookupFail,
        errSearchErr,
        errInvalidValue
};


struct SockException : public std::exception
{
    std::string text;


    SockException(std::string mess,Error err,long systemsrr)
    {
        //char tmp[20];
        //sprintf(buff)
        mError=err;

        if (systemsrr>0)
        {
#ifndef WIN32
            text=text+std::string(" errno=") + std::string(strerror(systemsrr));
#endif
        }
        else
        {
            text+=err;
        }
    }

    SockException(std::string error) throw()
    {
        text=error;
    }
    virtual ~SockException()  throw() {};

    char const* what() const throw() { return text.c_str(); }

    Error mError;
};


class TCPSocket
{
public:
    TCPSocket();


    /**
     * A socket object may be created from a file descriptor when that
     * descriptor was created either through a socket() or accept()
     * call.  This constructor is mostly for internal use.
     *
     * @param fd file descriptor of an already existing socket.
     */
    TCPSocket(SOCKET fd);

    void setSocket();


    mutable struct {
            bool thrown: 1;
            bool broadcast: 1;
            bool route: 1;
            bool keepalive: 1;
            bool loopback: 1;
            bool multicast: 1;
            bool completion: 1;
            bool linger: 1;
            unsigned ttl: 8;
    } flags;


    enum State {
            INITIAL,
            AVAILABLE,
            BOUND,
            CONNECTED,
            CONNECTING,
            STREAM
    };
    typedef enum State State;


    void setSegmentSize(unsigned mss);

    inline int getSegmentSize(void)
            {return segsize;};


    /**
     * Used to specify blocking mode for the socket.  A socket
     * can be made non-blocking by setting setCompletion(false)
     * or set to block on all access with setCompletion(true).
     * I do not believe this form of non-blocking socket I/O is supported
     * in winsock, though it provides an alternate asynchronous set of
     * socket services.
     *
     * @param immediate mode specify socket I/O call blocking mode.
     */
    void setCompletion(bool immediate);



    /**
     * Set the protocol stack network kernel send buffer size
     * associated with the socket.
     *
     * @return errSuccess on success, or error.
     * @param size of buffer in bytes.
     */
    Error sendBuffer(unsigned size);

    /**
     * Set the protocol stack network kernel receive buffer size
     * associated with the socket.
     *
     * @return errSuccess on success, or error.
     * @param size of buffer in bytes.
     */
    Error receiveBuffer(unsigned size);

    /**
     * Set the total protocol stack network kernel buffer size
     * for both send and receive together.
     *
     * @return errSuccess on success
     * @param size of buffer.
     */
    Error bufferSize(unsigned size);

    /**
     * Set the send limit.
     */
    Error sendLimit(int limit = 2048);

    /**
     * Set thr receive limit.
     */
    Error receiveLimit(int limit = 1);


    /**
     * Get the host address and port of the socket this socket
     * is connected to.  If the socket is currently not in a
     * connected state, then a host address of 0.0.0.0 is
     * returned.
     *
     * @param port ptr to port number of remote socket.
     * @return host address of remote socket.
     */
    IPV4Host getIPV4Peer(tpport_t *port = NULL) const;

    inline IPV4Host getPeer(tpport_t *port = NULL) const
            {return getIPV4Peer(port);}


    /**
     * A method to call in a derived TCPSocket class that is acting
     * as a server when a connection request is being accepted.  The
     * server can implement protocol specific rules to exclude the
     * remote socket from being accepted by returning false.  The
     * Peek method can also be used for this purpose.
     *
     * @return true if client should be accepted.
     * @param ia internet host address of the client.
     * @param port number of the client.
     */
    virtual bool onAccept(const IPV4Host &ia, tpport_t port);



    /**
     * Used as a common handler for connection failure processing.
     *
     * @return correct failure code to apply.
     */
    Error connectError(void);


    /**
     * This service is used to throw all socket errors which usually
     * occur during the socket constructor.
     *
     * @param error defined socket error id.
     * @param err string or message to pass.
     * @param systemError the system error# that caused the error
     */
    Error error(Error error, const char *err = NULL, long systemError = 0) const;



    /**
     * An unconnected socket may be created directly on the local
     * machine.  Sockets can occupy both the internet domain (AF_INET)
     * and UNIX socket domain (AF_UNIX) under unix.  The socket type
     * (SOCK_STREAM, SOCK_DGRAM) and protocol may also be specified.
     * If the socket cannot be created, an exception is thrown.
     *
     * @param domain socket domain to use.
     * @param type base type and protocol family of the socket.
     * @param protocol specific protocol to apply.
     */
     TCPSocket(int domain, int type, int protocol = 0);


    /**
     * A TCP "server" is created as a TCP socket that is bound
     * to a hardware address and port number on the local machine
     * and that has a backlog queue to listen for remote connection
     * requests.  If the server cannot be created, an exception is
     * thrown.
     *
     * @param bind local ip address or interface to use.
     * @param port number to bind socket under.
     * @param backlog size of connection request queue.
     * @param mss maximum segment size for accepted streams.
     */
     TCPSocket(const IPV4Address &bind, tpport_t port, unsigned backlog = 5, unsigned mss = 536);




    /**
     * Process a logical input line from a socket descriptor
     * directly.
     *
     * @param buf pointer to string.
     * @param len maximum length to read.
     * @param timeout for pending data in milliseconds.
     * @return number of bytes actually read.
     */
    ssize_t readLine(char *buf, size_t len, timeout_t timeout = 0);

    /**
     * Read in a block of len bytes with specific separator.  Can
     * be zero, or any other char.  If \\n or \\r, it's treated just
     * like a readLine().  Otherwise it looks for the separator.
     *
     * @param buf pointer to byte allocation.
     * @param len maximum length to read.
     * @param separator separator for a particular ASCII character
     * @param t timeout for pending data in milliseconds.
     * @return number of bytes actually read.
     */
    virtual ssize_t readData(void * buf,size_t len,char separator=0,timeout_t t=0);

    /**
     * Write a block of len bytes to socket.
     *
     * @param buf pointer to byte allocation.
     * @param len maximum length to write.
     * @param t timeout for pending data in milliseconds.
     * @return number of bytes actually read.
     */
    virtual ssize_t writeData(const void* buf,size_t len,timeout_t t=0);



    /**
     * Get the status of pending operations.  This can be used to
     * examine if input or output is waiting, or if an error has
     * occured on the descriptor.
     *
     * @return true if ready, false on timeout.
     * @param pend ready check to perform.
     * @param timeout in milliseconds, inf. if not specified.
     */
    bool isPending(Pending pending, timeout_t timeout);


    /**
     * Used as the default destructor for ending a socket.  This
     * will cleanly terminate the socket connection.  It is provided
     * for use in derived virtual destructors.
     */
    void endSocket(void);


    /**
     * Fetch out the socket.
     */
    inline SOCKET getSocket(void)
            {return so;};



    /**
     * Used to wait for pending connection requests.
     * @return true if data packets available.
     * @param timeout in milliseconds. TIMEOUT_INF if not specified.
     */
    inline bool isPendingConnection(timeout_t timeout = TIMEOUT_INF) /* not const -- jfc */
            {return TCPSocket::isPending(pendingInput, timeout);}



    /**
     * the actual socket descriptor, in Windows, unlike posix it
     * *cannot* be used as an file descriptor
     */
    SOCKET volatile so;
    State  volatile state;
    int segsize;



};

/**
 * TCP streams are used to represent TCP client connections to a server
 * by TCP protocol servers for accepting client connections.  The TCP
 * stream is a C++ "stream" class, and can accept streaming of data to
 * and from other C++ objects using the << and >> operators.
 *
 *  TCPStream itself can be formed either by connecting to a bound network
 *  address of a TCP server, or can be created when "accepting" a
 *  network connection from a TCP server.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short streamable TCP socket connection.
 */
class  TCPStream : protected std::streambuf, public TCPSocket, public std::iostream
{
private:
        int doallocate();

        void segmentBuffering(unsigned mss);

        friend TCPStream& crlf(TCPStream&);
        friend TCPStream& lfcr(TCPStream&);

protected:
        timeout_t timeout;
        size_t bufsize;
        //Family family;
        char *gbuf, *pbuf;

public:
        /**
         * The constructor required for building other classes or to
         * start an unconnected TCPStream for connect.
         */
        TCPStream(bool throwflag = true, timeout_t to = 0);

        /**
         * Disconnect the current session and prepare for a new one.
         */
        void disconnect(void);

        /**
         * Get protocol segment size.
         */
        int getSegmentSize(void);

protected:
        /**
         * Used to allocate the buffer space needed for iostream
         * operations.  This function is called by the constructor.
         *
         * @param size of stream buffers from constructor.
         */
        void allocate(size_t size);

        /**
         * Used to terminate the buffer space and cleanup the socket
         * connection.  This fucntion is called by the destructor.
         */
        void endStream(void);

        /**
         * This streambuf method is used to load the input buffer
         * through the established tcp socket connection.
         *
         * @return char from get buffer, EOF if not connected.
         */
        int underflow();

        /**
         * This streambuf method is used for doing unbuffered reads
         * through the establish tcp socket connection when in interactive mode.
         * Also this method will handle proper use of buffers if not in
         * interative mode.
         *
         * @return char from tcp socket connection, EOF if not connected.
         */
        int uflow();

        /**
         * This streambuf method is used to write the output
         * buffer through the established tcp connection.
         *
         * @param ch char to push through.
         * @return char pushed through.
         */
        int overflow(int ch);

        /**
         * Create a TCP stream by connecting to a TCP socket (on
         * a remote machine).
         *
         * @param host address of remote TCP server.
         * @param port number to connect.
         * @param mss maximum segment size of streaming buffers.
         */
        void connect(const IPV4Host &host, tpport_t port, unsigned mss = 536);

        /**
         * Connect a TCP stream to a named destination host and port
         * number, using getaddrinfo interface if available.
         *
         * @param name of host and service to connect
         * @param mss maximum segment size of stream buffer
         */
        void connect(const char *name, unsigned mss = 536);

        /**
         * Used in derived classes to refer to the current object via
         * it's iostream.  For example, to send a set of characters
         * in a derived method, one might use *tcp() << "test".
         *
         * @return stream pointer of this object.
         */
        std::iostream *tcp(void)
                {return ((std::iostream *)this);};

public:
        /**
         * Create a TCP stream by accepting a connection from a bound
         * TCP socket acting as a server.  This performs an "accept"
         * call.
         *
         * @param server socket listening
         * @param throwflag flag to throw errors.
         * @param timeout for all operations.
         */
        TCPStream(TCPSocket &server, bool throwflag = true, timeout_t timeout = 0);

        /**
         * Accept a connection from a TCP Server.
         *
         * @param server socket listening
         */
        void connect(TCPSocket &server);

        /**
         * Create a TCP stream by connecting to a TCP socket (on
         * a remote machine).
         *
         * @param host address of remote TCP server.
         * @param port number to connect.
         * @param mss maximum segment size of streaming buffers.
         * @param throwflag flag to throw errors.
         * @param timeout for all operations.
         */
        TCPStream(const IPV4Host &host, tpport_t port, unsigned mss = 536, bool throwflag = true, timeout_t timeout = 0);

        /**
         * Construct a named TCP Socket connected to a remote machine.
         *
         * @param name of remote service.
         * @param family of protocol.
         * @param mss maximum segment size of streaming buffers.
         * @param throwflag flag to throw errors.
         * @param timer for timeout for all operations.
         */
        TCPStream(const char *name, unsigned mss = 536, bool throwflag = false, timeout_t timer = 0);

        /**
         * Set the I/O operation timeout for socket I/O operations.
         *
         * @param timer to change timeout.
         */
        inline void setTimeout(timeout_t timer)
                {timeout = timer;};

        /**
         * A copy constructor creates a new stream buffer.
         *
         * @param source reference of stream to copy from.
         *
         */
        TCPStream(const TCPStream &source);

        /**
         * Flush and empty all buffers, and then remove the allocated
         * buffers.
         */
        virtual ~TCPStream();

        /**
         * Flushes the stream input and output buffers, writes
         * pending output.
         *
         * @return 0 on success.
         */
        int sync(void);


        /**
         * Get the status of pending stream data.  This can be used to
         * examine if input or output is waiting, or if an error or
         * disconnect has occured on the stream.  If a read buffer
         * contains data then input is ready and if write buffer
         * contains data it is first flushed and then checked.
         */
        bool isPending(Pending pend, timeout_t timeout = TIMEOUT_INF);

        /**
          * Examine contents of next waiting packet.
          *
          * @param buf pointer to packet buffer for contents.
          * @param len of packet buffer.
          * @return number of bytes examined.
          */
         inline ssize_t peek(void *buf, size_t len)
                 {return _IORET64 ::recv(so, (char *)buf, _IOLEN64 len, MSG_PEEK);};

        /**
         * Return the size of the current stream buffering used.
         *
         * @return size of stream buffers.
         */
        inline size_t getBufferSize(void) const
                {return bufsize;};


};


/**
 * @class SimpleTCPStream
 * @brief Simple TCP Stream, to be used with Common C++ Library
 *
 * This source is derived from a proposal made by Ville Vainio
 * (vvainio@tp.spt.fi).
 *
 * @author Mark S. Millard (msm@wizzer.com)
 * @date   2002-08-15
 * Copyright (C) 2002 Wizzer Works.
 **/
class SimpleTCPStream : public TCPSocket
{
private:

        IPV4Host getSender(tpport_t *port) const;

protected:
        /**
         * The constructor required for "SimpleTCPStream", a more C++ style
         * version of the SimpleTCPStream class.
         */
        SimpleTCPStream();

        /**
         * Used to terminate the buffer space and cleanup the socket
         * connection.  This fucntion is called by the destructor.
         */
        void endStream(void);

        /**
         * Create a TCP stream by connecting to a TCP socket (on
         * a remote machine).
         *
         * @param host address of remote TCP server.
         * @param port number to connect.
         * @param size of streaming input and output buffers.
         */
        void Connect(const IPV4Host &host, tpport_t port, size_t size);


public:
        /**
         * Create a TCP stream by accepting a connection from a bound
         * TCP socket acting as a server.  This performs an "accept"
         * call.
         *
         * @param server bound server tcp socket.
         * @param size of streaming input and output buffers.
         */
        SimpleTCPStream(TCPSocket &server, size_t size = 512);

        /**
         * Create a TCP stream by connecting to a TCP socket (on
         * a remote machine).
         *
         * @param host address of remote TCP server.
         * @param port number to connect.
         * @param size of streaming input and output buffers.
         */
        SimpleTCPStream(const IPV4Host &host, tpport_t port, size_t size = 512);

        /**
         * A copy constructor creates a new stream buffer.
         *
         * @param source A reference to the SimpleTCPStream to copy.
         */
        SimpleTCPStream(const SimpleTCPStream &source);

        /**
         * Flush and empty all buffers, and then remove the allocated
         * buffers.
         */
        virtual ~SimpleTCPStream();

        /**
         * @brief Get the status of pending stream data.
         *
         * This method can be used to examine if input or output is waiting,
         * or if an error or disconnect has occured on the stream.
         * If a read buffer contains data then input is ready. If write buffer
         * contains data, it is first flushed and then checked.
         *
         * @param pend Flag indicating means to pend.
         * @param timeout The length of time to wait.
         */
        bool isPending(Pending pend, timeout_t timeout = TIMEOUT_INF);

        void flush() {}

        /**
         * @brief Read bytes into a buffer.
         *
         * <long-description>
         *
         * @param bytes A pointer to buffer that will contain the bytes read.
         * @param length The number of bytes to read (exactly).
         * @param timeout Period to time out, in milleseconds.
         *
         * @return The number of bytes actually read, 0 on EOF.
         */
        ssize_t read(char *bytes, size_t length, timeout_t timeout = 0);


        ssize_t readAll(char *bytes, size_t length, timeout_t timeout = 0);


        /**
         * @brief Write bytes to buffer
         *
         * <long-description>
         *
         * @param bytes A pointer to a buffer containing the bytes to write.
         * @param length The number of bytes to write (exactly).
         * @param timeout Period to time out, in milleseconds.
         *
         * @return The number of bytes actually written.
         */
        ssize_t write(const char *bytes, size_t length, timeout_t timeout = 0);

        /**
         * @brief Peek at the incoming data.
         *
         * The data is copied into the buffer
         * but is not removed from the input queue. The function then returns
         * the number of bytes currently pending to receive.
         *
         * @param bytes A pointer to buffer that will contain the bytes read.
         * @param length The number of bytes to read (exactly).
         * @param timeout Period to time out, in milleseconds.
         *
         * @return The number of bytes pending on the input queue, 0 on EOF.
         */
        ssize_t peek(char *bytes, size_t length, timeout_t timeout = 0);

};


}

#endif // ICSOCKET_HPP
