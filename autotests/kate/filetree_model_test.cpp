/* This file is part of the KDE project
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "document_dummy.h"
#include "filetree_model_test.h"
#include "katefiletreemodel.h"

#include <QtTest>
QTEST_GUILESS_MAIN(FileTreeModelTest)


//BEGIN ResultNode
class ResultNode
{
  public:
    ResultNode() : name(), children() {}
    ResultNode(const ResultNode &other) : name(other.name), children(other.children) {}
    ResultNode(const char *_name) : ResultNode(QString::fromLatin1(_name)) {}
    ResultNode(const QString &_name) : name(_name), children() {}

    ResultNode &operator<<(ResultNode node) { children << node; return *this; }

    bool operator!=(const ResultNode &other) const { return !(*this == other); }
    bool operator==(const ResultNode &other) const
    {
      return (other.name == name) && (other.children == children);
    }

    friend QDebug operator<< (QDebug s, const ResultNode &node) { s << node.toString(); return s; }

    friend void debugOutput(QString &s, const ResultNode &rootNode, const int level = 0)
    {
      for (int i = 0; i < level; i++) {
        s += QLatin1String("  ");
      }

      const QString name = rootNode.name.isEmpty() ? QLatin1String("ROOT") : rootNode.name;

      s += QLatin1String("( ") + name;
      if (rootNode.children.isEmpty()) {
        s += QLatin1String(" )");
      } else {
        s += QLatin1String(",\n");
        foreach (const ResultNode &node, rootNode.children) {
          debugOutput(s, node, level + 1);
        }
        s += (level == 0) ? QLatin1String(");") : QLatin1String(")\n");
      }
    }

    QString toString() const
    {
      QString out;
      debugOutput(out, *this, 0);
      return out;
    }

    QString name;
    QList<ResultNode> children;
};

Q_DECLARE_METATYPE(ResultNode);

namespace QTest {
inline bool qCompare(const ResultNode &t1, const ResultNode &t2, const char *actual, const char *expected,
                     const char *file, int line)
{
  /* compare_helper is not helping that much, we need to prepare copy of data */
  const QByteArray a = t1.toString().toLatin1();
  const QByteArray b = t2.toString().toLatin1();

  char *val1 = new char[a.size()];
  char *val2 = new char[b.size()];

  memcpy(val1, a.constData(), a.size());
  memcpy(val2, b.constData(), b.size());

  return compare_helper(t1 == t2,
                        "Compared ResultNode Trees are not ths same",
                        val1, val2,
                        actual, expected, file, line);
}
}
//END ResultNode

void FileTreeModelTest::initTestCase() {}

void FileTreeModelTest::cleanupTestCase() {}

void FileTreeModelTest::init() {}

void FileTreeModelTest::cleanup() {}

void FileTreeModelTest::basic()
{
  QScopedPointer<DummyDocument> d1(new DummyDocument());
  QScopedPointer<DummyDocument> d2(new DummyDocument());

  KateFileTreeModel m(this);
  QCOMPARE(m.rowCount(QModelIndex()), 0);

  m.documentOpened(d1.data());
  QCOMPARE(m.rowCount(QModelIndex()), 1);

  m.documentOpened(d2.data());
  QCOMPARE(m.rowCount(QModelIndex()), 2);
}

void FileTreeModelTest::buildTree_data()
{
  QTest::addColumn<QList<DummyDocument *>>("documents");
  QTest::addColumn<ResultNode>("nodes");

  QTest::newRow("easy") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///a/foo.txt")
  ) << (
    ResultNode()
      << (ResultNode("a")
        << ResultNode("foo.txt"))
  );

  QTest::newRow("two") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///a/foo.txt")
    << new DummyDocument("file:///a/bar.txt")
  ) << (
    ResultNode()
      << (ResultNode("a")
        << ResultNode("foo.txt")
        << ResultNode("bar.txt"))
  );

  QTest::newRow("strangers") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///a/foo.txt")
    << new DummyDocument("file:///b/bar.txt")
  ) << (
    ResultNode()
      << (ResultNode("a")
        << ResultNode("foo.txt"))
      << (ResultNode("b")
        << ResultNode("bar.txt"))
  );

  QTest::newRow("lvl1 strangers") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///c/a/foo.txt")
    << new DummyDocument("file:///c/b/bar.txt")
  ) << (
    ResultNode()
      << (ResultNode("a")
        << ResultNode("foo.txt"))
      << (ResultNode("b")
        << ResultNode("bar.txt"))
  );

  QTest::newRow("multiples") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///c/a/foo.txt")
    << new DummyDocument("file:///c/b/bar.txt")
    << new DummyDocument("file:///c/a/bar.txt")
  ) << (
    ResultNode()
      << (ResultNode("a")
        << ResultNode("foo.txt")
        << ResultNode("bar.txt"))
      << (ResultNode("b")
        << ResultNode("bar.txt"))
  );

  /* TODO: not implemented; not a nice tree :(
  QTest::newRow("branches") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///c/a/foo.txt")
    << new DummyDocument("file:///c/b/bar.txt")
    << new DummyDocument("file:///d/a/foo.txt")
  ) << (
    ResultNode()
      << (ResultNode("c")
        << (ResultNode("a")
          << ResultNode("foo.txt")
          << ResultNode("bar.txt"))
        << (ResultNode("d/a")
          << ResultNode("foo.txt")))
  );
  */

  /* TODO: not implemented; not a nice tree :(
  QTest::newRow("levels") << ( QList<DummyDocument *>()
    << new DummyDocument("file:///c/a/foo.txt")
    << new DummyDocument("file:///c/b/bar.txt")
    << new DummyDocument("file:///d/foo.txt")
  ) << (
    ResultNode()
      << (ResultNode("c")
        << (ResultNode("a")
          << ResultNode("foo.txt"))
        << (ResultNode("b")
          << ResultNode("bar.txt")))
      << (ResultNode("d")
        << ResultNode("foo.txt"))
  );
  */

  QTest::newRow("remote simple") << ( QList<DummyDocument *>()
    << new DummyDocument("http://example.org/foo.txt")
  ) << (
    ResultNode()
      << (ResultNode("[example.org]")
        << ResultNode("foo.txt"))
  );

  QTest::newRow("remote nested") << ( QList<DummyDocument *>()
    << new DummyDocument("http://example.org/a/foo.txt")
  ) << (
    ResultNode()
      << (ResultNode("[example.org]a")
        << ResultNode("foo.txt"))
  );

  /* TODO: this one is also not completely ok, is it? */
  QTest::newRow("remote diverge") << ( QList<DummyDocument *>()
    << new DummyDocument("http://example.org/a/foo.txt")
    << new DummyDocument("http://example.org/b/foo.txt")
  ) << (
    ResultNode()
      << (ResultNode("[example.org]a")
        << ResultNode("foo.txt"))
      << (ResultNode("[example.org]b")
        << ResultNode("foo.txt"))
  );
}

void FileTreeModelTest::buildTree()
{
  KateFileTreeModel m(this);
  QFETCH(QList<DummyDocument *>, documents);
  QFETCH(ResultNode, nodes);

  foreach(DummyDocument *doc, documents) {
    m.documentOpened(doc);
  }

  ResultNode root;
  walkTree(m, QModelIndex(), root);

  QCOMPARE(nodes, root);
}

void FileTreeModelTest::walkTree(KateFileTreeModel &model, const QModelIndex &rootIndex, ResultNode &rootNode)
{
  if (!model.hasChildren(rootIndex)) {
    return;
  }

  const int rows = model.rowCount(rootIndex);
  for (int i = 0; i < rows; i++) {
    const QModelIndex idx = model.index(i, 0, rootIndex);
    ResultNode node(model.data(idx).toString());
    walkTree(model, idx, node);
    rootNode << node;
  }
}

#include "filetree_model_test.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
