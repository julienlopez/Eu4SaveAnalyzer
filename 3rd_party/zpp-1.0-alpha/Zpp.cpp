/*
**
** $id:$
**
** File: zpp.cpp -- a C++ iostream style interface to .ZIP files.
** uses zlib to do actual decompression (and eventually compression)
**
** uses zlib to do actual decompression (and eventually compression)
** Copyright (C) 1999 Michael Cuddy, Fen's Ende Software. All Rights Reserved
** modifications Copyright (C) 2000-2003 Eero Pajarre
**
** check http://zpp-library.sourceforge.net for latest version
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
** Change Log
** ----------
** $Log: Zpp.cpp,v $
** Revision 1.4  2003/06/23 06:46:30  epajarre
** couple of option related bugs fixed
**
** Revision 1.3  2003/06/22 19:51:21  epajarre
** couple of problems with ZPP_USE_STDIO fixed
**
** Revision 1.2  2003/06/22 19:24:02  epajarre
** Integrated all changes by me, testing still to be done
**
** Revision 1.1.1.1  2003/06/21 19:09:38  epajarre
** Initial import. (0.2) version)
**
** Revision 1.2  1999/08/22 05:21:11  mcuddy
** Revision 1.1  1999/06/10 03:35:00  mcuddy
** Initial revision
** Revision 1.1.1.1  1999/06/10 03:32:18  mcuddy
** Revision 1.2  1999/06/10 03:30:33  mcuddy
** Alpha changes
** Revision 1.1  1999/05/25 05:38:11  mcuddy
** Initial revision
**
*/

#include "zpp.h"		// main header file, includes all "interesting" things.

// #include "zlib/zlib.h"
#include "zlib.h"
#include "zpputil.h"

/////////////////////////////////////////////////////////////////////////
//
// zppZipFileInfo implementation
//
/////////////////////////////////////////////////////////////////////////

// seek to, and read local file header, return in 'hdr'.
// throws a zppError on failure.
void zppZipFileInfo::getLclHeader(zppLocalFileHeader *hdr)
{
	if (fp) throw zppError("can't get local file header for filesystem files");
	parentZip->getLclHeader(lclHdrOff,hdr);
}

// seek to, and read central dir header.  return in 'hdr'.
// throws a zppError on failure.
void zppZipFileInfo::getCentralHeader(zppCentralDirFileHeader *hdr)
{
	if (fp) throw zppError("can't get central dir header for filesystem files");
	parentZip->getCentralHeader(centralDirOff,hdr);
}

inline bool zppZipFileInfo::rawRead(long _P, char *_S, std::streamsize _N) {
	if (fp) {
#ifdef ZPP_USE_STDIO
                fseek(fp,_P,SEEK_SET);
                return _N==(int)fread((void *)_S,1,_N,fp);
#else
		fp->seekg(_P,std::ios::beg);
		return !(fp->read(_S,_N).fail());
#endif
	} else {
		if (!parentZip->rawSeek(dataOffset + _P)) return false;
		return parentZip->rawRead(_S,_N);
	}
}

inline int zppZipFileInfo::getPriority()
{
	if (fp) return -1;
	return parentZip->getPriority();
}

zppZipFileInfo::zppZipFileInfo(const char *file)
{
	if (!_InitFromFile(std::string(file))) throw zppError("can't open file");
}

zppZipFileInfo::zppZipFileInfo(const std::string &file)
{
	if (!_InitFromFile(file)) throw zppError("can't open file");
}

bool zppZipFileInfo::_InitFromFile(const std::string &_fileName)
{
#ifdef ZPP_USE_STDIO
        fp = fopen(_fileName.c_str(),"rb");
	if (!fp) return false;
#else
	fp = new std::ifstream;
 	fp->open(_fileName.c_str(),std::ios::in|std::ios::binary);
	if (fp->fail()) return false;
#endif
	// save central directory offset
	centralDirOff = 0;
#ifdef ZPP_INCLUDE_CRYPT
	isEncrypted = false;	// regular files are not encrypted
#endif /* ZPP_INCLUDE_CRYPT */

	centralDirSize = 0;

	// save storage method and other userful information
	method = ZPP_STORED;

	// get size of file by seeking to end and back.
#ifdef ZPP_USE_STDIO
        fseek(fp,0,SEEK_END);
	realSize = ftell(fp); cmpSize = realSize;
        fseek(fp,0,SEEK_SET);
#else
	fp->seekg(0,std::ios::end);
	realSize = fp->tellg(); cmpSize = realSize;
	fp->seekg(0,std::ios::beg);
#endif
	// save filename
	fileName = _fileName;

	lclHdrOff = 0;

	// store the information that we really care about; other info
	// which is not used as much isn't stored, but can be gotten
	// via a member function.
	dataOffset = 0;
	
	return true;
}

zppZipFileInfo::zppZipFileInfo(long cdoff, zppZipArchive *parent) :
	parentZip(parent) 
{
	char tmpBuf[128];
    zppLocalFileHeader 		lclHeader;	// the physical file header
	zppCentralDirFileHeader cdHeader;	// central directory header

#ifdef ZPP_INCLUDE_CRYPT
	isEncrypted = false;
#endif
	fp = NULL;

	// save central directory offset
	centralDirOff = cdoff;

	// get central header
	getCentralHeader(&cdHeader);

	centralDirSize = sizeof(cdHeader) + cdHeader.fnLength +
				cdHeader.extraLength + cdHeader.commentLength;

	// save storage method and other userful information
	method = (zppComprMethod) cdHeader.method;
	realSize = cdHeader.realSize;
	cmpSize = cdHeader.cmpSize;

	// now read filename...
	if (! parentZip->rawRead(tmpBuf,cdHeader.fnLength))
		throw zppError("Can't read filename from central directory");

	// make sure that filename is nul terminated
	tmpBuf[cdHeader.fnLength] = '\0';

	// and copy into our string
	fileName = tmpBuf;

	lclHdrOff = cdHeader.lclHdrOffset;

	getLclHeader(&lclHeader);

#ifdef ZPP_INCLUDE_CRYPT
	// check if file is encrypted
	if (lclHeader.bitFlag & (1<<0)) isEncrypted = true;
#endif /* ZPP_INCLUDE_CRYPT */

	// store the information that we really care about; other info
	// which is not used as much isn't stored, but can be gotten
	// via a member function.
	dataOffset = cdHeader.lclHdrOffset + sizeof(lclHeader) +
		lclHeader.fnLength + lclHeader.extraLength;

	// verify that the local header matches the cd header.
	
    if (lclHeader.version != cdHeader.version)
		throw zppError("local/central header mismatch: version");
    if (lclHeader.bitFlag != cdHeader.bitFlag)
		throw zppError("local/central header mismatch: bitFlag");
    if (lclHeader.method != cdHeader.method)
		throw zppError("local/central header mismatch: method");
    if (lclHeader.modTime != cdHeader.modTime)
		throw zppError("local/central header mismatch: modTime");
    if (lclHeader.modDate != cdHeader.modDate)
		throw zppError("local/central header mismatch: modDate");
    if (lclHeader.crc32 != cdHeader.crc32)
		throw zppError("local/central header mismatch: crc32");
    if (lclHeader.cmpSize != cdHeader.cmpSize)
		throw zppError("local/central header mismatch: cmpSize");
    if (lclHeader.realSize != cdHeader.realSize)
		throw zppError("local/central header mismatch: realSize");
    if (lclHeader.fnLength != cdHeader.fnLength)
		throw zppError("local/central header mismatch: fnLength");
    if (lclHeader.extraLength != cdHeader.extraLength)
		throw zppError("local/central header mismatch: extraLength");

	//
	// printf("'%s', magic = %x, vers = %x, extVer = %x, bitFlag = %x, method=%d\n"
	//	   "  modTime=%d, modDate=%d, crc32=%x, cmpSize=%x, realsz=%x,\n"
	//	   "  fnLen=%d, extraLen=%d, cmntLen=%d, disknum=%d, intattr=%x,\n"
	//	   "  extattr=%d, lclhdroff=%d, dataoff=%d\n",
    //		fileName.c_str(),
	//		cdHeader.magicNumber, cdHeader.versMadeBy, cdHeader.version, cdHeader.bitFlag, cdHeader.method,
    //		cdHeader.modTime, cdHeader.modDate, cdHeader.crc32, cdHeader.cmpSize, cdHeader.realSize,
    //		cdHeader.fnLength, cdHeader.extraLength, cdHeader.commentLength, cdHeader.diskNum, cdHeader.intAttrib,
    //		cdHeader.extAttrib, cdHeader.lclHdrOffset, dataOffset);
}

// dtor
zppZipFileInfo::~zppZipFileInfo()
{
#ifdef ZPP_USE_STDIO
	if (fp) fclose(fp); fp = NULL;
#else
	if (fp) delete (fp); fp = NULL;
#endif
}

/////////////////////////////////////////////////////////////////////////
//
// zppZipArchive implementation
//
/////////////////////////////////////////////////////////////////////////

//
// class static data
//
int zppZipArchive::defaultPriority = 0;
zppFileMap zppZipArchive::globalMap;
bool zppZipArchive::parseAttrFlag = true;

// make a canonical version of a pathname -- downcased and '\'
// converted to '/'.  modifies the string in place!
void zppZipArchive::canonizePath(std::string &str)
{
	std::string::iterator i;
#ifdef ZPP_IGNORE_DRIVE_LETTER  
       // remove x:/ from the start of a path
	if (str.length()>3 &&
	    str[1]==':' &&
	    str[2]=='/'){
	  str.erase(str.begin());
	  str.erase(str.begin());
	  str.erase(str.begin());
	}
#endif
	for ( i = str.begin(); i != str.end(); i++ ) {
		if ( (*i) == '\\' ) (*i) = '/';
		else (*i) = tolower(*i);
	}
}


//
// seek to, and read local header for a file at 'offset'.  read data
// into 'hdr'.  on failure, throw a zppError.
//
void zppZipArchive::getLclHeader(long offset, zppLocalFileHeader *hdr)
{
	// read local header and make sure it jibes with central dir header
	if (!rawSeek(offset))
		throw zppError("Can't seek to local file header");

	if (!rawRead((char*)hdr,sizeof(zppLocalFileHeader)))
		throw zppError("Can't read local header");

	if (hdr->magicNumber != ZPP_LOCAL_FILE_MAGIC)
		throw zppError("Local file header invalid magic number");
}

//
// seek to, and read central directory file header for a file at 'offset'.
// read data into 'hdr'.  on failure, throw a zppError.
//
void zppZipArchive::getCentralHeader(long offset, zppCentralDirFileHeader *hdr)
{
	// first, seek to position in central directory.
	if (!rawSeek(offset))
		throw zppError("Can't seek to central directory record");

	// and read central directory header.
	if (!rawRead((char*)hdr,sizeof(zppCentralDirFileHeader)))
		throw zppError("Can't read central directory record");

	// make sure that magic number matches.
	if (hdr->magicNumber != ZPP_CENTRAL_FILE_HEADER_MAGIC)
	    throw zppError("Invalid magic number in central file header");
}

//
// Locate the Central directory of a zipfile (at the end, just before
// the global comment).  This code shamelessly stolen from minizip by
// Giles Vollant.
//
#define ZPP_MAX_COMMENT_SIZE (0xFFFF)
#define ZPP_SEARCH_BUF_SIZE (0x400)
long zppZipArchive::searchCentralDir(long fileSize)
{
    char buf[ZPP_SEARCH_BUF_SIZE + 4];
    long backRead;
    long maxBack=ZPP_MAX_COMMENT_SIZE; /* maximum size of global comment */
    long posFound=0;
    
	if (!rawSeek(fileSize)) {
		// printf("zppSearchCentralDir: can't seek.\n");
		return 0;
	}

    if (maxBack>fileSize) maxBack = fileSize;

    backRead = 4;
    while (backRead<maxBack)
    {
		long readSize,readPos ;
		int i;
		if (backRead+ZPP_SEARCH_BUF_SIZE>maxBack)
			backRead = maxBack;
		else
			backRead+=ZPP_SEARCH_BUF_SIZE;
	
		readPos = fileSize-backRead ;
	
		readSize = ((ZPP_SEARCH_BUF_SIZE+4) < (fileSize-readPos)) ?
			 (ZPP_SEARCH_BUF_SIZE+4) : (fileSize-readPos);

		// printf("seek to %d\n",readPos);
		if (!rawSeek(readPos)) {
			// printf("can't seek to %d\n",readPos);
			break;
		}
		// printf("pos = %d\n",str->tellg());

		// printf("read %d bytes\n",readSize);
		if (!rawRead((char *)buf,readSize)) {
			// printf("can't read %d bytes.\n",readSize);
			break;
		}

		for (i=(int)readSize-3; (i--)>0;) {
			if (((*(buf+i))==0x50) && ((*(buf+i+1))==0x4b) && 
			((*(buf+i+2))==0x05) && ((*(buf+i+3))==0x06)) {
				// printf("found ECD at %d\n",readPos+i);
				posFound = readPos+i;
				break;
			}
		}
		if (posFound!=0) break;
	}
    return posFound;
}

void zppZipArchive::dumpGlobalMap()
{
	zppFileMap::iterator i;

	fprintf(stderr,"--- global map ---\n");
	for ( i = globalMap.begin(); i != globalMap.end(); i++ ) {
		fprintf(stderr,"  %s (%s prio %d from %s)\n",
			(*i).first.c_str(),
			(*i).second->getName().c_str(),
			(*i).second->getPriority(),
			(*i).second->getParentZip()->getName().c_str());
	}
	fprintf(stderr,"--- end of global map ---\n");
}

// this reads the zipfile comment and parses it into a key->value map.
void zppZipArchive::parseAttrMap()
{
	int len = getCommentLength();
	int line;
	char *wstart, *bol, *p, *eol;
	std::string key, data;

	if (len == 0) return;	// no comment, we're done.

	char *cmntBuf = new char[len+1];

	// get comment into buffer (we could do this in chunks, but
	// for now we'll just snag the whole thing.)
	getComment(cmntBuf);

	cmntBuf[len] = '\0';	// nul terminate.

	if (strncmp(cmntBuf,"%ZPP%",4)!=0) {
		// printf("comment buf didn't match magic string; not parsing.\n");
		delete[] cmntBuf;
		return;
	}

	// parse the whole comment.
	// I could put this in a string and use getline(), but I'm not sure
	// that I'm guaranteed that getline() will deal with all three types
	// of likely line-ending characters (MAC: \r  UNIX: \n and PC: \r\n)
	// so I do it the old fashioned way.
	eol = cmntBuf; line = 0;
	try {
		while (1) {
			if (*eol == '\0') break; // we're done.
			// isolate a line.
			bol = eol;
			line++;
			while (*eol && *eol != '\n' && *eol != '\r') eol++;
	
			// if we get a CR, it's a DOS style file or a MAC style file
			// make sure we clobber the eol char and skip it.
			if (*eol == '\r') { *eol++ = '\0'; } // mac or first PC eol char
			if (*eol == '\n') { *eol++ = '\0'; } // unix or second DOS eol char
	
			// debug: print line.
			// printf("%d: '%s'\n", line, bol);
	
			// start at beginning of line..
			p = bol;
			while (*p && isspace(*p)) p++;
	
			// at end of line or comment character, go get next line.
			if (*p==0 || *p == '#' || *p == '%') continue;
	
			// mark start of word
			wstart = p;
	
			// skip to first space or '='
			while (*p && !isspace(*p) && *p != '=') p++;
	
			if (*p == 0)
				throw zppError("syntax error parsing zipcomment: didn't find '='");
	
			// not at the "="? clobber end-of-token and keep looking.
			if (*p != '=') {
	
				// clobber whitespace trailing token name
				*p++ = '\0';
	
				// it's gotta just be spaces til' the token.
				while (*p && isspace(*p) && *p != '=') p++;
				if (*p != '=')
					throw zppError("syntax error parsing zipcomment: expecting '='");
	
			}
			// clobber and skip "="
			*p++ = 0;
	
			// create string to store token
			key = wstart;
	
			// printf("got token '%s'\n",key.c_str());
	
			// now look for a quote (" or ') to start a string or a non-ws
			// character.
			while (*p && isspace(*p)) p++;
			if (*p == '\0') 
				throw zppError("syntax error parsing zip comment: premature end of line\n");
	
			// if we got a quote (single or double)...
			if (*p == '"' || *p == '\'') {
				// ... we're parsing a string
				char termChar = *p;
	
				p++; // skip "open quote"
				wstart = p;
				while (*p && *p != termChar) p++;
				if (!*p) 
					throw zppError("syntax error parsing zip comment: missing string terminator\n");
	
				// clobber terminator.
				*p++ = '\0';
				data = std::string(wstart);
				// printf("got data %c%s%c\n",termChar,data.c_str(),termChar);
	
			} else {
				// otherwise, we're just grabbing a "token", eat up to first ws
				wstart = p;
				while (*p && !isspace(*p)) p++;
				if (*p) *p++ = '\0'; // clobber first whitespace
				data = std::string(wstart);
				// printf("got data <%s>\n",data.c_str());
			}
			// put key in map (overwrite duplicate keys) -- maybe we should
			// make this a multimap?
			attrMap[key] = data;
		}
	} catch (zppError e) {
		// if there was a parsing error, cleanup and re-throw
		delete[] cmntBuf;
		throw e;
	}
}

void zppZipArchive::parseZipDirectory()
{
    long eofOffset;
	std::string dirPrefix;

	// make sure that vector of files is empty
	files.clear();
	fileMap.clear();

	if (!rawSeek(0,std::ios_base::end))
		throw zppError("zppParseZip: can't seek on stream.");

#ifdef ZPP_USE_STDIO
	eofOffset = ftell(str);
	current_pos_v = eofOffset;
#else
	eofOffset = str->tellg();
#endif
	if (eofOffset < 0) 
		throw zppError("zppParseZip: can't tellg() on stream.");

	// printf("end of file= %d\n",eofOffset);

    // first, find and then read end-of-central directory structure.
    if ((ecdOffset = searchCentralDir(eofOffset)) == 0)
		throw zppError("zppParseZip: can't find end-of-central directory record");

	if (!rawSeek(ecdOffset))
		throw zppError("zppParseZip: can't seek to end-of-central directory");

	// printf("sizeof ECD = %d\n",sizeof(ecdHeader));
    if (!rawRead((char*)&ecdHeader,sizeof(ecdHeader)))
		throw zppError("zppParseZip: can't read end-of-central directory record");

    if (ecdHeader.magicNumber != ZPP_END_OF_CENTRAL_DIR_MAGIC)
		throw zppError("zppParseZip: invalid end of central dir magic #");

    // printf("magic = %x, disk = %d, dirDisk = %d, cntThisDsk = %d,\n"
    //	   "cntTotal = %d, dirSize = %d, dirOfffset = %d, cmntLen=%d\n",
	//    ecdHeader.magicNumber, ecdHeader.diskNum, ecdHeader.dirDiskNum, ecdHeader.entryCntThisDisk,
	//    ecdHeader.entryCntTotal, ecdHeader.dirSize, ecdHeader.dirOffset, ecdHeader.commentLength);

	// now read central directory..
	if (ecdHeader.diskNum != 0)
		throw zppError("zppParseZip: multi-disk archives not supported.");

	// if needed, parse attribute map.
	if (parseAttrFlag) {
		try {
			parseAttrMap();
		} catch (zppError e) {
			printf("%s\n",e.str.c_str());
		}
	}

	zppStrStrMap::iterator attr;

	attr = attrMap.find("ZPP_PRIORITY");
	if (attr != attrMap.end()) {
		int p = atoi((*attr).second.c_str());
		// printf("Got priority '%s' (%d) from zipcomment\n", (*attr).second.c_str(), p);
		priority = p;
	}

	attr = attrMap.find("ZPP_DIR_PREFIX");
	if (attr != attrMap.end()) {
		dirPrefix = (*attr).second;
		canonizePath(dirPrefix);
		// append a slash if needed.
		// XXX -- should this be "hostified"?
		if (*(dirPrefix.rbegin()) != '/') dirPrefix.append("/");
	} else dirPrefix = "";

	int pos = ecdHeader.dirOffset;
	int i;
	
	files.reserve(ecdHeader.entryCntTotal);
	
	for ( i = 0 ; i < ecdHeader.entryCntTotal; i++ ) {
		// printf("file #%d: pos = %d\n",i, pos);

		zppZipFileInfo file(pos,this);

		files.push_back(file);

		// advance position in directory by size of this object.
		pos += file.cdSize(); 
	}

	//
	// here we make a canonical filename -> file reference map.
	// XXX -- think about whether we need this map AND the vector
	// above; might be easier to just have the map.
	//
	zppZipDirectory::iterator iter;

	for (iter = files.begin(); iter != files.end(); iter++) {
		std::string tmp = (*iter).getName();
		canonizePath(tmp);
		// printf("org name = '%s', canonical name = '%s'\n", (*iter).getName().c_str(), tmp.c_str());
		fileMap[tmp] = iter;
		if (isGlobal) {
			//
			// search global directory for this (canonical) file name
			//
			zppFileMap::iterator i2 = globalMap.find(tmp);
			//
			// if we find one, and the priority
			// is less than the current file, replace
			// the existing key with the current file.
			//
			if ( i2 == globalMap.end()) {
				// printf("XXX -- Adding %s prio %d from %s\n", tmp.c_str(), (*iter).getPriority(), (*iter).getParentZip()->getName().c_str());
				globalMap[tmp] = iter;
			} else if ((*i2).second->getPriority() < priority) {

				// printf("XXX -- replaced %s prio %d from %s with prio %d from %s\n",
				//	((*i2).second)->getName().c_str(),
				//	(*i2).second->getPriority(),
				//	(*i2).second->getParentZip()->getName().c_str(),
				//	(*iter).getPriority(),
				//	(*iter).getParentZip()->getName().c_str());

				(*i2).second = iter;
			}
		}
	}
}


// ctor: input is filename to open.
// only std::ios::in is supported currently.
// if _makeGlobal == TRUE, then the ZipFile is added to the list
// of global zip files.
zppZipArchive::zppZipArchive(std::string &_fn, std::ios::openmode _mode, bool _makeGlobal)
{
	ourFile = false;
	zipName = _fn;
	canonizePath(zipName);
#ifdef ZPP_INCLUDE_CRYPT
	passwd = NULL;
#endif
	if (_mode & std::ios_base::in) {
		// make sure "binary" mode is selected.
		_mode |= std::ios_base::binary;
#ifdef ZPP_USE_STDIO
                FILE *infile = fopen(zipName.c_str(),"rb");
	        current_pos_v=0;
#else
		std::ifstream *infile = new std::ifstream();
		infile->open(zipName.c_str(),_mode);
#endif
		ourFile = true;
#ifdef ZPP_USE_STDIO
		if (!infile)
			throw zppError("can't open zip file");
#else
		if (infile->fail())
			throw zppError("can't open zip file");
#endif

		str = infile;
	} else {
		throw ("zppZipArchive:: only std::ios::in currently supported.");
	}

	priority = defaultPriority;
	isGlobal = _makeGlobal;

	// build internal data structures
	parseZipDirectory();
}

#ifndef ZPP_USE_STDIO
// ctor: input is iostream
// (implication: can have nested .zip files if incoming stream is
// a zip file itself; but must be seekable, which implies that
// the compression method is "store" or that the whole file
// fits in memory.
//
// ---------------------------------------------------------------
// NOTE: that by default, zip files constructed in this way are NOT
// made global!
// ---------------------------------------------------------------
//
zppZipArchive::zppZipArchive(std::istream *istr, std::string &_name, bool _makeGlobal) : str(istr)
{
	priority = defaultPriority;
	isGlobal = _makeGlobal;
	zipName = _name;
	canonizePath(zipName);
#ifdef ZPP_INCLUDE_CRYPT
	passwd = NULL;
#endif
	// build internal data structures
	parseZipDirectory();
}
#endif

// dtor.
zppZipArchive::~zppZipArchive()
{
	if (ourFile) {
#ifdef ZPP_USE_STDIO
	        fclose(str);
#else
		delete str;
#endif
	}
	if (isGlobal) {
		// XXX -- we have to remove all zip files that we own from the global list.
		// this could be painful ;-)
	}
}

//unsigned long zppZipArchive::crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
//{
//	return ::crc32(crc,buf,len);
//}

#ifdef ZPP_INCLUDE_CRYPT
void zppCryptState::updateKeys(unsigned char x)
{
	key0 = crc32(key0,&x,1);
	key1 = key1 + (key0 & 0x000000ff);
	key1 = key1 * 0x8088405 + 1;
	unsigned char tmp = key1>>24;
	key2 = crc32(key2,&tmp,1);
}
#endif /* ZPP_INCLUDE_CRYPT */

#ifdef ZPP_INCLUDE_OPENALL
std::list<zppZipArchive *> zppZipArchive::filesWeOpened;

// these functions open all .ZIP files that they can find in
// a specific directory.
// they require support in the form of a function: enumerateDir(),
// which is declared at the top of this file.
int zppZipArchive::openAll(const std::string &_path)
{
	std::list<std::string> *fileList = enumerateDir(_path,true);
	std::list<std::string>::iterator i;
	int count = 0;

	if (fileList == NULL) return 0;
	for (i = fileList->begin(); i != fileList->end(); i++ ) {
		zppZipArchive *zip;

		// open zip as std::ios::in, and global.
		zip = new zppZipArchive(*i);

		if (zip) {
			// got it... add it to our list.
			filesWeOpened.push_back(zip);
			count++;
		}
	}
	delete fileList;
	return count;
}

int zppZipArchive::openAll(char *_path)
{
	return openAll(std::string(_path));
}

		// call this, if you've called openAll()
void zppZipArchive::closeAll()
{
	zppZipArchiveList::iterator i;

	for ( i = filesWeOpened.begin(); i != filesWeOpened.end(); i++ ) {
		// delete open zip file.
		delete (*i);
	}
	filesWeOpened.clear();
}

zppZipArchive *zppZipArchive::findArchive(const std::string &name)
{
	zppZipArchiveList::iterator i;
	std::string tmp = name;
	canonizePath(tmp);

	for ( i = filesWeOpened.begin(); i != filesWeOpened.end(); i++ ) {
		// found zip file?
		if ((*i)->zipName == tmp) return (*i);
	}
	return NULL;
}

#endif /* ZPP_INCLUDE_OPENALL */

int zppZipArchive::getComment(char *pComment, int offset, int size)
{
	int toRead;
	// no comment, no work to do.
	if (ecdHeader.commentLength == 0) return 0;

	if (offset > ecdHeader.commentLength) return 0;
	if (offset < 0) return 0;

	// read the whole thing?
	if (size == -1) toRead = ecdHeader.commentLength;

	// clip read to existing comment data.
	if (toRead + offset > ecdHeader.commentLength)
		toRead = ecdHeader.commentLength - offset;

	// nothing to read? don't.
	if (toRead <= 0) return 0;

	// seek to where we are going to read from.
	if (!rawSeek(ecdOffset + sizeof(zppEndOfCentralDirHeader) + offset))
		throw("Can't seek past End-of-Central-Dir to get comment");

	if (!rawRead(pComment,toRead))
		throw("error reading zip comment");

	return toRead;
}

zppZipFileInfo *zppZipArchive::findInArchive(const std::string filename)
{
	// copy string 'cause canonizePath modifies it inplace.
	std::string tmp = filename;
	canonizePath(tmp);
	zppFileMap::iterator i2 = fileMap.find(tmp);

	if (i2 == fileMap.end()) {
		return NULL;
	}

	// oy, this is ugly -- here's the scoop:
	// (*i2).second is a zppZipDirectory::iterator,
	// dereferencing that gives us a zppZipFileInfo object
	// and we want to return the address of that.
	return (&(*((*i2).second)));
}


// this is the "static" version which searches all globally opened zip files.
zppZipFileInfo *zppZipArchive::find(const std::string &filename)
{
	// copy string 'cause canonizePath modifies it inplace.
	std::string tmp = filename;
	canonizePath(tmp);
	zppFileMap::iterator i2 = globalMap.find(tmp);

	if (i2 == globalMap.end()) return NULL;

	// oy, this is ugly -- here's the scoop:
	// (*i2).second is a zppZipDirectory::iterator,
	// dereferencing that gives us a zppZipFileInfo object
	// and we want to return the address of that.
	zppZipFileInfo &info=*(i2->second);
	return &info;
}

/////////////////////////////////////////////////////////////////////////
//
// zppZipReader implementation
//
/////////////////////////////////////////////////////////////////////////

//
// class static data
//
int zppZipReader::defaultReadBufSize = 16384;

// XXX -- for now, these just use calloc and free, they should use
// handlers setup in the zppZipReader object.
void *zppZipReader::zlibAlloc(void *opaque, unsigned int items, unsigned int size)
{
	return calloc(items,size);
}

// XXX -- for now, these just use calloc and free, they should use
// handlers setup in the zppZipReader object.
void zppZipReader::zlibFree(void *opaque, void *address)
{
	free(address);
}

//
// constructor for a zip-file reader object.
// if readBufSize == 0 && readBuffer == NULL, a default buffer size
// will be allocated.
// if readBuffer != NULL, then readBufSize specifies how much space is
// available in the readbuffer.
//

zppZipReader::zppZipReader(zppZipFileInfo *info, int rbSize, char *rdBuf)
{
	// initialize state info...
	readBufSize = rbSize;
	readBuf = rdBuf;
	ourBuffer = false;
	file = info;

	// if the read buffer size == 0, then use the default size.
	if (readBufSize == 0) {
		// but if the read buffer size is 0, then we're going to alloc a buffer...
		// so it better be null.
		if (readBuf != NULL)
			throw zppError("readBufSize == 0 && readBuffer != NULL");

		readBufSize = defaultReadBufSize;
	}

#ifdef ZPP_INCLUDE_CRYPT
	isCrypt = false;
	if (file->isCrypt() && file->getParentZip() != NULL) {
		zppZipArchive *parent = file->getParentZip();
		if (parent->getPasswd() == NULL) throw zppError("attempt to read encrypted file without passwd set");
		isCrypt = true;
	}
#endif /* ZPP_INCLUDE_CRYPT */

	// if the input read buffer is NULL, we allocate one and take
	// ownership of it.
	if (readBuf == NULL) {
		readBuf = new char[readBufSize];
		ourBuffer = true;
	}

	if (file->getMethod() != ZPP_STORED &&
	    file->getMethod() != ZPP_DEFLATED)
		throw zppError("Invalid compression method");
	
	streamInit = false;
	stream = new z_stream;
	resetStream();

}

void zppZipReader::tidy()
{
	if (streamInit) {
		switch(file->getMethod()) {
			case ZPP_DEFLATED:
				inflateEnd(stream);
				break;
			case ZPP_STORED:
				break;
			default:
				throw zppError("invalid compression method in zppZipReader dtor");
				break;
		}
		streamInit = false;
	}

	memset(stream,'\0',sizeof(stream));
}

void zppZipReader::resetStream()
{
	tidy();

	// init the stream structure for zlib
	stream->zalloc = (alloc_func) zppZipReader::zlibAlloc;
	stream->zfree = (free_func) zppZipReader::zlibFree;
	stream->opaque = (voidpf) this;
    stream->total_out = 0;
	stream->avail_in = 0;

	curPos = 0;							// current pos in decompressed stream
	uncmpLeft = file->getSize();		// uncompressed size of file
	cmpLeft = file->getCmpSize();		// compressed size of file
	streamInit = true;

#ifdef ZPP_INCLUDE_CRYPT
	if (isCrypt) {
		if (!file->rawRead(curPos, readBuf, 12))
			throw zppError("can't read encryption header");
		curPos += 12;
		crypt.initKeys(file->getParentZip()->getPasswd());
		crypt.setHeader((unsigned char*)readBuf);
	}
#endif

	// XXX -- init crc32 here.

	// XXX -- use a decompressor object here, I think.
	switch(file->getMethod()) {
		case ZPP_DEFLATED:
			/*
			** windowBits is passed < 0 to tell that there is no zlib header.
			** Note that in this case inflate *requires* an extra "dummy" byte
			** after the compressed stream in order to complete decompression and
			** return Z_STREAM_END. 
			*/
			if (inflateInit2(stream, -MAX_WBITS) != Z_OK) 
				throw zppError("error in inflateInit2()");
			break;
		case ZPP_STORED:
			break;
		default:
			throw zppError("Invalid decompression type");
			break;
	}
}

// get 'n' bytes from the stream we're decompressing into 'buf',
// return # of bytes gotten. (0 on eof)
long zppZipReader::read(char *buf, long len)
{
	long iRead = 0;		// total bytes read.

	stream->next_out = (Bytef*)buf;
	stream->avail_out = (uInt)len;
	
	// printf("reading %d: cmpLeft=%d, uncmpLeft=%d .. ", len, cmpLeft, uncmpLeft);
	// don't read past EOF.
	if (len > uncmpLeft) stream->avail_out = uncmpLeft;

	// while there's still some data left to get..
	while ( stream->avail_out > 0 ) {

		// if there's no more data left in the input buffer,
		// and we're not at EOF, get some more data.
		if ((stream->avail_in==0) && (cmpLeft > 0)) {

			uInt uReadThis = readBufSize;

			// don't run past end of compressed stream
			if ( (uInt)cmpLeft < uReadThis )
				uReadThis = (uInt)cmpLeft;

			// ah, at end of file.
			if (uReadThis == 0) {
				// printf("returning 0, at end of cmp data.\n");
				return 0;
			}

			// get more data from zip archive
			if (!file->rawRead(curPos, readBuf, uReadThis))
				throw zppError("read error on compressed stream");

#ifdef ZPP_INCLUDE_CRYPT
			if (isCrypt) {
				crypt.decrypt((unsigned char*)readBuf,uReadThis);
			}
#endif

			// adjust data offset and count
			curPos += uReadThis;
			cmpLeft -= uReadThis;
			
			// let zlib know how much data is to be read.
			stream->next_in = (Bytef*)readBuf;
			stream->avail_in = (uInt)uReadThis;
		}

		// now uncompress data into user's buffer
		switch(file->getMethod()) {
			case ZPP_STORED:
				{
					uInt uDoCopy,i ;
					if (stream->avail_out < stream->avail_in)
						uDoCopy = stream->avail_out ;
					else
						uDoCopy = stream->avail_in ;
					
					for (i=0;i<uDoCopy;i++)
						*(stream->next_out+i) = *(stream->next_in+i);
						
					// crc32 = compute_crc32(crc32, stream.next_out, uDoCopy);
	
					uncmpLeft -= uDoCopy;
					stream->avail_in -= uDoCopy;
					stream->avail_out -= uDoCopy;
					stream->next_out += uDoCopy;
					stream->next_in += uDoCopy;
					stream->total_out += uDoCopy;
					iRead += uDoCopy;
				}
				break;
			case ZPP_DEFLATED:
				{
					uLong uTotalOutBefore,uTotalOutAfter;
					const Bytef *bufBefore;
					uLong uOutThis;
					int flush=Z_SYNC_FLUSH;
					int err;

					uTotalOutBefore = stream->total_out;
					bufBefore = stream->next_out;

					err=inflate(stream,flush);

					uTotalOutAfter = stream->total_out;
					uOutThis = uTotalOutAfter-uTotalOutBefore;
			
					// crc32 = compute_crc32(crc32,bufBefore, (uInt)(uOutThis));

					uncmpLeft -= uOutThis;

					iRead += (uInt)(uTotalOutAfter - uTotalOutBefore);
            
					if (err==Z_STREAM_END) {
						// printf("returning %d, ran off end of cmp data.\n", iRead);
						return iRead;
					}
					if (err!=Z_OK)
						throw zppError("zlib error in inflate.");
				}
				break;
			default:
				throw zppError("unknown compression type -- shouldn't have gotten this far!");
				break;
		}
	}
	// fprintf(stderr,"returning %d\n", iRead);
	return iRead;
}

zppZipReader::~zppZipReader()
{
        tidy();
	if (ourBuffer) delete[] readBuf;
	if (stream) delete stream;
}

#if 0
int main()
{
    ifstream fp;
/*
    fp.open("test.zip", ios_base::in | ios_base::binary);

    if (fp.fail()) {
		perror("zpp: can't open test.zip");
		exit(1);
    }
	*/
	/*
	** this test assumes the existance of three .ZIP archives:
	** "test.zip", "test2.zip", and "test3.zip".
	** by putting different copies of "zipfmt.txt" inside of the
	** archives, you can see the priority scheme at work.  note that
	** files on the actual filesystem will override .ZIP files
	*/

	try {
		// build map of files in z.
		zppZipArchive zf1(std::string("test.zip"));
		zppZipArchive::dumpGlobalMap();

		zppZipArchive::setDefaultPriority(20);
		zppZipArchive zf2(std::string("test2.zip"));
		zppZipArchive::dumpGlobalMap();

		zppZipArchive::setDefaultPriority(10);
		zppZipArchive zf3(std::string("test3.zip"));
		zppZipArchive::dumpGlobalMap();

		zppZipFileInfo *i;

		// find the file in the archive
		i = zf1.find("zipfmt.txt");

		if (i == NULL) throw zppError("can't find file zipfmt.txt");

		printf("<<contents of zipfmt.txt>>\n");
		{
			zppZipReader r(i);
			char buf[13];
			int cnt;
			while ((cnt = r.read(buf,13)) > 0) {
				fwrite(buf,1,cnt,stdout);
			}
			printf("<< end of first copy >>\n");
			r.resetStream();
			while ((cnt = r.read(buf,13)) > 0) {
				fwrite(buf,1,cnt,stdout);
			}
			
		}
		printf("<<end of zipfmt.txt>>\n");

		fp.close();
	} catch ( zppError e ) {
		printf("zppError: %s\n",e.str.c_str());
	}

    return 0;
}
#endif /* 0 */
