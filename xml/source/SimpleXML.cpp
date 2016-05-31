/**
  SimpleXML.cpp
   This source file defines the XMLToken, XMLNode classes, the SimpleXML
     type and the XMLException class. These are all used for parsing
     simple XML files

**/

#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <string.h>
#include "SimpleXML.hpp"

using std::vector;
using std::string;
using std::istream;
using std::ifstream;
using std::stringstream;
using std::cerr;
using std::cout;
using std::endl;

/******************************************************************************
 * Static helper functions
 */

/**
 * \brief Return true if <ch> is a whitespace character (tab, space or newline)
 */
static bool
isWhiteSpace (char ch) {
  if ((ch == ' ') ||
      (ch == '\t') ||
      (ch == '\n'))
    return true;
  return false;
}

/**
 * \brief Remove whitespace from stream until the next char is a non-whitespace
 *        char (but don't remove it).
 */
static void
chomp (std::istream& istrm) {
  while (istrm.good() && isWhiteSpace(istrm.peek())) istrm.get();
};

/******************************************************************************
 * XMLToken Class
 */

/**
 * \brief Tokenize a stream of XML by chopping it up into XMLTokens and
 *        returning a vector of these tokens.
 *
 * This function consumes all data in the stream
 */
vector<XMLToken>
XMLToken::tokenize(istream& istrm) {
  vector<XMLToken> tokens;
  while (istrm.good()) {
    chomp(istrm);
    if (!istrm.good()) break;

    // parse tag
    if (istrm.peek() == '<') {
      string tagname;
      bool isEndTag = false;

      istrm.get(); // skip <
      chomp(istrm);
      if (!istrm.good()) break;
      if (istrm.peek() == '/') {
          istrm.get(); // skip /
          isEndTag = true;
      }

      while (istrm.good() && (istrm.peek() != '>'))
          tagname += istrm.get();

      // OLAS, patch to do a parse for files with tag attributes
      std::size_t found = tagname.find("=");
      if (found!=std::string::npos) {
          // There is an attribute ,skip it
          std::size_t spacef = tagname.find_first_of(" \t");
          std::string test=tagname.substr(0,spacef);
          tagname=test;
      }

      istrm.get(); // skip >
      tokens.push_back(XMLToken(tagname,isEndTag));
    } else {
      // parse string
      string buf = "";
      while (istrm.good() && (istrm.peek() != '<'))
          buf += istrm.get();
      tokens.push_back(XMLToken(buf));
    }
  }
  return tokens;
}

/**
 * \brief Return a string representation of this XMLToken object
 */
string
XMLToken::toString() const {
  stringstream ss;
  if (this->isTag()) ss << "<";
  if (this->isEndTag()) ss << "/";
  ss << this->getName();
  if (this->isTag()) ss << ">";
  return ss.str();
}


XmlLazyTag::XmlLazyTag(std::string name,std::vector<XMLToken> &outVec):
        fVector(outVec), fName(name)
{
    XMLToken   msg(name,START_TAG);
    outVec.push_back(msg);
}

XmlLazyTag::~XmlLazyTag()
{

    XMLToken  msge(fName,END_TAG);
    fVector.push_back(msge);
}

void serialiseXMLInt(std::string name,int value,std::vector<XMLToken> &outVec)
{
    char Buff[128];
    XMLToken   msg(name,START_TAG);
    outVec.push_back(msg);
    memset(Buff,0,128);

    sprintf(Buff,"%d",value);
    std::string myVal(Buff);
    XMLToken data(myVal);
    outVec.push_back(data);

    XMLToken  msge(name,END_TAG);             // <msg>
    outVec.push_back(msge);
}


void Serialize(std::string name,float value,std::vector<XMLToken> &outVec)
{
    char Buff[128];
    XMLToken   msg(name,START_TAG);
    outVec.push_back(msg);
    memset(Buff,0,128);

    sprintf(Buff,"%f",value);
    std::string myVal(Buff);
    XMLToken data(myVal);
    outVec.push_back(data);

    XMLToken  msge(name,END_TAG);             // </msg>
    outVec.push_back(msge);
}



void Serialize(std::string name,int value,std::vector<XMLToken> &outVec)
{
   serialiseXMLInt(name,value,outVec);
}

void Serialize(std::string name,std::string value,std::vector<XMLToken> &outVec)
{
    XMLToken   msg(name,START_TAG);
    outVec.push_back(msg);

    XMLToken data(value);
    outVec.push_back(data);

    XMLToken  msge(name,END_TAG);
    outVec.push_back(msge);
}



/******************************************************************************
 * XMLNode Class
 */

/**
 * \brief  Constructor, builds the XMLNode object by reading it from a
 *         file
 * \throws XMLException if the file couldn't be opened
 */
XMLNode::XMLNode(std::string fn) {
  ifstream strm(fn.c_str());
  if (!strm) {
    stringstream ss;
    ss << "failed to open file " << fn << " for reading XML object";
    throw XMLException(ss.str());
  }
  vector<XMLToken> tokens = XMLToken::tokenize(strm);
  this->build(tokens);
}

/**
 * \brief Constructor, builds the XMLNode object by reading it from an
 *        input stream (istream)
 *
 * This constructor will consume the whole stream
 */
XMLNode::XMLNode(std::istream& istrm) {
  vector<XMLToken> tokens = XMLToken::tokenize(istrm);
  this->build(tokens);
}


/**
 * \brief Copy constructor
 */
XMLNode::XMLNode(const XMLNode& x) {
  this->tagName = x.tagName;
  this->data = x.data;
  // handle with care..
  for (size_t i=0; i<x.children.size(); i++) {
    this->children.push_back(new XMLNode(*(x.children[i])));
  }
}


/**
* \brief  build a tree from a vector of tokens.
*
* We assume the tokens have a single outer tag for the root; this is checked.
*
* \param  tstart iterator pointing to first element in XMLToken sequence
* \param  tend iterator pointing to one beyond the last element in the XMLToken
*         sequence
* \throws XMLException if the first and last XMLTokens are not tags
*                      or if the first and last tags don't have matching names
*                      or it the last tag is not a closing tag (i.e. </tag>)
*                      or if there is only one Token between the start and end
*                            tags and it's a tag (i.e. orphaned tag)
*                      or if a tag nests both child tags and a data element
*/
void
XMLNode::build(vector<XMLToken>::iterator tstart,
               vector<XMLToken>::iterator tend) {

  vector<XMLToken>::iterator last=tend;
  /*
  last=tstart;
  vector<XMLToken>::iterator next=last+1;
  while (next!=tend)
  {
      last++;
      next++;
  }
  */

  if (!tstart->isTag() || !last->isTag()) {
    stringstream ss;
    ss << "failed to build XML node from XMLToken vector. Reason: first or"
       << "last token is not XML tag " << tstart->toString() << " End " ;
     throw XMLException(ss.str());
  }

  // first and last tokens are known to be tags now
  if (tstart->getName() != last->getName()) {
    stringstream ss;
    ss << "failed to build XML node from XMLToken vector."
       << "Reason: first and last tags have different names";
       throw XMLException(ss.str());
  }
  if (!last->isEndTag()) {
    stringstream ss;
    ss << "failed to build XML node from XMLToken vector."
       << "Reason: final tag is not a closing tag";
    throw XMLException(ss.str());
  }

  // if we get this far, first and last tags are present, there's nothing
  // else before/after them and they match each other, and the final
  // tag is a valid closing tag
  this->tagName = tstart->getName();

  // this might be a leaf with no data, in which case we're done
  if (tstart+1 == tend) return;

  // or it might be a leaf with data
  if (tstart+2 == tend) {
    if ((tstart+1)->isTag()) {
      stringstream ss;
      ss << "failed to build XML node from XMLToken vector."
         << "Reason: XMLToken appears to contain an orphan tag";
      throw XMLException(ss.str());
    }
    this->data = (tstart+1)->toString();
  }
  // okay, it's definitely other tag pairs
  else {
    vector<XMLToken>::iterator childStart = tstart + 1;
    while (childStart < tend) {
      if (!childStart->isTag()) {
        stringstream ss;
        ss << "failed to build XML node from XMLToken vector."
           << "Reason: XMLToken has mixed data and child tags";
        throw XMLException(ss.str());
      }

      // scan for the closing tag
      vector<XMLToken>::iterator childEnd = childStart + 1;
      while ((!childEnd->isTag()) ||
             (childEnd->getName() != childStart->getName())) childEnd++;

      // build child tree
      this->children.push_back(new XMLNode(childStart, childEnd));

      childStart = childEnd + 1;
    }
  }
}


/**
 * \brief get a vector of child nodes from this one which have a given tag
 *        name.
 *
 * The resulting vector might be empty. This method returns copies of the
 * nodes, not references to them, so this cannot be used to mess around with
 * the underlying data
 */
std::vector<XMLNode>
XMLNode::getChildren(const std::string& tagname) const {
  std::vector<XMLNode> res;
  for (size_t i=0; i<this->numChildren(); i++) {
    if (this->children[i]->getTagName() == tagname)
      res.push_back(*(this->children[i]));
  }
  return res;
}

/**
 * \brief get a vector of all child nodes nested in this XMLNode
 *
 * The resulting vector might be empty. This method returns copies of the
 * nodes, not references to them, so this cannot be used to mess around with
 * the underlying data
 */
std::vector<XMLNode>
XMLNode::getChildren() const {
  std::vector<XMLNode> r;
  size_t n = this->numChildren();
  for (size_t i=0; i<n; i++) r.push_back(*(this->children[i]));
  return r;
}

/**
 * \brief Return a 'pretty' string representation of this XMLNode object
 * \param tab This is used for formatting. All lines will be tabbed in by
 *            this many tabs. Nested XMLNodes will be tabbed in by tab+1, etc.
 */
string
XMLNode::toStringPretty(size_t tab) const {
  //string tabs(tab,'\t');
  string tabs="";
  stringstream ss;
  ss << tabs << "<" << tagName << ">"; // OLAS Here add attribute!!!
  if (this->children.size() == 0) {
      ss << "" <<  this->data << "";   //Data should not have extra spaces
   }
  else {
      for (size_t i=0; i<this->children.size(); i++)
        ss << std::endl << this->children[i]->toStringPretty(tab+1);
  }
  ss << tabs << "</" << this->tagName << ">";  // << endl
  return ss.str();
}

/**
 * \brief Return a string representation of this XMLNode object
 */
string
XMLNode::toString() const {
  return this->toStringPretty(0);
}

