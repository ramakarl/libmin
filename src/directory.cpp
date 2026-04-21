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
#include <cstring>

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


int Directory::CreateDir ( std::string path )
{
	path = Directory::NormalizeSlashies( path );
	
	std::vector< std::string > pathSet;
	int cnt = strSplitMultiple ( path, Directory::gPathDelim, pathSet );

	std::string currPath = "";
  int out = 0;
	std::vector< std::string >::iterator it;

	for ( it = pathSet.begin() ; it < pathSet.end(); ++it ) {

    currPath += *it;

    #ifdef _MSC_VER
      // Windows - create dir
		  out = CreateDirectoryA ( (currPath + *it).c_str() , NULL );
      if ( out == 0 ) {
  			DWORD d = GetLastError();
			  if ( d == ERROR_PATH_NOT_FOUND ) return 0; 
			  if ( d == ERROR_ALREADY_EXISTS ) { /* ignore */ }
		  }
    #else
      // Linux - create dir
      out = mkdir(currPath.c_str(), 0755);
      if (out != 0) {
        if (errno == ENOENT) return 0;
        if (errno == EEXIST) { /* ignore */ }
      }
    #endif

		currPath += Directory::gPathDelim;
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
  std::string path;

  #ifdef _MSC_VER
    // Windows - get current exe path
    char buffer[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA (NULL, buffer, MAX_PATH);
    path = buffer;
  #else
    // Linux - get current exe path
	  char buffer[PATH_MAX] = {0};
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) return "";
    buffer[len] = '\0';
    path = buffer;
  #endif

  size_t pos = path.rfind( getPathDelim() );
  if (pos != std::string::npos) {
    path = path.substr(0,pos);
  }
  path = strReplace( path, std::string(1,getPathDelimOpposite()), std::string(1,getPathDelim()) );

	return path;
}

/* 
std::string Directory::ws2s(const std::wstring& s)
{
  #ifdef _MSC_VER
    int slength = (int) s.length() + 1;
    int len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
    std::string r(len,0);
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
    if (!r.empty() && r.back()=='\0') r.pop_back();    
    return r;
  #else
    std::wstring_convert< std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(s);
  #endif	
}

std::wstring Directory::s2ws(const std::string& s)
{	
  #ifdef _MSC_VER
	  int slength = (int) s.length() + 1;
	  int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    std::wstring r(len,0);
	  MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    if (!r.empty() && r.back()=='\0') r.pop_back();    
    return r;
  #else
    std::wstring_convert< std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(s);
  #endif	
}
*/

void Directory::LoadDir ( std::string path, std::string ext )
{
	mPath = path;
	mFileFilter = ext;
	mList = DirList ( path, ext );
}

dir_list Directory::DirList( std::string path, std::string ext )
{
	path = Directory::NormalizeSlashies( path );

  dir_list out;

  #ifdef _MSC_VER	
    // Windows - list directory
	  WIN32_FIND_DATAA fileData;
	  HANDLE hFind = INVALID_HANDLE_VALUE;
	  LARGE_INTEGER filesize;

    path += ext;

    hFind = FindFirstFileA( path.c_str(), &fileData );

    if (hFind == INVALID_HANDLE_VALUE)
        return out;

		do {
      if (( fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && !( fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN  )) {
        dir_list_element e;
        e.length = 0;
        e.text = fileData.cFileName;
        e.extension = "DIR";
        e.type = FILE_TYPE_DIR;
        out.push_back( e );
      } else {
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

	  FindClose(hFind);

  #else
    // Linux - list directory
    DIR* dir = opendir(path.c_str());
    if (!dir) {
      perror ( "opendir failed" );
      return out;
    }    

    struct dirent* entry;    

    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;        
        if (name == "." || name == "..") continue;      // skip "." and ".."
            
        std::string fullPath = path + "/" + name;        
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) continue;

        dir_list_element e;
        e.text = name;
        if (S_ISDIR(st.st_mode)) {
            e.length = 0;
            e.extension = "DIR";
            e.type = FILE_TYPE_DIR;
        } else {
            e.length = (int)st.st_size;
            e.extension = strSplitRight(e.text, ".");
            e.type = FILE_TYPE_FILE;
        }
        out.push_back(e);
    }
    closedir(dir);

  #endif

	return out;
}

