#include "icsocket.hpp"
#ifndef WIN32
#include  <sys/time.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef	WIN32
#include <winsock2.h>
#include <io.h>
#define socket_errno	WSAGetLastError()
#endif

namespace ic {


#if defined(WIN32) && !defined(__MINGW32__)
static SOCKET dupSocket(SOCKET so,enum TCPSocket::State state)
{
        if (state == TCPSocket::STREAM)
                return dup((int)so);

        HANDLE pidHandle = GetCurrentProcess();
        HANDLE dupHandle;
        if(DuplicateHandle(pidHandle, reinterpret_cast<HANDLE>(so), pidHandle, &dupHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
                return reinterpret_cast<SOCKET>(dupHandle);
        return INVALID_SOCKET;
}
# define DUP_SOCK(s,state) dupSocket(s,state)
#else
# define DUP_SOCK(s,state) dup(s)
#endif


TCPSocket::TCPSocket()
{
    setSocket();
    so              = INVALID_SOCKET;
}

TCPSocket::TCPSocket(SOCKET fd)
{
    setSocket();
    so=fd;
    state = AVAILABLE;
}

void TCPSocket::setSegmentSize(unsigned mss)
{
#ifdef	TCP_MAXSEG
	if(mss > 1)
		setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif
        segsize = mss;
}


Error TCPSocket::connectError(void)
{
        const char* str = "Could not connect to remote host";
        switch(errno) {
#ifdef	EHOSTUNREACH
        case EHOSTUNREACH:
                return error(errConnectNoRoute,str,socket_errno);
#endif
#ifdef	ENETUNREACH
        case ENETUNREACH:
                return error(errConnectNoRoute,str,socket_errno);
#endif
        case EINPROGRESS:
                return error(errConnectBusy,str,socket_errno);
#ifdef	EADDRNOTAVAIL
        case EADDRNOTAVAIL:
                return error(errConnectInvalid,str,socket_errno);
#endif
        case ECONNREFUSED:
                return error(errConnectRefused,str,socket_errno);
        case ETIMEDOUT:
                return error(errConnectTimeout,str,socket_errno);
        default:
                return error(errConnectFailed,str,socket_errno);
        }
}


void TCPSocket::setSocket(void)
{
        flags.thrown    = false;
        flags.broadcast = false;
        flags.route     = true;
        flags.keepalive = false;
        flags.loopback  = true;
        flags.multicast = false;
        flags.linger	= false;
        flags.ttl	= 1;
        //errid           = errSuccess;
        //errstr          = NULL;
        //syserr          = 0;
        state           = INITIAL;
        so              = INVALID_SOCKET;
}

Error TCPSocket::error(Error err, const char *errs, long systemError) const
{
        //errid  = err;
        //errstr = errs;
        //syserr = systemError;
        if(!err)
                return err;

		if (errs)
		{
			std::cerr << errs;
			std::cerr << strerror(systemError); 
		}

        if(flags.thrown)
                return err;

        // prevents recursive throws

        flags.thrown = true;
        {
            if(!errs)
                errs = "";
            throw SockException(std::string(errs), err, systemError);
        }
        return err;
}

Error TCPSocket::sendBuffer(unsigned bufsize)
{
        if(setsockopt(so, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)))
                return errInvalidValue;
        return errSuccess;
}

Error TCPSocket::bufferSize(unsigned bufsize)
{
        Error err = receiveBuffer(bufsize);
        if(err == errSuccess)
                err = sendBuffer(bufsize);

        return err;
}

Error TCPSocket::receiveBuffer(unsigned bufsize)
{
        if(setsockopt(so, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)))
                return errInvalidValue;
        return errSuccess;
}


Error TCPSocket::sendLimit(int limit)
{
        if(setsockopt(so, SOL_SOCKET, SO_SNDLOWAT, (char *)&limit, sizeof(limit)))
                return errInvalidValue;

        return errSuccess;
}


ssize_t TCPSocket::readData(void *Target, size_t Size, char Separator, timeout_t timeout)
{
  if ((Separator == 0x0D) || (Separator == 0x0A))
        return (readLine ((char *) Target, Size, timeout));

  if (Size < 1)
        return (0);

  ssize_t nstat;

  if (Separator == 0)           // Flat-out read for a number of bytes.
        {
          if (timeout)
                if (!isPending (pendingInput, timeout)) {
                        error(errTimeout);
                        return (-1);
                  }
          nstat =::recv (so, (char *)Target, _IOLEN64 Size, 0);

          if (nstat < 0) {
                  error (errInput);
                  return (-1);
                }
          return (nstat);
        }
  /////////////////////////////////////////////////////////////
  // Otherwise, we have a special char separator to use
  /////////////////////////////////////////////////////////////
  bool found = false;
  size_t nleft = Size;
  int c;
  char *str = (char *) Target;

  memset (str, 0, Size);

  while (nleft && !found) {
          if (timeout)
                if (!isPending (pendingInput, timeout)) {
                        error(errTimeout);
                        return (-1);
                  }

          nstat =::recv (so, str, _IOLEN64 nleft, MSG_PEEK);
          if (nstat <= 0) {
                  error (errInput);
                  return (-1);
                }

          for (c = 0; (c < nstat) && !found; ++c)
                if (str[c] == Separator)
                  found = true;

          memset (str, 0, nleft);
          nstat =::recv (so, str, c, 0);
          if (nstat < 0)
                break;

          str += nstat;
          nleft -= nstat;
        }
  return (ssize_t)(Size - (ssize_t) nleft);
}

IPV4Host TCPSocket::getIPV4Peer(tpport_t *port) const
{
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        if(getpeername(so, (struct sockaddr *)&addr, &len)) {
#ifndef WIN32
                if(errno == ENOTCONN)
                        error(errNotConnected,"Could not get peer address",socket_errno);
                else
#endif
                        error(errResourceFailure,"Could not get peer address",socket_errno);
                if(port)
                        *port = 0;
                memset(&addr.sin_addr, 0, sizeof(addr.sin_addr));
        }
        else {
                if(port)
                        *port = ntohs(addr.sin_port);
        }
        return IPV4Host(addr.sin_addr);
}


ssize_t TCPSocket::writeData(const void *Source, size_t Size, timeout_t timeout)
{
  if (Size < 1)
        return (0);

  ssize_t nstat;
  const char *Slide = (const char *) Source;

  while (true) {
          if (timeout)
                if (!isPending (pendingOutput, timeout)) {
                        error(errOutput);
                        return (-1);
                  }

          nstat =::send (so, Slide, _IOLEN64 Size, MSG_NOSIGNAL);

          if (nstat <= 0) {
                  error(errOutput);
                  return (-1);
                }
          Size -= nstat;
          Slide += nstat;


          if (Size <= 0)
                break;
        }
  return (nstat);
}



bool TCPSocket::isPending(Pending pending, timeout_t timeout)
{
        int status;
        struct timeval tv;
        struct timeval *tvp = &tv;
        fd_set grp;

        if(timeout == TIMEOUT_INF)
                tvp = NULL;
        else {
                tv.tv_usec = (timeout % 1000) * 1000;
                tv.tv_sec = timeout / 1000;
        }

        FD_ZERO(&grp);
        SOCKET sosave = so;
        if(so == INVALID_SOCKET)
                return true;
        FD_SET(sosave, &grp);

        switch(pending) {
        case pendingInput:
                status = select((int)so + 1, &grp, NULL, NULL, tvp);
                break;
        case pendingOutput:
                status = select((int)so + 1, NULL, &grp, NULL, tvp);
                break;
        case pendingError:
                status = select((int)so + 1, NULL, NULL, &grp, tvp);
                break;
        }
        if(status < 1)
                return false;
        if(FD_ISSET(so, &grp))
                return true;
        return false;
}


TCPSocket::TCPSocket(int domain, int type, int protocol)
{
        setSocket();
        so = socket(domain, type, protocol);
        if(so == INVALID_SOCKET) {
                error(errCreateFailed,"Could not create socket",socket_errno);
                return;
        }

        //state = AVAILABLE;
}


// Subclasses can determine if we should accept this client
bool TCPSocket::onAccept(const IPV4Host &ia, tpport_t port)
{
    return true;
}


void TCPSocket::setCompletion(bool immediate)
{
        flags.completion = immediate;
#ifdef WIN32
        unsigned long flag;
        // note that this will not work on some versions of Windows for Workgroups. Tough. -- jfc
        switch( immediate ) {
        case false:
                // this will not work if you are using WSAAsyncSelect or WSAEventSelect.
                // -- perhaps I should throw an exception
                flag = 1;
//		ioctlsocket( so, FIONBIO, (unsigned long *) 1);
                break;
        case true:
                flag = 0;
//		ioctlsocket( so, FIONBIO, (unsigned long *) 0);
                break;
        }
        ioctlsocket(so, FIONBIO, &flag);
#else
        int fflags = fcntl(so, F_GETFL);

        switch( immediate ) {
        case false:
                fflags |= O_NONBLOCK;
                fcntl(so, F_SETFL, fflags);
                break;
        case true:
                fflags &=~ O_NONBLOCK;
                fcntl(so, F_SETFL, fflags);
                break;
        }
#endif
}


void TCPSocket::endSocket(void)
{
        if(TCPSocket::state == STREAM) {
                state = INITIAL;
#ifdef	WIN32
                if(so != (UINT)-1) {
                        SOCKET sosave = so;
                        so = INVALID_SOCKET;
                        closesocket((int)sosave);
                }
#else
                if(so > -1) {
                        SOCKET sosave = so;
                        so = INVALID_SOCKET;
                        close(sosave);
                }
#endif
                return;
        }

        state = INITIAL;
        if(so == INVALID_SOCKET)
                return;

#ifdef	SO_LINGER
        struct linger linger;

        if(flags.linger) {
                linger.l_onoff = 1;
                linger.l_linger = 60;
        }
        else
                linger.l_onoff = linger.l_linger = 0;
        setsockopt(so, SOL_SOCKET, SO_LINGER, (char *)&linger,
                (socklen_t)sizeof(linger));
#endif
//	shutdown(so, 2);
#ifdef WIN32
        closesocket(so);
#else
        close(so);
#endif
        so = INVALID_SOCKET;
}



TCPSocket::TCPSocket(const IPV4Address &ia, tpport_t port, unsigned backlog , unsigned mss )
{
    setSocket();
    so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(so == INVALID_SOCKET) {
            error(errCreateFailed,"Could not create socket",socket_errno);
            return;
    }


    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = getaddress(ia);
    addr.sin_port = htons(port);

#if defined(SO_REUSEADDR)
    int opt = 1;
    setsockopt(so, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, (socklen_t)sizeof(opt));
#endif
    if(bind(so, (struct sockaddr *)&addr, sizeof(addr))) {
            endSocket();
            error(errBindingFailed,"Could not bind socket",socket_errno);
            return;
    }

    setSegmentSize(mss);

    if(listen(so, backlog)) {
            endSocket();
            error(errBindingFailed,"Could not listen on socket",socket_errno);
            return;
    }
    state = BOUND;
}


ssize_t TCPSocket::readLine(char *str, size_t request, timeout_t timeout)
{
        bool crlf = false;
        bool nl = false;
        size_t nleft = request - 1; // leave also space for terminator
        int nstat,c;

        if(request < 1)
                return 0;

        str[0] = 0;

        while(nleft && !nl) {
                if(timeout) {
                        if(!isPending(pendingInput, timeout)) {
                                error(errTimeout,"Read timeout", 0);
                                return -1;
                        }
                }
                nstat = ::recv(so, str, _IOLEN64 nleft, MSG_PEEK);
                if(nstat <= 0) {
                        error(errInput,"Could not read from socket", socket_errno);
                        return -1;
                }

                // FIXME: if unique char in buffer is '\r' return "\r"
                //        if buffer end in \r try to read another char?
                //        and if timeout ??
                //        remember last \r

                for(c=0; c < nstat; ++c) {
                        if(str[c] == '\n') {
                                if (c > 0 && str[c-1] == '\r')
                                        crlf = true;
                                ++c;
                                nl = true;
                                break;
                        }
                }
                nstat = ::recv(so, str, _IOLEN64 c, 0);
                // TODO: correct ???
                if(nstat < 0)
                        break;

                // adjust ending \r\n in \n
                if(crlf) {
                        --nstat;
                        str[nstat - 1] = '\n';
                }

                str += nstat;
                nleft -= nstat;
        }
        *str = 0;
        return (ssize_t)(request - nleft - 1);
}



TCPStream::TCPStream(TCPSocket &server, bool throwflag, timeout_t to) :
        std::streambuf(), TCPSocket(accept(server.getSocket(), NULL, NULL)),
        std::iostream((std::streambuf *)this)
        ,bufsize(0)
        ,gbuf(NULL)
        ,pbuf(NULL)
{
        tpport_t port;

#ifdef	HAVE_OLD_IOSTREAM
//        init((streambuf *)this);
#endif

        timeout = to;
        //setError(throwflag);
        IPV4Host host = getPeer(&port);
        if(!server.onAccept(host, port)) {
                endSocket();
                error(errConnectRejected);
                clear(std::ios::failbit | rdstate());
                return;
        }

        segmentBuffering(server.getSegmentSize());
        TCPSocket::state = CONNECTED;
}


TCPStream::TCPStream(const IPV4Host &host, tpport_t port, unsigned size, bool throwflag, timeout_t to) :
        std::streambuf(), TCPSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP),
        std::iostream((std::streambuf *)this),
        bufsize(0),gbuf(NULL),pbuf(NULL) {

        /*setSocket();
        so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(so == INVALID_SOCKET) {
                error(errCreateFailed,"Could not create socket",socket_errno);
                return;
        }
        */


#ifdef	HAVE_OLD_IOSTREAM
//        init((streambuf *)this);
#endif
        // AF_INET family = IPV4;
        timeout = to;
        //setError(throwflag);
        connect(host, port, size);
}


TCPStream::~TCPStream()
{
                try { endStream(); }
                catch( ... ) { if ( ! std::uncaught_exception()) throw;};
}

#ifndef	WIN32
void TCPStream::connect(const char *target, unsigned mss)
{
        char namebuf[128];
        char *cp;
        struct addrinfo hint, *list = NULL, *next, *first;
        bool connected = false;

        snprintf(namebuf, sizeof(namebuf), "%s", target);
        cp = strrchr(namebuf, '/');
        if(!cp)
                cp = strrchr(namebuf, ':');

        if(!cp) {
                endStream();
                connectError();
                return;
        }

        *(cp++) = 0;

        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        if(getaddrinfo(namebuf, cp, &hint, &list) || !list) {
                endStream();
                connectError();
                return;
        }

        first = list;

#ifdef	TCP_MAXSEG
        if(mss)
                setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif

        while(list) {
                if(!::connect(so, list->ai_addr, (socklen_t)list->ai_addrlen)) {
                        connected = true;
                        break;
                }
                next = list->ai_next;
                list = next;
        }

        freeaddrinfo(first);

        if(!connected) {
                endStream();
                connectError();
                return;
        }

        segmentBuffering(mss);
        TCPSocket::state = CONNECTED;
}
#else
void TCPStream::connect(const char *target, unsigned mss)
{
	char namebuf[128];
	char *cp;
	bool connected = false;
	struct servent *svc;
	tpport_t port;

	sprintf(namebuf,  "%s", target);
	cp = strrchr(namebuf, '/');
	if(!cp)
		cp = strrchr(namebuf, ':');

	if(!cp) {
		endStream();
		connectError();
		return;
	}

	*(cp++) = 0;

	if(isdigit(*cp))
		port = atoi(cp);
	else {
		
		svc = getservbyname(cp, "tcp");
		if(svc)
			port = ntohs(svc->s_port);
		
		if(!svc) {
			endStream();
			connectError();
			return;
		}
	}

	connect(IPV4Host(namebuf), port, mss);
}

#endif




void TCPStream::connect(const IPV4Host &host, tpport_t port, unsigned mss)
{
        size_t i;
        fd_set fds;
        struct timeval to;
        bool connected = false;
        int rtn;
        long sockopt;
        socklen_t len = sizeof(sockopt);

#ifdef	TCP_MAXSEG
        if(mss)
                setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif

        for(i = 0 ; i < host.getAddressCount(); i++) {
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_addr = host.getAddress(i);
                addr.sin_port = htons(port);

                if(timeout)
                        setCompletion(false);

                // Win32 will crash if you try to connect to INADDR_ANY.
                if ( INADDR_ANY == addr.sin_addr.s_addr )
                        addr.sin_addr.s_addr = INADDR_LOOPBACK;
                rtn = ::connect(so, (struct sockaddr *)&addr, (socklen_t)sizeof(addr));
                if(!rtn) {
                        connected = true;
                        break;
                }

#ifndef WIN32
                if(errno == EINPROGRESS)
#else
                if(WSAGetLastError() == WSAEINPROGRESS)
#endif
                {
                        FD_ZERO(&fds);
                        FD_SET(so, &fds);
                        to.tv_sec = timeout / 1000;
                        to.tv_usec = timeout % 1000 * 1000;

                        // timeout check for connect completion

                        if(::select((int)so + 1, NULL, &fds, NULL, &to) < 1)
                                continue;

                        getsockopt(so, SOL_SOCKET, SO_ERROR, (char *)&sockopt, &len);
                        if(!sockopt) {
                                connected = true;
                                break;
                        }
                        endSocket();
                        so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        if(so == INVALID_SOCKET)
                                break;
                }
        }

        setCompletion(true);
        if(!connected) {
                rtn = errno;
                endStream();
                errno = rtn;
                connectError();
                return;
        }

        segmentBuffering(mss);
        TCPSocket::state = CONNECTED;
        }


TCPStream::TCPStream(const char *target, unsigned mss, bool throwflag, timeout_t to) :
        std::streambuf(), TCPSocket(PF_INET, SOCK_STREAM, IPPROTO_TCP),
        std::iostream((std::streambuf *)this),
        timeout(to),
        bufsize(0),gbuf(NULL),pbuf(NULL) {
        //family = IF_INET;
#ifdef	HAVE_OLD_IOSTREAM
//        init((streambuf *)this);
#endif
        //setError(throwflag);
        connect(target, mss);
                }

TCPStream::TCPStream(bool throwflag, timeout_t to) :
        std::streambuf(), TCPSocket(PF_INET, SOCK_STREAM, IPPROTO_TCP),
        std::iostream((std::streambuf *)this),
        timeout(to),
                bufsize(0),gbuf(NULL),pbuf(NULL) {
        //family = fam;
#ifdef	HAVE_OLD_IOSTREAM
//        init((streambuf *)this);
#endif
        //setError(throwflag);
                }

TCPStream::TCPStream(const TCPStream &source) :
        std::streambuf(), TCPSocket(DUP_SOCK(source.so,source.state)),
        std::iostream((std::streambuf *)this)
{
        //family = source.family;
#ifdef	HAVE_OLD_IOSTREAM
//        init((streambuf *)this);
#endif
        bufsize = source.bufsize;
        allocate(bufsize);
}

void TCPStream::connect(TCPSocket &tcpip)
{
        tpport_t port;

        endStream();
        //family = IPV4;
        so = accept(tcpip.getSocket(), NULL, NULL);
        if(so == INVALID_SOCKET)
                return;

        IPV4Host host = getPeer(&port);
        if(!tcpip.onAccept(host, port)) {
                endSocket();
                clear(std::ios::failbit | rdstate());
                return;
        }

        segmentBuffering(tcpip.getSegmentSize());
        TCPSocket::state = CONNECTED;
                        }

void TCPStream::segmentBuffering(unsigned mss)
{
        unsigned max = 0;
        socklen_t alen = sizeof(max);

        if(mss == 1)	// special interactive
        {
                allocate(1);
                return;
        }


		
#ifdef	TCP_MAXSEG
	if(mss)
		setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, sizeof(max));
	getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, &alen);
#endif

        if(max && max < mss)
                mss = max;

        if(!mss) {
                if(max)
                        mss = max;
                else
                        mss = 536;
                allocate(mss);
                return;
        }

        
#ifdef	TCP_MAXSEG
		setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif

        if(mss < 80)
                mss = 80;

        if(mss * 7 < 64000)
                bufferSize(mss * 7);
        else if(mss * 6 < 64000)
                bufferSize(mss * 6);
        else
                bufferSize(mss * 5);

        if(mss < 512)
                sendLimit(mss * 4);

        allocate(mss);
                        }

int TCPStream::getSegmentSize(void)
{
        unsigned mss = 0;
        socklen_t alen = sizeof(mss);

#ifdef	TCP_MAXSEG
        getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, &alen);
#endif
        if(!mss)
                return (int)bufsize;

        return mss;
                        }

void TCPStream::disconnect(void)
{
        if(TCPSocket::state == AVAILABLE)
                return;

        endStream();
        so = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(so != INVALID_SOCKET)
                TCPSocket::state = AVAILABLE;
 }

void TCPStream::endStream(void)
{
        if(bufsize) sync();
        if(gbuf)
                delete[] gbuf;
        if(pbuf)
                delete[] pbuf;
        gbuf = pbuf = NULL;
        bufsize = 0;
        clear();
        endSocket();
                        }

void TCPStream::allocate(size_t size)
{
        if(size < 2) {
                bufsize = 1;
                gbuf = pbuf = 0;
                return;
        }

        gbuf = new char[size];
        pbuf = new char[size];
        if(!pbuf || !gbuf) {
                error(errResourceFailure, "Could not allocate socket stream buffers");
                return;
        }
        bufsize = size;
        clear();

        // old, setb(gbuf, gbuf + size, 0);
        setg(gbuf, gbuf + size, gbuf + size);
        setp(pbuf, pbuf + size);
                        }

int TCPStream::doallocate()
{
        if(bufsize)
                return 0;

        allocate(1);
        return 1;
                        }

int TCPStream::uflow()
{
        int ret = underflow();

        if (ret == EOF)
                return EOF;

        if (bufsize != 1)
                gbump(1);

        return ret;
                        }

int TCPStream::underflow()
{
        ssize_t rlen = 1;
        unsigned char ch;

        if(bufsize == 1) {
                if(TCPSocket::state == STREAM)
                        rlen = ::read((int)so, (char *)&ch, 1);
                else if(timeout && !TCPSocket::isPending(pendingInput, timeout)) {
                    clear(std::ios::failbit | rdstate());
                        error(errTimeout,"Socket read timed out",socket_errno);
                        return EOF;
                }
                else
                        rlen = readData(&ch, 1);
                if(rlen < 1) {
                        if(rlen < 0) {
                            clear(std::ios::failbit | rdstate());
                                error(errInput,"Could not read from socket",socket_errno);
                        }
                        return EOF;
                }
                return ch;
        }

        if(!gptr())
                return EOF;

        if(gptr() < egptr())
                return (unsigned char)*gptr();

        rlen = (ssize_t)((gbuf + bufsize) - eback());
        if(TCPSocket::state == STREAM)
                rlen = ::read((int)so, (char *)eback(), _IOLEN64 rlen);
        else if(timeout && !TCPSocket::isPending(pendingInput, timeout)) {
            clear(std::ios::failbit | rdstate());
                error(errTimeout,"Socket read timed out",socket_errno);
                return EOF;
        }
        else
                rlen = readData(eback(), rlen);
        if(rlen < 1) {
//		clear(ios::failbit | rdstate());
                if(rlen < 0)
                                                error(errNotConnected,"Connection error",socket_errno);
                else {
                        error(errInput,"Could not read from socket",socket_errno);
                        clear(std::ios::failbit | rdstate());
                }
                return EOF;
        }
        error(errSuccess);

        setg(eback(), eback(), eback() + rlen);
        return (unsigned char) *gptr();
                        }

bool TCPStream::isPending(Pending pending, timeout_t timer)
{
        if(pending == pendingInput && in_avail())
                return true;
        else if(pending == pendingOutput)
                flush();

        return TCPSocket::isPending(pending, timer);
                        }

int TCPStream::sync(void)
{
        overflow(EOF);
        setg(gbuf, gbuf + bufsize, gbuf + bufsize);
        return 0;
                        }

#ifdef	HAVE_SNPRINTF
size_t TCPStream::printf(const char *format, ...)
{
        va_list args;
        size_t len;
        char *buf;

        va_start(args, format);
        overflow(EOF);
        len = pptr() - pbase();
        buf = pptr();
        vsnprintf(buf, len, format, args);
        va_end(args);
        len = strlen(buf);
        if(Socket::state == STREAM)
                return ::write((int)so, buf, _IOLEN64 len);
        else
                return writeData(buf, len);
                        }
#endif

int TCPStream::overflow(int c)
{
        unsigned char ch;
        ssize_t rlen, req;

        if(bufsize == 1) {
                if(c == EOF)
                        return 0;

                ch = (unsigned char)(c);
                if(TCPSocket::state == STREAM)
                        rlen = ::write((int)so, (const char *)&ch, 1);
                else
                        rlen = writeData(&ch, 1);
                if(rlen < 1) {
                        if(rlen < 0) {
                            clear(std::ios::failbit | rdstate());
                                error(errOutput,"Could not write to socket",socket_errno);
                        }
                        return EOF;
                }
                else
                        return c;
        }

        if(!pbase())
                return EOF;

        req = (ssize_t)(pptr() - pbase());
        if(req) {
                if(TCPSocket::state == STREAM)
                        rlen = ::write((int)so, (const char *)pbase(), req);
                else
                        rlen = writeData(pbase(), req);
                if(rlen < 1) {
                        if(rlen < 0) {
                            clear(std::ios::failbit | rdstate());
                                error(errOutput,"Could not write to socket",socket_errno);
                        }
                        return EOF;
                }
                req -= rlen;
        }

        // if write "partial", rebuffer remainder

        if(req)
//		memmove(pbuf, pptr() + rlen, req);
                memmove(pbuf, pbuf + rlen, req);
        setp(pbuf, pbuf + bufsize);
        pbump(req);

        if(c != EOF) {
                *pptr() = (unsigned char)c;
                pbump(1);
        }
        return c;
                        }


std::ostream& operator<<(std::ostream &os, const IPV4Address &ia)
{
        os << inet_ntoa(getaddress(ia));
        return os;
}

}
