#pragma once

#include <QMainWindow>
#include <memory>

class TsTestApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit TsTestApp(QWidget *parent = nullptr);
    ~TsTestApp();

private:
   class Impl;
   std::unique_ptr<Impl> impl_;
};
