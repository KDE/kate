/*
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MatchModel_h
#define MatchModel_h

#include <QAbstractItemModel>
#include <QBrush>
#include <QPointer>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <QUrl>

#include <KTextEditor/Cursor>
#include <KTextEditor/MovingRange>
#include <KTextEditor/Range>
#include <ktexteditor/application.h>

/**
 * data holder for one match in one file
 * used to transfer and hold multiple matches at once via signals to avoid heavy costs for files with a lot of matches
 */
class KateSearchMatch
{
public:
    QString preMatchStr;
    QString matchStr;
    QString postMatchStr;
    QString replaceText;
    KTextEditor::Range range;
    bool checked;
};

class MatchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum SearchPlaces { CurrentFile, OpenFiles, Folder, Project, AllProjects };
    Q_ENUM(SearchPlaces)

    enum SearchState { SearchDone, Preparing, Searching, Replacing };
    Q_ENUM(SearchState)

    enum MatchDataRoles {
        FileUrlRole = Qt::UserRole,
        DocumentRole,
        FileNameRole,
        StartLineRole,
        StartColumnRole,
        EndLineRole,
        EndColumnRole,
        PreMatchRole,
        MatchRole,
        PostMatchRole,
        ReplacedRole,
        ReplaceTextRole,
        PlainTextRole
    };
    Q_ENUM(MatchDataRoles)

    static constexpr int PreContextLen = 80;
    static constexpr int PostContextLen = 100;

    typedef KateSearchMatch Match;

    /// Utility function that is used to figure out how much context text we want to show
    /// before and after the match
    /// @p lineLength length of the line we are in
    /// @return a pair, first = preContextStart i.e., the start position in line
    /// second = postContextLen i.e., how much of post text we will show
    static constexpr std::pair<int, int> contextLengths(int lineLength, int matchStart, int matchEnd)
    {
        int preContextStart = 0;
        const int requiredPostLen = lineLength - matchEnd;
        if (requiredPostLen < MatchModel::PostContextLen) {
            int extra = MatchModel::PostContextLen - requiredPostLen;
            preContextStart = qMax(0, matchStart - (MatchModel::PreContextLen + extra));
        } else {
            preContextStart = qMax(0, matchStart - MatchModel::PreContextLen);
        }

        int postContextLen = 0;
        const int requiredPreLen = matchStart;
        if (matchStart < MatchModel::PreContextLen) {
            int extra = MatchModel::PreContextLen - requiredPreLen;
            postContextLen = MatchModel::PostContextLen + extra;
        } else {
            postContextLen = MatchModel::PostContextLen;
        }

        return {preContextStart, postContextLen};
    }

private:
    struct MatchFile {
        QUrl fileUrl;
        QVector<KateSearchMatch> matches;
        QPointer<KTextEditor::Document> doc;
        Qt::CheckState checkState = Qt::Checked;
    };

public:
    MatchModel(QObject *parent = nullptr);
    ~MatchModel() override;

    void setDocumentManager(KTextEditor::Application *manager);

    void setMatchColors(const QString &foreground, const QString &background, const QString &replaceBackground);

    void setSearchPlace(MatchModel::SearchPlaces searchPlace);

    void setSearchState(MatchModel::SearchState searchState);

    void setBaseSearchPath(const QString &baseSearchPath);

    void setProjectName(const QString &projectName);

    /** This function clears all matches in all files */
    void clear();

    bool isEmpty() const
    {
        return m_matchFiles.isEmpty();
    }

    const QVector<KateSearchMatch> &fileMatches(KTextEditor::Document *doc) const;

    void updateMatchRanges(const QVector<KTextEditor::MovingRange *> &ranges);

    void uncheckAll();

    static QString generateReplaceString(const QRegularExpressionMatch &match, const QString &replaceString);

public Q_SLOTS:

    /** This function returns the row index of the specified file.
     * If the file does not exist in the model, the file will be added to the model. */
    int matchFileRow(const QUrl &fileUrl, KTextEditor::Document *doc) const;

    /** This function is used to add a new file */
    /** @p doc may be null if we are searching disk files for instance */
    void addMatches(const QUrl &fileUrl, const QVector<KateSearchMatch> &searchMatches, KTextEditor::Document *doc);

    /** This function is used to set the last added file to the search list.
     * This is done to update the match tree when we generate the search file list. */
    void setFileListUpdate(const QString &path);

    /** Initiate a replace of all matches that have been checked.
     * The actual replacing is split up into slot calls that are added to the event loop */
    void replaceChecked(const QRegularExpression &regExp, const QString &replaceString);

    /** Cancel the replacing of checked matches. NOTE: This will only be handled when the next file is handled */
    void cancelReplace();

Q_SIGNALS:
    void replaceDone();

    // QModelIndex api. Use with care if you are accessing it directly or access through 'Results' instead
public:
    bool isMatch(const QModelIndex &itemIndex) const;
    QModelIndex fileIndex(const QUrl &url, KTextEditor::Document *doc) const;
    QModelIndex firstMatch() const;
    QModelIndex lastMatch() const;
    QModelIndex firstFileMatch(KTextEditor::Document *doc) const;
    QModelIndex closestMatchAfter(KTextEditor::Document *doc, const KTextEditor::Cursor &cursor) const;
    QModelIndex closestMatchBefore(KTextEditor::Document *doc, const KTextEditor::Cursor &cursor) const;
    QModelIndex nextMatch(const QModelIndex &itemIndex) const;
    QModelIndex prevMatch(const QModelIndex &itemIndex) const;

    KTextEditor::Range matchRange(const QModelIndex &matchIndex) const;

    /** This function is used to replace a single match */
    bool replaceSingleMatch(KTextEditor::Document *doc, const QModelIndex &matchIndex, const QRegularExpression &regExp, const QString &replaceString);

    // Model-View model functions
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private Q_SLOTS:
    void doReplaceNextMatch();

private:
    bool replaceMatch(KTextEditor::Document *doc, const QModelIndex &matchIndex, const QRegularExpression &regExp, const QString &replaceString);

    QString infoHtmlString() const;
    QString fileToHtmlString(const MatchFile &matchFile) const;
    QString matchToHtmlString(const Match &match) const;

    QString infoToPlainText() const;
    QString fileToPlainText(const MatchFile &matchFile) const;
    QString matchToPlainText(const Match &match) const;

    bool setFileChecked(int fileRow, bool checked);

    Match *matchFromIndex(const QModelIndex &matchIndex);

    QVector<MatchFile> m_matchFiles;
    QHash<QUrl, int> m_matchFileIndexHash;
    // for unsaved documents with no url
    QHash<KTextEditor::Document *, int> m_matchUnsavedFileIndexHash;
    QString m_searchBackgroundColor;
    QString m_foregroundColor;
    QString m_replaceHighlightColor;

    Qt::CheckState m_infoCheckState = Qt::Checked;
    SearchPlaces m_searchPlace = CurrentFile;
    SearchState m_searchState = SearchDone;
    QString m_resultBaseDir;
    QString m_projectName;
    QUrl m_lastMatchUrl;
    QString m_lastSearchPath;
    QTimer m_infoUpdateTimer;

    // Replacing related objects
    KTextEditor::Application *m_docManager = nullptr;
    int m_replaceFile = -1;
    QRegularExpression m_regExp;
    QString m_replaceText;
    bool m_cancelReplace = true;

    friend class Results;
};

// tests
// output = {start, postContextLen}

// precontext starts from 0 instead of at a higher place as post context is small
static_assert(MatchModel::contextLengths(140, 110, 114) == std::pair<int, int>{0, 100});
static_assert(MatchModel::contextLengths(140, 90, 114) == std::pair<int, int>{0, 100});

// post context length increases from hundered as precontext is small here
static_assert(MatchModel::contextLengths(140, 40, 50) == std::pair<int, int>{0, 140});
static_assert(MatchModel::contextLengths(140, 20, 50) == std::pair<int, int>{0, 160});

Q_DECLARE_METATYPE(KateSearchMatch)

#endif
