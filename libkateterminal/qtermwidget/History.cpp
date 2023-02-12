#include "History.h"

using namespace Konsole;

HistoryType::HistoryType() = default;

HistoryType::~HistoryType() = default;

bool HistoryType::isUnlimited() const
{
    return maximumLineCount() == -1;
}

HistoryScroll::HistoryScroll(HistoryType *t)
    : _historyType(t)
{
}

HistoryScroll::~HistoryScroll() = default;

bool HistoryScroll::hasScroll() const
{
    return true;
}

HistoryTypeNone::HistoryTypeNone() = default;

bool HistoryTypeNone::isEnabled() const
{
    return false;
}

void HistoryTypeNone::scroll(std::unique_ptr<HistoryScroll> &old) const
{
    old = std::make_unique<HistoryScrollNone>();
}

int HistoryTypeNone::maximumLineCount() const
{
    return 0;
}

HistoryScrollNone::HistoryScrollNone()
    : HistoryScroll(new HistoryTypeNone())
{
}

HistoryScrollNone::~HistoryScrollNone() = default;

bool HistoryScrollNone::hasScroll() const
{
    return false;
}

int HistoryScrollNone::getLines() const
{
    return 0;
}

int HistoryScrollNone::getMaxLines() const
{
    return 0;
}

int HistoryScrollNone::getLineLen(int) const
{
    return 0;
}

bool HistoryScrollNone::isWrappedLine(int /*lineno*/) const
{
    return false;
}

LineProperty HistoryScrollNone::getLineProperty(int /*lineno*/) const
{
    return LineProperty();
}

void HistoryScrollNone::setLineProperty(int /*lineno*/, LineProperty /*prop*/)
{
}

void HistoryScrollNone::getCells(int, int, int, Character[]) const
{
}

void HistoryScrollNone::addCells(const Character[], int)
{
}

void HistoryScrollNone::addCellsMove(Character[], int)
{
}

void HistoryScrollNone::addLine(LineProperty)
{
}

void HistoryScrollNone::removeCells()
{
}

int Konsole::HistoryScrollNone::reflowLines(const int, std::map<int, int> *)
{
    return 0;
}

// Reasonable line size
static const int LINE_SIZE = 1024;

CompactHistoryType::CompactHistoryType(unsigned int nbLines)
    : _maxLines(nbLines)
{
}

bool CompactHistoryType::isEnabled() const
{
    return true;
}

int CompactHistoryType::maximumLineCount() const
{
    return _maxLines;
}

void CompactHistoryType::scroll(std::unique_ptr<HistoryScroll> &old) const
{
    if (auto *newBuffer = dynamic_cast<CompactHistoryScroll *>(old.get())) {
        newBuffer->setMaxNbLines(_maxLines);
        return;
    }
    auto newScroll = std::make_unique<CompactHistoryScroll>(_maxLines);

    Character line[LINE_SIZE];
    int lines = (old != nullptr) ? old->getLines() : 0;
    int i = qMax((lines - (int)_maxLines), 0);
    std::vector<Character> tmp_line;
    for (; i < lines; i++) {
        int size = old->getLineLen(i);
        if (size > LINE_SIZE) {
            tmp_line.resize(size);
            old->getCells(i, 0, size, tmp_line.data());
            newScroll->addCellsMove(tmp_line.data(), size);
            newScroll->addLine(old->getLineProperty(i));
        } else {
            old->getCells(i, 0, size, line);
            newScroll->addCells(line, size);
            newScroll->addLine(old->getLineProperty(i));
        }
    }
    old = std::move(newScroll);
}

CompactHistoryScroll::CompactHistoryScroll(const unsigned int maxLineCount)
    : HistoryScroll(new CompactHistoryType(maxLineCount))
    , _maxLineCount(0)
{
    setMaxNbLines(maxLineCount);
}

void CompactHistoryScroll::removeLinesFromTop(size_t lines)
{
    if (_lineDatas.size() > 1) {
        const unsigned int removing = _lineDatas.at(lines - 1).index;
        _lineDatas.erase(_lineDatas.begin(), _lineDatas.begin() + lines);

        _cells.erase(_cells.begin(), _cells.begin() + (removing - _indexBias));
        _indexBias = removing;
    } else {
        _lineDatas.clear();
        _cells.clear();
    }
}

void CompactHistoryScroll::addCells(const Character a[], const int count)
{
    _cells.insert(_cells.end(), a, a + count);

    // store the (biased) start of next line + default flag
    // the flag is later updated when addLine is called
    _lineDatas.push_back({static_cast<unsigned int>(_cells.size() + _indexBias), LineProperty()});

    if (_lineDatas.size() > _maxLineCount + 5) {
        removeLinesFromTop(5);
    }
}

void CompactHistoryScroll::addCellsMove(Character characters[], const int count)
{
    std::move(characters, characters + count, std::back_inserter(_cells));

    // store the (biased) start of next line + default flag
    // the flag is later updated when addLine is called
    _lineDatas.push_back({static_cast<unsigned int>(_cells.size() + _indexBias), LineProperty()});

    if (_lineDatas.size() > _maxLineCount + 5) {
        removeLinesFromTop(5);
    }
}

void CompactHistoryScroll::addLine(const LineProperty lineProperty)
{
    auto &flag = _lineDatas.back().flag;
    flag = lineProperty;
}

int CompactHistoryScroll::getLines() const
{
    return _lineDatas.size();
}

int CompactHistoryScroll::getMaxLines() const
{
    return _maxLineCount;
}

int CompactHistoryScroll::getLineLen(int lineNumber) const
{
    if (size_t(lineNumber) >= _lineDatas.size()) {
        return 0;
    }

    return lineLen(lineNumber);
}

void CompactHistoryScroll::getCells(const int lineNumber, const int startColumn, const int count, Character buffer[]) const
{
    if (count == 0) {
        return;
    }
    Q_ASSERT((size_t)lineNumber < _lineDatas.size());

    Q_ASSERT(startColumn >= 0);
    Q_ASSERT(startColumn <= lineLen(lineNumber) - count);

    auto startCopy = _cells.begin() + startOfLine(lineNumber) + startColumn;
    auto endCopy = startCopy + count;
    std::copy(startCopy, endCopy, buffer);
}

void CompactHistoryScroll::setMaxNbLines(const int lineCount)
{
    Q_ASSERT(lineCount >= 0);
    _maxLineCount = lineCount;

    if (_lineDatas.size() > _maxLineCount) {
        int linesToRemove = _lineDatas.size() - _maxLineCount;
        removeLinesFromTop(linesToRemove);
    }
}

void CompactHistoryScroll::removeCells()
{
    if (_lineDatas.size() > 1) {
        /** Here we remove a line from the "end" of the buffers **/

        // Get last line start
        int lastLineStart = startOfLine(_lineDatas.size() - 1);

        // remove info about this line
        _lineDatas.pop_back();

        // remove the actual line content
        _cells.erase(_cells.begin() + lastLineStart, _cells.end());
    } else {
        _cells.clear();
        _lineDatas.clear();
    }
}

bool CompactHistoryScroll::isWrappedLine(const int lineNumber) const
{
    Q_ASSERT((size_t)lineNumber < _lineDatas.size());
    return (_lineDatas.at(lineNumber).flag & LINE_WRAPPED) > 0;
}

LineProperty CompactHistoryScroll::getLineProperty(const int lineNumber) const
{
    Q_ASSERT((size_t)lineNumber < _lineDatas.size());
    return _lineDatas.at(lineNumber).flag;
}

void CompactHistoryScroll::setLineProperty(const int lineNumber, LineProperty prop)
{
    Q_ASSERT((size_t)lineNumber < _lineDatas.size());
    _lineDatas.at(lineNumber).flag = prop;
}

int CompactHistoryScroll::reflowLines(const int columns, std::map<int, int> *deltas)
{
    std::vector<LineData> newLineData;

    auto reflowLineLen = [](int start, int end) {
        return end - start;
    };
    auto setNewLine = [](std::vector<LineData> &change, unsigned int index, LineProperty flag) {
        change.push_back({index, flag});
    };

    int currentPos = 0;
    int newPos = 0;
    int delta = 0;
    while (currentPos < getLines()) {
        int startLine = startOfLine(currentPos);
        int endLine = startOfLine(currentPos + 1);
        LineProperty lineProperty = getLineProperty(currentPos);

        // Join the lines if they are wrapped
        while (currentPos < getLines() - 1 && isWrappedLine(currentPos)) {
            currentPos++;
            endLine = startOfLine(currentPos + 1);
        }

        // Now reflow the lines
        while (reflowLineLen(startLine, endLine) > columns && !(lineProperty & (LINE_DOUBLEHEIGHT))) {
            startLine += columns;
            setNewLine(newLineData, startLine + _indexBias, lineProperty | LINE_WRAPPED);
            setNewLine(newLineData, startLine + _indexBias, lineProperty);
            newPos++;
        }
        setNewLine(newLineData, endLine + _indexBias, lineProperty & ~LINE_WRAPPED);
        setNewLine(newLineData, endLine + _indexBias, lineProperty);
        currentPos++;
        newPos++;
        if (deltas && delta != newPos - currentPos) {
            (*deltas)[currentPos - getLines()] = newPos - currentPos - delta;
            delta = newPos - currentPos;
        }
    }
    _lineDatas = std::move(newLineData);

    int deletedLines = 0;
    size_t totalLines = getLines();
    if (totalLines > _maxLineCount) {
        deletedLines = totalLines - _maxLineCount;
        removeLinesFromTop(deletedLines);
    }

    return deletedLines;
}
