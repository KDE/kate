/*
    SPDX-FileCopyrightText: 2025 Dennis Lübke <kde@dennis2society.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "gpgmeppwrapper.hpp"

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <memory>

/**
 * Holds the per-document UI state that is saved and restored when switching
 * between document tabs at runtime.
 */
struct DocumentUIState {
    bool symmetricEncryption = false;
    int selectedRowIndex = 0;
    QString selectedFingerprint;
    QString selectedEmailAddress;
    bool isDecrypted = false;
};

// forward declaration
class GPGKeyDetails;

class KateGPGPlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit KateGPGPlugin(QObject *parent, const QList<QVariant> & = QList<QVariant>());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    // Shared per-document UI state — all windows access the same map so that
    // opening the same document in a second window reflects the correct state
    // (e.g. which key / symmetric mode was used when the file was decrypted).
    QMap<KTextEditor::Document *, DocumentUIState> &documentStates();

    // Emit this after updating isDecrypted in the shared map so every open
    // window refreshes its encryption status indicator for the given document.
    void notifyEncryptionStateChanged(KTextEditor::Document *doc);

Q_SIGNALS:
    void documentEncryptionStateChanged(KTextEditor::Document *doc);

private:
    QMap<KTextEditor::Document *, DocumentUIState> m_documentStates;
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
    KateGPGPlugin *m_plugin = nullptr;
    // The top level toolview widget
    std::unique_ptr<QWidget> m_toolview;

    std::unique_ptr<GPGMeWrapper> m_gpgWrapper;

    int m_selectedRowIndex = 0;

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
    QCheckBox *m_symmetricEncryptioCheckbox;
    QCheckBox *m_showOnlyPrivateKeysCheckbox;
    QCheckBox *m_hideExpiredKeysCheckbox;
    QTableWidget *m_gpgKeyTable;
    QStringList m_gpgKeyTableHeader;

    // Per-document UI state is stored in the plugin (shared across all windows)
    KTextEditor::Document *m_currentDocument = nullptr;
    // Status bar label showing whether the current GPG tab is decrypted or encrypted
    QLabel *m_encryptionStatusLabel = nullptr;

    // private functions
    void updateKeyTable();

    const QTableWidgetItem convertKeyDetailsToTableItem(const GPGKeyDetails &keyDetails_);

    void makeTableCell(const QString cellValue, uint row, uint col);

    // Functions to hook into Kate's save dialog
    // (used for auto-encryption on save)
    void connectToOpenAndSaveDialog(KTextEditor::View *view);
    void onDocumentWillSave(KTextEditor::Document *doc);
    void onDocumentOpened(KTextEditor::View *view);
    void decryptView(KTextEditor::View *v);

    // Function to generate translatable Kate-conform error/warning messages
    QVariantMap generateMessage(const QString translatebleMessage, const QString messageType);

    // Update the encryption status indicator in Kate's status bar for the given document
    void updateEncryptionStatusLabel(KTextEditor::Document *doc);
};
