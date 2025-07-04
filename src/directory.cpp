//--------------------------------------------------------------------------------
// Copyright 2007-2022 (c) Quanta Sciences, Rama Hoetzlein, ramakarl.com
//
// 
// * Derivative works may append the above copyright notice but should not remove or modify earlier notices.
//
// MIT License:
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, including without 
// limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS 
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "directory.h"

#include "string_helper.h"

#include <sys/stat.h>

#ifdef _WIN32

  #include <windows.h>

#else

  #include <limits.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <dirent.h>
  #include <errno.h>
  #include <vector>
  #include <iostream>
  #include <linux/limits.h>

#endif

std::string Directory::gPathDelim = "\\";

Directory::Directory ()
{
	mPath = "";
	#ifdef _MSC_VER
		gPathDelim = "\\";
	#else
		gPathDelim = "/";
	#endif
}

std::string Directory::NormalizeSlashies( std::string path )
{
	#ifdef _MSC_VER
		strReplace( path, "/", "\\" );
		if ( strRight(path,1).at(0) != '\\' ) path += "\\";
		return path;
	#else
		strReplace( path, "\\", "/" );
		if ( strRight(path,1).at(0) != '/' ) path += "/";
		return path;
	#endif	
}


dir_list Directory::GetDirectoryItems( dir_list input )
{
	dir_list out;

	for ( unsigned int i=0; i < input.size(); i++ ) {
		if ( input[i].type == FILE_TYPE_DIR && input[i].text != "." ) out.push_back( input[i] );
	}
	return out;
}

dir_list Directory::GetFileItems( dir_list input )
{
	dir_list out;

	for ( unsigned int i=0; i < input.size(); i++ ){
		if ( input[i].type == FILE_TYPE_FILE && input[i].length >= 0 ) out.push_back( input[i] );
	}
	return out;
}


std::string Directory::GetExtension( std::string path )
{
	path = Directory::NormalizeSlashies( path );

	std::vector< std::string > temp;	
	int cnt = strSplitMultiple ( path, ".", temp );

	if ( cnt > 1 ) {
		return temp[ cnt-1 ];
	}
	return "";
}


bool Directory::FileExists( std::string filename ) 
{
	struct stat stFileInfo;
	bool exists = false;
	int intStat;

	intStat = stat( filename.c_str(), &stFileInfo );

	if(intStat == 0) {
		exists = true;
	}

	return( exists );
}

void Directory::LoadDir ( std::string path, std::string ext )
{
	mPath = path;
	mFileFilter = ext;
	mList = DirList ( path, ext );
}


#ifdef _WIN32

	int Directory::CreateDir ( std::string path )
	{
		path = Directory::NormalizeSlashies( path );

		int out = 0;

		std::vector< std::string > pathSet;

		int cnt = strSplitMultiple ( path, Directory::gPathDelim, pathSet );

		std::string currPath = "";

		std::vector< std::string >::iterator it;

		for ( it = pathSet.begin() ; it < pathSet.end(); it++ ) {

			out = CreateDirectory ( (currPath + *it).c_str() , NULL );

			currPath += *it + Directory::gPathDelim ;

			if ( out == 0 ) {
				DWORD d = GetLastError();
				if ( d == ERROR_PATH_NOT_FOUND ) { return 0; }
				if ( d == ERROR_ALREADY_EXISTS ) { /* continue */ }
			}

		}

		return out;
	}


	std::string Directory::GetCollapsedPath( std::string path )
	{
		// Create a fully resolved path
		path = GetExecutablePath() + "/" + path;

		// Normalize slashes (\ vs /)
		path = Directory::NormalizeSlashies( path );

		elem_vec_t	out;
		std::vector< std::string >	temp;
		std::string mFileFound = "";

		// Split path 
		int cnt = strSplitMultiple ( path, Directory::gPathDelim, temp );

		for ( unsigned int i = 0; i < temp.size(); i++ ) {
			if ( ( temp[i].compare("..") == 0 ) && out.size() > 0 ) {
				out.pop_back();
			} else if ( temp[i].size() > 0 && temp[i].find(".") == -1 ) {
				text_element_t element;
				element.text = temp[i];
				element.length = -1;
				out.push_back( element );
			}

			if (  temp[i].find(".") != -1 ) {
				mFileFound = temp[i];
			}
		}

		std::string outStr = "";

		for ( unsigned int k = 0; k < out.size(); k++ )
		{
			outStr += out[k].text;
			outStr += Directory::gPathDelim;
		}

		// remember to remove the last extraneous slash
		return outStr.substr( 0, outStr.length() - 1 );
	}

	std::string Directory::GetExecutablePath()
	{
		LPTSTR szAppPath[MAX_PATH];

		std::string strAppDirectory;

		::GetModuleFileName(NULL, szAppPath[0], (sizeof(szAppPath) - 1)/sizeof(TCHAR));

		// Extract directory
		strAppDirectory = (char*) szAppPath;		// use ws2s for conversion to wchar
		strAppDirectory = strAppDirectory.substr(0, strAppDirectory.rfind("\\"));

		strAppDirectory = strReplace( strAppDirectory, "\\", "/" );

		return strAppDirectory;
	}

	std::string Directory::ws2s(const std::wstring& s)
	{
		int len;
		int slength = (int)s.length() + 1;
		len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
		char* buf = (char*) malloc ( len );		// temporary, don't register with memory checker
		WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
		std::string r(buf);
		free ( buf );
		return r;
	}

	std::wstring Directory::s2ws(const std::string& s)
	{
		int len;
		int slength = (int)s.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
		wchar_t* buf = (wchar_t*) malloc ( len *sizeof(wchar_t) );		// temporary, don't register with memory checker
		MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
		std::wstring r(buf);
		free ( buf );
		return r;
	}

	dir_list Directory::DirList( std::string path, std::string ext )
	{
		path = Directory::NormalizeSlashies( path ) + ext;	

		//NOTE this is not unicode compliant
		WIN32_FIND_DATAA fileData;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		LARGE_INTEGER filesize;

		DWORD dwError=0;
	   
		dir_list out; 

		hFind = FindFirstFileA( path.c_str(), &fileData );

		if (hFind == INVALID_HANDLE_VALUE) 
		{
			out.clear();
			return out;
		} 

		do
		{
		  if (( fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && !( fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN  ))
		  {
			dir_list_element e;
			e.length = 0;
			e.text = fileData.cFileName;
			e.extension = "DIR";
			e.type = FILE_TYPE_DIR;
			out.push_back( e );
		  }
		  else
		  {
			filesize.LowPart = fileData.nFileSizeLow;
			filesize.HighPart = fileData.nFileSizeHigh;

			dir_list_element e;
			e.length = (int) filesize.QuadPart;
			e.text = fileData.cFileName;
			e.extension = strSplitRight ( e.text, "." );
			e.type = FILE_TYPE_FILE;
			out.push_back( e );
		  }
		}
		while (FindNextFileA(hFind, &fileData) != 0);

		dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES) 
		{
		  //
		}

		FindClose(hFind);
		return out;
	}

#endif

#ifdef __unix__

	#include <sys/stat.h>  // For mkdir
	#include <sys/types.h>
	#include <errno.h>
	#include <string>

	int Directory::CreateDir ( std::string path ) {

	    // mode 0755 = rwxr-xr-x (owner rwx, group rx, others rx)
	    int result = mkdir(path.c_str(), 0755);
	    if (result == 0) return 0; // success

	    // Already exists, treat as success
	    if (errno == EEXIST) return 0;

	    // Otherwise return errno as error code
	    return errno;
	}

	std::string Directory::GetExecutablePath()
	{
		char result[ PATH_MAX ];
		ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
		return std::string( result, (count > 0) ? count : 0 );
	}


	dir_list Directory::DirList( std::string path, std::string ext )
	{
	    path = Directory::NormalizeSlashies( path );

	    dir_list out;

	    DIR* dir = opendir(path.c_str());
	    if (!dir) return out;

	    struct dirent* entry;
	    struct stat st;

	    while ((entry = readdir(dir)) != nullptr)
	    {
		std::string filename = entry->d_name;

		// Skip "." and ".."
		if (filename == "." || filename == "..") continue;

		std::string fullPath = path + "/" + filename;

		if (stat(fullPath.c_str(), &st) == -1) continue;

		if (S_ISDIR(st.st_mode))
		{
		    // Directory, filter out hidden directories (starting with '.')
		    if (!filename.empty() && filename[0] == '.') continue;

		    dir_list_element e;
		    e.length = 0;
		    e.text = filename;
		    e.extension = "DIR";
		    e.type = FILE_TYPE_DIR;
		    out.push_back(e);
		}
		else if (S_ISREG(st.st_mode))
		{
		    // File: check extension filter
		    // if (!matchesExtension(filename, ext))
		    //    continue;

		    dir_list_element e;
		    e.length = (int)st.st_size;		    
		    e.extension = strSplitRight(filename, ".");   // filename will have ext removed
		    e.text = filename;
		    e.type = FILE_TYPE_FILE;
		    out.push_back(e);
		}
		// Ignore other file types (links, sockets, etc.)
	    }

	    closedir(dir);
	 
	    return out;
	}

	std::string Directory::GetCollapsedPath( std::string path )
	{
		path = Directory::NormalizeSlashies( path );

		elem_vec_t	out;

		std::string mFileFound = "";

		std::vector< std::string > pathSet;

		int cnt = strSplitMultiple ( path, Directory::gPathDelim, pathSet );

		for ( unsigned int i = 0; i < pathSet.size(); i++ )
		{
			if ( ( pathSet[i].compare("..") == 0 ) && out.size() > 0 ) {
				out.pop_back();
			} else if ( pathSet[i].size() > 0 && pathSet[i].find(".") == -1 ) {
				text_element_t element;
				element.text = pathSet[i];
				element.length = -1;
				out.push_back( element );
			}

			if (  pathSet[i].find(".") != -1 ) {
				mFileFound = pathSet[i];
			}
		}

		std::string outStr = "";

		for ( unsigned int k = 0; k < out.size(); k++ )
		{
			outStr += out[k].text;
			outStr += getPathDelim();
		}

		// remember to remove the last extraneous slash
		return outStr.substr( 0, outStr.length() - 1 );
	}


#endif
