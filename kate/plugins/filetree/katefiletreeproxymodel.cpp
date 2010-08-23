#include "katefiletreeproxymodel.h"
#include "katefiletreemodel.h"

#include <ktexteditor/document.h>

KateFileTreeProxyModel::KateFileTreeProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  qDebug() << __PRETTY_FUNCTION__ << ": BEGIN!";
}

bool KateFileTreeProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
  //qDebug() << __PRETTY_FUNCTION__ << ": BEGIN!";
  KateFileTreeModel *model = static_cast<KateFileTreeModel*>(sourceModel());
  
  bool left_isdir = model->isDir(left);
  bool right_isdir = model->isDir(right);
  
  QString left_name = model->data(left).toString();
  QString right_name = model->data(right).toString();

  qDebug() << __PRETTY_FUNCTION__ << ":" << left_name << "<" << right_name;
  if(left_isdir != right_isdir) {
      //qDebug() << __PRETTY_FUNCTION__ << ": DIR! (END)";
      //bool name_lt = QString::localeAwareCompare(left_name, right_name) > 0;
      qDebug() << __PRETTY_FUNCTION__ << ": dir" << (((left_isdir - right_isdir)) > 0);
      return ((left_isdir - right_isdir)) > 0;
  }
  
  //QString left_ext = left_name.section('.', -1);
  //QString right_ext = right_name.section('.', -1);
  
  //if(!left_children && !right_children) {
  //    qDebug() << __PRETTY_FUNCTION__ << ": " << left_ext << right_ext;
  //    bool ext_res = QString::localeAwareCompare(left_ext, right_ext) > 0;
  //    bool file_res = QString::localeAwareCompare(left_name, right_name) > 0;
  //    return ext_res - file_res;
  //}
  
//  if(left_name == "CMakeLists.txt" && right_name != "CMakeLists.txt")
//    return false;
//  if(left_name != "CMakeLists.txt" && right_name == "CMakeLists.txt")
//    return true;
  
  qDebug() << __PRETTY_FUNCTION__ << ":" << (QString::localeAwareCompare(left_name, right_name) < 0);
  return QString::localeAwareCompare(left_name, right_name) < 0;
}

QModelIndex KateFileTreeProxyModel::docIndex(KTextEditor::Document *doc)
{
  qDebug() << __PRETTY_FUNCTION__ << ": !";
  return mapFromSource(static_cast<KateFileTreeModel*>(sourceModel())->docIndex(doc));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
