#ifndef KATECODEFOLDINGHELPERS_H
#define KATECODEFOLDINGHELPERS_H

class KateLineInfo
{
  public:
  bool topLevel;                        // if depth == 0
  bool startsVisibleBlock;              // true if line contains an unfolded start node
  bool startsInVisibleBlock;            // true if line contains a folded start node
  bool endsBlock;                       // true if line contains a valid end node
  bool invalidBlockEnd;                 // true if line contains an invalid end node
  int depth;                            // maximum level of a line's node
};

#endif // KATECODEFOLDINGHELPERS_H
