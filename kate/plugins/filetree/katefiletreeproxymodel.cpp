#include "katefiletreeproxymodel.h"
#include "katefiletreemodel.h"

#include <ktexteditor/document.h>

#include "katefiletreedebug.h"

KateFileTreeProxyModel::KateFileTreeProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  kDebug(debugArea()) << "BEGIN!";
}

bool KateFileTreeProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
  //kDebug(debugArea()) << ": BEGIN!";
  KateFileTreeModel *model = static_cast<KateFileTreeModel*>(sourceModel());
  
  bool left_isdir = model->isDir(left);
  bool right_isdir = model->isDir(right);
  
  QString left_name = model->data(left).toString();
  QString right_name = model->data(right).toString();

  //kDebug(debugArea()) << left_name << "<" << right_name;
  if(left_isdir != right_isdir) {
      //kDebug(debugArea()) << ": DIR! (END)";
      //bool name_lt = QString::localeAwareCompare(left_name, right_name) > 0;
      //kDebug(debugArea()) << "dir" << (((left_isdir - right_isdir)) > 0);
      return ((left_isdir - right_isdir)) > 0;
  }
  
  //QString left_ext = left_name.section('.', -1);
  //QString right_ext = right_name.section('.', -1);
  
  //if(!left_children && !right_children) {
  //    kDebug(debugArea()) << ": " << left_ext << right_ext;
  //    bool ext_res = QString::localeAwareCompare(left_ext, right_ext) > 0;
  //    bool file_res = QString::localeAwareCompare(left_name, right_name) > 0;
  //    return ext_res - file_res;
  //}
  
//  if(left_name == "CMakeLists.txt" && right_name != "CMakeLists.txt")
//    return false;
//  if(left_name != "CMakeLists.txt" && right_name == "CMakeLists.txt")
//    return true;
  
  //kDebug(debugArea()) << (QString::localeAwareCompare(left_name, right_name) < 0);
  return QString::localeAwareCompare(left_name, right_name) < 0;
}

QModelIndex KateFileTreeProxyModel::docIndex(KTextEditor::Document *doc)
{
  kDebug(debugArea()) << "!";
  return mapFromSource(static_cast<KateFileTreeModel*>(sourceModel())->docIndex(doc));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
