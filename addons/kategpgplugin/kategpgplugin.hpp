/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <gpgmeppwrapper.hpp>
#include <memory>

// forward declaration
class GPGKeyDetails;

class KateGPGPlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit KateGPGPlugin(QObject *parent, const QList<QVariant> & = QList<QVariant>())
        : KTextEditor::Plugin(parent)
    {
    }

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class KateGPGPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit KateGPGPluginView(KateGPGPlugin *plugin, KTextEditor::MainWindow *mainwindow);

    ~KateGPGPluginView();

    void onViewChanged(KTextEditor::View *v);

public Q_SLOTS:
    void onTableViewSelection(); // listen to changes in the GPG key list table
    void onPreferredEmailAddressChanged();
    void onShowOnlyPrivateKeysChanged();
    void onHideExpiredKeysChanged();
    void decryptButtonPressed();
    void encryptButtonPressed();

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    // The top level toolview widget
    std::unique_ptr<QWidget> m_toolview;

    // const QString m_kateConfig = QString::fromUtf8("katerc");
    const QString m_pluginConfigGroupName = QStringLiteral("gpgplugin");
    GPGMeWrapper *m_gpgWrapper = nullptr;

    int m_selectedRowIndex;

    QPushButton *m_gpgDecryptButton = nullptr;
    QPushButton *m_gpgEncryptButton = nullptr;

    QVBoxLayout *m_verticalLayout;
    QLabel *m_titleLabel;
    QLabel *m_preferredEmailAddressLabel;
    QLabel *m_preferredGPGKeyIDLabel;
    QString m_title;
    QString m_preferredEmailAddress;
    QLineEdit *m_preferredEmailLineEdit;
    QString m_preferredGPGKeyID;
    QLabel *m_EmailAddressSelectLabel;
    QComboBox *m_preferredEmailAddressComboBox;
    QLineEdit *m_selectedKeyIndexEdit;
    QCheckBox *m_saveAsASCIICheckbox;
    QCheckBox *m_symmetricEncryptioCheckbox;
    QCheckBox *m_showOnlyPrivateKeysCheckbox;
    QCheckBox *m_hideExpiredKeysCheckbox;
    QTableWidget *m_gpgKeyTable;
    QStringList m_gpgKeyTableHeader;

    KConfigGroup m_group;

    // private functions
    void updateKeyTable();

    const QTableWidgetItem convertKeyDetailsToTableItem(const GPGKeyDetails &keyDetails_);

    void makeTableCell(const QString cellValue, uint row, uint col);

    void readPluginConfig();
    void savePluginConfig();

    // Functions to hook into Kate's save dialog
    // (used for auto-encryption on save)
    void connectToOpenAndSaveDialog(KTextEditor::Document *doc);
    void onDocumentWillSave(KTextEditor::Document *doc);
    void onDocumentOpened(KTextEditor::Document *doc);

    // Function to generate translatable Kate-conform error/warning messages
    const QVariantMap generateMessage(const QString translatebleMessage, const QString messageType);
};
