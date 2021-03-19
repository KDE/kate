#ifndef KATEPROJECTCODEANALYSISTOOLCLAZY_H
#define KATEPROJECTCODEANALYSISTOOLCLAZY_H

#include <kateprojectcodeanalysistool.h>

class KateProjectCodeAnalysisToolClazy : public KateProjectCodeAnalysisTool
{
public:
    explicit KateProjectCodeAnalysisToolClazy(QObject *parent = nullptr);

    ~KateProjectCodeAnalysisToolClazy() override = default;

    QString name() const override;

    QString description() const override;

    QString fileExtensions() const override;

    QStringList filter(const QStringList &files) const override;

    QString path() const override;

    QStringList arguments() override;

    QString notInstalledMessage() const override;

    QStringList parseLine(const QString &line) const override;

    QString stdinMessages() override;
};

#endif // KATEPROJECTCODEANALYSISTOOLCLAZY_H
