
#include <string>
#include <iostream>
#include "ipv4addr.hpp"
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "socketserver.h"
#include "SimpleXML.hpp"
#include "FileWatcher.h"
#include <vector>
#include <iostream>
#include <strstream>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>



//-----------------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------------

int gPort=7681;

std::string gFileName=std::string("test.txt");


/// Struct to store data
typedef struct {
    std::string label;
    std::string colour;
    std::string value;
} itemData;


std::vector<itemData> gData;




//-----------------------------------------------------------------------------
// simple timer class
//-----------------------------------------------------------------------------

struct timer
{
    typedef std::chrono::steady_clock clock ;
    typedef std::chrono::seconds seconds ;

    void reset() { start = clock::now() ; }

    unsigned long long seconds_elapsed() const
    { return std::chrono::duration_cast<seconds>( clock::now() - start ).count() ; }

    private: clock::time_point start = clock::now() ;
};


//-----------------------------------------------------------------------------
// fileExists, returns true if file exists
//-----------------------------------------------------------------------------
bool fileExists(std::string fileName)
{

    std::ifstream test;

    test.open(fileName.c_str());

    if (test.fail())
    {
        test.close();
        return false;
    }
    else
    {
        test.close();
        return true;
    }
}

//-----------------------------------------------------------------------------
// parseFile, puts data from file into gData
//-----------------------------------------------------------------------------
void parseFile(std::string fileName)
{
    gData.clear();
    if (fileExists(fileName))
    {
          std::ifstream fs;
          fs.open(fileName.c_str());
          std::string lineOfFile;

          while (fs.good())
          {
              getline(fs, lineOfFile, '\n');
              std::istringstream ss(lineOfFile);
              {
                  itemData item;
                  std::string token;
                  std::getline(ss, token, ':');
                  item.label=token;

                  std::getline(ss, token, ':');
                  item.colour=token;

                  std::getline(ss, token, '\n');
                  item.value=token;
                  gData.push_back(item);
              }

          }
          fs.close();
    } else
    {
        std::cerr << "No such file exists yet" << fileName << std::endl  ;

    }
}

//-----------------------------------------------------------------------------
// sendData, sends data as XML on websocket
//-----------------------------------------------------------------------------
void sendData() {

    try {
        std::vector<XMLToken> myXml;
        myXml.clear();
        {
            XmlLazyTag myTag("data", myXml);

            for (int i=0;i<gData.size();i++)
            {
                XmlLazyTag myTag("item", myXml);
                {
                    Serialize("label",gData[i].label.c_str(),myXml);
                    Serialize("colour",gData[i].colour.c_str(),myXml);
                    Serialize("value",gData[i].value.c_str(),myXml);
                }
            }
        }
        XMLNode total(myXml.begin(),myXml.end()-1);
        std::string test=total.toString();
        sendDataOnSockets(test.c_str(),strlen(test.c_str()-1));
    } catch (XMLException e)
    {
        std::cerr << "Caught Xml exception "  << e.what();
    }
    catch( std::exception& e )
    {
        std::cerr <<  "An exception has occurred: " << e.what();
    }

}




//-----------------------------------------------------------------------------
// class UpdateListener
//-----------------------------------------------------------------------------

class UpdateListener : public FW::FileWatchListener
{
public:
    UpdateListener() {}
    void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename,
        FW::Action action)
    {
        std::cout << "DIR (" << dir + ") FILE (" + filename + ") has event " << action << std::endl;
        parseFile(gFileName);
        sendData();
    }
};


//-----------------------------------------------------------------------------
// void mainLoop(int argc, char *argv[])
//-----------------------------------------------------------------------------
void
mainLoop (int argc, char *argv[])
{

    // Fetch command line arguments

     if (argc<2) {

         std::cerr << "Usage: " << argv[0] << "  [-p winsocket-port]  filename" << std::endl  ;
     }

    for(int i=1; i < argc; i++)
    {

      if(argv[i] != NULL && !strncmp(argv[i], "-p", 3))
      {
          gPort=atoi(argv[i+1]);
      }


      if(argv[i] != NULL && !strncmp(argv[i], "-h", 3))
      {
          std::cerr << "Usage: " << argv[0] << "  [-p winsocket-port]  filename" << std::endl  ;
          exit(0);
      }

    }

    try
    {
        // create the file watcher object
        FW::FileWatcher fileWatcher;

        // Initial parse of file
        parseFile(gFileName);

        // add a watch to the system
        FW::WatchID watchID = fileWatcher.addWatch(gFileName.c_str(), new UpdateListener(), true);

        std::cout << "\n\nPress ^C to exit" << std::endl;

        for (;;)
        {
          // Poll change in file
          fileWatcher.update();

          // poll websocket
          pollSocket();
        }
    }
    catch( std::exception& e )
    {
        fprintf(stderr, "An exception has occurred: %s\n", e.what());
    }
    catch( ... )
    {
        fprintf(stderr, "A terminal exception has occurred, exiting\n");
    }

}




//-----------------------------------------------------------------------------
// int main (int argc, char **argv)
//-----------------------------------------------------------------------------
int
main (int argc, char **argv)
{
#ifdef TEST

  //SimpleXML myTest("test1.xml");

  //std::vector<XMLNode> children=myTest.getChildren();
  //for (int q=0;q<children.size();q++)
  //{
  //    std::cout << "\nChild ==" << children[q].toString();
  //}

  std::vector<XMLToken> myXml;
  myXml.clear();

  {
      XmlLazyTag myTag("data", myXml);
      {
          XmlLazyTag myTag("item", myXml);
          Serialize("label","#A",myXml);
          Serialize("colour","RED",myXml);
          Serialize("value","12",myXml);
      }
  }

  try {
      XMLNode total(myXml.begin(),myXml.end()-1);
      std::string test=total.toString();
      std::cout << test;
  } catch (XMLException e)
  {
      std::cout << "Caught Xml exception "  << e.what();
  }

#endif

  try
  {
    // websocket init
    socket_main(argc,argv,gPort,true);

    mainLoop (argc, argv);

    std::cerr << "Process is terminating";
  }
  catch (...)
  {
      std::cerr << "Exception!!" << std::endl;
  }
}

//-----------------------------------------------------------------------------

//EOF