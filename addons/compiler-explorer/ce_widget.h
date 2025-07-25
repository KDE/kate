#pragma once

#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <QPointer>

class CEPluginView;
class AsmView;
class AsmViewModel;

class CEWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CEWidget(CEPluginView *pluginView, KTextEditor::MainWindow *mainWindow);
    ~CEWidget() override;

    Q_INVOKABLE bool shouldClose();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void createTopBar(class QVBoxLayout *mainLayout);
    void createMainViews(class QVBoxLayout *mainLayout);
    void setAvailableLanguages(const QByteArray &data);
    void setAvailableCompilers(const QByteArray &data);
    void repopulateCompilersCombo(const QString &lang);
    void initOptionsComboBox();
    bool compilationFailed(const QJsonObject &obj);
    void processAndShowAsm(const QByteArray &data);
    void doCompile();
    QString currentCompiler() const;
    void addExtraActionstoTextEditor();
    void warnIfBadArgs(const QStringList &args);
    void sendMessage(const QString &plainText, bool warn);

    void removeViewAsActiveXMLGuiClient();

    CEPluginView *m_pluginView = nullptr;

    /// The document that we are working with
    QPointer<KTextEditor::Document> doc = nullptr;

    KTextEditor::MainWindow *const m_mainWindow = nullptr;
    class QPointer<KTextEditor::View> m_textEditor = nullptr;
    class AsmView *m_asmView = nullptr;
    class AsmViewModel *m_model = nullptr;
    class QLineEdit *m_lineEdit = nullptr;
    class QComboBox *m_languagesCombo = nullptr;
    class QComboBox *m_compilerCombo = nullptr;
    class QToolButton *m_optsCombo = nullptr;
    class QPushButton *m_compileButton = nullptr;

    struct Compiler {
        QString name;
        QVariant id;
        QString language;
    };
    std::vector<Compiler> m_compilers;

    std::vector<Compiler> compilersForLanguage(const QString &lang) const;

Q_SIGNALS:
    void lineHovered(int line);
};
