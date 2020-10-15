/*
**
** $id:$
**
** File: util.cpp -- utility functions for ZPP library.
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
** $Log: Util.cpp,v $
** Revision 1.2  2003/06/22 19:24:02  epajarre
** Integrated all changes by me, testing still to be done
**
** Revision 1.1.1.1  2003/06/21 19:09:37  epajarre
** Initial import. (0.2) version)
**
**
*/

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* _WINDOWS */

#if _MSC_VER <= 1200
#pragma warning ( disable : 4786 )
#endif /* _MSC_VER */


#include <list>
#include <string>
using namespace std;


/*
** This function is used by the function zppZipArchive::openAll() to
** enumerate files in a directory.
**
** This implementation is for win32; other operating systems will need to
** add their own implementations.
*/
#ifdef _WINDOWS
typedef list<string> stringList;

stringList *enumerateDir(const string &wildPath, bool fullPath)
{
    HANDLE h;
    WIN32_FIND_DATA findData;
    stringList *fileList = new stringList;
	string path;
    DWORD rc;

	int sep = wildPath.find_last_of("/\\");
	if (sep == -1) {
		path = "";
	} else {
		path = wildPath.substr(0,sep);
	}

	h = FindFirstFile(wildPath.c_str(), &findData);
	if (h == INVALID_HANDLE_VALUE) {
		rc = GetLastError();
		//cout << "Find first fails." << endl;
		return 0;
    }

    //if (fullPath) {
	//	fileList->push_back( path + "\\" + (string) findData.cFileName);
    //} else {
	//	fileList->push_back ( (string) findData.cFileName );
    //}

    do {
		if (fullPath) {
			string fname = path + findData.cFileName;
	    	fileList->push_back( fname );
		} else {
			string fname(findData.cFileName);
	    	fileList->push_back ( fname );
		}
    } while (FindNextFile(h, &findData));
		
    rc = GetLastError();
    FindClose(h);

    // neither of these errors is fatal.
    if (rc == ERROR_NO_MORE_FILES || rc == ERROR_FILE_NOT_FOUND) return fileList;

    // damn, some kind of error.
    delete fileList;
    return 0;
}
#endif /* _WINDOWS */
