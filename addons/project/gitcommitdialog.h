#ifndef GITCOMMITDIALOG_H
#define GITCOMMITDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

#include <memory>

class SingleLineEdit;
class BadLengthHighlighter;

class QFont;

class GitCommitDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GitCommitDialog(const QString &lastCommit, const QFont &font, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    QString subject() const;
    QString description() const;

private:
    Q_SLOT void updateLineSizeLabel();

    SingleLineEdit *m_le;
    QPlainTextEdit m_pe;
    QPushButton ok;
    QPushButton cancel;
    QLabel m_leLen;
    QLabel m_peLen;
    BadLengthHighlighter *m_hl;
};

#endif // GITCOMMITDIALOG_H
