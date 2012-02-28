#ifndef KATE_COLOR_TREE_WIDGET_H
#define KATE_COLOR_TREE_WIDGET_H

#include <QtGui/QTreeWidget>

class KConfigGroup;
class KateColorTreeItem;

class KateColorTreeWidget : public QTreeWidget
{
  Q_OBJECT

  public:
    explicit KateColorTreeWidget(QWidget *parent = 0);

  public:
    void readConfig(KConfigGroup& config);
    void writeConfig(KConfigGroup& config);

  public:
    virtual void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

  public Q_SLOTS:
    void selectDefaults();
    
    void testReadConfig();
    void testWriteConfig();

  Q_SIGNALS:
    void changed();

  protected:
    virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event);
    
  private:
    void init();

  private:
    // editor background colors
    KateColorTreeItem* m_textArea;
    KateColorTreeItem* m_selectedText;
    KateColorTreeItem* m_currentLine;
    KateColorTreeItem* m_searchHighlight;
    KateColorTreeItem* m_replaceHighlight;

    // icon border
    KateColorTreeItem* m_background;
    KateColorTreeItem* m_lineNumbers;
    KateColorTreeItem* m_wordWrapMarker;
    KateColorTreeItem* m_savedLine;
    KateColorTreeItem* m_modifiedLine;

    // text decorations
    KateColorTreeItem* m_spellingMistake;
    KateColorTreeItem* m_spaceMarker;
    KateColorTreeItem* m_bracketHighlight;

    // marker colors
    KateColorTreeItem* m_bookmark;
    KateColorTreeItem* m_activeBreakpoint;
    KateColorTreeItem* m_reachedBreakpoint;
    KateColorTreeItem* m_disabledBreakpoint;
    KateColorTreeItem* m_execution;
    KateColorTreeItem* m_warning;
    KateColorTreeItem* m_error;

    // text templates
    KateColorTreeItem* m_templateBackground;
    KateColorTreeItem* m_templateEditablePlaceholder;
    KateColorTreeItem* m_templateFocusedEditablePlaceholder;
    KateColorTreeItem* m_templateNotEditablePlaceholder;
};

#endif

// kate: indent-width 2; replace-tabs on;
