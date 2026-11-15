/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QFrame>
#include <QString>
#include <QStringList>

class QLabel;
class QToolButton;

/**
 * @class ACPCollapsibleTextWidget
 * @brief Displays a block of text that is collapsed when it gets too long
 *
 * Tool call inputs and outputs can be arbitrarily long and would otherwise
 * push the rest of the conversation out of view. Long text is therefore shown
 * as a preview consisting of the first and last few lines, separated by a
 * button that states how much is hidden.
 *
 * Both the header and the separator are buttons, so the text can be expanded
 * and collapsed again with the mouse or the keyboard. Short text is displayed
 * in full, without a separator.
 *
 * The block is framed and always labelled, so that adjacent blocks such as the
 * input and the output of a tool call can be told apart.
 *
 * @see ACPChatMessageWidget which uses this for tool call input and output
 */
class ACPCollapsibleTextWidget : public QFrame
{
    Q_OBJECT

public:
    /**
     * @brief Construct a collapsible text block
     * @param label Name of the block, shown in the header (e.g. "Output")
     * @param parent Qt parent widget
     */
    explicit ACPCollapsibleTextWidget(const QString &label, QWidget *parent = nullptr);

    /** @brief Set the displayed text */
    void setText(const QString &text);

    /** @brief Get the displayed text */
    QString text() const;

    /** @brief Use a monospace font for the text */
    void setMonospace(bool monospace);

    /**
     * @brief Set the number of lines that are displayed without collapsing
     * @param lines Line count above which the text is collapsed
     */
    void setCollapsedLineLimit(int lines);

    /**
     * @brief Set how many lines are shown at each end of collapsed text
     * @param lines Number of leading and trailing lines
     */
    void setContextLineCount(int lines);

    /**
     * @brief Allow collapsing the text no matter how short it is
     * @param always true to always offer the toggle
     *
     * Used for text that is secondary regardless of its size, such as the
     * reasoning of an agent.
     */
    void setAlwaysCollapsible(bool always);

    /** @brief Whether the text is long enough to be collapsed */
    bool isCollapsible() const;

    /** @brief Whether the full text is currently shown */
    bool isExpanded() const;

    /** @brief Show or hide the full text */
    void setExpanded(bool expanded);

Q_SIGNALS:
    /** @brief Emitted when the user expanded or collapsed the text */
    void expandedChanged(bool expanded);

private:
    /** @brief Apply the current text and expansion state to the child widgets */
    void updateDisplay();

    /** @brief Number of lines hidden by the preview, 0 if lines are not hidden */
    int hiddenLineCount() const;

    /** @brief Number of characters hidden by the preview of a single long line */
    int hiddenCharacterCount() const;

    /** @brief Join the given lines, shortening overly long ones */
    QString previewBlock(int from, int count) const;

    QString m_label; ///< Header label of the block
    QString m_text; ///< Full text
    QStringList m_lines; ///< Full text, split into lines
    bool m_expanded = false; ///< Whether the full text is shown
    int m_collapsedLineLimit = 10; ///< Collapse above this many lines
    int m_contextLines = 2; ///< Lines kept visible at each end
    int m_collapsedCharLimit = 2000; ///< Collapse above this many characters
    bool m_alwaysCollapsible = false; ///< Offer the toggle regardless of the size of the text

    QLabel *m_nameLabel = nullptr; ///< Always visible name of the block
    QToolButton *m_headerButton = nullptr; ///< Expands and collapses the text
    QLabel *m_headLabel = nullptr; ///< Full text, or its leading lines
    QToolButton *m_moreButton = nullptr; ///< States how much text is hidden
    QLabel *m_tailLabel = nullptr; ///< Trailing lines of collapsed text
};
