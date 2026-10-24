/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef GITREPOSITORY_H
#define GITREPOSITORY_H

#include <QString>
#include <QStringList>

/**
	@brief The GitRepository class
	Small, read-only helper around the git command line, used to tell
	whether a project file is under version control and whether it has
	changed since the last commit.

	Git is run as an external program with QProcess: no git library is
	linked, so nothing new is needed to build QElectroTech, and a machine
	without git simply reports FileState::GitUnavailable.

	Every call is synchronous and bounded by a timeout, so a slow or
	hanging git never blocks the editor for long.
*/
class GitRepository
{
	public:
		enum class FileState {
			GitUnavailable,   ///< git is not installed or not in PATH
			NotInRepository,  ///< the file is not inside a git working tree
			Untracked,        ///< inside a repository, but never added (or ignored)
			Unmodified,       ///< committed, no change since the last commit
			Modified,         ///< committed, but changed on disk or staged
			Unknown           ///< git failed or timed out
		};

		struct Status {
			FileState state = FileState::GitUnavailable;
			QString branch;          ///< current branch, empty when detached
			QString repository_root; ///< top folder of the working tree
		};

		static bool isGitAvailable();
		static Status status(const QString &file_path);

	private:
		static bool runGit(const QString &working_dir,
						   const QStringList &arguments,
						   QString *output = nullptr,
						   int timeout_ms = 3000);
};

#endif // GITREPOSITORY_H
