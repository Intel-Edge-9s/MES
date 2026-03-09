#include "scm_manage_auto_dialog.h"
#include "ui_scm_manage_auto_dialog.h"

ScmManageAutoDialog::ScmManageAutoDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScmManageAutoDialog)
{
    ui->setupUi(this);
}

ScmManageAutoDialog::~ScmManageAutoDialog()
{
    delete ui;
}
