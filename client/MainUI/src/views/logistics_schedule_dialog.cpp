#include "logistics_schedule_dialog.h"
#include "ui_logistics_schedule_dialog.h"

LogisticsScheduleDialog::LogisticsScheduleDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogisticsScheduleDialog)
{
    ui->setupUi(this);
    ui->title_label->setText("Logistics Schedule");

    // 날짜 기본값을 오늘 날짜로 설정
    ui->receive_at_edit->setDate(QDate::currentDate());

    // inventory 테이블에서 원재료 데이터 불러오기
    load_inventory_items();
}

LogisticsScheduleDialog::~LogisticsScheduleDialog()
{
    delete ui;
}

void LogisticsScheduleDialog::load_inventory_items()
{
    // DB 연결 확인
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "DB 연결이 열려있지 않습니다.";
        return;
    }

    // inventory 테이블에서 id, item_code, item_name 조회
    QSqlQuery query("SELECT id, item_code, item_name FROM inventory", db);

    if (!query.exec()) {
        qDebug() << "inventory 조회 에러:" << query.lastError().text();
        return;
    }

    // 배열 초기화
    item_id_list.clear();
    item_code_list.clear();
    item_name_list.clear();
    ui->item_code_combo->clear();

    // 방법 B: 배열 3개에 순서대로 저장
    while (query.next()) {
        QString id   = query.value("id").toString();
        QString code = query.value("item_code").toString();
        QString name = query.value("item_name").toString();

        item_id_list.append(id);      // id 배열에 저장
        item_code_list.append(code);  // code 배열에 저장
        item_name_list.append(name);  // name 배열에 저장

        ui->item_code_combo->addItem(code); // ComboBox에는 code만 표시
    }

    qDebug() << "총" << item_id_list.size() << "개의 원재료를 불러왔습니다.";
}

void LogisticsScheduleDialog::on_confirm_button_clicked()
{
    // 입력값 수집
    int selected_index = ui->item_code_combo->currentIndex();
    QString quantity   = ui->quantity_edit->text();
    QString receive_at = ui->receive_at_edit->date().toString("yyyy-MM-dd");

    // 입력값 검증
    if (selected_index < 0 || quantity.isEmpty()) {
        qDebug() << "입력값이 비어있습니다.";
        return;
    }

    // 방법 B: 같은 인덱스로 id, code, name 가져오기
    QString selected_id   = item_id_list[selected_index];
    QString selected_code = item_code_list[selected_index];
    QString selected_name = item_name_list[selected_index];

    // DB 연결 확인
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "DB 연결이 열려있지 않습니다.";
        return;
    }

    // SQL INSERT 실행 (id는 UUID()로 자동 생성)
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO inventory_order_logs "
        "(id, item_id, item_code, item_name, stock, status, receive_at) "
        "VALUES (UUID(), :item_id, :item_code, :item_name, :stock, 'PENDING', :receive_at)"
        );
    query.bindValue(":item_id",    selected_id);
    query.bindValue(":item_code",  selected_code);
    query.bindValue(":item_name",  selected_name);
    query.bindValue(":stock",      quantity.toInt());
    query.bindValue(":receive_at", receive_at);

    if (query.exec()) {
        qDebug() << "[입고 스케줄 생성 성공]";
        accept(); // 팝업 닫기
    } else {
        qDebug() << "[입고 스케줄 생성 실패]" << query.lastError().text();
    }
}

void LogisticsScheduleDialog::on_cancel_button_clicked()
{
    reject(); // 팝업 닫기
}
