#ifndef BCRYPT_H
#define BCRYPT_H

#include <string>

// library export mode
 #if !defined ( BCRYPT_STATIC )
    #if defined ( BCRYPT_EXPORT )		// build DLL itself. export funcs.
        // #pragma message ( "Exporting DLL funcs." )
        #if defined(_WIN32) || defined(__CYGWIN__)
            #define APIMODE	  __declspec(dllexport)
        #else
            #define APIMODE   __attribute__((visibility("default")))
        #endif
    #else										          // using DLL. import funcs.
        #if defined(_WIN32) || defined(__CYGWIN__)
            #define APIMODE		__declspec(dllimport)
        #else
            #define APIMODE 	//https://stackoverflow.com/questions/2164827/explicitly-exporting-shared-library-functions-in-linux
        #endif
    #endif          
#else
    // #pragma message ( "Building bcrypt static." )
    #define APIMODE
#endif        

// bcrypt functions
//
namespace bcrypt {

    APIMODE std::string generateSalt (unsigned int rounds);

    APIMODE std::string generateHash (const std::string& password, const std::string& salt );

    APIMODE bool validatePassword(const std::string &chk_hash, const std::string &orig_hash );

    APIMODE bool validatePlainPassword ( const std::string &plain_password, const std::string &orig_hash);

}

#endif // BCRYPT_H
