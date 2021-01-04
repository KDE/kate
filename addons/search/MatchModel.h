/***************************************************************************
 *   This file is part of Kate search plugin
 *   SPDX-FileCopyrightText: 2020 Kåre Särs <kare.sars@iki.fi>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 ***************************************************************************/

#ifndef MatchModel_h
#define MatchModel_h

#include <QAbstractItemModel>
#include <QString>
#include <QUrl>
#include <QBrush>

#include <KTextEditor/Range>


/**
 * data holder for one match in one file
 * used to transfer multiple matches at once via signals to avoid heavy costs for files with a lot of matches
 */
class KateSearchMatch
{
public:
    QString lineContent;
    int matchLen;
    KTextEditor::Range matchRange;
};

Q_DECLARE_METATYPE(KateSearchMatch)



class MatchModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    enum MatchDataRoles {
        FileUrlRole = Qt::UserRole,
        FileNameRole,
        StartLineRole,
        StartColumnRole,
        EndLineRole,
        EndColumnRole,
        MatchLenRole,
        PreMatchRole,
        MatchRole,
        PostMatchRole,
        ReplacedRole,
        ReplaceTextRole
    };
    Q_ENUM(MatchDataRoles)

    struct Match {
        int startLine = 0;
        int startColumn = 0;
        int endLine = 0;
        int endColumn = 0;
        int matchLen = 0;
        QString preMatchStr;
        QString matchStr;
        QString postMatchStr;
        QString replaceText;
        bool checked = true;
    };

    struct MatchFile {
        QUrl fileUrl;
        QVector<Match> matches;
        Qt::CheckState checkState = Qt::Checked;
    };

    MatchModel(QObject *parent = nullptr);
    ~MatchModel() override;

    void setMatchColors(const QColor &foreground, const QColor &background, const QColor &replaseBackground);

public Q_SLOTS:

    /** This function clears all matches in all files */
    void clear();

    /** This function returns the row index of the specified file.
     * If the file does not exist in the model, the file will be added to the model. */
    int matchFileRow(const QUrl& fileUrl);

    /** This function is used to add a new file */
    void addMatches(const QUrl &fileUrl, const QVector<KateSearchMatch> &searchMatches);

//     /** This function is used to modify a match */
//     void replaceMatch(const QModelIndex &matchIndex, const QRegularExpression &regexp, const QString &replaceText);
//
//     /** Replace all matches that have been checked */
//     void replaceChecked(const QRegularExpression &regexp, const QString &replace);

Q_SIGNALS:

public:
    // Model-View model functions
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    QString matchToHtmlString(const Match &match) const;
    QString fileItemToHtmlString(const MatchFile &matchFile) const;

    bool setFileChecked(int fileRow, bool checked);

    QVector<MatchFile> m_matchFiles;
    QHash<QUrl, int> m_matchFileIndexHash;
    QColor m_searchBackgroundColor;
    QColor m_foregroundColor;
    QColor m_replaceHighlightColor;

    Qt::CheckState m_infoCheckState = Qt::Checked;
    QString m_infoHtmlString;
};

#endif
