/*
Original code by Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#ifndef TINYXML2_INCLUDED
#define TINYXML2_INCLUDED

#if defined(ANDROID_NDK) || defined(__BORLANDC__) || defined(__QNXNTO__)
#   include <ctype.h>
#   include <limits.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#	if defined(__PS3__)
#		include <stddef.h>
#	endif
#else
#   include <cctype>
#   include <climits>
#   include <cstdio>
#   include <cstdlib>
#   include <cstring>
#endif
#include <stdint.h>
#include "include/tstr.h"
#include <vector>

/*
   TODO: intern strings instead of allocation.
*/
/*
	gcc:
        g++ -Wall -DTINYXML2_DEBUG tinyxml2.cpp xmltest.cpp -o gccxmltest.exe

    Formatting, Artistic Style:
        AStyle.exe --style=1tbs --indent-switches --break-closing-brackets --indent-preprocessor tinyxml2.cpp tinyxml2.h
*/

#if defined( _DEBUG ) || defined (__DEBUG__)
#   ifndef TINYXML2_DEBUG
#       define TINYXML2_DEBUG
#   endif
#endif

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif

#ifdef _WIN32
#   ifdef TINYXML2_EXPORT
#       define TINYXML2_LIB __declspec(dllexport)
#   elif defined(TINYXML2_IMPORT)
#       define TINYXML2_LIB __declspec(dllimport)
#   else
#       define TINYXML2_LIB
#   endif
#elif __GNUC__ >= 4
#   define TINYXML2_LIB __attribute__((visibility("default")))
#else
#   define TINYXML2_LIB
#endif


#if !defined(TIXMLASSERT)
#if defined(TINYXML2_DEBUG)
#   if defined(_MSC_VER)
#       // "(void)0," is for suppressing C4127 warning in "assert(false)", "assert(true)" and the like
#       define TIXMLASSERT( x )           if ( !((void)0,(x))) { __debugbreak(); }
#   elif defined (ANDROID_NDK)
#       include <android/log.h>
#       define TIXMLASSERT( x )           if ( !(x)) { __android_log_assert( "assert", "grinliz", "ASSERT in '%s' at %d.", __FILE__, __LINE__ ); }
#   else
#       include <assert.h>
#       define TIXMLASSERT                assert
#   endif
#else
#   define TIXMLASSERT( x )               {}
#endif
#endif

/* Versioning, past 1.0.14:
	http://semver.org/
*/
static const int TIXML2_MAJOR_VERSION = 8;
static const int TIXML2_MINOR_VERSION = 0;
static const int TIXML2_PATCH_VERSION = 0;

#define TINYXML2_MAJOR_VERSION 8
#define TINYXML2_MINOR_VERSION 0
#define TINYXML2_PATCH_VERSION 0

// A fixed element depth limit is problematic. There needs to be a
// limit to avoid a stack overflow. However, that limit varies per
// system, and the capacity of the stack. On the other hand, it's a trivial
// attack that can result from ill, malicious, or even correctly formed XML,
// so there needs to be a limit in place.
static const int TINYXML2_MAX_ELEMENT_DEPTH = 100;

namespace tinyxml2
{
namespace __TinyXml2_AutoFree { void push_free_memory(char* A, wchar_t* W); }

class XMLDocument;
class XMLElement;
class XMLAttribute;
class XMLComment;
class XMLText;
class XMLDeclaration;
class XMLUnknown;
class XMLPrinter;

/*
	A class that wraps strings. Normally stores the start and end
	pointers into the XML file itself, and will apply normalization
	and entity translation if actually read. Can also store (and memory
	manage) a traditional char[]

    Isn't clear why TINYXML2_LIB is needed; but seems to fix #719
*/
class TINYXML2_LIB StrPair
{
public:
    enum Mode {
        NEEDS_ENTITY_PROCESSING			= 0x01,
        NEEDS_NEWLINE_NORMALIZATION		= 0x02,
        NEEDS_WHITESPACE_COLLAPSING     = 0x04,

        TEXT_ELEMENT		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
        TEXT_ELEMENT_LEAVE_ENTITIES		= NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_NAME		            = 0,
        ATTRIBUTE_VALUE		            = NEEDS_ENTITY_PROCESSING | NEEDS_NEWLINE_NORMALIZATION,
        ATTRIBUTE_VALUE_LEAVE_ENTITIES  = NEEDS_NEWLINE_NORMALIZATION,
        COMMENT							= NEEDS_NEWLINE_NORMALIZATION
    };

    StrPair() : _flags( 0 ), _start( 0 ), _end( 0 ) {}
    ~StrPair();

    void Set( char* start, char* end, int flags ) {
        TIXMLASSERT( start );
        TIXMLASSERT( end );
        Reset();
        _start  = start;
        _end    = end;
        _flags  = flags | NEEDS_FLUSH;
    }

    const char* GetStr();

    bool Empty() const {
        return _start == _end;
    }

    void SetInternedStr( const char* str ) {
        Reset();
        _start = const_cast<char*>(str);
    }

    void SetStr( const char* str, int flags=0 );

    char* ParseText( char* in, const char* endTag, int strFlags, int* curLineNumPtr );
    char* ParseName( char* in );

    void TransferTo( StrPair* other );
	void Reset();

private:
    void CollapseWhitespace();

    enum {
        NEEDS_FLUSH = 0x100,
        NEEDS_DELETE = 0x200
    };

    int     _flags;
    char*   _start;
    char*   _end;

    StrPair( const StrPair& other );	// not supported
    void operator=( const StrPair& other );	// not supported, use TransferTo()
};


/*
	A dynamic array of Plain Old Data. Doesn't support constructors, etc.
	Has a small initial memory pool, so that low or no usage will not
	cause a call to new/delete
*/
template <class T, int INITIAL_SIZE>
class DynArray
{
public:
    DynArray() :
        _mem( _pool ),
        _allocated( INITIAL_SIZE ),
        _size( 0 )
    {
    }

    ~DynArray() {
        if ( _mem != _pool ) {
            delete [] _mem;
        }
    }

    void Clear() {
        _size = 0;
    }

    void Push( T t ) {
        TIXMLASSERT( _size < INT_MAX );
        EnsureCapacity( _size+1 );
        _mem[_size] = t;
        ++_size;
    }

    T* PushArr( int count ) {
        TIXMLASSERT( count >= 0 );
        TIXMLASSERT( _size <= INT_MAX - count );
        EnsureCapacity( _size+count );
        T* ret = &_mem[_size];
        _size += count;
        return ret;
    }

    T Pop() {
        TIXMLASSERT( _size > 0 );
        --_size;
        return _mem[_size];
    }

    void PopArr( int count ) {
        TIXMLASSERT( _size >= count );
        _size -= count;
    }

    bool Empty() const					{
        return _size == 0;
    }

    T& operator[](int i)				{
        TIXMLASSERT( i>= 0 && i < _size );
        return _mem[i];
    }

    const T& operator[](int i) const	{
        TIXMLASSERT( i>= 0 && i < _size );
        return _mem[i];
    }

    const T& PeekTop() const            {
        TIXMLASSERT( _size > 0 );
        return _mem[ _size - 1];
    }

    int Size() const					{
        TIXMLASSERT( _size >= 0 );
        return _size;
    }

    int Capacity() const				{
        TIXMLASSERT( _allocated >= INITIAL_SIZE );
        return _allocated;
    }

	void SwapRemove(int i) {
		TIXMLASSERT(i >= 0 && i < _size);
		TIXMLASSERT(_size > 0);
		_mem[i] = _mem[_size - 1];
		--_size;
	}

    const T* Mem() const				{
        TIXMLASSERT( _mem );
        return _mem;
    }

    T* Mem() {
        TIXMLASSERT( _mem );
        return _mem;
    }

private:
    DynArray( const DynArray& ); // not supported
    void operator=( const DynArray& ); // not supported

    void EnsureCapacity( int cap ) {
        TIXMLASSERT( cap > 0 );
        if ( cap > _allocated ) {
            TIXMLASSERT( cap <= INT_MAX / 2 );
            const int newAllocated = cap * 2;
            T* newMem = new T[newAllocated];
            TIXMLASSERT( newAllocated >= _size );
            memcpy( newMem, _mem, sizeof(T)*_size );	// warning: not using constructors, only works for PODs
            if ( _mem != _pool ) {
                delete [] _mem;
            }
            _mem = newMem;
            _allocated = newAllocated;
        }
    }

    T*  _mem;
    T   _pool[INITIAL_SIZE];
    int _allocated;		// objects allocated
    int _size;			// number objects in use
};


/*
	Parent virtual class of a pool for fast allocation
	and deallocation of objects.
*/
class MemPool
{
public:
    MemPool() {}
    virtual ~MemPool() {}

    virtual int ItemSize() const = 0;
    virtual void* Alloc() = 0;
    virtual void Free( void* ) = 0;
    virtual void SetTracked() = 0;
};


/*
	Template child class to create pools of the correct type.
*/
template< int ITEM_SIZE >
class MemPoolT : public MemPool
{
public:
    MemPoolT() : _blockPtrs(), _root(0), _currentAllocs(0), _nAllocs(0), _maxAllocs(0), _nUntracked(0)	{}
    ~MemPoolT() {
        MemPoolT< ITEM_SIZE >::Clear();
    }

    void Clear() {
        // Delete the blocks.
        while( !_blockPtrs.Empty()) {
            Block* lastBlock = _blockPtrs.Pop();
            delete lastBlock;
        }
        _root = 0;
        _currentAllocs = 0;
        _nAllocs = 0;
        _maxAllocs = 0;
        _nUntracked = 0;
    }

    virtual int ItemSize() const	{
        return ITEM_SIZE;
    }
    int CurrentAllocs() const		{
        return _currentAllocs;
    }

    virtual void* Alloc() {
        if ( !_root ) {
            // Need a new block.
            Block* block = new Block();
            _blockPtrs.Push( block );

            Item* blockItems = block->items;
            for( int i = 0; i < ITEMS_PER_BLOCK - 1; ++i ) {
                blockItems[i].next = &(blockItems[i + 1]);
            }
            blockItems[ITEMS_PER_BLOCK - 1].next = 0;
            _root = blockItems;
        }
        Item* const result = _root;
        TIXMLASSERT( result != 0 );
        _root = _root->next;

        ++_currentAllocs;
        if ( _currentAllocs > _maxAllocs ) {
            _maxAllocs = _currentAllocs;
        }
        ++_nAllocs;
        ++_nUntracked;
        return result;
    }

    virtual void Free( void* mem ) {
        if ( !mem ) {
            return;
        }
        --_currentAllocs;
        Item* item = static_cast<Item*>( mem );
#ifdef TINYXML2_DEBUG
        memset( item, 0xfe, sizeof( *item ) );
#endif
        item->next = _root;
        _root = item;
    }
    void Trace( const char* name ) {
        printf( "Mempool %s watermark=%d [%dk] current=%d size=%d nAlloc=%d blocks=%d\n",
                name, _maxAllocs, _maxAllocs * ITEM_SIZE / 1024, _currentAllocs,
                ITEM_SIZE, _nAllocs, _blockPtrs.Size() );
    }

    void SetTracked() {
        --_nUntracked;
    }

    int Untracked() const {
        return _nUntracked;
    }

	// This number is perf sensitive. 4k seems like a good tradeoff on my machine.
	// The test file is large, 170k.
	// Release:		VS2010 gcc(no opt)
	//		1k:		4000
	//		2k:		4000
	//		4k:		3900	21000
	//		16k:	5200
	//		32k:	4300
	//		64k:	4000	21000
    // Declared public because some compilers do not accept to use ITEMS_PER_BLOCK
    // in private part if ITEMS_PER_BLOCK is private
    enum { ITEMS_PER_BLOCK = (4 * 1024) / ITEM_SIZE };

private:
    MemPoolT( const MemPoolT& ); // not supported
    void operator=( const MemPoolT& ); // not supported

    union Item {
        Item*   next;
        char    itemData[ITEM_SIZE];
    };
    struct Block {
        Item items[ITEMS_PER_BLOCK];
    };
    DynArray< Block*, 10 > _blockPtrs;
    Item* _root;

    int _currentAllocs;
    int _nAllocs;
    int _maxAllocs;
    int _nUntracked;
};



/**
	Implements the interface to the "Visitor pattern" (see the Accept() method.)
	If you call the Accept() method, it requires being passed a XMLVisitor
	class to handle callbacks. For nodes that contain other nodes (Document, Element)
	you will get called with a VisitEnter/VisitExit pair. Nodes that are always leafs
	are simply called with Visit().

	If you return 'true' from a Visit method, recursive parsing will continue. If you return
	false, <b>no children of this node or its siblings</b> will be visited.

	All flavors of Visit methods have a default implementation that returns 'true' (continue
	visiting). You need to only override methods that are interesting to you.

	Generally Accept() is called on the XMLDocument, although all nodes support visiting.

	You should never change the document from a callback.

	@sa XMLNode::Accept()
*/
class TINYXML2_LIB XMLVisitor
{
public:
    virtual ~XMLVisitor() {}

    /// Visit a document.
    virtual bool VisitEnter( const XMLDocument& /*doc*/ )			{
        return true;
    }
    /// Visit a document.
    virtual bool VisitExit( const XMLDocument& /*doc*/ )			{
        return true;
    }

    /// Visit an element.
    virtual bool VisitEnter( const XMLElement& /*element*/, const XMLAttribute* /*firstAttribute*/ )	{
        return true;
    }
    /// Visit an element.
    virtual bool VisitExit( const XMLElement& /*element*/ )			{
        return true;
    }

    /// Visit a declaration.
    virtual bool Visit( const XMLDeclaration& /*declaration*/ )		{
        return true;
    }
    /// Visit a text node.
    virtual bool Visit( const XMLText& /*text*/ )					{
        return true;
    }
    /// Visit a comment node.
    virtual bool Visit( const XMLComment& /*comment*/ )				{
        return true;
    }
    /// Visit an unknown node.
    virtual bool Visit( const XMLUnknown& /*unknown*/ )				{
        return true;
    }
};

// WARNING: must match XMLDocument::_errorNames[]
enum XMLError {
    XML_SUCCESS = 0,
    XML_NO_ATTRIBUTE,
    XML_WRONG_ATTRIBUTE_TYPE,
    XML_ERROR_FILE_NOT_FOUND,
    XML_ERROR_FILE_COULD_NOT_BE_OPENED,
    XML_ERROR_FILE_READ_ERROR,
    XML_ERROR_PARSING_ELEMENT,
    XML_ERROR_PARSING_ATTRIBUTE,
    XML_ERROR_PARSING_TEXT,
    XML_ERROR_PARSING_CDATA,
    XML_ERROR_PARSING_COMMENT,
    XML_ERROR_PARSING_DECLARATION,
    XML_ERROR_PARSING_UNKNOWN,
    XML_ERROR_EMPTY_DOCUMENT,
    XML_ERROR_MISMATCHED_ELEMENT,
    XML_ERROR_PARSING,
    XML_CAN_NOT_CONVERT_TEXT,
    XML_NO_TEXT_NODE,
	XML_ELEMENT_DEPTH_EXCEEDED,

	XML_ERROR_COUNT
};


/*
	Utility functionality.
*/
class TINYXML2_LIB XMLUtil
{
public:
    static const char* SkipWhiteSpace( const char* p, int* curLineNumPtr )	{
        TIXMLASSERT( p );

        while( IsWhiteSpace(*p) ) {
            if (curLineNumPtr && *p == '\n') {
                ++(*curLineNumPtr);
            }
            ++p;
        }
        TIXMLASSERT( p );
        return p;
    }
    static char* SkipWhiteSpace( char* const p, int* curLineNumPtr ) {
        return const_cast<char*>( SkipWhiteSpace( const_cast<const char*>(p), curLineNumPtr ) );
    }

    // Anything in the high order range of UTF-8 is assumed to not be whitespace. This isn't
    // correct, but simple, and usually works.
    static bool IsWhiteSpace( char p )					{
        return !IsUTF8Continuation(p) && isspace( static_cast<unsigned char>(p) );
    }

    inline static bool IsNameStartChar( unsigned char ch ) {
        if ( ch >= 128 ) {
            // This is a heuristic guess in attempt to not implement Unicode-aware isalpha()
            return true;
        }
        if ( isalpha( ch ) ) {
            return true;
        }
        return ch == ':' || ch == '_';
    }

    inline static bool IsNameChar( unsigned char ch ) {
        return IsNameStartChar( ch )
               || isdigit( ch )
               || ch == '.'
               || ch == '-';
    }

    inline static bool IsPrefixHex( const char* p) {
        p = SkipWhiteSpace(p, 0);
        return p && *p == '0' && ( *(p + 1) == 'x' || *(p + 1) == 'X');
    }

    inline static bool StringEqual( const char* p, const char* q, int nChar=INT_MAX )  {
        if ( p == q ) {
            return true;
        }
        TIXMLASSERT( p );
        TIXMLASSERT( q );
        TIXMLASSERT( nChar >= 0 );
        return strncmp( p, q, nChar ) == 0;
    }

    inline static bool IsUTF8Continuation( const char p ) {
        return ( p & 0x80 ) != 0;
    }

    static const char* ReadBOM( const char* p, bool* hasBOM );
    // p is the starting location,
    // the UTF-8 value of the entity will be placed in value, and length filled in.
    static const char* GetCharacterRef( const char* p, char* value, int* length );
    static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );

    // converts primitive types to strings
    static void ToStr( int v, char* buffer, int bufferSize );
    static void ToStr( unsigned v, char* buffer, int bufferSize );
    static void ToStr( bool v, char* buffer, int bufferSize );
    static void ToStr( float v, char* buffer, int bufferSize );
    static void ToStr( double v, char* buffer, int bufferSize );
	static void ToStr(int64_t v, char* buffer, int bufferSize);
    static void ToStr(uint64_t v, char* buffer, int bufferSize);

    // converts strings to primitive types
    static bool	ToInt( const char* str, int* value );
    static bool ToUnsigned( const char* str, unsigned* value );
    static bool	ToBool( const char* str, bool* value );
    static bool	ToFloat( const char* str, float* value );
    static bool ToDouble( const char* str, double* value );
	static bool ToInt64(const char* str, int64_t* value);
    static bool ToUnsigned64(const char* str, uint64_t* value);
	// Changes what is serialized for a boolean value.
	// Default to "true" and "false". Shouldn't be changed
	// unless you have a special testing or compatibility need.
	// Be careful: static, global, & not thread safe.
	// Be sure to set static const memory as parameters.
	static void SetBoolSerialization(const char* writeTrue, const char* writeFalse);

private:
	static const char* writeBoolTrue;
	static const char* writeBoolFalse;
};


/** XMLNode is a base class for every object that is in the
	XML Document Object Model (DOM), except XMLAttributes.
	Nodes have siblings, a parent, and children which can
	be navigated. A node is always in a XMLDocument.
	The type of a XMLNode can be queried, and it can
	be cast to its more defined type.

	A XMLDocument allocates memory for all its Nodes.
	When the XMLDocument gets deleted, all its Nodes
	will also be deleted.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration( leaf )

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributes (not on tree)
							Comment (leaf)
							Unknown (leaf)

	@endverbatim
*/
class TINYXML2_LIB XMLNode
{
    friend class XMLDocument;
    friend class XMLElement;
public:

    /// Get the XMLDocument that owns this XMLNode.
    const XMLDocument* GetDocument() const	{
        TIXMLASSERT( _document );
        return _document;
    }
    /// Get the XMLDocument that owns this XMLNode.
    XMLDocument* GetDocument()				{
        TIXMLASSERT( _document );
        return _document;
    }

    /// Safely cast to an Element, or null.
    virtual XMLElement*		ToElement()		{
        return 0;
    }
    /// Safely cast to Text, or null.
    virtual XMLText*		ToText()		{
        return 0;
    }
    /// Safely cast to a Comment, or null.
    virtual XMLComment*		ToComment()		{
        return 0;
    }
    /// Safely cast to a Document, or null.
    virtual XMLDocument*	ToDocument()	{
        return 0;
    }
    /// Safely cast to a Declaration, or null.
    virtual XMLDeclaration*	ToDeclaration()	{
        return 0;
    }
    /// Safely cast to an Unknown, or null.
    virtual XMLUnknown*		ToUnknown()		{
        return 0;
    }

    virtual const XMLElement*		ToElement() const		{
        return 0;
    }
    virtual const XMLText*			ToText() const			{
        return 0;
    }
    virtual const XMLComment*		ToComment() const		{
        return 0;
    }
    virtual const XMLDocument*		ToDocument() const		{
        return 0;
    }
    virtual const XMLDeclaration*	ToDeclaration() const	{
        return 0;
    }
    virtual const XMLUnknown*		ToUnknown() const		{
        return 0;
    }

    /** The meaning of 'value' changes for the specific type.
    	@verbatim
    	Document:	empty (NULL is returned, not an empty string)
    	Element:	name of the element
    	Comment:	the comment text
    	Unknown:	the tag contents
    	Text:		the text string
    	@endverbatim
    */
    const char* Value() const;

    /** Set the Value of an XML node.
    	@sa Value()
    */
    void SetValue( const char* val, bool staticMem=false );

    /// Gets the line number the node is in, if the document was parsed from a file.
    int GetLineNum() const { return _parseLineNum; }

    /// Get the parent of this node on the DOM.
    const XMLNode*	Parent() const			{
        return _parent;
    }

    XMLNode* Parent()						{
        return _parent;
    }

    /// Returns true if this node has no children.
    bool NoChildren() const					{
        return !_firstChild;
    }

    /// Get the first child node, or null if none exists.
    const XMLNode*  FirstChild() const		{
        return _firstChild;
    }

    XMLNode*		FirstChild()			{
        return _firstChild;
    }

    /** Get the first child element, or optionally the first child
        element with the specified name.
    */
    const XMLElement* FirstChildElement( const char* name = 0 ) const;

    XMLElement* FirstChildElement( const char* name = 0 )	{
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->FirstChildElement( name ));
    }

    /// Get the last child node, or null if none exists.
    const XMLNode*	LastChild() const						{
        return _lastChild;
    }

    XMLNode*		LastChild()								{
        return _lastChild;
    }

    /** Get the last child element or optionally the last child
        element with the specified name.
    */
    const XMLElement* LastChildElement( const char* name = 0 ) const;

    XMLElement* LastChildElement( const char* name = 0 )	{
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->LastChildElement(name) );
    }

    /// Get the previous (left) sibling node of this node.
    const XMLNode*	PreviousSibling() const					{
        return _prev;
    }

    XMLNode*	PreviousSibling()							{
        return _prev;
    }

    /// Get the previous (left) sibling element of this node, with an optionally supplied name.
    const XMLElement*	PreviousSiblingElement( const char* name = 0 ) const ;

    XMLElement*	PreviousSiblingElement( const char* name = 0 ) {
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->PreviousSiblingElement( name ) );
    }

    /// Get the next (right) sibling node of this node.
    const XMLNode*	NextSibling() const						{
        return _next;
    }

    XMLNode*	NextSibling()								{
        return _next;
    }

    /// Get the next (right) sibling element of this node, with an optionally supplied name.
    const XMLElement*	NextSiblingElement( const char* name = 0 ) const;

    XMLElement*	NextSiblingElement( const char* name = 0 )	{
        return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->NextSiblingElement( name ) );
    }

    /**
    	Add a child node as the last (right) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
    */
    XMLNode* InsertEndChild( XMLNode* addThis );

    XMLNode* LinkEndChild( XMLNode* addThis )	{
        return InsertEndChild( addThis );
    }
    /**
    	Add a child node as the first (left) child.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the node does not
		belong to the same document.
    */
    XMLNode* InsertFirstChild( XMLNode* addThis );
    /**
    	Add a node after the specified child node.
		If the child node is already part of the document,
		it is moved from its old location to the new location.
		Returns the addThis argument or 0 if the afterThis node
		is not a child of this node, or if the node does not
		belong to the same document.
    */
    XMLNode* InsertAfterChild( XMLNode* afterThis, XMLNode* addThis );

    /**
    	Delete all the children of this node.
    */
    void DeleteChildren();

    /**
    	Delete a child of this node.
    */
    void DeleteChild( XMLNode* node );

    /**
    	Make a copy of this node, but not its children.
    	You may pass in a Document pointer that will be
    	the owner of the new Node. If the 'document' is
    	null, then the node returned will be allocated
    	from the current Document. (this->GetDocument())

    	Note: if called on a XMLDocument, this will return null.
    */
    virtual XMLNode* ShallowClone( XMLDocument* document ) const = 0;

	/**
		Make a copy of this node and all its children.

		If the 'target' is null, then the nodes will
		be allocated in the current document. If 'target'
        is specified, the memory will be allocated is the
        specified XMLDocument.

		NOTE: This is probably not the correct tool to
		copy a document, since XMLDocuments can have multiple
		top level XMLNodes. You probably want to use
        XMLDocument::DeepCopy()
	*/
	XMLNode* DeepClone( XMLDocument* target ) const;

    /**
    	Test if 2 nodes are the same, but don't test children.
    	The 2 nodes do not need to be in the same Document.

    	Note: if called on a XMLDocument, this will return false.
    */
    virtual bool ShallowEqual( const XMLNode* compare ) const = 0;

    /** Accept a hierarchical visit of the nodes in the TinyXML-2 DOM. Every node in the
    	XML tree will be conditionally visited and the host will be called back
    	via the XMLVisitor interface.

    	This is essentially a SAX interface for TinyXML-2. (Note however it doesn't re-parse
    	the XML for the callbacks, so the performance of TinyXML-2 is unchanged by using this
    	interface versus any other.)

    	The interface has been based on ideas from:

    	- http://www.saxproject.org/
    	- http://c2.com/cgi/wiki?HierarchicalVisitorPattern

    	Which are both good references for "visiting".

    	An example of using Accept():
    	@verbatim
    	XMLPrinter printer;
    	tinyxmlDoc.Accept( &printer );
    	const char* xmlcstr = printer.CStr();
    	@endverbatim
    */
    virtual bool Accept( XMLVisitor* visitor ) const = 0;

	/**
		Set user data into the XMLNode. TinyXML-2 in
		no way processes or interprets user data.
		It is initially 0.
	*/
	void SetUserData(void* userData)	{ _userData = userData; }

	/**
		Get user data set into the XMLNode. TinyXML-2 in
		no way processes or interprets user data.
		It is initially 0.
	*/
	void* GetUserData() const			{ return _userData; }

protected:
    explicit XMLNode( XMLDocument* );
    virtual ~XMLNode();

    virtual char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr);

    XMLDocument*	_document;
    XMLNode*		_parent;
    mutable StrPair	_value;
    int             _parseLineNum;

    XMLNode*		_firstChild;
    XMLNode*		_lastChild;

    XMLNode*		_prev;
    XMLNode*		_next;

	void*			_userData;

private:
    MemPool*		_memPool;
    void Unlink( XMLNode* child );
    static void DeleteNode( XMLNode* node );
    void InsertChildPreamble( XMLNode* insertThis ) const;
    const XMLElement* ToElementWithName( const char* name ) const;

    XMLNode( const XMLNode& );	// not supported
    XMLNode& operator=( const XMLNode& );	// not supported
};


/** XML text.

	Note that a text node can have child element nodes, for example:
	@verbatim
	<root>This is <b>bold</b></root>
	@endverbatim

	A text node can have 2 ways to output the next. "normal" output
	and CDATA. It will default to the mode it was parsed from the XML file and
	you generally want to leave it alone, but you can change the output mode with
	SetCData() and query it with CData().
*/
class TINYXML2_LIB XMLText : public XMLNode
{
    friend class XMLDocument;
public:
    virtual bool Accept( XMLVisitor* visitor ) const;

    virtual XMLText* ToText()			{
        return this;
    }
    virtual const XMLText* ToText() const	{
        return this;
    }

    /// Declare whether this should be CDATA or standard text.
    void SetCData( bool isCData )			{
        _isCData = isCData;
    }
    /// Returns true if this is a CDATA text element.
    bool CData() const						{
        return _isCData;
    }

    virtual XMLNode* ShallowClone( XMLDocument* document ) const;
    virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
    explicit XMLText( XMLDocument* doc )	: XMLNode( doc ), _isCData( false )	{}
    virtual ~XMLText()												{}

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
    bool _isCData;

    XMLText( const XMLText& );	// not supported
    XMLText& operator=( const XMLText& );	// not supported
};


/** An XML Comment. */
class TINYXML2_LIB XMLComment : public XMLNode
{
    friend class XMLDocument;
public:
    virtual XMLComment*	ToComment()					{
        return this;
    }
    virtual const XMLComment* ToComment() const		{
        return this;
    }

    virtual bool Accept( XMLVisitor* visitor ) const;

    virtual XMLNode* ShallowClone( XMLDocument* document ) const;
    virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
    explicit XMLComment( XMLDocument* doc );
    virtual ~XMLComment();

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr);

private:
    XMLComment( const XMLComment& );	// not supported
    XMLComment& operator=( const XMLComment& );	// not supported
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXML-2 will happily read or write files without a declaration,
	however.

	The text of the declaration isn't interpreted. It is parsed
	and written as a string.
*/
class TINYXML2_LIB XMLDeclaration : public XMLNode
{
    friend class XMLDocument;
public:
    virtual XMLDeclaration*	ToDeclaration()					{
        return this;
    }
    virtual const XMLDeclaration* ToDeclaration() const		{
        return this;
    }

    virtual bool Accept( XMLVisitor* visitor ) const;

    virtual XMLNode* ShallowClone( XMLDocument* document ) const;
    virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
    explicit XMLDeclaration( XMLDocument* doc );
    virtual ~XMLDeclaration();

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
    XMLDeclaration( const XMLDeclaration& );	// not supported
    XMLDeclaration& operator=( const XMLDeclaration& );	// not supported
};


/** Any tag that TinyXML-2 doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.

	DTD tags get thrown into XMLUnknowns.
*/
class TINYXML2_LIB XMLUnknown : public XMLNode
{
    friend class XMLDocument;
public:
    virtual XMLUnknown*	ToUnknown()					{
        return this;
    }
    virtual const XMLUnknown* ToUnknown() const		{
        return this;
    }

    virtual bool Accept( XMLVisitor* visitor ) const;

    virtual XMLNode* ShallowClone( XMLDocument* document ) const;
    virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
    explicit XMLUnknown( XMLDocument* doc );
    virtual ~XMLUnknown();

    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
    XMLUnknown( const XMLUnknown& );	// not supported
    XMLUnknown& operator=( const XMLUnknown& );	// not supported
};



/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not XMLNodes. You may only query the
	Next() attribute in a list.
*/
class TINYXML2_LIB XMLAttribute
{
    friend class XMLElement;
public:
    /// The name of the attribute.
    const char* Name() const;

    /// The value of the attribute.
    const char* Value() const;

    /// Gets the line number the attribute is in, if the document was parsed from a file.
    int GetLineNum() const { return _parseLineNum; }

    /// The next attribute in the list.
    const XMLAttribute* Next() const {
        return _next;
    }

    /** IntValue interprets the attribute as an integer, and returns the value.
        If the value isn't an integer, 0 will be returned. There is no error checking;
    	use QueryIntValue() if you need error checking.
    */
	int	IntValue() const {
		int i = 0;
		QueryIntValue(&i);
		return i;
	}

	int64_t Int64Value() const {
		int64_t i = 0;
		QueryInt64Value(&i);
		return i;
	}

    uint64_t Unsigned64Value() const {
        uint64_t i = 0;
        QueryUnsigned64Value(&i);
        return i;
    }

    /// Query as an unsigned integer. See IntValue()
    unsigned UnsignedValue() const			{
        unsigned i=0;
        QueryUnsignedValue( &i );
        return i;
    }
    /// Query as a boolean. See IntValue()
    bool	 BoolValue() const				{
        bool b=false;
        QueryBoolValue( &b );
        return b;
    }
    /// Query as a double. See IntValue()
    double 	 DoubleValue() const			{
        double d=0;
        QueryDoubleValue( &d );
        return d;
    }
    /// Query as a float. See IntValue()
    float	 FloatValue() const				{
        float f=0;
        QueryFloatValue( &f );
        return f;
    }

    /** QueryIntValue interprets the attribute as an integer, and returns the value
    	in the provided parameter. The function will return XML_SUCCESS on success,
    	and XML_WRONG_ATTRIBUTE_TYPE if the conversion is not successful.
    */
    XMLError QueryIntValue( int* value ) const;
    /// See QueryIntValue
    XMLError QueryUnsignedValue( unsigned int* value ) const;
	/// See QueryIntValue
	XMLError QueryInt64Value(int64_t* value) const;
    /// See QueryIntValue
    XMLError QueryUnsigned64Value(uint64_t* value) const;
	/// See QueryIntValue
    XMLError QueryBoolValue( bool* value ) const;
    /// See QueryIntValue
    XMLError QueryDoubleValue( double* value ) const;
    /// See QueryIntValue
    XMLError QueryFloatValue( float* value ) const;

    /// Set the attribute to a string value.
    void SetAttribute( const char* value );
    /// Set the attribute to value.
    void SetAttribute( int value );
    /// Set the attribute to value.
    void SetAttribute( unsigned value );
	/// Set the attribute to value.
	void SetAttribute(int64_t value);
    /// Set the attribute to value.
    void SetAttribute(uint64_t value);
    /// Set the attribute to value.
    void SetAttribute( bool value );
    /// Set the attribute to value.
    void SetAttribute( double value );
    /// Set the attribute to value.
    void SetAttribute( float value );

private:
    enum { BUF_SIZE = 200 };

    XMLAttribute() : _name(), _value(),_parseLineNum( 0 ), _next( 0 ), _memPool( 0 ) {}
    virtual ~XMLAttribute()	{}

    XMLAttribute( const XMLAttribute& );	// not supported
    void operator=( const XMLAttribute& );	// not supported
    void SetName( const char* name );

    char* ParseDeep( char* p, bool processEntities, int* curLineNumPtr );

    mutable StrPair _name;
    mutable StrPair _value;
    int             _parseLineNum;
    XMLAttribute*   _next;
    MemPool*        _memPool;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class TINYXML2_LIB XMLElement : public XMLNode
{
    friend class XMLDocument;
public:
    /// Get the name of an element (which is the Value() of the node.)
    const char* Name() const		{
        return Value();
    }
    /// Set the name of the element.
    void SetName( const char* str, bool staticMem=false )	{
        SetValue( str, staticMem );
    }

    virtual XMLElement* ToElement()				{
        return this;
    }
    virtual const XMLElement* ToElement() const {
        return this;
    }
    virtual bool Accept( XMLVisitor* visitor ) const;

    /** Given an attribute name, Attribute() returns the value
    	for the attribute of that name, or null if none
    	exists. For example:

    	@verbatim
    	const char* value = ele->Attribute( "foo" );
    	@endverbatim

    	The 'value' parameter is normally null. However, if specified,
    	the attribute will only be returned if the 'name' and 'value'
    	match. This allow you to write code:

    	@verbatim
    	if ( ele->Attribute( "foo", "bar" ) ) callFooIsBar();
    	@endverbatim

    	rather than:
    	@verbatim
    	if ( ele->Attribute( "foo" ) ) {
    		if ( strcmp( ele->Attribute( "foo" ), "bar" ) == 0 ) callFooIsBar();
    	}
    	@endverbatim
    */
    const char* Attribute( const char* name, const char* value=0 ) const;

    /** Given an attribute name, IntAttribute() returns the value
    	of the attribute interpreted as an integer. The default
        value will be returned if the attribute isn't present,
        or if there is an error. (For a method with error
    	checking, see QueryIntAttribute()).
    */
	int IntAttribute(const char* name, int defaultValue = 0) const;
    /// See IntAttribute()
	unsigned UnsignedAttribute(const char* name, unsigned defaultValue = 0) const;
	/// See IntAttribute()
	int64_t Int64Attribute(const char* name, int64_t defaultValue = 0) const;
    /// See IntAttribute()
    uint64_t Unsigned64Attribute(const char* name, uint64_t defaultValue = 0) const;
	/// See IntAttribute()
	bool BoolAttribute(const char* name, bool defaultValue = false) const;
    /// See IntAttribute()
	double DoubleAttribute(const char* name, double defaultValue = 0) const;
    /// See IntAttribute()
	float FloatAttribute(const char* name, float defaultValue = 0) const;

    /** Given an attribute name, QueryIntAttribute() returns
    	XML_SUCCESS, XML_WRONG_ATTRIBUTE_TYPE if the conversion
    	can't be performed, or XML_NO_ATTRIBUTE if the attribute
    	doesn't exist. If successful, the result of the conversion
    	will be written to 'value'. If not successful, nothing will
    	be written to 'value'. This allows you to provide default
    	value:

    	@verbatim
    	int value = 10;
    	QueryIntAttribute( "foo", &value );		// if "foo" isn't found, value will still be 10
    	@endverbatim
    */
    XMLError QueryIntAttribute( const char* name, int* value ) const				{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryIntValue( value );
    }

	/// See QueryIntAttribute()
    XMLError QueryUnsignedAttribute( const char* name, unsigned int* value ) const	{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryUnsignedValue( value );
    }

	/// See QueryIntAttribute()
	XMLError QueryInt64Attribute(const char* name, int64_t* value) const {
		const XMLAttribute* a = FindAttribute(name);
		if (!a) {
			return XML_NO_ATTRIBUTE;
		}
		return a->QueryInt64Value(value);
	}

    /// See QueryIntAttribute()
    XMLError QueryUnsigned64Attribute(const char* name, uint64_t* value) const {
        const XMLAttribute* a = FindAttribute(name);
        if(!a) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryUnsigned64Value(value);
    }

	/// See QueryIntAttribute()
    XMLError QueryBoolAttribute( const char* name, bool* value ) const				{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryBoolValue( value );
    }
    /// See QueryIntAttribute()
    XMLError QueryDoubleAttribute( const char* name, double* value ) const			{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryDoubleValue( value );
    }
    /// See QueryIntAttribute()
    XMLError QueryFloatAttribute( const char* name, float* value ) const			{
        const XMLAttribute* a = FindAttribute( name );
        if ( !a ) {
            return XML_NO_ATTRIBUTE;
        }
        return a->QueryFloatValue( value );
    }

	/// See QueryIntAttribute()
	XMLError QueryStringAttribute(const char* name, const char** value) const {
		const XMLAttribute* a = FindAttribute(name);
		if (!a) {
			return XML_NO_ATTRIBUTE;
		}
		*value = a->Value();
		return XML_SUCCESS;
	}



    /** Given an attribute name, QueryAttribute() returns
    	XML_SUCCESS, XML_WRONG_ATTRIBUTE_TYPE if the conversion
    	can't be performed, or XML_NO_ATTRIBUTE if the attribute
    	doesn't exist. It is overloaded for the primitive types,
		and is a generally more convenient replacement of
		QueryIntAttribute() and related functions.

		If successful, the result of the conversion
    	will be written to 'value'. If not successful, nothing will
    	be written to 'value'. This allows you to provide default
    	value:

    	@verbatim
    	int value = 10;
    	QueryAttribute( "foo", &value );		// if "foo" isn't found, value will still be 10
    	@endverbatim
    */
	XMLError QueryAttribute( const char* name, int* value ) const {
		return QueryIntAttribute( name, value );
	}

	XMLError QueryAttribute( const char* name, unsigned int* value ) const {
		return QueryUnsignedAttribute( name, value );
	}

	XMLError QueryAttribute(const char* name, int64_t* value) const {
		return QueryInt64Attribute(name, value);
	}

    XMLError QueryAttribute(const char* name, uint64_t* value) const {
        return QueryUnsigned64Attribute(name, value);
    }

    XMLError QueryAttribute( const char* name, bool* value ) const {
		return QueryBoolAttribute( name, value );
	}

	XMLError QueryAttribute( const char* name, double* value ) const {
		return QueryDoubleAttribute( name, value );
	}

	XMLError QueryAttribute( const char* name, float* value ) const {
		return QueryFloatAttribute( name, value );
	}

	XMLError QueryAttribute(const char* name, const char** value) const {
		return QueryStringAttribute(name, value);
	}

	/// Sets the named attribute to value.
    void SetAttribute( const char* name, const char* value )	{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
    void SetAttribute( const char* name, int value )			{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
    void SetAttribute( const char* name, unsigned value )		{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }

	/// Sets the named attribute to value.
	void SetAttribute(const char* name, int64_t value) {
		XMLAttribute* a = FindOrCreateAttribute(name);
		a->SetAttribute(value);
	}

    /// Sets the named attribute to value.
    void SetAttribute(const char* name, uint64_t value) {
        XMLAttribute* a = FindOrCreateAttribute(name);
        a->SetAttribute(value);
    }

    /// Sets the named attribute to value.
    void SetAttribute( const char* name, bool value )			{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
    void SetAttribute( const char* name, double value )		{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }
    /// Sets the named attribute to value.
    void SetAttribute( const char* name, float value )		{
        XMLAttribute* a = FindOrCreateAttribute( name );
        a->SetAttribute( value );
    }

    /**
    	Delete an attribute.
    */
    void DeleteAttribute( const char* name );

    /// Return the first attribute in the list.
    const XMLAttribute* FirstAttribute() const {
        return _rootAttribute;
    }
    /// Query a specific attribute in the list.
    const XMLAttribute* FindAttribute( const char* name ) const;

    /** Convenience function for easy access to the text inside an element. Although easy
    	and concise, GetText() is limited compared to getting the XMLText child
    	and accessing it directly.

    	If the first child of 'this' is a XMLText, the GetText()
    	returns the character string of the Text node, else null is returned.

    	This is a convenient method for getting the text of simple contained text:
    	@verbatim
    	<foo>This is text</foo>
    		const char* str = fooElement->GetText();
    	@endverbatim

    	'str' will be a pointer to "This is text".

    	Note that this function can be misleading. If the element foo was created from
    	this XML:
    	@verbatim
    		<foo><b>This is text</b></foo>
    	@endverbatim

    	then the value of str would be null. The first child node isn't a text node, it is
    	another element. From this XML:
    	@verbatim
    		<foo>This is <b>text</b></foo>
    	@endverbatim
    	GetText() will return "This is ".
    */
    const char* GetText() const;

    /** Convenience function for easy access to the text inside an element. Although easy
    	and concise, SetText() is limited compared to creating an XMLText child
    	and mutating it directly.

    	If the first child of 'this' is a XMLText, SetText() sets its value to
		the given string, otherwise it will create a first child that is an XMLText.

    	This is a convenient method for setting the text of simple contained text:
    	@verbatim
    	<foo>This is text</foo>
    		fooElement->SetText( "Hullaballoo!" );
     	<foo>Hullaballoo!</foo>
		@endverbatim

    	Note that this function can be misleading. If the element foo was created from
    	this XML:
    	@verbatim
    		<foo><b>This is text</b></foo>
    	@endverbatim

    	then it will not change "This is text", but rather prefix it with a text element:
    	@verbatim
    		<foo>Hullaballoo!<b>This is text</b></foo>
    	@endverbatim

		For this XML:
    	@verbatim
    		<foo />
    	@endverbatim
    	SetText() will generate
    	@verbatim
    		<foo>Hullaballoo!</foo>
    	@endverbatim
    */
	void SetText( const char* inText );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
    void SetText( int value );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
    void SetText( unsigned value );
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
	void SetText(int64_t value);
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
    void SetText(uint64_t value);
	/// Convenience method for setting text inside an element. See SetText() for important limitations.
    void SetText( bool value );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
    void SetText( double value );
    /// Convenience method for setting text inside an element. See SetText() for important limitations.
    void SetText( float value );

    /**
    	Convenience method to query the value of a child text node. This is probably best
    	shown by example. Given you have a document is this form:
    	@verbatim
    		<point>
    			<x>1</x>
    			<y>1.4</y>
    		</point>
    	@endverbatim

    	The QueryIntText() and similar functions provide a safe and easier way to get to the
    	"value" of x and y.

    	@verbatim
    		int x = 0;
    		float y = 0;	// types of x and y are contrived for example
    		const XMLElement* xElement = pointElement->FirstChildElement( "x" );
    		const XMLElement* yElement = pointElement->FirstChildElement( "y" );
    		xElement->QueryIntText( &x );
    		yElement->QueryFloatText( &y );
    	@endverbatim

    	@returns XML_SUCCESS (0) on success, XML_CAN_NOT_CONVERT_TEXT if the text cannot be converted
    			 to the requested type, and XML_NO_TEXT_NODE if there is no child text to query.

    */
    XMLError QueryIntText( int* ival ) const;
    /// See QueryIntText()
    XMLError QueryUnsignedText( unsigned* uval ) const;
	/// See QueryIntText()
	XMLError QueryInt64Text(int64_t* uval) const;
	/// See QueryIntText()
	XMLError QueryUnsigned64Text(uint64_t* uval) const;
	/// See QueryIntText()
    XMLError QueryBoolText( bool* bval ) const;
    /// See QueryIntText()
    XMLError QueryDoubleText( double* dval ) const;
    /// See QueryIntText()
    XMLError QueryFloatText( float* fval ) const;

	int IntText(int defaultValue = 0) const;

	/// See QueryIntText()
	unsigned UnsignedText(unsigned defaultValue = 0) const;
	/// See QueryIntText()
	int64_t Int64Text(int64_t defaultValue = 0) const;
    /// See QueryIntText()
    uint64_t Unsigned64Text(uint64_t defaultValue = 0) const;
	/// See QueryIntText()
	bool BoolText(bool defaultValue = false) const;
	/// See QueryIntText()
	double DoubleText(double defaultValue = 0) const;
	/// See QueryIntText()
    float FloatText(float defaultValue = 0) const;

    /**
        Convenience method to create a new XMLElement and add it as last (right)
        child of this node. Returns the created and inserted element.
    */
    XMLElement* InsertNewChildElement(const char* name);
    /// See InsertNewChildElement()
    XMLComment* InsertNewComment(const char* comment);
    /// See InsertNewChildElement()
    XMLText* InsertNewText(const char* text);
    /// See InsertNewChildElement()
    XMLDeclaration* InsertNewDeclaration(const char* text);
    /// See InsertNewChildElement()
    XMLUnknown* InsertNewUnknown(const char* text);


    // internal:
    enum ElementClosingType {
        OPEN,		// <foo>
        CLOSED,		// <foo/>
        CLOSING		// </foo>
    };
    ElementClosingType ClosingType() const {
        return _closingType;
    }
    virtual XMLNode* ShallowClone( XMLDocument* document ) const;
    virtual bool ShallowEqual( const XMLNode* compare ) const;

protected:
    char* ParseDeep( char* p, StrPair* parentEndTag, int* curLineNumPtr );

private:
    XMLElement( XMLDocument* doc );
    virtual ~XMLElement();
    XMLElement( const XMLElement& );	// not supported
    void operator=( const XMLElement& );	// not supported

    XMLAttribute* FindOrCreateAttribute( const char* name );
    char* ParseAttributes( char* p, int* curLineNumPtr );
    static void DeleteAttribute( XMLAttribute* attribute );
    XMLAttribute* CreateAttribute();

    enum { BUF_SIZE = 200 };
    ElementClosingType _closingType;
    // The attribute list is ordered; there is no 'lastAttribute'
    // because the list needs to be scanned for dupes before adding
    // a new attribute.
    XMLAttribute* _rootAttribute;
};


enum Whitespace {
    PRESERVE_WHITESPACE,
    COLLAPSE_WHITESPACE
};


/** A Document binds together all the functionality.
	It can be saved, loaded, and printed to the screen.
	All Nodes are connected and allocated to a Document.
	If the Document is deleted, all its Nodes are also deleted.
*/
class TINYXML2_LIB XMLDocument : public XMLNode
{
    friend class XMLElement;
    // Gives access to SetError and Push/PopDepth, but over-access for everything else.
    // Wishing C++ had "internal" scope.
    friend class XMLNode;
    friend class XMLText;
    friend class XMLComment;
    friend class XMLDeclaration;
    friend class XMLUnknown;
public:
    /// constructor
    XMLDocument( bool processEntities = true, Whitespace whitespaceMode = PRESERVE_WHITESPACE );
    ~XMLDocument();

    virtual XMLDocument* ToDocument()				{
        TIXMLASSERT( this == _document );
        return this;
    }
    virtual const XMLDocument* ToDocument() const	{
        TIXMLASSERT( this == _document );
        return this;
    }

    /**
    	Parse an XML file from a character string.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.

    	You may optionally pass in the 'nBytes', which is
    	the number of bytes which will be parsed. If not
    	specified, TinyXML-2 will assume 'xml' points to a
    	null terminated string.
    */
    XMLError Parse( const char* xml, size_t nBytes=static_cast<size_t>(-1) );

    /**
    	Load an XML file from disk.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
    XMLError LoadFile( const char* filename );

    /**
    	Load an XML file from disk. You are responsible
    	for providing and closing the FILE*.

        NOTE: The file should be opened as binary ("rb")
        not text in order for TinyXML-2 to correctly
        do newline normalization.

    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
    XMLError LoadFile( FILE* );

    /**
    	Save the XML file to disk.
    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
    XMLError SaveFile( const char* filename, bool compact = false );

    /**
    	Save the XML file to disk. You are responsible
    	for providing and closing the FILE*.

    	Returns XML_SUCCESS (0) on success, or
    	an errorID.
    */
    XMLError SaveFile( FILE* fp, bool compact = false );

    bool ProcessEntities() const		{
        return _processEntities;
    }
    Whitespace WhitespaceMode() const	{
        return _whitespaceMode;
    }

    /**
    	Returns true if this document has a leading Byte Order Mark of UTF8.
    */
    bool HasBOM() const {
        return _writeBOM;
    }
    /** Sets whether to write the BOM when writing the file.
    */
    void SetBOM( bool useBOM ) {
        _writeBOM = useBOM;
    }

    /** Return the root element of DOM. Equivalent to FirstChildElement().
        To get the first node, use FirstChild().
    */
    XMLElement* RootElement()				{
        return FirstChildElement();
    }
    const XMLElement* RootElement() const	{
        return FirstChildElement();
    }

    /** Print the Document. If the Printer is not provided, it will
        print to stdout. If you provide Printer, this can print to a file:
    	@verbatim
    	XMLPrinter printer( fp );
    	doc.Print( &printer );
    	@endverbatim

    	Or you can use a printer to print to memory:
    	@verbatim
    	XMLPrinter printer;
    	doc.Print( &printer );
    	// printer.CStr() has a const char* to the XML
    	@endverbatim
    */
    void Print( XMLPrinter* streamer=0 ) const;
    virtual bool Accept( XMLVisitor* visitor ) const;

    /**
    	Create a new Element associated with
    	this Document. The memory for the Element
    	is managed by the Document.
    */
    XMLElement* NewElement( const char* name );
    /**
    	Create a new Comment associated with
    	this Document. The memory for the Comment
    	is managed by the Document.
    */
    XMLComment* NewComment( const char* comment );
    /**
    	Create a new Text associated with
    	this Document. The memory for the Text
    	is managed by the Document.
    */
    XMLText* NewText( const char* text );
    /**
    	Create a new Declaration associated with
    	this Document. The memory for the object
    	is managed by the Document.

    	If the 'text' param is null, the standard
    	declaration is used.:
    	@verbatim
    		<?xml version="1.0" encoding="UTF-8"?>
    	@endverbatim
    */
    XMLDeclaration* NewDeclaration( const char* text=0 );
    /**
    	Create a new Unknown associated with
    	this Document. The memory for the object
    	is managed by the Document.
    */
    XMLUnknown* NewUnknown( const char* text );

    /**
    	Delete a node associated with this document.
    	It will be unlinked from the DOM.
    */
    void DeleteNode( XMLNode* node );

    /// Clears the error flags.
    void ClearError();

    /// Return true if there was an error parsing the document.
    bool Error() const {
        return _errorID != XML_SUCCESS;
    }
    /// Return the errorID.
    XMLError  ErrorID() const {
        return _errorID;
    }
	const char* ErrorName() const;
    static const char* ErrorIDToName(XMLError errorID);

    /** Returns a "long form" error description. A hopefully helpful
        diagnostic with location, line number, and/or additional info.
    */
	const char* ErrorStr() const;

    /// A (trivial) utility function that prints the ErrorStr() to stdout.
    void PrintError() const;

    /// Return the line where the error occurred, or zero if unknown.
    int ErrorLineNum() const
    {
        return _errorLineNum;
    }

    /// Clear the document, resetting it to the initial state.
    void Clear();

	/**
		Copies this document to a target document.
		The target will be completely cleared before the copy.
		If you want to copy a sub-tree, see XMLNode::DeepClone().

		NOTE: that the 'target' must be non-null.
	*/
	void DeepCopy(XMLDocument* target) const;

	// internal
    char* Identify( char* p, XMLNode** node );

	// internal
	void MarkInUse(const XMLNode* const);

    virtual XMLNode* ShallowClone( XMLDocument* /*document*/ ) const	{
        return 0;
    }
    virtual bool ShallowEqual( const XMLNode* /*compare*/ ) const	{
        return false;
    }

private:
    XMLDocument( const XMLDocument& );	// not supported
    void operator=( const XMLDocument& );	// not supported

    bool			_writeBOM;
    bool			_processEntities;
    XMLError		_errorID;
    Whitespace		_whitespaceMode;
    mutable StrPair	_errorStr;
    int             _errorLineNum;
    char*			_charBuffer;
    int				_parseCurLineNum;
	int				_parsingDepth;
	// Memory tracking does add some overhead.
	// However, the code assumes that you don't
	// have a bunch of unlinked nodes around.
	// Therefore it takes less memory to track
	// in the document vs. a linked list in the XMLNode,
	// and the performance is the same.
	DynArray<XMLNode*, 10> _unlinked;

    MemPoolT< sizeof(XMLElement) >	 _elementPool;
    MemPoolT< sizeof(XMLAttribute) > _attributePool;
    MemPoolT< sizeof(XMLText) >		 _textPool;
    MemPoolT< sizeof(XMLComment) >	 _commentPool;

	static const char* _errorNames[XML_ERROR_COUNT];

    void Parse();

    void SetError( XMLError error, int lineNum, const char* format, ... );

	// Something of an obvious security hole, once it was discovered.
	// Either an ill-formed XML or an excessively deep one can overflow
	// the stack. Track stack depth, and error out if needed.
	class DepthTracker {
	public:
		explicit DepthTracker(XMLDocument * document) {
			this->_document = document;
			document->PushDepth();
		}
		~DepthTracker() {
			_document->PopDepth();
		}
	private:
		XMLDocument * _document;
	};
	void PushDepth();
	void PopDepth();

    template<class NodeType, int PoolElementSize>
    NodeType* CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool );
};

template<class NodeType, int PoolElementSize>
inline NodeType* XMLDocument::CreateUnlinkedNode( MemPoolT<PoolElementSize>& pool )
{
    TIXMLASSERT( sizeof( NodeType ) == PoolElementSize );
    TIXMLASSERT( sizeof( NodeType ) == pool.ItemSize() );
    NodeType* returnNode = new (pool.Alloc()) NodeType( this );
    TIXMLASSERT( returnNode );
    returnNode->_memPool = &pool;

	_unlinked.Push(returnNode);
    return returnNode;
}

/**
	A XMLHandle is a class that wraps a node pointer with null checks; this is
	an incredibly useful thing. Note that XMLHandle is not part of the TinyXML-2
	DOM structure. It is a separate utility class.

	Take an example:
	@verbatim
	<Document>
		<Element attributeA = "valueA">
			<Child attributeB = "value1" />
			<Child attributeB = "value2" />
		</Element>
	</Document>
	@endverbatim

	Assuming you want the value of "attributeB" in the 2nd "Child" element, it's very
	easy to write a *lot* of code that looks like:

	@verbatim
	XMLElement* root = document.FirstChildElement( "Document" );
	if ( root )
	{
		XMLElement* element = root->FirstChildElement( "Element" );
		if ( element )
		{
			XMLElement* child = element->FirstChildElement( "Child" );
			if ( child )
			{
				XMLElement* child2 = child->NextSiblingElement( "Child" );
				if ( child2 )
				{
					// Finally do something useful.
	@endverbatim

	And that doesn't even cover "else" cases. XMLHandle addresses the verbosity
	of such code. A XMLHandle checks for null pointers so it is perfectly safe
	and correct to use:

	@verbatim
	XMLHandle docHandle( &document );
	XMLElement* child2 = docHandle.FirstChildElement( "Document" ).FirstChildElement( "Element" ).FirstChildElement().NextSiblingElement();
	if ( child2 )
	{
		// do something useful
	@endverbatim

	Which is MUCH more concise and useful.

	It is also safe to copy handles - internally they are nothing more than node pointers.
	@verbatim
	XMLHandle handleCopy = handle;
	@endverbatim

	See also XMLConstHandle, which is the same as XMLHandle, but operates on const objects.
*/
class TINYXML2_LIB XMLHandle
{
public:
    /// Create a handle from any node (at any depth of the tree.) This can be a null pointer.
    explicit XMLHandle( XMLNode* node ) : _node( node ) {
    }
    /// Create a handle from a node.
    explicit XMLHandle( XMLNode& node ) : _node( &node ) {
    }
    /// Copy constructor
    XMLHandle( const XMLHandle& ref ) : _node( ref._node ) {
    }
    /// Assignment
    XMLHandle& operator=( const XMLHandle& ref )							{
        _node = ref._node;
        return *this;
    }

    /// Get the first child of this handle.
    XMLHandle FirstChild() 													{
        return XMLHandle( _node ? _node->FirstChild() : 0 );
    }
    /// Get the first child element of this handle.
    XMLHandle FirstChildElement( const char* name = 0 )						{
        return XMLHandle( _node ? _node->FirstChildElement( name ) : 0 );
    }
    /// Get the last child of this handle.
    XMLHandle LastChild()													{
        return XMLHandle( _node ? _node->LastChild() : 0 );
    }
    /// Get the last child element of this handle.
    XMLHandle LastChildElement( const char* name = 0 )						{
        return XMLHandle( _node ? _node->LastChildElement( name ) : 0 );
    }
    /// Get the previous sibling of this handle.
    XMLHandle PreviousSibling()												{
        return XMLHandle( _node ? _node->PreviousSibling() : 0 );
    }
    /// Get the previous sibling element of this handle.
    XMLHandle PreviousSiblingElement( const char* name = 0 )				{
        return XMLHandle( _node ? _node->PreviousSiblingElement( name ) : 0 );
    }
    /// Get the next sibling of this handle.
    XMLHandle NextSibling()													{
        return XMLHandle( _node ? _node->NextSibling() : 0 );
    }
    /// Get the next sibling element of this handle.
    XMLHandle NextSiblingElement( const char* name = 0 )					{
        return XMLHandle( _node ? _node->NextSiblingElement( name ) : 0 );
    }

    /// Safe cast to XMLNode. This can return null.
    XMLNode* ToNode()							{
        return _node;
    }
    /// Safe cast to XMLElement. This can return null.
    XMLElement* ToElement() 					{
        return ( _node ? _node->ToElement() : 0 );
    }
    /// Safe cast to XMLText. This can return null.
    XMLText* ToText() 							{
        return ( _node ? _node->ToText() : 0 );
    }
    /// Safe cast to XMLUnknown. This can return null.
    XMLUnknown* ToUnknown() 					{
        return ( _node ? _node->ToUnknown() : 0 );
    }
    /// Safe cast to XMLDeclaration. This can return null.
    XMLDeclaration* ToDeclaration() 			{
        return ( _node ? _node->ToDeclaration() : 0 );
    }

private:
    XMLNode* _node;
};


/**
	A variant of the XMLHandle class for working with const XMLNodes and Documents. It is the
	same in all regards, except for the 'const' qualifiers. See XMLHandle for API.
*/
class TINYXML2_LIB XMLConstHandle
{
public:
    explicit XMLConstHandle( const XMLNode* node ) : _node( node ) {
    }
    explicit XMLConstHandle( const XMLNode& node ) : _node( &node ) {
    }
    XMLConstHandle( const XMLConstHandle& ref ) : _node( ref._node ) {
    }

    XMLConstHandle& operator=( const XMLConstHandle& ref )							{
        _node = ref._node;
        return *this;
    }

    const XMLConstHandle FirstChild() const											{
        return XMLConstHandle( _node ? _node->FirstChild() : 0 );
    }
    const XMLConstHandle FirstChildElement( const char* name = 0 ) const				{
        return XMLConstHandle( _node ? _node->FirstChildElement( name ) : 0 );
    }
    const XMLConstHandle LastChild()	const										{
        return XMLConstHandle( _node ? _node->LastChild() : 0 );
    }
    const XMLConstHandle LastChildElement( const char* name = 0 ) const				{
        return XMLConstHandle( _node ? _node->LastChildElement( name ) : 0 );
    }
    const XMLConstHandle PreviousSibling() const									{
        return XMLConstHandle( _node ? _node->PreviousSibling() : 0 );
    }
    const XMLConstHandle PreviousSiblingElement( const char* name = 0 ) const		{
        return XMLConstHandle( _node ? _node->PreviousSiblingElement( name ) : 0 );
    }
    const XMLConstHandle NextSibling() const										{
        return XMLConstHandle( _node ? _node->NextSibling() : 0 );
    }
    const XMLConstHandle NextSiblingElement( const char* name = 0 ) const			{
        return XMLConstHandle( _node ? _node->NextSiblingElement( name ) : 0 );
    }


    const XMLNode* ToNode() const				{
        return _node;
    }
    const XMLElement* ToElement() const			{
        return ( _node ? _node->ToElement() : 0 );
    }
    const XMLText* ToText() const				{
        return ( _node ? _node->ToText() : 0 );
    }
    const XMLUnknown* ToUnknown() const			{
        return ( _node ? _node->ToUnknown() : 0 );
    }
    const XMLDeclaration* ToDeclaration() const	{
        return ( _node ? _node->ToDeclaration() : 0 );
    }

private:
    const XMLNode* _node;
};


/**
	Printing functionality. The XMLPrinter gives you more
	options than the XMLDocument::Print() method.

	It can:
	-# Print to memory.
	-# Print to a file you provide.
	-# Print XML without a XMLDocument.

	Print to Memory

	@verbatim
	XMLPrinter printer;
	doc.Print( &printer );
	SomeFunction( printer.CStr() );
	@endverbatim

	Print to a File

	You provide the file pointer.
	@verbatim
	XMLPrinter printer( fp );
	doc.Print( &printer );
	@endverbatim

	Print without a XMLDocument

	When loading, an XML parser is very useful. However, sometimes
	when saving, it just gets in the way. The code is often set up
	for streaming, and constructing the DOM is just overhead.

	The Printer supports the streaming case. The following code
	prints out a trivially simple XML file without ever creating
	an XML document.

	@verbatim
	XMLPrinter printer( fp );
	printer.OpenElement( "foo" );
	printer.PushAttribute( "foo", "bar" );
	printer.CloseElement();
	@endverbatim
*/
class TINYXML2_LIB XMLPrinter : public XMLVisitor
{
public:
    /** Construct the printer. If the FILE* is specified,
    	this will print to the FILE. Else it will print
    	to memory, and the result is available in CStr().
    	If 'compact' is set to true, then output is created
    	with only required whitespace and newlines.
    */
    XMLPrinter( FILE* file=0, bool compact = false, int depth = 0 );
    virtual ~XMLPrinter()	{}

    /** If streaming, write the BOM and declaration. */
    void PushHeader( bool writeBOM, bool writeDeclaration );
    /** If streaming, start writing an element.
        The element must be closed with CloseElement()
    */
    void OpenElement( const char* name, bool compactMode=false );
    /// If streaming, add an attribute to an open element.
    void PushAttribute( const char* name, const char* value );
    void PushAttribute( const char* name, int value );
    void PushAttribute( const char* name, unsigned value );
	void PushAttribute( const char* name, int64_t value );
	void PushAttribute( const char* name, uint64_t value );
	void PushAttribute( const char* name, bool value );
    void PushAttribute( const char* name, double value );
    /// If streaming, close the Element.
    virtual void CloseElement( bool compactMode=false );

    /// Add a text node.
    void PushText( const char* text, bool cdata=false );
    /// Add a text node from an integer.
    void PushText( int value );
    /// Add a text node from an unsigned.
    void PushText( unsigned value );
	/// Add a text node from a signed 64bit integer.
	void PushText( int64_t value );
	/// Add a text node from an unsigned 64bit integer.
	void PushText( uint64_t value );
	/// Add a text node from a bool.
    void PushText( bool value );
    /// Add a text node from a float.
    void PushText( float value );
    /// Add a text node from a double.
    void PushText( double value );

    /// Add a comment
    void PushComment( const char* comment );

    void PushDeclaration( const char* value );
    void PushUnknown( const char* value );

    virtual bool VisitEnter( const XMLDocument& /*doc*/ );
    virtual bool VisitExit( const XMLDocument& /*doc*/ )			{
        return true;
    }

    virtual bool VisitEnter( const XMLElement& element, const XMLAttribute* attribute );
    virtual bool VisitExit( const XMLElement& element );

    virtual bool Visit( const XMLText& text );
    virtual bool Visit( const XMLComment& comment );
    virtual bool Visit( const XMLDeclaration& declaration );
    virtual bool Visit( const XMLUnknown& unknown );

    /**
    	If in print to memory mode, return a pointer to
    	the XML file in memory.
    */
    const char* CStr() const {
        return _buffer.Mem();
    }
    /**
    	If in print to memory mode, return the size
    	of the XML file in memory. (Note the size returned
    	includes the terminating null.)
    */
    int CStrSize() const {
        return _buffer.Size();
    }
    /**
    	If in print to memory mode, reset the buffer to the
    	beginning.
    */
    void ClearBuffer( bool resetToFirstElement = true ) {
        _buffer.Clear();
        _buffer.Push(0);
		_firstElement = resetToFirstElement;
    }

protected:
	virtual bool CompactMode( const XMLElement& )	{ return _compactMode; }

	/** Prints out the space before an element. You may override to change
	    the space and tabs used. A PrintSpace() override should call Print().
	*/
    virtual void PrintSpace( int depth );
    virtual void Print( const char* format, ... );
    virtual void Write( const char* data, size_t size );
    virtual void Putc( char ch );

    inline void Write(const char* data) { Write(data, strlen(data)); }

    void SealElementIfJustOpened();
    bool _elementJustOpened;
    DynArray< const char*, 10 > _stack;

private:
    /**
       Prepares to write a new node. This includes sealing an element that was
       just opened, and writing any whitespace necessary if not in compact mode.
     */
    void PrepareForNewNode( bool compactMode );
    void PrintString( const char*, bool restrictedEntitySet );	// prints out, after detecting entities.

    bool _firstElement;
    FILE* _fp;
    int _depth;
    int _textDepth;
    bool _processEntities;
	bool _compactMode;

    enum {
        ENTITY_RANGE = 64,
        BUF_SIZE = 200
    };
    bool _entityFlag[ENTITY_RANGE];
    bool _restrictedEntityFlag[ENTITY_RANGE];

    DynArray< char, 20 > _buffer;

    // Prohibit cloning, intentionally not implemented
    XMLPrinter( const XMLPrinter& );
    XMLPrinter& operator=( const XMLPrinter& );
};


}	// tinyxml2

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif



















#define __TINYXML_DEBUG__ 0

#if __TINYXML_DEBUG__
typedef char value_type;
static int char_type;
#else
template<typename value_type, int char_type>
#endif

class CTinyXml2_IUnkown
{
private:
#define char_type_ansi          0
#define char_type_utf8          1
#define char_type_unicode       2


    //static int char_type;   // 0=ansi, 1=utf8, 2=unicode
    //typedef char                value_type;
    typedef value_type*         pointer;
    typedef const value_type*   const_pointer;
    typedef value_type&         reference;
    typedef const value_type&   const_reference;

private:
    tinyxml2::XMLDocument* m_doc;
    wstr m_file;    // 从文件加载时记录文件位置, 如果 在调用 SaveFile() 时这个变量为空也会设置

    inline static const const_pointer to_index(int num)
    {
        _str s;
        s.Format("@%d", num);
        return ToReturn(s.c_str());
    }
    inline static const const_pointer to_string(int num)
    {
        return ToReturn(_str::to_string(num).c_str());
    }

    inline static const int to_number(const_pointer pstr)
    {
        return wstr::stoi(pstr);
    }

    // 将字符串转换成utf8文本, 根据当前类是什么类型来转换
    inline static const char* ToUtf8(const_pointer pstr)
    {
        if (!pstr || !pstr[0]) return "";
        if (char_type == char_type_utf8)return (const char*)pstr;

        char* ret = (char_type == char_type_ansi)
            ? _str::AnsiToUtf8((const char*)pstr)
            : _str::UnicodeToUtf8((const wchar_t*)pstr);
        tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
        return ret;
    }

    // 将文本转换成ansi, 根据当前类是什么类型来转换
    inline static const char* ToAnsi(const_pointer pstr)
    {
        if (!pstr || !pstr[0]) return "";
        if (char_type == char_type_ansi)return (const char*)pstr;

        char* ret = (char_type == char_type_utf8)
            ? _str::Utf8ToAnsi((const char*)pstr)
            : _str::UnicodeToAnsi((const wchar_t*)pstr);
        tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
        return ret;
    }

    // 将文本转换成unicode, 根据当前类是什么类型来转换
    inline static const wchar_t* ToUnicode(const_pointer pstr)
    {
        if (!pstr || !pstr[0]) return L"";
        if (char_type == char_type_unicode)return (const wchar_t*)pstr;

        wchar_t* ret = (char_type == char_type_ansi)
            ? _str::AnsiToUnicode((const char*)pstr)
            : _str::Utf8ToUnicode((const char*)pstr);
        tinyxml2::__TinyXml2_AutoFree::push_free_memory(0, ret);
        return ret;
    }

    // 将ansi文本转换成返回值需要的类型, 根据当前类是什么类型来转换
    inline static const_pointer ToReturn(const char* pstr)
    {
        if (!pstr || !pstr[0]) return (const_pointer)L"";
        if (char_type == char_type_ansi)    // 需要返回ansi
        {
            size_t len = strlen(pstr) + 1;
            char* p = new char[len];
            memcpy(p, pstr, len);
            tinyxml2::__TinyXml2_AutoFree::push_free_memory(p, 0);
            return (const_pointer)p;
        }

        if (char_type == char_type_utf8)   // 需要返回utf8
        {
            char* ret = _str::AnsiToUtf8((const char*)pstr);
            tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
            return (const_pointer)ret;
        }

        // 走到这里就是需要返回unicode文本
        wchar_t* ret = _str::AnsiToUnicode((const char*)pstr);
        tinyxml2::__TinyXml2_AutoFree::push_free_memory(0, ret);
        return (const_pointer)ret;
    }
public:
    CTinyXml2_IUnkown() :m_doc(0), m_file(260) { }
    CTinyXml2_IUnkown(const_pointer pszRootName) :m_doc(0), m_file(260) { Create(pszRootName); }
    ~CTinyXml2_IUnkown() { if (m_doc)delete m_doc; }

    // 将unicode字符串转换成当前对象需要的文本格式
    inline static const_pointer ToPointer(const wchar_t* pstr)
    {
        if (!pstr || char_type == char_type_unicode)return (const_pointer)pstr;

        char* ret = (char_type == char_type_ansi)
            ? _str::UnicodeToAnsi(pstr)
            : _str::UnicodeToUtf8(pstr);
        tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
        return (const_pointer)ret;
    }
    // 将ansi字符串转换成当前对象需要的文本格式
    inline static const_pointer ToPointer(const char* pstr)
    {
        if (!pstr || char_type == char_type_ansi)return (const_pointer)pstr;
        if (char_type == char_type_unicode)
        {
            wchar_t* ret = _str::AnsiToUnicode(pstr);
            tinyxml2::__TinyXml2_AutoFree::push_free_memory(0, ret);
            return (const_pointer)ret;
        }
        char* ret = _str::AnsiToUtf8(pstr);
        tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
        return (const_pointer)ret;
    }
public:
    // 清空当前xml, 析构时会自动触发
    inline void Clear()
    {
        if (m_doc)m_doc->Clear();
    }

    inline tinyxml2::XMLDocument* GetDocument()
    {
        return m_doc;
    }

    // 创建xml, 会清空当前已有xml数据
    // pszRootName = 根节点名, 为0则创建xml空对象
    inline bool Create(const_pointer pszRootName)
    {
        if (!m_doc)m_doc = new tinyxml2::XMLDocument;
        m_doc->Clear();
        if (!pszRootName || !pszRootName[0]) return true;

        tinyxml2::XMLElement* element = m_doc->NewElement(ToAnsi(pszRootName));
        return m_doc->InsertEndChild(element) != 0;
    }

    // 设置xml头
    inline bool SetDeclaration(const_pointer pszText)
    {
        if (!m_doc)return false;
        tinyxml2::XMLDeclaration* declaration = m_doc->NewDeclaration();
        if (!declaration)return false;
        declaration->SetValue(ToAnsi(pszText));
        m_doc->InsertFirstChild(declaration);
        return true;
    }

    // 解析xml数据, 会清空当前已有xml数据
    inline bool Parse(const_pointer p)
    {
        if (!m_doc)m_doc = new tinyxml2::XMLDocument;
        return m_doc->Parse(ToAnsi(p)) == tinyxml2::XMLError::XML_SUCCESS;
    }

    // 解析xml数据, 会清空当前已有xml数据
    inline void Print(tinyxml2::XMLPrinter* streamer = 0)
    {
        if (!m_doc)return;;
        return m_doc->Print(streamer);
    }

    // 从文件加载xml, 会清空当前已有xml数据
    inline bool LoadFile(const_pointer filename)
    {
        SetSaveFile(filename);
        if (!m_doc)m_doc = new tinyxml2::XMLDocument;

#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
        FILE* fp = 0;
        const errno_t err = _wfopen_s(&fp, m_file.c_str(), L"rb");
#else
        FILE* fp = _wfopen(m_file.c_str(), L"rb");
#endif
        if (!fp)
        {
            Clear();
            return 0;
        }

        int ret = m_doc->LoadFile(fp) == tinyxml2::XMLError::XML_SUCCESS;
        fclose(fp);
        return ret;
    }

    // 从已经打开的文件加载xml, 会清空当前已有xml数据
    inline bool LoadFile(FILE* fp)
    {
        if (!m_doc)m_doc = new tinyxml2::XMLDocument;
        return m_doc->LoadFile(fp) == tinyxml2::XMLError::XML_SUCCESS;
    }

    // 保存当前xml到磁盘, 如果内部记录的文件为空时会保存当前文件路径
    // filename = 保存的路径, 如果为空, 则使用内部记录的路径, 如果内部没有记录路径则调用失败
    // compact = 不格式化文本, 默认为false, 默认会格式化文本
    inline bool SaveFile(const_pointer filename = 0, bool compact = false)
    {
        if (m_file.empty())SetSaveFile(filename);
        if (!m_doc)return false;
        if (filename)
            return m_doc->SaveFile(ToAnsi(filename), compact) == tinyxml2::XMLError::XML_SUCCESS;
        return m_doc->SaveFile(m_file.w2a().c_str(), compact) == tinyxml2::XMLError::XML_SUCCESS;

    }

    // 保存当前xml数据到打开的文件中
    inline bool SaveFile(FILE* fp, bool compact = false)
    {
        if (!m_doc)return false;
        return m_doc->SaveFile(fp, compact) == tinyxml2::XMLError::XML_SUCCESS;
    }

    // 设置保存时使用的目录
    inline void SetSaveFile(const_pointer file) { m_file = ToUnicode(file); }

    // 获取内部记录的文件路径
    inline const_pointer GetSaveFile()
    {

        wchar_t* ret = 0;
        if (char_type == char_type_ansi)    // 需要返回ansi
        {
            char* ret = _str::UnicodeToAnsi(m_file.c_str());
            tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
            return (const_pointer)ret;
        }

        if (char_type == char_type_utf8)   // 需要返回utf8
        {
            char* ret = _str::UnicodeToUtf8(m_file.c_str());
            tinyxml2::__TinyXml2_AutoFree::push_free_memory(ret, 0);
            return (const_pointer)ret;
        }

        // 走到这里就是需要返回unicode文本
        return (const_pointer)m_file.c_str();
    }

public:

    // 取根节点名, 如果未创建则返回空
    inline const_pointer GetRootName() const
    {
        tinyxml2::XMLNode* node = m_doc->RootElement();
        if (!node)return 0;
        return ToReturn(node->Value());
    }

    // 取子节点数, 取得参数节点的所有子节点的个数
    // node = 节点指针, 读取这个节点下的子节点数量
    inline int GetChildNodeCount(tinyxml2::XMLNode* node) const
    {
        if (!node)return 0;
        node = node->FirstChild();
        int countNode = 0;
        while (node)
        {
            countNode++;
            node = node->NextSibling();
        }
        return countNode;
    }

    // 取子节点数, 取得参数节点的所有子节点的个数
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    inline int GetChildNodeCount(const_pointer pszPath = 0) const
    {
        if (!m_doc)return 0;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        return GetChildNodeCount(node);
    }
    // 取节点名, 取得参数节点的名字, <节点名 属性名="属性值"/>
    // node = 节点指针, 读取这个节点的节点名
    inline const_pointer GetNodeName(tinyxml2::XMLNode* node) const
    {
        if (!node)return 0;
        return ToReturn(node->Value());
    }
    // 取节点名, 取得参数节点的名字, <节点名 属性名="属性值"/>
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    inline const_pointer GetNodeName(const_pointer pszPath = 0) const
    {
        if (!m_doc)return 0;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        return ToReturn(node->Value());
    }
    // 取节点值, 取得参数节点的值, <节点名 属性名="属性值"/>
    // node = 节点指针, 读取这个节点的节点值
    inline const_pointer GetNodeValue(tinyxml2::XMLNode* node) const
    {
        if (!node)return 0;
        tinyxml2::XMLElement* ele = node->ToElement();
        if (!ele) return 0;
        return ToReturn(ele->GetText());
    }
    // 取节点值, 取得参数节点的值, <节点名 属性名="属性值"/>
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    inline const_pointer GetNodeValue(const_pointer pszPath = 0) const
    {
        if (!m_doc)return 0;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        return GetNodeValue(node);
    }

    // 枚举指定路径节点的所有属性, 返回属性数量, <节点名 属性名="属性值"/>
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pNames = 接收属性名数组, 留空则不获取
    // pValues = 属性名对应的值, 留空则不获取
    inline int EnumAttribute(const_pointer pszPath = 0, std::vector<const_pointer>* pNames = 0, std::vector<const_pointer>* pValues = 0) const
    {
        return GetAttrName(pszPath, pNames, pValues);
    }
    // 枚举指定路径节点的所有属性, 返回属性数量, <节点名 属性名="属性值"/>
    // node = 节点指针, 属性值从这个节点获取
    // pNames = 接收属性名数组, 留空则不获取
    // pValues = 属性名对应的值, 留空则不获取
    inline int EnumAttribute(tinyxml2::XMLNode* node, std::vector<const_pointer>* pNames = 0, std::vector<const_pointer>* pValues = 0) const
    {
        return GetAttrName(node, pNames, pValues);
    }
    // 取所有属性名, 返回属性数量, <节点名 属性名="属性值"/>
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pNames = 接收属性名数组, 留空则不获取
    // pValues = 接收属性值数组, 留空则不获取
    inline int GetAttrName(const_pointer pszPath = 0, std::vector<const_pointer>* pNames = 0, std::vector<const_pointer>* pValues = 0) const
    {
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        return GetAttrName(node, pNames, pValues);
    }

    // 取所有属性名, 返回属性数量, <节点名 属性名="属性值"/>
    // node = 节点指针, 属性值从这个节点获取
    // pNames = 接收属性名数组, 留空则不获取
    // pValues = 接收属性值数组, 留空则不获取
    inline int GetAttrName(tinyxml2::XMLNode* node, std::vector<const_pointer>* pNames = 0, std::vector<const_pointer>* pValues = 0) const
    {
        if (pNames)  pNames->clear();
        if (pValues) pValues->clear();
        if (!m_doc || !node)return 0;


        tinyxml2::XMLElement* ele = node->ToElement();
        if (!ele) return 0;
        const tinyxml2::XMLAttribute* attr = ele->FirstAttribute();
        if (!attr) return 0;
        int count = 0;
        while (attr)
        {
            count++;
            if (pNames)  pNames->push_back(ToReturn(attr->Name()));
            if (pValues) pValues->push_back(ToReturn(attr->Value()));
            attr = attr->Next();
        }
        return count;
    }

    // 取属性值, 取得参数节点的特定属性的值, <节点名 属性名="属性值"/>
    // node = 节点指针, 从这个节点获取属性值
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // nDefValue = 获取失败返回的默认值
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline int GetAttrValue_int(tinyxml2::XMLNode* node, const_pointer pszAttrName, int nDefValue = 0, BOOL* isAttribute = 0) const
    {
        return to_number(GetAttrValue(node, pszAttrName, to_string(nDefValue), isAttribute));
    }
    // 取属性值, 取得参数节点的特定属性的值, <节点名 属性名="属性值"/>
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // nDefValue = 获取失败返回的默认值
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline int GetAttrValue_int(const_pointer pszPath, const_pointer pszAttrName, int nDefValue = 0, BOOL* isAttribute = 0) const
    {
        if (isAttribute)*isAttribute = FALSE;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        return GetAttrValue_int(node, pszAttrName, nDefValue, isAttribute);
    }

    // 取属性值, 取得参数节点的特定属性的值, <节点名 属性名="属性值"/>
    // index = 索引, 索引从0开始
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // nDefValue = 获取失败返回的默认值
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline int GetAttrValue_int(int index, const_pointer pszAttrName, int nDefValue = 0, BOOL* isAttribute = 0) const
    {
        if (isAttribute)*isAttribute = FALSE;
        tinyxml2::XMLNode* node;
        if (!GetNode(to_index(index + 1), node)) return 0;
        return GetAttrValue_int(node, pszAttrName, nDefValue, isAttribute);
    }

    // 取属性值, 取得参数节点的特定属性的值, <节点名 属性名="属性值"/>
    // node = 节点指针, 从这个节点获取属性值
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // lpszDefValue = 获取失败返回的默认值
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline const_pointer GetAttrValue(tinyxml2::XMLNode* node, const_pointer pszAttrName, const_pointer lpszDefValue = 0, BOOL* isAttribute = 0) const
    {
        if (isAttribute)*isAttribute = FALSE;
        if (!node || !pszAttrName)return 0;
        tinyxml2::XMLElement* ele = node->ToElement();
        if (!ele) return lpszDefValue;
        const char* nameA = ToAnsi(pszAttrName);

        const tinyxml2::XMLAttribute* a = ele->FindAttribute(nameA);
        if (!a) return lpszDefValue;
        if (isAttribute)*isAttribute = TRUE;
        return ToReturn(a->Value());
    }
    // 取属性值, 取得参数节点的特定属性的值, <节点名 属性名="属性值"/>
    // pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // lpszDefValue = 获取失败返回的默认值
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline const_pointer GetAttrValue(const_pointer pszPath, const_pointer pszAttrName, const_pointer lpszDefValue= 0, BOOL* isAttribute = 0) const
    {
        if (isAttribute)*isAttribute = FALSE;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        return GetAttrValue(node, pszAttrName, lpszDefValue, isAttribute);
    }

    // 取属性值, 取得参数节点的特定属性的值, <节点名 属性名="属性值"/>
    // index = 索引, 索引从0开始
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // lpszDefValue = 获取失败返回的默认值
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline const_pointer GetAttrValue(int index, const_pointer pszAttrName, const_pointer lpszDefValue = 0, BOOL* isAttribute = 0) const
    {
        if (isAttribute)*isAttribute = FALSE;
        tinyxml2::XMLNode* node;
        if (!GetNode(to_index(index + 1), node)) return 0;
        return GetAttrValue(node, pszAttrName, lpszDefValue, isAttribute);
    }
    //// 插入备注文本, 值为0则删除该备注文本
    //// pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    //// pszNodeValue = 节点值  <节点名>节点值</节点名>
    //inline bool InsertComment(tinyxml2::XMLNode* node, const_pointer pszCommentText = 0)
    //{
    //    if (!node)return 0;
    //    if (pszCommentText)
    //    {
    //        tinyxml2::XMLComment* child = m_doc->NewComment(pszCommentText);   // 备注
    //        node->ToComment();
    //        child->DeleteChild();
    //        node->get;
    //    }
    //    else
    //    {
    //        DelNode(node);
    //    }

    //}

    //// 插入备注文本, 值为0则删除该备注文本
    //// pszPath = 路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    //// pszNodeValue = 节点值  <节点名>节点值</节点名>
    //inline bool InsertComment(const_pointer pszPath, const_pointer pszCommentText = 0)
    //{
    //    if (!m_doc)return 0;
    //    tinyxml2::XMLNode* node;
    //    if (!GetNode(pszPath, node)) return 0;
    //    return InsertComment(node, pszCommentText);
    //}

    // 插入子节点, 新建一个节点，并把该参数节点作为该新节点的父节点，然后插入到XML树中
    // pszParentPath = 父节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点, 新加入的节点路径 = @1/@1/@2/@节点数量
    // pszNodeName = 新的节点名, 为0则调用失败  <节点名></节点名>
    // pszNodeValue = 节点值  <节点名>节点值</节点名>
    // isCDATA = 如果本参数为真，则节点值文本将被 "<![CDATA[" 和 "]]>" 包围，这意味着该文本可以包含任意字符(以不影响CDATA解析为限}
    inline bool InsertChildNode(const_pointer pszParentPath, const_pointer pszNodeName, const_pointer pszNodeValue = 0, bool isCDATA = false, tinyxml2::XMLNode** ppNode = 0)
    {
        if (!m_doc || !pszNodeName)return 0;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszParentPath, node)) return 0;
        return InsertChildNode(node, pszNodeName, pszNodeValue, isCDATA, ppNode);
    }



    // 插入子节点, 新建一个节点，并把该参数节点作为该新节点的父节点，然后插入到XML树中
    // parentNode = 父节点
    // pszNodeName = 新的节点名, 为0则调用失败  <节点名></节点名>
    // pszNodeValue = 节点值  <节点名>节点值</节点名>
    // isCDATA = 如果本参数为真，则节点值文本将被 "<![CDATA[" 和 "]]>" 包围，这意味着该文本可以包含任意字符(以不影响CDATA解析为限}
    inline bool InsertChildNode(tinyxml2::XMLNode* parentNode, const_pointer pszNodeName, const_pointer pszNodeValue = 0, bool isCDATA = false, tinyxml2::XMLNode** ppNode = 0)
    {
        if (!m_doc || !pszNodeName || !parentNode)return 0;
        tinyxml2::XMLElement* child = m_doc->NewElement(ToAnsi(pszNodeName));
        if (pszNodeValue && pszNodeValue[0])
        {
            if (isCDATA)
            {
                tinyxml2::XMLText* CDATA = m_doc->NewText(ToAnsi(pszNodeValue));
                CDATA->SetCData(true);
                parentNode->InsertEndChild(CDATA);
            }
            else
            {
                child->SetText(ToAnsi(pszNodeValue));
            }
        }
        tinyxml2::XMLNode* ret = 0;
        ret = parentNode->InsertEndChild(child);
        if (ppNode)*ppNode = ret;
        return ret != 0;
    }

    // 插入节点, 插入到路径节点后面
    // pszPath 最后的路径为@0 则加入到第一个节点, @-1 则加入到最后一个节点, 否则加入到 指定路径的下一个节点
    // pszNodeName = 新的节点名, 为0则调用失败  <节点名></节点名>
    // pszNodeValue = 节点值  <节点名>节点值</节点名>
    // isCDATA = 如果本参数为真，则节点值文本将被 "<![CDATA[" 和 "]]>" 包围，这意味着该文本可以包含任意字符(以不影响CDATA解析为限}
    inline bool InsertNode(const_pointer pszPath, const_pointer pszNodeName, const_pointer pszNodeValue = 0, bool isCDATA = false, tinyxml2::XMLNode** ppNode = 0)
    {
        if (!m_doc || !pszNodeName)return 0;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return 0;
        tinyxml2::XMLElement* child = m_doc->NewElement(ToAnsi(pszNodeName));

        if (pszNodeValue && pszNodeValue[0])
        {
            if (isCDATA)
            {
                tinyxml2::XMLText* CDATA = m_doc->NewText(ToAnsi(pszNodeValue));
                CDATA->SetCData(true);
                node->InsertEndChild(CDATA);
            }
            else
            {
                child->SetText(pszNodeValue);
            }
        }

        tinyxml2::XMLNode* Parent = node->Parent();
        tinyxml2::XMLNode* ret = 0;
        const char* path = ToAnsi(pszPath);
        size_t len = strlen(path);
        if (strcmp(path + len - 2, "@0") == 0)       // 第0个, 插入到最前面
        {
            ret = Parent->InsertFirstChild(child);
        }
        else if (strcmp(path + len - 3, "@-1") == 0) // 插入到最后
        {
            ret = Parent->InsertEndChild(child);
        }
        else
        {
            ret = Parent->InsertAfterChild(node, child);
        }
        if (ppNode)*ppNode = ret;
        return ret != 0;
    }

    // 插入属性, 在参数节点中插入一个新的属性，并给属性赋值
    // node = 节点指针, 从这个节点加入属性
    // pszAttrName = 新属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // pszAttrValue = 属性值  <节点名 属性名="属性值"/>
    // isAttribute = 要获取的这个属性是否存在, 可以为0, 如果属性不存在, 这个值为0, 存在则为1
    inline bool InsertAttr(tinyxml2::XMLNode* node, const_pointer pszAttrName, const_pointer pszAttrValue, BOOL* isAttribute = 0)
    {
        if (!node || !pszAttrName || !pszAttrName[0]) return false;
        tinyxml2::XMLElement* ele = node->ToElement();
        if (!ele)return false;
        LPCSTR name = ToAnsi(pszAttrName);
        LPCSTR value = ToAnsi(pszAttrValue);
        if (isAttribute)
            *isAttribute = (ele->FindAttribute(name) != 0);
        
        ele->SetAttribute(name, value);
        return true;
    }
    // 插入属性, 在参数节点中插入一个新的属性，并给属性赋值
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszAttrName = 新属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    // pszAttrValue = 属性值  <节点名 属性名="属性值"/>
    inline bool InsertAttr(const_pointer pszPath, const_pointer pszAttrName, const_pointer pszAttrValue, BOOL* isAttribute = 0)
    {
        if (!m_doc || !pszAttrName || !pszAttrName[0])return false;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return false;
        return InsertAttr(node, pszAttrName, pszAttrValue, isAttribute);
    }

    // 删除节点, 删除给定节点和他的所有子节点
    // node = 节点指针, 删除这个节点
    inline bool DelNode(tinyxml2::XMLNode* node)
    {
        if (!node)return false;
        tinyxml2::XMLDocument* doc = node->GetDocument();
        if (!doc)return false;
        doc->DeleteNode(node);
        return true;
    }
    // 删除节点, 删除给定节点和他的所有子节点
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    inline bool DelNode(const_pointer pszPath)
    {
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return false;
        return DelNode(node);
    }

    // 删除节点, 删除给定节点和他的所有子节点
    // index = 节点索引, 索引从0开始, 0=根节点
    inline bool DelNode(int index)
    {
        tinyxml2::XMLNode* node;
        const_pointer name = to_index(index + 1);
        if (!GetNode(name, node)) return false;
        return DelNode(node);
    }

    // 删除属性
    // node = 节点指针, 从这个节点删除属性
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    inline bool DelAttr(tinyxml2::XMLNode* node, const_pointer pszAttrName)
    {
        if (!node || !pszAttrName || !pszAttrName[0])return false;
        tinyxml2::XMLElement* ele = node->ToElement();
        ele->DeleteAttribute(ToAnsi(pszAttrName));
        return true;
    }
    // 删除属性
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszAttrName = 属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    inline bool DelAttr(const_pointer pszPath, const_pointer pszAttrName)
    {
        if (!m_doc || !pszAttrName || !pszAttrName[0])return false;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return false;
        return DelAttr(node, pszAttrName);
    }

    // 修改节点名, 修改指定节点的名字
    // node = 节点指针, 从这个节点设置节点名
    // pszNodeName = 新节点名, 为0则调用失败  <节点名 属性名="属性值"/>
    inline bool SetNodeName(tinyxml2::XMLNode* node, const_pointer pszNodeName)
    {
        if (!m_doc || !pszNodeName)return false;
        //if (!pszNodeName)pszNodeName = "";
        node->SetValue(ToAnsi(pszNodeName));
        return true;
    }
    // 修改节点名, 修改指定节点的名字
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszNodeName = 新节点名, 为0则调用失败  <节点名 属性名="属性值"/>
    inline bool SetNodeName(const_pointer pszPath, const_pointer pszNodeName)
    {
        if (!m_doc || !pszNodeName)return false;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return false;
        return SetNodeName(node, pszNodeName);
    }

    // 修改节点值, 修改指定节点的值
    // node = 节点指针, 从这个节点设置节点值
    // pszNodeValue = 新节点值   <节点名 属性名="属性值">节点值</节点名>
    inline bool SetNodeValue(tinyxml2::XMLNode* node, const_pointer pszNodeValue)
    {
        if (!node)return false;
        if (!pszNodeValue)pszNodeValue = (const_pointer)L"";
        tinyxml2::XMLElement* ele = node->ToElement();
        if (!ele) return false;
        ele->SetText(ToAnsi(pszNodeValue));
        return true;
    }
    // 修改节点值, 修改指定节点的值
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszNodeValue = 新节点值   <节点名 属性名="属性值">节点值</节点名>
    inline bool SetNodeValue(const_pointer pszPath, const_pointer pszNodeValue)
    {
        if (!m_doc)return false;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return false;
        return SetNodeValue(node, pszNodeValue);
    }

    //// 修改属性名, 修改指定节点的属性名称
    //// pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    //// pszOldAttrName = 旧属性名, 为0则调用失败  <节点名 属性名="属性值"/> 
    //// pszNewAttrName = 新属性名, 为0则调用失败  <节点名 属性名="属性值"/>
    //inline bool SetAttrName(const_pointer pszPath, const_pointer pszOldAttrName, const_pointer pszNewAttrName)
    //{
    //    tinyxml2::XMLNode* node;
    //    if (!GetNode(pszPath, node)) return false;
    //    const tinyxml2::XMLAttribute* attr;
    //    tinyxml2::XMLElement* ele;
    //    if (!GetAttr(node, ele, pszOldAttrName, attr))return false;
    //    //attr->SetName(pszNewAttrName);
    //    return true;
    //}


    // 修改属性值, 修改指定节点的属性值
    // node = 节点指针, 从这个节点设置属性值
    // pszAttrName = 旧属性值, 为0则调用失败  <节点名 属性名="属性值"/> 
    // nNewAttrValue = 新属性值  <节点名 属性名="属性值"/>
    // isAttribute = 接收这个属性是否存在
    inline bool SetAttrValue(tinyxml2::XMLNode* node, const_pointer pszAttrName, int nNewAttrValue, BOOL* isAttribute = 0)
    {
        return SetAttrValue(node, pszAttrName, to_string(nNewAttrValue), isAttribute);
    }
    // 修改属性值, 修改指定节点的属性值
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszAttrName = 旧属性值, 为0则调用失败  <节点名 属性名="属性值"/> 
    // nNewAttrValue = 新属性值  <节点名 属性名="属性值"/>
    // isAttribute = 接收这个属性是否存在
    inline bool SetAttrValue(const_pointer pszPath, const_pointer pszAttrName, int nNewAttrValue, BOOL* isAttribute = 0)
    {
        return SetAttrValue(pszPath, pszAttrName, to_string(nNewAttrValue), isAttribute);
    }
    // 修改属性值, 修改指定节点的属性值
    // index = 索引, 索引从0开始
    // pszAttrName = 旧属性值, 为0则调用失败  <节点名 属性名="属性值"/> 
    // nNewAttrValue = 新属性值  <节点名 属性名="属性值"/>
    // isAttribute = 接收这个属性是否存在
    inline bool SetAttrValue(int index, const_pointer pszAttrName, int nNewAttrValue, BOOL* isAttribute = 0)
    {
        return SetAttrValue(to_index(index + 1), pszAttrName, to_string(nNewAttrValue), isAttribute);
    }


    // 修改属性值, 修改指定节点的属性值
    // node = 节点指针, 从这个节点设置属性值
    // pszAttrName = 旧属性值, 为0则调用失败  <节点名 属性名="属性值"/> 
    // pszAttrValue = 新属性值  <节点名 属性名="属性值"/>
    // isAttribute = 接收这个属性是否存在
    inline bool SetAttrValue(tinyxml2::XMLNode* node, const_pointer pszAttrName, const_pointer pszNewAttrValue, BOOL* isAttribute = 0)
    {
        if (!node || !pszAttrName) return false;
        if (!pszNewAttrValue) pszNewAttrValue = (const_pointer)L"";
        tinyxml2::XMLElement* ele = node->ToElement();
        if (!ele)return false;
        LPCSTR name = ToAnsi(pszAttrName);
        LPCSTR value = ToAnsi(pszNewAttrValue);

        if (isAttribute)
            *isAttribute = (ele->FindAttribute(name) != 0);
        

        ele->SetAttribute(name, value);
        return true;
    }
    // 修改属性值, 修改指定节点的属性值
    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    // pszAttrName = 旧属性值, 为0则调用失败  <节点名 属性名="属性值"/> 
    // pszAttrValue = 新属性值  <节点名 属性名="属性值"/>
    // isAttribute = 接收这个属性是否存在
    inline bool SetAttrValue(const_pointer pszPath, const_pointer pszAttrName, const_pointer pszAttrValue, BOOL* isAttribute = 0)
    {
        if (!pszAttrValue)return false;
        tinyxml2::XMLNode* node;
        if (!GetNode(pszPath, node)) return false;
        return SetAttrValue(node, pszAttrName, pszAttrValue, isAttribute);
    }
    // 修改属性值, 修改指定节点的属性值
    // index = 索引, 索引从0开始
    // pszAttrName = 旧属性值, 为0则调用失败  <节点名 属性名="属性值"/> 
    // pszAttrValue = 新属性值  <节点名 属性名="属性值"/>
    // isAttribute = 接收这个属性是否存在
    inline bool SetAttrValue(int index, const_pointer pszAttrName, const_pointer pszAttrValue, BOOL* isAttribute = 0)
    {
        if (!pszAttrValue)return false;
        tinyxml2::XMLNode* node;
        if (!GetNode(to_index(index + 1), node)) return false;
        return SetAttrValue(node, pszAttrName, pszAttrValue, isAttribute);
    }
    // 获取xml数据, 返回xml文本
    inline const_pointer GetXmlData()
    {
        if (!m_doc) return 0;
        tinyxml2::XMLPrinter p;
        m_doc->Print(&p);
        return ToPointer(p.CStr());
    }

public:
    // 测试两个节点是否相同, 单不测试子节点, 2个节点不必位于同一Document中
    // 注意：如果在XMLDocument上调用，则将返回false。.
    inline bool Compare(const tinyxml2::XMLNode* compare) const
    {
        return m_doc->ShallowEqual(compare);
    }
public:

    // pszPath = 节点全路径, 根节点/子节点/叶子节点, 或者 @1/@1/@2, 为0则为根节点
    inline bool GetNode(const_pointer pszPath, tinyxml2::XMLNode*& node) const
    {
        if (!m_doc)return false;
        strArrA arr;
        const char* pszPathA = ToAnsi(pszPath);
        size_t len = _str::strtok(pszPathA, &arr, "/");
        node = m_doc->RootElement();
        if (!node)return false;
        if (len == 0)
        {
            if (pszPathA)
                node = node->FirstChild();
            return node != 0;
        }

        const char* name = node->Value();
        if (strcmp(name, arr[0]) != 0 && strcmp("@1", arr[0]) != 0) return false;   // 第一个不是根节点或者不是 @1 直接返回假

        for (size_t i = 1; i < len && node; i++)	// 从根节点起第二个节点开始遍历
        {
            const char* p = arr[i];
            if (p[0] == '@')
            {
                node = node->FirstChild();
                if (!node)return 0;
                int num = atoi(p + 1);
                for (int n = 1; n < num && node; n++)	// 遍历子节点
                {
                    node = node->NextSibling();
                }
            }
            else
            {
                node = node->FirstChildElement(p);
            }
        }
        return node != 0;
    }


public:
#if defined( _DEBUG ) || defined (__DEBUG__)

#define __TINYXML2_DEBUG_STRING \
L"<?xml version=\"1.0\"?>\n"                                                    \
"<根节点>\n"                                                                     \
"	<CaptureFilter>\n"                                                          \
"		<Module Name=\"Kernel32.dll\">\n"                                       \
"			<Api Name=\"QueryPerformanceCounter\"/>\n"                          \
"			<Api Name=\"QueryPerformanceFrequency\"/>\n"                        \
"		</Module>\n"                                                            \
"		<Module Name=\"User32.dll\">\n"                                         \
"			<Api Name=\"AdjustWindowRect\"/>\n"                                 \
"			<Api Name=\"AdjustWindowRectEx\"/>\n"                               \
"			<Api Name=\"AllowActivateDetachQueuesSetFocus\"/>\n"                \
"		</Module>\n"                                                            \
"	</CaptureFilter>\n"                                                         \
"	<test Name=\"123中文test\" />\n"                                             \
"	<test1 Name=\"123中文test1\">这里是节点值</test1>\n"                           \
"	<test2 Name=\"123中文test2\" 属性名=\"属性值test2\">这里是节点值test2</test2>\n" \
"	<需要取这里 属性名=\"属性值\">这里是节点值 中文</需要取这里>\n"                     \
"	<test2 Name=\"123中文22\" 属性名=\"属性值\">这里是节点值22</test2>\n"            \
"</根节点>\n"


    // 测试程序, 写进代码里, 方便下次查看
    inline void debug_test(CTinyXml2_IUnkown& xml)
    {
        bool b = xml.Parse(ToPointer(__TINYXML2_DEBUG_STRING));
        const_pointer aa1 = xml.GetAttrValue(ToPointer(L"@1/@1/@1/@2"), ToPointer(L"Name")); // QueryPerformanceFrequency
        const_pointer aa2 = xml.GetAttrValue(ToPointer(L"@1/@1/@2"), ToPointer(L"Name"));    // User32.dll
        const_pointer aa3 = xml.GetAttrValue(ToPointer(L"@1/@1/@2/@3"), ToPointer(L"Name")); // AllowActivateDetachQueuesSetFocus

        const_pointer bb1 = xml.GetAttrValue(ToPointer(L"根节点/test1"), ToPointer(L"Name")); // 123中文test1
        const_pointer bb2 = xml.GetAttrValue(ToPointer(L"根节点/@5"), ToPointer(L"属性名"));  // 属性值
        const_pointer bb3 = xml.GetAttrValue(ToPointer(L"@1/@6"), ToPointer(L"Name"));      // 123中文22
        const_pointer bb4 = xml.GetAttrValue(ToPointer(L"@1/@6"), ToPointer(L"属性名"));    // 属性值

        const_pointer as1 = xml.GetNodeName(ToPointer(L"@1/@5"));           // 需要取这里
        const_pointer as2 = xml.GetNodeValue(ToPointer(L"@1/需要取这里"));    // 这里是节点值 中文

        std::vector<const_pointer> values;
        std::vector<const_pointer> names;
        int count = xml.GetAttrName(ToPointer(L"@1/@6"), &names, &values);


        xml.SetAttrValue(ToPointer(L"@1/@6"), ToPointer(L"属性名"), 0);
        xml.SetNodeName(ToPointer(L"@1/@5"), ToPointer(L"给新的名字"));
        xml.SetNodeValue(ToPointer(L"@1/@6"), ToPointer(L""));
        xml.SetAttrValue(ToPointer(L"@1/@6"), ToPointer(L"吃撒大家看类似的经历卡上"), ToPointer(L"werdfqwe54544sd3f1"));
        xml.DelAttr(ToPointer(L"@1/@6"), ToPointer(L"Name"));

        xml.InsertNode(ToPointer(L"@1/@0"), ToPointer(L"加入新节点"), ToPointer(L"test2子节点的节点值"), true);
        xml.InsertChildNode(ToPointer(L"@1/@3"), ToPointer(L"新子节点"), ToPointer(L"新子节点值"));
        xml.InsertChildNode(ToPointer(L"@1/@3"), ToPointer(L"新子节点"), ToPointer(L"新子节点值"), true);
        xml.InsertNode(ToPointer(L"@1/@0"), ToPointer(L"加入到头部"), ToPointer(L"加入到头部"));
        xml.InsertNode(ToPointer(L"@1/@-1"), ToPointer(L"加入到尾部"), ToPointer(L"加入到尾部"));
        xml.InsertNode(ToPointer(L"@1/@1"), ToPointer(L"这个在哪里"));
        xml.DelNode(ToPointer(L"@1/@3"));

        //xml.m_doc->Print();
        xml.m_doc->Print();
    }
#endif

};

#if __TINYXML_DEBUG__
typedef CTinyXml2_IUnkown   CTinyXml2, CTinyXml2;
typedef CTinyXml2_IUnkown   CTinyXml2U;
typedef CTinyXml2_IUnkown   CTinyXml2W;
#else
typedef CTinyXml2_IUnkown<char, char_type_ansi>         CTinyXml2, CTinyXml2;
typedef CTinyXml2_IUnkown<char, char_type_utf8>         CTinyXml2U;
typedef CTinyXml2_IUnkown<wchar_t, char_type_unicode>   CTinyXml2W;
#endif


#endif // TINYXML2_INCLUDED
