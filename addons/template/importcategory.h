#pragma once

#include "abstractdatamodel.h"

#include <QDialog>
#include <QFileInfo>
#include <QModelIndex>
#include <QString>
#include <QVariant>

namespace Ui
{
class ImportCategory;
}
class QPushButton;

class ImportCategory : public QDialog
{
    Q_OBJECT

public:
    struct CategoryData : AbstractData {
        ~CategoryData()
        {
        }
        enum DataRoles {
            CategoryRole = Qt::UserRole,
        };

        QVariant data(int role = Qt::DisplayRole, int column = 0) override;
        int columns() override
        {
            return 1;
        }

        QString category;
    };

    explicit ImportCategory(QWidget *parent = nullptr);
    ~ImportCategory();

    std::optional<QString> getCategoryPath(const QString &defaultTemlateName);

private:
    void loadRootCategories(const QFileInfo &info);
    void loadCategories(const QFileInfo &info, const QModelIndex &parent);

    void categoryIndexChanged(const QModelIndex &newIndex);
    void categoryEditChanged();
    void templateNameChanged();

    void addCategory();
    void addSubCategory();

    Ui::ImportCategory *ui;
    QPushButton *m_importButton;
    AbstractDataModel m_categoryModel;
};
