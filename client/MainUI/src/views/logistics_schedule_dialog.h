#ifndef LOGISTICS_SCHEDULE_DIALOG_H
#define LOGISTICS_SCHEDULE_DIALOG_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace Ui {
class LogisticsScheduleDialog;
}

class LogisticsScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogisticsScheduleDialog(QWidget *parent = nullptr);
    ~LogisticsScheduleDialog();

private slots:
    void on_confirm_button_clicked();
    void on_cancel_button_clicked();

private:
    Ui::LogisticsScheduleDialog *ui;

    // 배열 2개로 id와 code 매핑
    QList<QString> item_id_list;    // DB에 저장할 item id
    QList<QString> item_code_list;  // 화면에 보여줄 item code
    QList<QString> item_name_list;  // item name

    void load_inventory_items(); // inventory 테이블에서 데이터 불러오기
};

#endif // LOGISTICS_SCHEDULE_DIALOG_H
