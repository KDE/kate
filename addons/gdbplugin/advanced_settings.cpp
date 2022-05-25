// Description: Advanced settings dialog for gdb
//
//
// SPDX-FileCopyrightText: 2012 Kåre Särs <kare.sars@iki.fi>
//
//  SPDX-License-Identifier: LGPL-2.0-only

#include "advanced_settings.h"

#ifdef WIN32
static const QLatin1Char pathSeparator(';');
#else
static const QLatin1Char pathSeparator(':');
#endif

#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>

const QString AdvancedGDBSettings::F_GDB = QStringLiteral("gdb");
const QString AdvancedGDBSettings::F_SRC_PATHS = QStringLiteral("srcPaths");
const static QString F_LOCAL_REMOTE = QStringLiteral("localRemote");
const static QString F_REMOTE_BAUD = QStringLiteral("remoteBaud");
const static QString F_SO_ABSOLUTE = QStringLiteral("soAbsolute");
const static QString F_SO_RELATIVE = QStringLiteral("soRelative");
const static QString F_CUSTOM_INIT = QStringLiteral("customInit");

AdvancedGDBSettings::AdvancedGDBSettings(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    u_gdbBrowse->setIcon(QIcon::fromTheme(QStringLiteral("application-x-ms-dos-executable")));
    connect(u_gdbBrowse, &QToolButton::clicked, this, &AdvancedGDBSettings::slotBrowseGDB);

    u_setSoPrefix->setIcon(QIcon::fromTheme(QStringLiteral("folder")));
    connect(u_setSoPrefix, &QToolButton::clicked, this, &AdvancedGDBSettings::slotSetSoPrefix);

    u_addSoSearchPath->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    u_delSoSearchPath->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    connect(u_addSoSearchPath, &QToolButton::clicked, this, &AdvancedGDBSettings::slotAddSoPath);
    connect(u_delSoSearchPath, &QToolButton::clicked, this, &AdvancedGDBSettings::slotDelSoPath);

    u_addSrcPath->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    u_delSrcPath->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    connect(u_addSrcPath, &QToolButton::clicked, this, &AdvancedGDBSettings::slotAddSrcPath);
    connect(u_delSrcPath, &QToolButton::clicked, this, &AdvancedGDBSettings::slotDelSrcPath);

    connect(u_buttonBox, &QDialogButtonBox::accepted, this, &AdvancedGDBSettings::accept);
    connect(u_buttonBox, &QDialogButtonBox::rejected, this, &AdvancedGDBSettings::reject);

    connect(u_localRemote, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &AdvancedGDBSettings::slotLocalRemoteChanged);
}

AdvancedGDBSettings::~AdvancedGDBSettings()
{
}

const QJsonObject AdvancedGDBSettings::configs() const
{
    QJsonObject conf;

    // gdb
    conf[F_GDB] = u_gdbCmd->text();

    // local/remote, baud
    switch (u_localRemote->currentIndex()) {
    case 1:
        conf[F_LOCAL_REMOTE] = QStringLiteral("target remote %1:%2").arg(u_tcpHost->text(), u_tcpPort->text());
        break;
    case 2:
        conf[F_LOCAL_REMOTE] = QStringLiteral("target remote %1").arg(u_ttyPort->text());
        conf[F_REMOTE_BAUD] = QStringLiteral("set remotebaud %1").arg(u_baudCombo->currentText());
        break;
    default:
        break;
    }

    // solib absolute
    if (!u_soAbsPrefix->text().isEmpty()) {
        conf[F_SO_ABSOLUTE] = QStringLiteral("set solib-absolute-prefix %1").arg(u_soAbsPrefix->text());
    }

    // solib search path
    if (u_soSearchPaths->count() > 0) {
        QString paths = QStringLiteral("set solib-search-path ");
        for (int i = 0; i < u_soSearchPaths->count(); ++i) {
            if (i != 0) {
                paths += pathSeparator;
            }
            paths += u_soSearchPaths->item(i)->text();
        }
        conf[F_SO_RELATIVE] = paths;
    }

    // source paths
    if (u_srcPaths->count() > 0) {
        QJsonArray paths;
        for (int i = 0; i < u_srcPaths->count(); ++i) {
            const QString path = u_srcPaths->item(i)->text().trimmed();
            if (!path.isEmpty()) {
                paths << path;
            }
        }
        if (!paths.isEmpty()) {
            conf[F_SRC_PATHS] = paths;
        }
    }

    // custom init
    const auto cinit = u_customInit->toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (!cinit.isEmpty()) {
        conf[F_CUSTOM_INIT] = QJsonArray::fromStringList(cinit);
    }

    return conf;
}

QStringList AdvancedGDBSettings::commandList(const QJsonObject &config)
{
    QStringList commands;

    auto insertString = [&commands, config](const QString &field) {
        const QString value = config[field].toString().trimmed();
        if (!value.isEmpty()) {
            commands << value;
        }
    };

    insertString(F_LOCAL_REMOTE);
    insertString(F_REMOTE_BAUD);
    insertString(F_SO_ABSOLUTE);
    insertString(F_SO_RELATIVE);

    for (const auto &value : config[F_CUSTOM_INIT].toArray()) {
        commands << value.toString();
    }

    return commands;
}

QJsonObject AdvancedGDBSettings::upgradeConfigV4_5(const QStringList &cfgs)
{
    const int size = cfgs.count();

    QJsonObject conf;

    auto insert = [&conf, cfgs, size](CustomStringOrder index, const QString &field) {
        if (index >= size)
            return;

        const QString value = cfgs[index].trimmed();
        if (!value.isEmpty()) {
            conf[field] = value;
        }
    };

    // gdb
    insert(GDBIndex, F_GDB);
    // localremoteindex
    insert(LocalRemoteIndex, F_LOCAL_REMOTE);
    // remotebaudindex
    insert(RemoteBaudIndex, F_REMOTE_BAUD);
    // soabsoluteindex
    insert(SoAbsoluteIndex, F_SO_ABSOLUTE);
    // sorelativeindex
    insert(SoRelativeIndex, F_SO_RELATIVE);
    // srcpathsindex
    if (SrcPathsIndex < size) {
        QString allPaths = cfgs[SrcPathsIndex];
        if (allPaths.startsWith(QStringLiteral("set directories "))) {
            allPaths = allPaths.mid(16);
        }
        QStringList paths = allPaths.split(pathSeparator, Qt::SkipEmptyParts);
        if (!paths.isEmpty()) {
            conf[F_SRC_PATHS] = QJsonArray::fromStringList(paths);
        }
    }
    // customstart
    if (CustomStartIndex < size) {
        QJsonArray parts;
        for (int i = CustomStartIndex; i < size; i++) {
            parts << cfgs[i];
        }
        conf[F_CUSTOM_INIT] = parts;
    }

    return conf;
}

void AdvancedGDBSettings::setConfigs(const QJsonObject &cfgs)
{
    // clear all info
    u_gdbCmd->setText(QStringLiteral("gdb"));
    u_localRemote->setCurrentIndex(0);
    u_soAbsPrefix->clear();
    u_soSearchPaths->clear();
    u_srcPaths->clear();
    u_customInit->clear();
    u_tcpHost->setText(QString());
    u_tcpPort->setText(QString());
    u_ttyPort->setText(QString());
    u_baudCombo->setCurrentIndex(0);

    // GDB
    if (cfgs.contains(F_GDB)) {
        u_gdbCmd->setText(cfgs[F_GDB].toString());
    }

    // Local / Remote
    const auto localRemote = cfgs[F_LOCAL_REMOTE].toString();

    int start;
    int end;
    if (localRemote.isEmpty()) {
        u_localRemote->setCurrentIndex(0);
        u_remoteStack->setCurrentIndex(0);
    } else if (localRemote.contains(pathSeparator)) {
        u_localRemote->setCurrentIndex(1);
        u_remoteStack->setCurrentIndex(1);
        start = localRemote.lastIndexOf(QLatin1Char(' '));
        end = localRemote.indexOf(pathSeparator);
        u_tcpHost->setText(localRemote.mid(start + 1, end - start - 1));
        u_tcpPort->setText(localRemote.mid(end + 1));
    } else {
        u_localRemote->setCurrentIndex(2);
        u_remoteStack->setCurrentIndex(2);
        start = localRemote.lastIndexOf(QLatin1Char(' '));
        u_ttyPort->setText(localRemote.mid(start + 1));

        const auto remoteBaud = cfgs[F_REMOTE_BAUD].toString();
        start = remoteBaud.lastIndexOf(QLatin1Char(' '));
        setComboText(u_baudCombo, remoteBaud.mid(start + 1));
    }

    // Solib absolute path
    if (cfgs.contains(F_SO_ABSOLUTE)) {
        start = 26; // "set solib-absolute-prefix "
        u_soAbsPrefix->setText(cfgs[F_SO_ABSOLUTE].toString().mid(start));
    }

    // Solib search path
    if (cfgs.contains(F_SO_RELATIVE)) {
        start = 22; // "set solib-search-path "
        QString tmp = cfgs[F_SO_RELATIVE].toString().mid(start);
        u_soSearchPaths->addItems(tmp.split(pathSeparator));
    }

    if (cfgs.contains(F_SRC_PATHS)) {
        QStringList paths;
        for (const auto &value : cfgs[F_SRC_PATHS].toArray()) {
            paths << value.toString();
        }
        u_srcPaths->addItems(paths);
    }

    // Custom init
    if (cfgs.contains(F_CUSTOM_INIT)) {
        for (const auto &line : cfgs[F_CUSTOM_INIT].toArray()) {
            u_customInit->appendPlainText(line.toString());
        }
    }

    slotLocalRemoteChanged();
}

void AdvancedGDBSettings::slotBrowseGDB()
{
    u_gdbCmd->setText(QFileDialog::getOpenFileName(this, QString(), u_gdbCmd->text(), QStringLiteral("application/x-executable")));
    if (u_gdbCmd->text().isEmpty()) {
        u_gdbCmd->setText(QStringLiteral("gdb"));
    }
}

void AdvancedGDBSettings::setComboText(QComboBox *combo, const QString &str)
{
    if (!combo) {
        return;
    }

    for (int i = 0; i < combo->count(); i++) {
        if (combo->itemText(i) == str) {
            combo->setCurrentIndex(i);
            return;
        }
    }
    // The string was not found -> add it
    combo->addItem(str);
    combo->setCurrentIndex(combo->count() - 1);
}

void AdvancedGDBSettings::slotSetSoPrefix()
{
    QString prefix = QFileDialog::getExistingDirectory(this);
    if (prefix.isEmpty()) {
        return;
    }

    u_soAbsPrefix->setText(prefix);
}

void AdvancedGDBSettings::slotAddSoPath()
{
    QString path = QFileDialog::getExistingDirectory(this);
    if (path.isEmpty()) {
        return;
    }

    u_soSearchPaths->addItem(path);
}

void AdvancedGDBSettings::slotDelSoPath()
{
    QListWidgetItem *item = u_soSearchPaths->takeItem(u_soSearchPaths->currentRow());
    delete item;
}

void AdvancedGDBSettings::slotAddSrcPath()
{
    QString path = QFileDialog::getExistingDirectory(this);
    if (path.isEmpty()) {
        return;
    }

    u_srcPaths->addItem(path);
}

void AdvancedGDBSettings::slotDelSrcPath()
{
    QListWidgetItem *item = u_srcPaths->takeItem(u_srcPaths->currentRow());
    delete item;
}

void AdvancedGDBSettings::slotLocalRemoteChanged()
{
    u_soAbsPrefixLabel->setEnabled(u_localRemote->currentIndex() != 0);
    u_soSearchLabel->setEnabled(u_localRemote->currentIndex() != 0);
    u_soAbsPrefix->setEnabled(u_localRemote->currentIndex() != 0);
    u_soSearchPaths->setEnabled(u_localRemote->currentIndex() != 0);
    u_setSoPrefix->setEnabled(u_localRemote->currentIndex() != 0);
    u_addDelSoPaths->setEnabled(u_localRemote->currentIndex() != 0);
}
