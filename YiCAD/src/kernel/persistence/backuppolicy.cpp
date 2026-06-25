// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>      *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "Debug.h"
#include "backuppolicy.h"

#include "FilterOcdIO.h"
#include <Tools.h>
#include <sstream>

//boost
#include <boost/algorithm/string/replace.hpp>

// Helper class to handle different backup policies
BackupPolicy::BackupPolicy()
{
	policy = Standard;
	numberOfFiles = 1;
	useBakExtension = false;
	saveBackupDateFormat = "%Y%m%d-%H%M%S";
}
BackupPolicy::~BackupPolicy()
{
}
void BackupPolicy::setPolicy(Policy p)
{
	policy = p;
}
void BackupPolicy::setNumberOfFiles(int count)
{
	numberOfFiles = count;
}
void BackupPolicy::useBackupExtension(bool on)
{
	useBakExtension = on;
}
void BackupPolicy::setDateFormat(const std::string& fmt)
{
	saveBackupDateFormat = fmt;
}
void BackupPolicy::apply(const std::string& sourcename, const std::string& targetname)
{
	switch (policy)
	{
	case Standard:
		applyStandard(sourcename, targetname);
		break;
	case TimeStamp:
		applyTimeStamp(sourcename, targetname);
		break;
	}
}


void BackupPolicy::applyStandard(const std::string& sourcename, const std::string& targetname)
{
	// if saving the project data succeeded rename to the actual file name
	FileInfo fi(targetname);
	if (fi.exists())
	{
		if (numberOfFiles > 0)
		{
			int nSuff = 0;
			std::string fn = fi.fileName();
			FileInfo di(fi.dirPath());
			std::vector<FileInfo> backup;
			std::vector<FileInfo> files = di.getDirectoryContent();
			for (std::vector<FileInfo>::iterator it = files.begin(); it != files.end(); ++it)
			{
				std::string file = it->fileName();
				if (file.substr(0, fn.length()) == fn)
				{
					// starts with the same file name
					std::string suf(file.substr(fn.length()));
					if (suf.size() > 0)
					{
						std::string::size_type nPos = suf.find_first_not_of("0123456789");
						if (nPos == std::string::npos)
						{
							// store all backup files
							backup.push_back(*it);
							nSuff = std::max<int>(nSuff, std::atol(suf.c_str()));
						}
					}
				}
			}

			if (!backup.empty() && (int)backup.size() >= numberOfFiles)
			{
				// delete the oldest backup file we found
				FileInfo del = backup.front();
				for (std::vector<FileInfo>::iterator it = backup.begin(); it != backup.end(); ++it)
				{
					if (it->lastModified() < del.lastModified())
						del = *it;
				}

				del.deleteFile();
				fn = del.filePath();
			}
			else
			{
				// create a new backup file
				std::stringstream str;
				str << fi.filePath() << (nSuff + 1);
				fn = str.str();
			}

			if (fi.renameFile(fn.c_str()) == false)
				throw OneException("Cannot rename project file to backup file\n");
		}
		else
		{
			fi.deleteFile();
		}
	}

	FileInfo tmp(sourcename);
	if (tmp.renameFile(targetname.c_str()) == false)
	{
		throw OneException("Cannot rename tmp save file to project file");
	}
}
void BackupPolicy::applyTimeStamp(const std::string& sourcename, const std::string& targetname)
{
	FileInfo fi(targetname);

	std::string fn = sourcename;
	std::string ext = fi.extension();
	std::string bn; // full path with no extension but with "."
	std::string pbn; // base name of the project + "."
	if (!ext.empty())
	{
		bn = fi.filePath().substr(0, fi.filePath().length() - ext.length());
		pbn = fi.fileName().substr(0, fi.fileName().length() - ext.length());
	}
	else
	{
		bn = fi.filePath() + ".";
		pbn = fi.fileName() + ".";
	}

	bool backupManagementError = false; // Note error and report at the end
	if (fi.exists())
	{
		if (numberOfFiles > 0)
		{
			// replace . by - in format to avoid . between base name and extension
			boost::replace_all(saveBackupDateFormat, ".", "-");
			{
				// Remove all extra backups
				std::string fn = fi.fileName();
				FileInfo di(fi.dirPath());
				std::vector<FileInfo> backup;
				std::vector<FileInfo> files = di.getDirectoryContent();
				for (std::vector<FileInfo>::iterator it = files.begin(); it != files.end(); ++it)
				{
					if (it->isFile())
					{
						std::string file = it->fileName();
						std::string fext = it->extension();
						std::string fextUp = fext;
						std::transform(fextUp.begin(), fextUp.end(), fextUp.begin(), (int (*)(int))toupper);
						// re-enforcing identification of the backup file


						// old case : the name starts with the full name of the project and follows with numbers
						if ((startsWith(file, fn) &&
							(file.length() > fn.length()) &&
							checkDigits(file.substr(fn.length()))) ||
							// .Bak case : The bame starts with the base name of the project + "."
							// + complement with no "." + ".Bak"
							((fextUp == "BAK") && startsWith(file, pbn) &&
								(checkValidComplement(file, pbn, fext))))
						{
							backup.push_back(*it);
						}
					}
				}

				if (!backup.empty() && (int)backup.size() >= numberOfFiles)
				{
					std::sort(backup.begin(), backup.end(), fileComparisonByDate);
					// delete the oldest backup file we found
					// Base::FileInfo del = backup.front();
					int nb = 0;
					for (std::vector<FileInfo>::iterator it = backup.begin(); it != backup.end(); ++it)
					{
						nb++;
						if (nb >= numberOfFiles)
						{
							try
							{
								if (!it->deleteFile())
								{
									backupManagementError = true;
								}
							}
							catch (...)
							{
								backupManagementError = true;
							}
						}
					}

				}
			}  //end remove backup

			// create a new backup file
			{
				int ext = 1;
				if (useBakExtension)
				{
					std::stringstream str;
					TimeInfo ti = fi.lastModified();
					time_t s = ti.getSeconds();
					struct tm* timeinfo = localtime(&s);
					char buffer[100];

					strftime(buffer, sizeof(buffer), saveBackupDateFormat.c_str(), timeinfo);
					str << bn << buffer;

					fn = str.str();
					bool done = false;

					if ((fn == "") || (fn[fn.length() - 1] == ' ') || (fn[fn.length() - 1] == '-'))
					{
						if (fn[fn.length() - 1] == ' ')
						{
							fn = fn.substr(0, fn.length() - 1);
						}
					}
					else
					{
						if (renameFileNoErase(fi, fn + ".Bak") == false)
						{
							fn = fn + "-";
						}
						else
						{
							done = true;
						}
					}

					if (!done)
					{
						while (ext < numberOfFiles + 10)
						{
							if (renameFileNoErase(fi, fn + std::to_string(ext) + ".Bak"))
								break;
							ext++;
						}
					}
				}
				else
				{
					// changed but simpler and solves also the delay sometimes introduced by google drive
					while (ext < numberOfFiles + 10)
					{
						if (renameFileNoErase(fi, fi.filePath() + std::to_string(ext)))
							break;
						ext++;
					}
				}

				if (ext >= numberOfFiles + 10)
				{
					DEBUG->print("File not saved: Cannot rename project file to backup file\n");
					//throw Base::FileException("File not saved: Cannot rename project file to backup file", fi);
				}
			}
		}
		else
		{
			try
			{
				fi.deleteFile();
			}
			catch (...)
			{
				DEBUG->print("Cannot remove backup file: %s\n", fi.fileName().c_str());
				backupManagementError = true;
			}
		}
	}

	FileInfo tmp(sourcename);
	if (tmp.renameFile(targetname.c_str()) == false)
	{
		throw OneException("Save interrupted: Cannot rename temporary file to project file");
	}

	if (backupManagementError)
	{
		throw OneException("Warning: Save complete, but error while managing backup history.");
	}
}
bool BackupPolicy::fileComparisonByDate(const FileInfo& i, const FileInfo& j)
{
	return (i.lastModified() > j.lastModified());
}
bool BackupPolicy::startsWith(const std::string& st1,
	const std::string& st2) const
{
	return st1.substr(0, st2.length()) == st2;
}
bool BackupPolicy::checkValidString(const std::string& cmpl, const std::regex& e) const
{
	std::smatch what;
	bool res = std::regex_search(cmpl, what, e);
	return res;
}
bool BackupPolicy::checkValidComplement(const std::string& file, const std::string& pbn, const std::string& ext) const
{
	std::string cmpl = file.substr(pbn.length(), file.length() - pbn.length() - ext.length() - 1);
	std::regex e(R"(^[^.]*$)");
	return checkValidString(cmpl, e);
}
bool BackupPolicy::checkDigits(const std::string& cmpl) const
{
	std::regex e(R"(^[0-9]*$)");
	return checkValidString(cmpl, e);
}
bool BackupPolicy::renameFileNoErase(FileInfo fi, const std::string& newName)
{
	FileInfo nf(newName);
	if (!nf.exists())
	{
		return fi.renameFile(newName.c_str());
	}
	return false;
}
