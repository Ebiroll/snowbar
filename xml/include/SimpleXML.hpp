/**
  SimpleXML.hpp
  This header declares the XMLToken, XMLNode classes, the SimpleXML
      type and the XMLException class. These are all used for parsing
       simple XML files

**/

#ifndef SIMXML_HPP
#define SIMXML_HPP

#include <iostream>
#include <sstream>
#include <vector>
#include <string>


/**
 * \brief An exception class for XML exceptions
 */


struct BaseException {
   BaseException(std::string m) : message(m) {}

   std::string what() const {return message;}
  std::string message;
};



class XMLException : public BaseException {
public:
  XMLException(std::string s = std::string()) : BaseException(s) {}
};

#define START_TAG  false
#define END_TAG    true

/**
 * \brief An XMLToken is either a single start tag, a single end tag or a
 *        data string
 *
 * This class if mainly used as a helper for tokenizing an XML string/stream
 */
class XMLToken { 
public:
	XMLToken(std::string s) : name(s), istag(false), isendtag(false) {;}
	XMLToken(std::string s, bool end) : name(s), istag(true), isendtag(end) {;}

	bool isTag() const { return this->istag; }
	bool isData() const { return !this->isTag(); }
	bool isEndTag() const { return this->isendtag; }
	std::string getName() const { return this->name; }
	std::string toString() const;

	/*** public static functions ***/
	static std::vector<XMLToken> tokenize(std::istream& istrm);

private:
	/** \brief name of this token (tag name if it's a tag, else data value) **/
	std::string name;

	/** \brief true if this is a tag, false if it's just data **/
	bool istag;

	/** \brief true if this is an end tag (e.g. </someTag>), else false **/
	bool isendtag;
};

class XmlLazyTag
{
public:
    XmlLazyTag(std::string name,std::vector<XMLToken> &o);
    ~XmlLazyTag();

    std::vector<XMLToken> &fVector;
    std::string           fName;
};








void serialiseXMLInt(std::string name,int value,std::vector<XMLToken> &o);

void Serialize(std::string key,int value,std::vector<XMLToken> &o);

void Serialize(std::string key,float value,std::vector<XMLToken> &o);

void Serialize(std::string key,std::string value,std::vector<XMLToken> &o);



/**
 * \brief An XML Node tag pair with the same name and has either a data body
 *        or other nested XMLNodes
 */
class XMLNode {
public:
  /*** constructors, destructors and object initialisation ***/
  XMLNode(std::string fn);
  XMLNode(std::istream& istrm);
  XMLNode(const XMLNode& x);
  XMLNode(std::vector<XMLToken>::iterator tstart, 
      std::vector<XMLToken>::iterator tend) {
  	this->build(tstart, tend);
  }
  ~XMLNode() {
    for (size_t i=0; i<this->children.size(); i++) 
      delete this->children[i];
  }
 
  /*** building the tree ***/
  void build(std::vector<XMLToken>& tokens) {
    this->build(tokens.begin(), tokens.end()-1);
  }
  void build(std::vector<XMLToken>::iterator tstart, 
      std::vector<XMLToken>::iterator tend);

  /*** inspectors ***/
  std::string toString() const;
  std::string toStringPretty(size_t tab) const;
  size_t numChrildren() const { return this->children.size(); }
  std::string getTagName () const { return this->tagName; }
  bool isLeaf() const { return this->numChildren() == 0; }
  std::string getData() const { return data; }
  size_t numChildren() const { return this->children.size(); }
  
  /*** inspectors -- finding sub trees ***/
  std::vector<XMLNode> getChildren() const;
  std::vector<XMLNode> getChildren(const std::string& tagname) const;
  
private:
  /** \brief name of tag forming this XMLNode (e.g. <name> ... </name>) **/
  std::string tagName;
  /** \brief data in this node; node will have data or children, not both **/
  std::string data;
  /** \brief child nodes nested within this node; note that the node will
   *         contain either data or children, but not both
   */
  std::vector<XMLNode*> children;
};

/*
class XMLTag : public XMLNode {

};

class XMLData : public XMLNode {

};
*/


typedef XMLNode SimpleXML;

#endif

