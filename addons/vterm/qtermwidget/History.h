/*
    SPDX-FileCopyrightText: 1997, 1998 Lars Doelle <lars.doelle@on-line.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef HISTORYTYPE_H
#define HISTORYTYPE_H

#include <QTemporaryFile>
#include <QVector>
#include <deque>
#include <map>
#include <memory>

#include "Character.h"

typedef unsigned char LineProperty;

namespace Konsole
{
class HistoryScroll;
class HistoryType
{
public:
    HistoryType();
    virtual ~HistoryType();

    /**
     * Returns true if the history is enabled ( can store lines of output )
     * or false otherwise.
     */
    virtual bool isEnabled() const = 0;
    /**
     * Returns the maximum number of lines which this history type
     * can store or -1 if the history can store an unlimited number of lines.
     */
    virtual int maximumLineCount() const = 0;
    /**
     * Converts from one type of HistoryScroll to another or if given the
     * same type, returns it.
     */
    virtual void scroll(std::unique_ptr<HistoryScroll> &) const = 0;
    /**
     * Returns true if the history size is unlimited.
     */
    bool isUnlimited() const;
};

//////////////////////////////////////////////////////////////////////
// Abstract base class for file and buffer versions
//////////////////////////////////////////////////////////////////////
class HistoryType;

class HistoryScroll
{
public:
    explicit HistoryScroll(HistoryType *);
    virtual ~HistoryScroll();

    virtual bool hasScroll() const;

    // access to history
    virtual int getLines() const = 0;
    virtual int getMaxLines() const = 0;
    virtual int getLineLen(const int lineno) const = 0;
    virtual void getCells(const int lineno, const int colno, const int count, Character res[]) const = 0;
    virtual bool isWrappedLine(const int lineNumber) const = 0;
    virtual LineProperty getLineProperty(const int lineno) const = 0;
    virtual void setLineProperty(const int lineno, LineProperty prop) = 0;

    // adding lines.
    virtual void addCells(const Character a[], const int count) = 0;
    virtual void addCellsMove(Character a[], const int count) = 0;
    // convenience method - this is virtual so that subclasses can take advantage
    // of QVector's implicit copying
    virtual void addCellsVector(const QVector<Character> &cells)
    {
        addCells(cells.data(), cells.size());
    }

    virtual void addLine(const LineProperty lineProperty = LineProperty()) = 0;

    // modify history
    virtual void removeCells() = 0;
    virtual int reflowLines(const int columns, std::map<int, int> *deltas = nullptr) = 0;

    //
    // FIXME:  Passing around constant references to HistoryType instances
    // is very unsafe, because those references will no longer
    // be valid if the history scroll is deleted.
    //
    const HistoryType &getType() const
    {
        return *_historyType;
    }

protected:
    std::unique_ptr<HistoryType> _historyType;
    const int MAX_REFLOW_LINES = 20000;
};

class HistoryTypeNone : public HistoryType
{
public:
    HistoryTypeNone();

    bool isEnabled() const override;
    int maximumLineCount() const override;

    void scroll(std::unique_ptr<HistoryScroll> &) const override;
};

class CompactHistoryType : public HistoryType
{
public:
    explicit CompactHistoryType(unsigned int nbLines);

    bool isEnabled() const override;
    int maximumLineCount() const override;

    void scroll(std::unique_ptr<HistoryScroll> &) const override;

protected:
    unsigned int _maxLines;
};

class CompactHistoryScroll final : public HistoryScroll
{
    typedef QVector<Character> TextLine;

public:
    explicit CompactHistoryScroll(const unsigned int maxLineCount = 1000);
    ~CompactHistoryScroll() override = default;

    int getLines() const override;
    int getMaxLines() const override;
    int getLineLen(const int lineNumber) const override;
    void getCells(const int lineNumber, const int startColumn, const int count, Character buffer[]) const override;
    bool isWrappedLine(const int lineNumber) const override;
    LineProperty getLineProperty(const int lineNumber) const override;
    void setLineProperty(const int lineno, LineProperty prop) override;

    void addCells(const Character a[], const int count) override;
    void addCellsMove(Character a[], const int count) override;
    void addLine(const LineProperty lineProperty = LineProperty()) override;

    void removeCells() override;

    void setMaxNbLines(const int lineCount);

    int reflowLines(const int columns, std::map<int, int> *deltas = nullptr) override;

private:
    /**
     * This is the actual buffer that contains the cells
     */
    std::deque<Character> _cells;

    /**
     * Each entry contains the start of the next line and the current line's
     * properties.  The start of lines is biased by _indexBias, i.e. an index
     * value of _indexBias corresponds to _cells' zero index.
     *
     * The use of a biased line start means we don't need to traverse the
     * vector recalculating line starts when removing lines from the top, and
     * also we don't need to traverse the vector to compute the start of a line
     * as we would have to do if we stored line lengths.
     *
     * unsigned int means we're limited in common architectures to 4 million
     * characters, but CompactHistoryScroll is limited by the UI to 1_000_000
     * lines (see historyLineSpinner in src/widgets/HistorySizeWidget.ui), so
     * enough for 1_000_000 lines of an average ~4295 length (and each
     * Character takes 16 bytes, so that's 64Gb!).
     */
    struct LineData {
        unsigned int index;
        LineProperty flag;
    };
    /**
     * This buffer contains the data about each line
     * The size of this buffer is the number of lines we have.
     */
    std::vector<LineData> _lineDatas;
    unsigned int _indexBias = 0;

    /**
     * Max number of lines we can hold
     */
    size_t _maxLineCount;

    /**
     * Remove @p lines from the "start" of above buffers
     */
    void removeLinesFromTop(size_t lines);

    inline int lineLen(const int line) const
    {
        return line == 0 ? _lineDatas.at(0).index - _indexBias : _lineDatas.at(line).index - _lineDatas.at(line - 1).index;
    }

    /**
     * Get the start of @p line in _cells buffer
     *
     * index actually contains the start of the next line.
     */
    inline int startOfLine(const int line) const
    {
        return line == 0 ? 0 : _lineDatas.at(line - 1).index - _indexBias;
    }
};

class HistoryScrollNone : public HistoryScroll
{
public:
    HistoryScrollNone();
    ~HistoryScrollNone() override;

    bool hasScroll() const override;

    int getLines() const override;
    int getMaxLines() const override;
    int getLineLen(const int lineno) const override;
    void getCells(const int lineno, const int colno, const int count, Character res[]) const override;
    bool isWrappedLine(const int lineno) const override;
    LineProperty getLineProperty(const int lineno) const override;
    void setLineProperty(const int lineno, LineProperty prop) override;

    void addCells(const Character a[], const int count) override;
    void addCellsMove(Character a[], const int count) override;
    void addLine(const LineProperty lineProperty = LineProperty()) override;

    // Modify history (do nothing here)
    void removeCells() override;
    int reflowLines(const int, std::map<int, int> * = nullptr) override;
};

}

#endif
