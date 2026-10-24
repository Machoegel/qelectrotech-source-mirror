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
#include "gitrepository.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

/**
	@brief GitRepository::isGitAvailable
	@return true if a git executable can be found in PATH.
	Looked up once per run: installing git while QElectroTech is open
	needs a restart to be noticed.
*/
bool GitRepository::isGitAvailable()
{
	static const bool available =
			!QStandardPaths::findExecutable(QStringLiteral("git")).isEmpty();
	return available;
}

/**
	@brief GitRepository::status
	@param file_path : absolute path of a saved project file
	@return where @a file_path stands in git: in a repository or not,
	tracked or not, changed since the last commit or not, and the
	current branch.
*/
GitRepository::Status GitRepository::status(const QString &file_path)
{
	Status result;
	if (!isGitAvailable()) {
		result.state = FileState::GitUnavailable;
		return result;
	}

	const QFileInfo info(file_path);
	if (file_path.isEmpty() || !info.dir().exists()) {
		//A project that was never saved has no place on disk yet
		result.state = FileState::NotInRepository;
		return result;
	}
	const QString dir = info.absolutePath();
	const QString name = info.fileName();

	QString root;
	if (!runGit(dir, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")}, &root)) {
		result.state = FileState::NotInRepository;
		return result;
	}
	result.repository_root = QDir::cleanPath(root);

		//Fails on a detached HEAD: the branch then stays empty
	QString branch;
	if (runGit(dir, {QStringLiteral("symbolic-ref"), QStringLiteral("--quiet"),
					 QStringLiteral("--short"), QStringLiteral("HEAD")}, &branch)) {
		result.branch = branch;
	}

		//Not known to git at all: never added, or ignored
	if (!runGit(dir, {QStringLiteral("ls-files"), QStringLiteral("--error-unmatch"),
					  QStringLiteral("--"), name})) {
		result.state = FileState::Untracked;
		return result;
	}

	QString changes;
	if (!runGit(dir, {QStringLiteral("status"), QStringLiteral("--porcelain=v1"),
					  QStringLiteral("--"), name}, &changes)) {
		result.state = FileState::Unknown;
		return result;
	}
	result.state = changes.isEmpty() ? FileState::Unmodified
									 : FileState::Modified;
	return result;
}

/**
	@brief GitRepository::runGit
	Run git with @a arguments in @a working_dir and wait for it.
	@param output : if not null, receives git's standard output, trimmed
	@return true if git ran, finished within @a timeout_ms and exited with 0
*/
bool GitRepository::runGit(const QString &working_dir,
						   const QStringList &arguments,
						   QString *output,
						   int timeout_ms)
{
	QProcess git;
	git.setWorkingDirectory(working_dir);

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		//Only reading: don't take the index lock, so a commit made at the
		//same moment in another program (GitHub Desktop, a terminal) is
		//never refused because of us
	env.insert(QStringLiteral("GIT_OPTIONAL_LOCKS"), QStringLiteral("0"));
		//Never wait for a password or a pager that nobody will answer
	env.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
	env.insert(QStringLiteral("GIT_PAGER"), QStringLiteral("cat"));
	git.setProcessEnvironment(env);

	git.start(QStringLiteral("git"), arguments);
	if (!git.waitForStarted(timeout_ms)) {
		return false;
	}
	if (!git.waitForFinished(timeout_ms)) {
		git.kill();
		git.waitForFinished(1000);
		return false;
	}
	if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
		return false;
	}
	if (output) {
		*output = QString::fromUtf8(git.readAllStandardOutput()).trimmed();
	}
	return true;
}
