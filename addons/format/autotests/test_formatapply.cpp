#include "test_formatapply.h"

#include "FormatApply.h"
#include <KTextEditor/Editor>

#include <QTest>

QTEST_MAIN(FormatApplyTest)

void FormatApplyTest::testFormatApply()
{
    const char patch[] =
        R"(diff --git a/home/waqar/kde/src/utilities/kate/build/m.cpp b/tmp/kate.ypQGnu
index dc6e4c4df..03f3d79d9 100644
--- a/home/waqar/kde/src/utilities/kate/build/m.cpp
+++ b/tmp/kate.ypQGnu
@@ -1,4 +1,5 @@
 #include <cstdio>
-int main(){
+int main()
+{
     puts("hello");
 }
)";

    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(this);
    const char unformatted[] =
        R"(#include <cstdio>
int main(){
    puts("hello");
})";

    doc->setText(QString::fromUtf8(unformatted));
    auto iface = qobject_cast<KTextEditor::MovingInterface *>(doc);
    const auto edits = parseDiff(iface, QString::fromUtf8(patch));

    QVERIFY(!edits.empty());

    applyPatch(doc, edits);

    const QString formatted = QStringLiteral(
        R"(#include <cstdio>
int main()
{
    puts("hello");
})");
    QCOMPARE(doc->text(), formatted);
}
