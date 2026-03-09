#ifndef SCM_MANAGE_AUTO_DIALOG_H
#define SCM_MANAGE_AUTO_DIALOG_H

#include <QWidget>

namespace Ui {
class ScmManageAutoDialog;
}

class ScmManageAutoDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ScmManageAutoDialog(QWidget *parent = nullptr);
    ~ScmManageAutoDialog();

private:
    Ui::ScmManageAutoDialog *ui;
};

#endif // SCM_MANAGE_AUTO_DIALOG_H
