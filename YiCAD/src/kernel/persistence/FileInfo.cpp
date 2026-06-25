// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

# include <algorithm>
# include <cassert>
# include <codecvt>
# include <cstring>
# include <locale>
#if defined(_WIN32)
# include <io.h>
# include <Windows.h>
#else
# include <dirent.h>
# include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>


#include <FileInfo.h>
#include "Stream.h"
#include "Tools.h"

#ifndef R_OK
#define R_OK    4   /* Test for read permission    */
#endif
#ifndef W_OK
#define W_OK    2   /* Test for write permission   */
#endif
#ifndef X_OK
#define X_OK    1   /* Test for execute permission */
#endif
#ifndef F_OK
#define F_OK    0   /* Test for existence          */
#endif

//**********************************************************************************
// helper
#if defined(_WIN32)
std::string ConvertFromWideString(const std::wstring& string)
{
    int neededSize = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), -1, 0, 0,0,0);
    char * CharString = new char[static_cast<size_t>(neededSize)];
    WideCharToMultiByte(CP_UTF8, 0, string.c_str(), -1, CharString, neededSize,0,0);
    std::string String(CharString);
    delete [] CharString;
    CharString = NULL;
    return String;
}

std::wstring ConvertToWideString(const std::string& string)
{
    int neededSize = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, 0, 0);
    wchar_t* wideCharString = new wchar_t[static_cast<size_t>(neededSize)];
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wideCharString, neededSize);
    std::wstring wideString(wideCharString);
    delete [] wideCharString;
    wideCharString = NULL;
    return wideString;
}
#endif


//**********************************************************************************
// FileInfo


FileInfo::FileInfo (const char* _FileName)
{
    setFile(_FileName);
}

FileInfo::FileInfo (const std::string &_FileName)
{
    setFile(_FileName.c_str());
}

const std::string &FileInfo::getTempPath()
{
    static std::string tempPath;

    if (tempPath == "") {
#if defined(WIN32)
        wchar_t buf[MAX_PATH + 2];
        GetTempPathW(MAX_PATH + 1,buf);
        int neededSize = WideCharToMultiByte(CP_UTF8, 0, buf, -1, 0, 0, 0, 0);
        char* dest = new char[static_cast<size_t>(neededSize)];
        WideCharToMultiByte(CP_UTF8, 0, buf, -1,dest, neededSize, 0, 0);
        tempPath = dest;
        delete [] dest;
#else
        const char* tmp = getenv("TMPDIR");
        if (tmp && tmp[0] != '\0') {
            tempPath = tmp;
            FileInfo fi(tempPath);
            if (tempPath.empty() || !fi.isDir()) // still empty or non-existent
                tempPath = "/tmp/";
            else if (tempPath.at(tempPath.size()-1)!='/')
                tempPath.append("/");
        }
        else {
            tempPath = "/tmp/";
        }
#endif
    }

    return tempPath;
}

std::string FileInfo::getTempFileName(const char* FileName, const char* Path)
{
#if defined(WIN32)
    wchar_t buf[MAX_PATH + 2];

    // Path where the file is located
    std::wstring path;
    if (Path)
        path = ConvertToWideString(std::string(Path));
    else
        path = ConvertToWideString(getTempPath());

    // File name in the path
    std::wstring file;
    if (FileName)
        file = ConvertToWideString(std::string(FileName));
    else
        file = L"TempFile";


    // this already creates the file
    GetTempFileNameW(path.c_str(),file.c_str(),0,buf);
    DeleteFileW(buf);

    return std::string(ConvertFromWideString(std::wstring(buf)));
#else
    std::string buf;

    // Path where the file is located
    if (Path)
        buf = Path;
    else
        buf = getTempPath();

    // File name in the path
    if (FileName) {
        buf += "/";
        buf += FileName;
        buf += "XXXXXX";
    }
    else {
        buf += "/fileXXXXXX";
    }

    std::vector<char> vec;
    std::copy(buf.begin(), buf.end(), std::back_inserter(vec));
    vec.push_back('\0');

    /* coverity[secure_temp] mkstemp uses 0600 as the mode and is safe */
    int id = mkstemp(vec.data());
    if (id > -1) {
        FILE* file = fdopen(id, "w");
        fclose(file);
        vec.pop_back(); // remove '\0'
        std::string str(vec.begin(), vec.end());
        buf.swap(str);
        unlink(buf.c_str());
    }
    return buf;
#endif
}

std::filesystem::path FileInfo::stringToPath(const std::string& str)
{
#if defined(_WIN32)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::filesystem::path path(converter.from_bytes(str));
#else
    std::filesystem::path path(str);
#endif
    return path;
}

std::string FileInfo::pathToString(const std::filesystem::path& p)
{
#if defined(_WIN32)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(p.wstring());
#else
    return p.string();
#endif
}

void FileInfo::setFile(const char* name)
{
    if (!name) {
        FileName.clear();
        return;
    }

    FileName = name;

    // keep the UNC paths intact
    if (FileName.substr(0,2) == std::string("\\\\"))
        std::replace(FileName.begin()+2, FileName.end(), '\\', '/');
    else
        std::replace(FileName.begin(), FileName.end(), '\\', '/');
}

std::string FileInfo::filePath () const
{
    return FileName;
}

std::string FileInfo::fileName () const
{
    return FileName.substr(FileName.find_last_of('/')+1);
}

std::string FileInfo::dirPath () const
{
    std::size_t last_pos;
    std::string retval;
    last_pos = FileName.find_last_of('/');
    if (last_pos != std::string::npos) {
        retval = FileName.substr(0, last_pos);
    }
    else {
#if defined(_WIN32)
        wchar_t buf[MAX_PATH + 1];
        GetCurrentDirectoryW(MAX_PATH, buf);
        retval = std::string(ConvertFromWideString(std::wstring(buf)));
#else
        char buf[PATH_MAX+1];
        const char* cwd = getcwd(buf, PATH_MAX);
        retval = std::string(cwd ? cwd : ".");
#endif
    }
    return retval;
}

std::string FileInfo::fileNamePure () const
{
    std::string temp = fileName();
    std::string::size_type pos = temp.find_last_of('.');

    if (pos != std::string::npos)
        return temp.substr(0,pos);
    else
        return temp;
}

std::wstring FileInfo::toStdWString() const
{
#ifdef _WIN32
    return ConvertToWideString(FileName);
#else
    // On other platforms it's discouraged to use wchar_t for file names
    throw OneException("Cannot use FileInfo::toStdWString() on this platform");
#endif
}

std::string FileInfo::extension () const
{
    std::string::size_type pos = FileName.find_last_of('.');
    if (pos == std::string::npos)
        return std::string();
    return FileName.substr(pos+1);
}

std::string FileInfo::completeExtension () const
{
    std::string::size_type pos = FileName.find_first_of('.');
    if (pos == std::string::npos)
        return std::string();
    return FileName.substr(pos+1);
}

bool FileInfo::hasExtension (const char* Ext) const
{
#if defined (_WIN32)
    return _stricmp(Ext, extension().c_str()) == 0;
#else
    return strcasecmp(Ext,extension().c_str()) == 0;
#endif
}

bool FileInfo::exists () const
{
#if defined (FC_OS_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(), F_OK) == 0;
#else
    return access(FileName.c_str(), F_OK) == 0;
#endif
}

bool FileInfo::isReadable () const
{
#if defined (_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(), R_OK) == 0;
#else
    return access(FileName.c_str(), R_OK) == 0;
#endif
}

bool FileInfo::isWritable () const
{
#if defined (_WIN32)
    std::wstring wstr = toStdWString();
    return _waccess(wstr.c_str(), W_OK) == 0;
#else
    return access(FileName.c_str(), W_OK) == 0;
#endif
}

bool FileInfo::setPermissions (Permissions perms)
{
    int mode = 0;

    if (perms & FileInfo::ReadOnly)
        mode |= S_IREAD;
    if (perms & FileInfo::WriteOnly)
        mode |= S_IWRITE;

    if (mode == 0) // bad argument
        return false;

#if defined (_WIN32)
    std::wstring wstr = toStdWString();
    return _wchmod(wstr.c_str(), mode) == 0;
#else
    return chmod(FileName.c_str(), mode) == 0;
#endif

}

bool FileInfo::isFile () const
{
#ifdef _WIN32
    if (exists()) {
        std::wstring wstr = toStdWString();
        FILE* fd = _wfopen(wstr.c_str(), L"rb");
        bool ok = (fd != 0);
        if (fd) fclose(fd);
        return ok;
    }
#else
    if (exists()) {
        // If we can open it must be an existing file, otherwise we assume it
        // is a directory (which doesn't need to be true for any cases)
        std::ifstream str(FileName.c_str(), std::ios::in | std::ios::binary);
        if (!str)
            return false;
        str.close();
        return true;
    }

    else
        return false;
#endif
}

bool FileInfo::isDir () const
{
    if (exists()) {
        // if we can chdir then it must be a directory, otherwise we assume it
        // is a file (which doesn't need to be true for any cases)
#if defined (_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;

        if (_wstat(wstr.c_str(), &st) != 0)
            return false;
        return ((st.st_mode & _S_IFDIR) != 0);

#else
        struct stat st;
        if (stat(FileName.c_str(), &st) != 0) {
            return false;
        }
        return S_ISDIR(st.st_mode);
#endif
    }
    else
        return false;

    // TODO: Check for valid path name
    //return true;
}

unsigned int FileInfo::size () const
{
    // not implemented
    assert(0);
    return 0;
}

TimeInfo FileInfo::lastModified() const
{
    TimeInfo ti = TimeInfo::null();
    if (exists()) {

#if defined (_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            ti.setTime_t(st.st_mtime);
        }

#else
        struct stat st;
        if (stat(FileName.c_str(), &st) == 0) {
            ti.setTime_t(st.st_mtime);
        }
#endif

    }
    return ti;
}

TimeInfo FileInfo::lastRead() const
{
    TimeInfo ti = TimeInfo::null();
    if (exists()) {

#if defined (_WIN32)
        std::wstring wstr = toStdWString();
        struct _stat st;
        if (_wstat(wstr.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }

#else
        struct stat st;
        if (stat(FileName.c_str(), &st) == 0) {
            ti.setTime_t(st.st_atime);
        }
#endif

    }
    return ti;
}

bool FileInfo::deleteFile() const
{
#if defined (_WIN32)
    std::wstring wstr = toStdWString();
    return ::_wremove(wstr.c_str()) == 0;
#else
    return (::remove(FileName.c_str())==0);
#endif
}

bool FileInfo::renameFile(const char* NewName)
{
    bool res;
#if defined (_WIN32)
    std::wstring oldname = toStdWString();
    std::wstring newname = ConvertToWideString(NewName);
    res = ::_wrename(oldname.c_str(),newname.c_str()) == 0;
#else
    res = ::rename(FileName.c_str(),NewName) == 0;
#endif
    if (!res) {
        int code = errno;
        std::clog << "Error in renameFile: " << strerror(code) << " (" << code << ")" << std::endl;
    }
    else {
        FileName = NewName;
    }

    return res;
}

bool FileInfo::copyTo(const char* NewName) const
{
#if defined (_WIN32)
    std::wstring oldname = toStdWString();
    std::wstring newname = ConvertToWideString(NewName);
    return CopyFileW(oldname.c_str(),newname.c_str(),true) != 0;
#else
    FileInfo fi1(FileName);
    FileInfo fi2(NewName);
    ifstream file(fi1, std::ios::in | std::ios::binary);
    file.unsetf(std::ios_base::skipws);
    ofstream copy(fi2, std::ios::out | std::ios::binary);
    file >> copy.rdbuf();
    return file.is_open() && copy.is_open();
#endif
}

bool FileInfo::createDirectory() const
{
#if defined (_WIN32)
    std::wstring wstr = toStdWString();
    return _wmkdir(wstr.c_str()) == 0;
#else
    return mkdir(FileName.c_str(), 0777) == 0;
#endif
}

bool FileInfo::createDirectories() const
{
    try {
        std::filesystem::path path(stringToPath(FileName));
        if (std::filesystem::exists(path))
            return true;
        std::filesystem::create_directories(path);
        return true;
    }
    catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

bool FileInfo::deleteDirectory() const
{
    if (!isDir())
        return false;
#if defined (_WIN32)
    std::wstring wstr = toStdWString();
    return _wrmdir(wstr.c_str()) == 0;
#else
    return rmdir(FileName.c_str()) == 0;
#endif
}

bool FileInfo::deleteDirectoryRecursive() const
{
    if (isDir() == false )
        return false;
    std::vector<FileInfo> List = getDirectoryContent();

    for (std::vector<FileInfo>::iterator It = List.begin();It!=List.end();++It) {
        if (It->isDir()) {
            It->deleteDirectoryRecursive();
        }
        else if (It->isFile()) {
            It->setPermissions(FileInfo::ReadWrite);
            It->deleteFile();
        }
    }
    return deleteDirectory();
}

std::vector<FileInfo> FileInfo::getDirectoryContent() const
{
    std::vector<FileInfo> List;
#if defined (_WIN32)
    struct _wfinddata_t dentry;

    intptr_t hFile;

    // Find first directory entry
    std::wstring wstr = toStdWString();
    wstr += L"/*";

    if ((hFile = _wfindfirst( wstr.c_str(), &dentry)) == -1L)
        return List;

    while (_wfindnext(hFile, &dentry) == 0)
        if (wcscmp(dentry.name,L"..") != 0)
            List.push_back(FileInfo(FileName + "/" +ConvertFromWideString(std::wstring(dentry.name))));

    _findclose(hFile);

#else
    DIR* dp(nullptr);
    struct dirent* dentry(nullptr);
    if (!(dp = opendir(FileName.c_str())))
    {
        return List;
    }

    while ((dentry = readdir(dp)))
    {
        std::string dir = dentry->d_name;
        if (dir != "." && dir != "..")
            List.emplace_back(FileName + "/" + dir);
    }
    closedir(dp);
#endif
    return List;
}
