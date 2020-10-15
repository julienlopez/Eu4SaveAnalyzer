/*
**
** $id:$
**
** File: izstream.cpp -- a C++ iostream style interface to .ZIP files.
** uses zlib to do actual decompression (and eventually compression)
**
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
** $Log: izstream.cpp,v $
** Revision 1.3  2003/06/23 06:46:30  epajarre
** couple of option related bugs fixed
**
** Revision 1.2  2003/06/22 19:24:02  epajarre
** Integrated all changes by me, testing still to be done
**
** Revision 1.1.1.1  2003/06/21 19:09:37  epajarre
** Initial import. (0.2) version)
**
** Revision 1.2  1999/08/22 05:21:12  mcuddy
** Revision 1.1  1999/06/10 03:35:12  mcuddy
** Initial revision
** Revision 1.2  1999/06/10 03:30:43  mcuddy
** Alpha changes
** Revision 1.1  1999/05/25 05:38:29  mcuddy
** Initial revision
**
*/
#include "zpp.h"


#if _MSC_VER <= 1200
#pragma warning ( disable : 4786 )
#endif /* _MSC_VER */

/////////////////////////////////////////////////////////////////////////
//
// zppstreambuf implementation
//
/////////////////////////////////////////////////////////////////////////

int zppstreambuf::defaultBufferSize = 4 * 1024;

zppstreambuf::~zppstreambuf()
{

        if (zReader) delete zReader;
        zReader = NULL;

	if (ownBuffer && buffer) delete[] buffer; 
	if (ownZFile && zFile) delete zFile;

	zFile = NULL; ownBuffer = false; buffer = NULL; bufferSize = 0;
}

// currently, can't write zppstreambufs.
int zppstreambuf::overflow(int ch)
{
	return traits_type::eof();
}

int zppstreambuf::underflow()
{
	if (gptr() < egptr()) return *gptr();

	int cnt = zReader->read(buffer,bufferSize);

	// if we didn't get any data from zip reader, must be at eof.
	if (cnt <= 0) return traits_type::eof();

	setg( buffer, buffer, buffer+cnt);
	return traits_type::to_int_type(*buffer);
}

// currently, only seek to 0 is supported.
std::streampos zppstreambuf::seekoff(std::streamoff _off, std::ios::seekdir _dir, std::ios::openmode _mode)
{
	if (_off == 0 && _dir == std::ios_base::beg) {
		zReader->resetStream();
		setg(0,0,0);
		underflow();
		return std::streampos(0);
	}

	return EOF; //streampos(_BADOFF);
}

// currently, only seek to 0 is supported.
std::streampos zppstreambuf::seekpos(std::streampos _pos, std::ios::openmode _mode)
{
	if (_pos == std::streampos(0)) {
		zReader->resetStream();
		setg(0,0,0);
		underflow();
		return std::streampos(0);
	}
	return EOF; //streampos(_BADOFF);
}

bool zppstreambuf::_Init(int _bufsize, char *_buffer)
{
	// init fields.
	zReader = NULL;
	zFile = NULL;
	seekable = false;
	ownBuffer = false;
	ownZFile = false;

	// if we aren't passed in a buffer, make one.
	bufferSize = _bufsize;

	if (bufferSize <= 0) bufferSize = defaultBufferSize;
	if (_buffer == NULL)  {
		buffer = new char[bufferSize];
		if (buffer == NULL) throw zppError("can't allocate buffer");
		ownBuffer = true;
	} else buffer = _buffer;

	return true;
}

bool zppstreambuf::_Open(const std::string &_fn, zppZipArchive *_zip, bool _seekable)
{
#ifndef ZPP_IGNORE_PLAIN_FILE
	try {
		// try to access file on local disk first.
		zFile = new zppZipFileInfo(_fn);
		ownZFile = true;
	} catch (zppError e) {
#endif
		// if we get here, then we couldn't find the file on the
		// local filesystem.
		seekable = _seekable;
		if (seekable) throw zppError("seekable current unimplemented");

		if (_zip)
			zFile = _zip->findInArchive(_fn);
		else
			zFile = zppZipArchive::find(_fn);

		ownZFile = false;

		if (zFile == NULL) {
			zReader = NULL;
			return false;
		}
#ifndef ZPP_IGNORE_PLAIN_FILE
	}
#endif
	zReader = new zppZipReader(zFile);

	if (zReader == NULL) throw zppError("couldn't make zReader");

	return true;
}

bool zppstreambuf::open(const char *_fn, bool _seekable)
{
	if (zReader || zFile) throw zppError("duplicate open on zppstreambuf");

	return _Open(std::string(_fn),NULL, _seekable);
}

bool zppstreambuf::open(const char *_fn, zppZipArchive *_zip, bool _seekable)
{
	if (zReader || zFile) throw zppError("duplicate open on zppstreambuf");

	return _Open(std::string(_fn),_zip, _seekable);
}

bool zppstreambuf::open(std::string _fn, bool _seekable)
{
	if (zReader || zFile) throw zppError("duplicate open on zppstreambuf");

	return _Open(_fn,NULL, _seekable);
}

bool zppstreambuf::open(std::string _fn, zppZipArchive *_zip, bool _seekable)
{
	if (zReader || zFile) throw zppError("duplicate open on zppstreambuf");

	return _Open(std::string(_fn), _zip, _seekable);
}


void zppstreambuf::_Tidy()
{
	// kill reader structure and "detatch" from zFile
	if (zReader) delete zReader;
	zReader = NULL;
	zFile = NULL;
}

bool zppstreambuf::close()
{
  if (zReader) delete zReader;
  zReader = NULL;
  
  if (ownZFile && zFile) delete zFile;
  
  zFile = NULL;ownZFile=false;
  seekable=false;
  return true;
}

/////////////////////////////////////////////////////////////////////////
//
// test harness 
//
/////////////////////////////////////////////////////////////////////////
#if 0
int main()
{
	/*
	** basic test of reading and re-reading a file out of a zip
	** archive; ex1.cpp is better.
	*/
    
	try {
		int i;
		// build map of files in z.
		zppZipArchive zf1(std::string("test.zip"));
		zppZipArchive::dumpGlobalMap();

		izppstream fp("zipfmt.txt");
	
		printf("<<contents of zipfmt.txt>>\n");
		while (!fp.eof()) {
			char c;
			fp.read(&c,1);
			cout << c;
		}
		printf("<<end of zipfmt.txt>>\n");

		printf("<<second copy of zipfmt.txt>>\n");
		fp.clear(std::ios::goodbit);
		fp.seekg(0,std::ios::beg);
		i = 0;
		while (++i < 1000) {
			char c;
			fp.read(&c,1);
			cout << c;
		}
		printf("<<end of zipfmt.txt>>\n");

		printf("<<third copy of zipfmt.txt>>\n");
		fp.clear(std::ios::goodbit);
		fp.seekg(0,std::ios::beg);
		i = 0;
		while (!fp.eof()) {
			char c;
			fp.read(&c,1);
			cout << c;
		}
		printf("<<end of zipfmt.txt>>\n");

	} catch ( zppError e ) {
		printf("zppError: %s\n",e.str.c_str());
	}

    return 0;
}
#endif /* 0 */
