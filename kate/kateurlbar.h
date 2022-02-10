/*
    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_URL_BAR_H
#define KATE_URL_BAR_H

#include "kateviewspace.h"
#include <QFrame>

class KateUrlBar : public QWidget
{
    Q_OBJECT
public:
    explicit KateUrlBar(KateViewSpace *parent = nullptr);

Q_SIGNALS:
    void openUrlRequested(const QUrl &url);

private:
    // helper struct containing dir name and path
    struct DirNamePath {
        QString name;
        QString path;
    };

    std::pair<QString, QVector<DirNamePath>> splittedUrl(const QUrl &u);
    class QLabel *separatorLabel();
    class QToolButton *dirButton(const QString &dirName, const QString &path);
    class QLabel *fileLabel(const QString &file);
    class QHBoxLayout *m_layout;
    void pathClicked();
    QVector<std::pair<QToolButton *, QString>> m_paths;
};

#endif
