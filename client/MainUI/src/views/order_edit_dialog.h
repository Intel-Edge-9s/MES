#ifndef ORDER_EDIT_DIALOG_H
#define ORDER_EDIT_DIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace Ui {
class OrderEditDialog;
}

class OrderEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OrderEditDialog(QWidget *parent = nullptr);
    ~OrderEditDialog();

private:
    Ui::OrderEditDialog *ui;

    void loadInventoryList();
};

#endif // ORDER_EDIT_DIALOG_H
