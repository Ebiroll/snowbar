
#include "icsocket.hpp"

#include <vector>
#ifndef WIN32
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#else
#include <conio.h>
#endif


// Sends all data on connect, defined in main.cpp
extern   void sendData();

extern  std::string getData();


typedef struct
{
    bool hasReceivedUpgrade;
    ic::SimpleTCPStream* peerStream;
} clientState;


std::vector<clientState*> gPeerStreams;
ic::TCPSocket* gServerSocket=NULL;

///////////// Test data //////////////
char *gTestData=NULL;
int gTestDataLen=0;
std::vector<FILE *>testfile;
std::vector<char *>testData;
std::vector<int> testDataLen;
#define NO_OF_TESTFILES 10
/////////////////////////////////////

// If this is set an upgrade is required before sending data
bool gWaitForUpgrade=false;

#ifndef WIN32
int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif

void sendDataOnOneSocket(ic::SimpleTCPStream* ss,const char *data,int len)
{

    char header[10];
    int headerlen=2;
    char *tmp;

    header[0]=0x81;

    if (len<127)
    {
      header[1]=len;
      headerlen=2;
    } else if (len<65535)
    {
        header[1]=0x7e;
        unsigned short *ptr=(unsigned short *)&header[2];
        *ptr=htons(len);
        headerlen=4;
    }

    tmp=(char *)malloc(headerlen+len);
    memcpy(tmp,header,headerlen);
    memcpy(&tmp[headerlen],data,len);

    ss->write(tmp,headerlen+len);
    free(tmp);
}




//-----------------------------------------------------------------------------
// void sendDataOnSockets(char *data,int len)
// Goes through all sockets and sends the data on all active sockets
//-----------------------------------------------------------------------------
void sendDataOnSockets(const char *data,int len)
{

  for (int j=0;j<gPeerStreams.size();j++)
  {
      if (gPeerStreams[j]!=NULL)
      {

          if (gPeerStreams[j]->peerStream->isPending( ic::pendingError, 10))
          {
              std::cerr << "Stream error" << std::endl;
              delete gPeerStreams[j]->peerStream;
              delete gPeerStreams[j];
              gPeerStreams[j]=NULL;
          }
          else
          {
              sendDataOnOneSocket(gPeerStreams[j]->peerStream,data,len);
          }
      }

  }

}


//-----------------------------------------------------------------------------
// void socket_main(int argc, char **argv)
// Main entypoint for the socket server data
//-----------------------------------------------------------------------------
int socket_main(int argc, char **argv,int port,bool webClient)
{

    gWaitForUpgrade=webClient;

#ifdef WIN32
    WSADATA	wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        abort();
    }
    // 	WSACleanup();

#endif



    InetAddress addr("0.0.0.0");
    gServerSocket=new ic::TCPSocket(addr,port);


    testfile.resize(NO_OF_TESTFILES);
    testData.resize(NO_OF_TESTFILES);
    testDataLen.resize(NO_OF_TESTFILES);

    for (int j=0;j<NO_OF_TESTFILES;j++)
    {
        char Buff[240];
        testData[j]=NULL;
        sprintf(Buff,"test%d.xml",j);
        testfile[j]=fopen(Buff,"r");
        if(testfile[j])
        {
            char Buffer[2048];
            int bytesread=1;
            int totalBytes=0;
            while (bytesread>0)
            {
                bytesread=fread(Buffer,1,2048,testfile[j]);
                if (bytesread>0)
                {
                    totalBytes+=bytesread;
                }
            }
            rewind(testfile[j]);

            testData[j]=(char *)malloc(totalBytes+ 2 );
            //testData[j]=testData[j];

            testDataLen[j]=totalBytes;

            bytesread=fread(testData[j],totalBytes,1,testfile[j]);
            char *tmp=testData[j];
            tmp[totalBytes]=0;
        }
    }


    return 1;
}
//////////////////////////////////////////
extern "C" unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);

extern "C" int lws_b64_encode_string(const char *in, int in_len, char *out, int out_size);


int
handshake_0405(char *inbuffer,char *outbuffer)
{
    unsigned char simpleBuffer[1024];

    unsigned char magicresp[256];

    unsigned char hash[20];
    int n;
    char *response;
    char *p;
    int accept_len;

    for(int q=0;q<1024;q++)
    {
        simpleBuffer[q]=0;
    }

   char* mykey=strstr ( inbuffer, "Sec-WebSocket-Key:" );
   if (mykey)
   {
       mykey+=strlen("Sec-WebSocket-Key:");
       while (*mykey==' ')
       {
           mykey++;
       }
       char *endkey=mykey;

       while (*endkey!='\r')
       {
           endkey++;
       }
       *endkey=0;
    }

    /*
     * since key length is restricted above (currently 128), cannot
     * overflow
     */
    n = sprintf((char *)simpleBuffer,
                "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                mykey);

    printf("FOUND:::::::::::%s",simpleBuffer);

    SHA1(simpleBuffer, n, hash);

    accept_len = lws_b64_encode_string((char *)hash, 20,
            (char *)simpleBuffer,
            1024);


    /* create the response packet */

    /* make a buffer big enough for everything */

    #define MAX_WEBSOCKET_04_KEY_LEN 128

    response = (char *)simpleBuffer + MAX_WEBSOCKET_04_KEY_LEN;
    p = response;
    *p=0;
    strcpy(p, "HTTP/1.1 101 Switching Protocols\x0d\x0a"
              "Upgrade: WebSocket\x0d\x0a"
              "Connection: Upgrade\x0d\x0a"
              "Sec-WebSocket-Accept: ");
    strcat(p, (char *)simpleBuffer);
    p += accept_len;

    strcat(p, "\x0d\x0aSec-WebSocket-Protocol: load-monitor");


//    n = lws_hdr_copy(wsi, p, 128, WSI_TOKEN_PROTOCOL);
//        if (n < 0)
//            goto bail;
//        p += n;

    /* end of response packet */

        strcat(p, "\x0d\x0a\x0d\x0a");

        /* okay send the handshake response accepting the connection */

        printf("issuing resp pkt %d len\n", (int)(p - response));
#ifdef DEBUG
        fwrite(response, 1,  p - response, stderr);
#endif
//        n = libwebsocket_write(wsi, (unsigned char *)response,
//                          p - response, LWS_WRITE_HTTP);


    printf("Responding %s",response);
    strcpy(outbuffer,response);
    /* alright clean up and set ourselves into established state */



    return 0;


bail:
    /* free up his parsing allocations */


    return -1;
}

/////////////////////////////////////////////////////




//

//-----------------------------------------------------------------------------
// void sendInsertOneByOne()
// Sends insert one at at time
//-----------------------------------------------------------------------------
void sendInsertOneByOne(ic::SimpleTCPStream* sendStream)
{
  std::string ret="";


  sendDataOnOneSocket(sendStream,ret.c_str(),ret.size());


}

//-----------------------------------------------------------------------------
// void pollSocket()
// Periodic polling of the socket server
//-----------------------------------------------------------------------------
void pollSocket()
{

    if (gServerSocket)
    {
        if (gServerSocket->isPendingConnection(100))
        {
            ic::SimpleTCPStream* newConnection;

            newConnection=new ic::SimpleTCPStream(*gServerSocket);
            clientState *tmp=new clientState;
            tmp->hasReceivedUpgrade=false;
            tmp->peerStream=newConnection;
            gPeerStreams.push_back(tmp);

            // Send all data if we do not wait for upgrade
            if (!gWaitForUpgrade)
            {

                // Send the current parsed list
                sendData();
            }
        }
    }
    int readBytes=0;

    for (int j=0;j<gPeerStreams.size();j++)
    {
        if (gPeerStreams[j]!=NULL)
        {
            if (gPeerStreams[j]->peerStream->isPending( ic::pendingInput, 10))
            {
                char Response[1024];
                char Buff[1024];

                try {
                   readBytes=gPeerStreams[j]->peerStream->read(Buff,1024,6);
                }
                catch (ic::SockException &exep)
                {
                    std::cout << exep.what();
                    if (exep.mError==ic::errTimeout)
                    {
                       readBytes=47;
                    }
                }
                catch(...)
                {

                    // Catch other than timeout
                }
                printf("Received %s",Buff);



                if (!gPeerStreams[j]->hasReceivedUpgrade)
                {
                    handshake_0405(Buff,Response);

                    //char *l1= "HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSocket-Protocol: load-monitor\r\n\r\n" ;
                    //char *l1= "HTTP/1.1 101 Web Socket Protocol Handshake\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSocket-Protocol: load-monitor\r\n" ;
                    char *l1=Response;

                    if (readBytes>0)
                    {
                        try
                        {
                            gPeerStreams[j]->hasReceivedUpgrade=true;
                            gPeerStreams[j]->peerStream->write(l1,strlen(l1));

                            if (gWaitForUpgrade)
                            {
                                // Upgrade complete now we send the data
			      std::string data=getData();
			      sendDataOnOneSocket(gPeerStreams[j]->peerStream,data.c_str(),data.length());

                            }

                        }
                        catch (...)
                        {

                        }
                    }
                }
                // No data received, probably disconnected
                if (readBytes<=0)
                {
                    std::cerr << "Probably stream error" << std::endl;
                    delete gPeerStreams[j];
                    gPeerStreams[j]=NULL;

                }


            }

            if (gPeerStreams[j]!=NULL && gPeerStreams[j]->peerStream->isPending( ic::pendingError, 10))
            {
                std::cerr << "Stream error" << std::endl;
                delete gPeerStreams[j];
                gPeerStreams[j]=NULL;
            }

        }




    }

#if 0

    if (_kbhit())
    {
        char c=getchar();

        switch(c)
        {
        case '0':
            {
                gTestData=NULL;
                gTestDataLen=0;
                printf("0\n");
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '1':
            {
                gTestData=testData[1];
                gTestDataLen=testDataLen[1];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
            }
            break;
        case '2':
            {
                gTestData=testData[2];
                gTestDataLen=testDataLen[2];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '3':
            {
                gTestData=testData[3];
                gTestDataLen=testDataLen[3];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '4':
            {
                gTestData=testData[4];
                gTestDataLen=testDataLen[4];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '5':
            {
                gTestData=testData[5];
                gTestDataLen=testDataLen[5];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '6':
            {
                gTestData=testData[6];
                gTestDataLen=testDataLen[6];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '7':
            {
                gTestData=testData[7];
                gTestDataLen=testDataLen[7];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '8':
            {
                gTestData=testData[8];
                gTestDataLen=testDataLen[8];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;
        case '9':
            {
                gTestData=testData[9];
                gTestDataLen=testDataLen[9];
                printf(gTestData);
                sendDataOnSockets(gTestData,gTestDataLen);
                //libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_FLIGHT_SERVER]);
            }
            break;

        }

    }
#endif



}
