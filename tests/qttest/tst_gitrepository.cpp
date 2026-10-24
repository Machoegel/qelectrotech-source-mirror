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
#include "git/gitrepository.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

/**
	Walks one project file through the states GitRepository reports, in
	a throw-away repository: outside git, untracked, committed, modified.
*/
class TestGitRepository : public QObject
{
	Q_OBJECT

	private:
		static bool git(const QString &dir, const QStringList &args)
		{
			QProcess p;
			p.setWorkingDirectory(dir);
			p.start(QStringLiteral("git"), args);
			return p.waitForFinished(10000)
					&& p.exitStatus() == QProcess::NormalExit
					&& p.exitCode() == 0;
		}

		static void writeFile(const QString &path, const QByteArray &content)
		{
			QFile f(path);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write(content);
		}

	private slots:
		void initTestCase()
		{
			if (!GitRepository::isGitAvailable()) {
				QSKIP("git is not installed");
			}
		}

		void notSaved()
		{
			QCOMPARE(GitRepository::status(QString()).state,
					 GitRepository::FileState::NotInRepository);
		}

		void outsideRepository()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			const QString file = dir.filePath(QStringLiteral("project.qet"));
			writeFile(file, "<project/>");
			QCOMPARE(GitRepository::status(file).state,
					 GitRepository::FileState::NotInRepository);
		}

		void lifeCycle()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			const QString root = dir.path();
			QVERIFY(git(root, {"init", "-q", "-b", "main"}));
			QVERIFY(git(root, {"config", "user.email", "test@example.org"}));
			QVERIFY(git(root, {"config", "user.name", "Test"}));

			const QString file = dir.filePath(QStringLiteral("my project.qet"));
			writeFile(file, "<project/>");

			GitRepository::Status s = GitRepository::status(file);
			QCOMPARE(s.state, GitRepository::FileState::Untracked);
			QCOMPARE(QFileInfo(s.repository_root).canonicalFilePath(),
					 QFileInfo(root).canonicalFilePath());

			QVERIFY(git(root, {"add", "my project.qet"}));
			QVERIFY(git(root, {"commit", "-q", "-m", "first"}));
			s = GitRepository::status(file);
			QCOMPARE(s.state, GitRepository::FileState::Unmodified);
			QCOMPARE(s.branch, QStringLiteral("main"));

			writeFile(file, "<project version=\"2\"/>");
			QCOMPARE(GitRepository::status(file).state,
					 GitRepository::FileState::Modified);
		}

		void ignoredCountsAsUntracked()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			const QString root = dir.path();
			QVERIFY(git(root, {"init", "-q"}));
			writeFile(dir.filePath(QStringLiteral(".gitignore")), "*.qet\n");
			const QString file = dir.filePath(QStringLiteral("project.qet"));
			writeFile(file, "<project/>");
			QCOMPARE(GitRepository::status(file).state,
					 GitRepository::FileState::Untracked);
		}
};

QTEST_GUILESS_MAIN(TestGitRepository)
#include "tst_gitrepository.moc"
