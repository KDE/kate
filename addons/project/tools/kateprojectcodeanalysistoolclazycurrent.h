#ifndef KATEPROJECTCODEANALYSISTOOLCLANGTIDY_H
#define KATEPROJECTCODEANALYSISTOOLCLANGTIDY_H

#include "kateprojectcodeanalysistoolclazy.h"

class KateProjectCodeAnalysisToolClazyCurrent : public KateProjectCodeAnalysisToolClazy
{
public:
    KateProjectCodeAnalysisToolClazyCurrent(QObject *parent);

    QString name() const override;
    QString description() const override;
    QStringList arguments() override;
};

#endif // KATEPROJECTCODEANALYSISTOOLCLANGTIDY_H
