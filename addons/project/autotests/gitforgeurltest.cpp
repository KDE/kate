/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "git/gitforgeurl.h"

#include <QTest>

Q_DECLARE_METATYPE(GitForge::Provider)

class GitForgeUrlTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void parseRemote_data();
    void parseRemote();
    void rejectRemote_data();
    void rejectRemote();
    void resolveRepository();
    void buildUrl_data();
    void buildUrl();
    void lineRange_data();
    void lineRange();
    void providerApiUrls();
    void providerApiResponse();
};

void GitForgeUrlTest::parseRemote_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("path");

    QTest::newRow("https") << QStringLiteral("https://github.com/owner/repo.git") << QStringLiteral("github.com") << -1 << QStringLiteral("owner/repo");
    QTest::newRow("http-port") << QStringLiteral("http://git.example.com:8080/group/repo.git") << QStringLiteral("git.example.com") << 8080
                               << QStringLiteral("group/repo");
    QTest::newRow("ssh") << QStringLiteral("ssh://git@gitlab.com:2222/group/subgroup/repo.git") << QStringLiteral("gitlab.com") << 2222
                         << QStringLiteral("group/subgroup/repo");
    QTest::newRow("git") << QStringLiteral("git://github.com/owner/repo.git") << QStringLiteral("github.com") << -1 << QStringLiteral("owner/repo");
    QTest::newRow("scp") << QStringLiteral("git@gitlab.com:group/repo.git") << QStringLiteral("gitlab.com") << -1 << QStringLiteral("group/repo");
    QTest::newRow("scp-ipv6") << QStringLiteral("git@[2001:db8::1]:group/repo.git") << QStringLiteral("2001:db8::1") << -1 << QStringLiteral("group/repo");
}

void GitForgeUrlTest::parseRemote()
{
    QFETCH(QString, input);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, path);
    const auto remote = GitForge::parseRemote(input);
    QVERIFY(remote);
    QCOMPARE(remote->host, host);
    QCOMPARE(remote->port, port);
    QCOMPARE(remote->repositoryPath, path);
}

void GitForgeUrlTest::rejectRemote_data()
{
    QTest::addColumn<QString>("input");
    QTest::newRow("local") << QStringLiteral("../repo.git");
    QTest::newRow("file") << QStringLiteral("file:///tmp/repo.git");
    QTest::newRow("missing-project") << QStringLiteral("https://github.com/repo.git");
    QTest::newRow("traversal") << QStringLiteral("git@example.com:group/../repo.git");
    QTest::newRow("query") << QStringLiteral("https://github.com/owner/repo.git?x=1");
}

void GitForgeUrlTest::rejectRemote()
{
    QFETCH(QString, input);
    QVERIFY(!GitForge::parseRemote(input));
}

void GitForgeUrlTest::resolveRepository()
{
    const auto github = GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("git@github.com:owner/repo.git")), GitForge::defaultHostMappings());
    QVERIFY(github);
    QCOMPARE(github->provider, GitForge::Provider::GitHub);
    QVERIFY(!GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("git@github.com:owner/repo.git")), {}));

    const auto mapping
        = GitForge::hostMapping(QStringLiteral("git.example.com:2222"), GitForge::Provider::GitLab, QUrl(QStringLiteral("https://code.example.com/git")));
    QVERIFY(mapping);
    const auto custom = GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("ssh://git@git.example.com:2222/team/repo.git")), { *mapping });
    QVERIFY(custom);
    QCOMPARE(custom->provider, GitForge::Provider::GitLab);
    QCOMPARE(custom->webBaseUrl, QUrl(QStringLiteral("https://code.example.com/git")));

    QVERIFY(!GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("git@unknown.example:team/repo.git")), {}));
    QVERIFY(!GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("git@github.com:owner/group/repo.git")), GitForge::defaultHostMappings()));

    const auto wildcard
        = GitForge::hostMapping(QStringLiteral("git.example.com"), GitForge::Provider::GitHub, QUrl(QStringLiteral("https://github.example.com")));
    QVERIFY(wildcard);
    const auto exact
        = GitForge::hostMapping(QStringLiteral("git.example.com:2222"), GitForge::Provider::GitLab, QUrl(QStringLiteral("https://gitlab.example.com")));
    QVERIFY(exact);
    const auto exactResult
        = GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("ssh://git@git.example.com:2222/team/repo.git")), { *wildcard, *exact });
    QVERIFY(exactResult);
    QCOMPARE(exactResult->provider, GitForge::Provider::GitLab);

    const auto relativeBase
        = GitForge::hostMapping(QStringLiteral("example.com"), GitForge::Provider::GitLab, QUrl(QStringLiteral("https://example.com/gitlab")));
    QVERIFY(relativeBase);
    const auto relativeResult
        = GitForge::resolveRepository(*GitForge::parseRemote(QStringLiteral("https://example.com/gitlab/team/repo.git")), { *relativeBase });
    QVERIFY(relativeResult);
    QCOMPARE(relativeResult->path, QStringLiteral("team/repo"));
}

void GitForgeUrlTest::buildUrl_data()
{
    QTest::addColumn<GitForge::Provider>("provider");
    QTest::addColumn<QString>("base");
    QTest::addColumn<QString>("ref");
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("first");
    QTest::addColumn<int>("last");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("github-range") << GitForge::Provider::GitHub << QStringLiteral("https://github.com") << QStringLiteral("main")
                                  << QStringLiteral("src/file.cpp") << 10 << 18 << QByteArray("https://github.com/owner/repo/blob/main/src/file.cpp#L10-L18");
    QTest::newRow("gitlab-line") << GitForge::Provider::GitLab << QStringLiteral("https://gitlab.example/git") << QStringLiteral("feature/topic")
                                 << QStringLiteral("a file.cpp") << 4 << 4
                                 << QByteArray("https://gitlab.example/git/owner/repo/-/blob/feature/"
                                               "topic/a%20file.cpp#L4");
}

void GitForgeUrlTest::buildUrl()
{
    QFETCH(GitForge::Provider, provider);
    QFETCH(QString, base);
    QFETCH(QString, ref);
    QFETCH(QString, path);
    QFETCH(int, first);
    QFETCH(int, last);
    QFETCH(QByteArray, expected);
    const GitForge::Repository repository{ provider, QUrl(base), QStringLiteral("owner/repo") };
    const auto url = GitForge::blobUrl(repository, ref, path, GitForge::LineRange{ first, last });
    QVERIFY(url);
    QCOMPARE(url->toEncoded(), expected);
}

void GitForgeUrlTest::lineRange_data()
{
    QTest::addColumn<int>("startLine");
    QTest::addColumn<int>("endLine");
    QTest::addColumn<int>("endColumn");
    QTest::addColumn<int>("first");
    QTest::addColumn<int>("last");
    QTest::newRow("single-line") << 4 << 4 << 8 << 5 << 5;
    QTest::newRow("multi-line") << 4 << 5 << 3 << 5 << 6;
    QTest::newRow("end-column-zero") << 4 << 5 << 0 << 5 << 5;
    QTest::newRow("whole-lines") << 4 << 7 << 0 << 5 << 7;
}

void GitForgeUrlTest::lineRange()
{
    QFETCH(int, startLine);
    QFETCH(int, endLine);
    QFETCH(int, endColumn);
    QFETCH(int, first);
    QFETCH(int, last);
    const auto range = GitForge::selectedLineRange(startLine, endLine, endColumn);
    QCOMPARE(range.first, first);
    QCOMPARE(range.last, last);
}

void GitForgeUrlTest::providerApiUrls()
{
    const auto urls = GitForge::providerApiUrls(QUrl(QStringLiteral("https://code.example.com/gitlab/")));
    QCOMPARE(urls.size(), 2);
    QCOMPARE(urls.at(0).first, GitForge::Provider::GitLab);
    QCOMPARE(urls.at(0).second,
        QUrl(QStringLiteral("https://code.example.com/gitlab/api/v4/"
                            "projects?simple=true&per_page=1")));
    QCOMPARE(urls.at(1).first, GitForge::Provider::GitHub);
    QCOMPARE(urls.at(1).second, QUrl(QStringLiteral("https://code.example.com/gitlab/api/v3")));

    QVERIFY(GitForge::providerApiUrls(QUrl(QStringLiteral("file:///tmp/gitlab"))).isEmpty());
    QVERIFY(GitForge::providerApiUrls(QUrl(QStringLiteral("https://code.example.com/?query=invalid"))).isEmpty());
}

void GitForgeUrlTest::providerApiResponse()
{
    const QByteArray gitLab = R"([{"id":1,"name":"public-project"}])";
    const QByteArray privateGitLab = R"({"message":"401 Unauthorized"})";
    const QByteArray gitHub = R"({"current_user_url":"https://api.example/user","repository_url":"https://api.example/repos/{owner}/{repo}"})";

    QVERIFY(GitForge::isProviderApiResponse(GitForge::Provider::GitLab, 200, gitLab));
    QVERIFY(GitForge::isProviderApiResponse(GitForge::Provider::GitLab, 401, privateGitLab));
    QVERIFY(GitForge::isProviderApiResponse(GitForge::Provider::GitHub, 200, gitHub));
    QVERIFY(!GitForge::isProviderApiResponse(GitForge::Provider::GitHub, 200, gitLab));
    QVERIFY(!GitForge::isProviderApiResponse(GitForge::Provider::GitLab, 401, gitLab));
    QVERIFY(!GitForge::isProviderApiResponse(GitForge::Provider::GitLab, 200, QByteArrayLiteral("not json")));
}

QTEST_GUILESS_MAIN(GitForgeUrlTest)

#include "gitforgeurltest.moc"
