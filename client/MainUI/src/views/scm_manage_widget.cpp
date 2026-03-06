#include "scm_manage_widget.h"
#include "ui_scm_manage_widget.h"
#include "logistics_schedule_dialog.h"
#include <QSqlQuery>               //
#include <QSqlError>
#include <QDebug>

ScmManageWidget::ScmManageWidget(QWidget *parent) :
    BasePageWidget(parent),
    ui(new Ui::ScmManageWidget)
{
    ui->setupUi(this);

    setupStockStatusTableConfigs();
    setupStockOrderTableConfigs();
    loadInventoryData();
    loadInventoryOrderData();
}

QTableWidgetItem* createCenteredItem(const QString &text) {
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter); // 💡 핵심: 가운데 정렬 설정
    return item;
}

void ScmManageWidget::setupStockStatusTableConfigs(){
    QStringList headers = {"Code", "Name", "Stock", "Location"};

    ui->tableStockStatus->setColumnCount(headers.size());
    ui->tableStockStatus->setHorizontalHeaderLabels(headers);
    ui->tableStockStatus->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 세로 스크롤바가 필요할 때만 나타나도록 설정
    ui->tableStockOrder->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 마우스 휠 한 번에 이동하는 픽셀 수 조절 (부드러운 스크롤)
    ui->tableStockOrder->verticalHeader()->setDefaultSectionSize(35); // 행 높이 고정

    // 테이블 끝에 도달했을 때 빈 공간이 생기지 않도록 설정
    ui->tableStockOrder->verticalHeader()->setStretchLastSection(false);
}

void ScmManageWidget::setupStockOrderTableConfigs(){
    QStringList headers = {"User", "Item", "Stock", "Status", "Created at","Receive at", "Updated at"};
    ui->tableStockOrder->setColumnCount(8); // 7 → 8로 변경 (id 숨김 컬럼 추가)
    ui->tableStockOrder->setHorizontalHeaderLabels(headers);
    ui->tableStockOrder->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); //
    ui->tableStockOrder->setColumnHidden(7, true); // id 컬럼 숨김 처리

    // 세로 스크롤바가 필요할 때만 나타나도록 설정
    ui->tableStockOrder->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 마우스 휠 한 번에 이동하는 픽셀 수 조절 (부드러운 스크롤)
    ui->tableStockOrder->verticalHeader()->setDefaultSectionSize(35); // 행 높이 고정

    // 테이블 끝에 도달했을 때 빈 공간이 생기지 않도록 설정
    ui->tableStockOrder->verticalHeader()->setStretchLastSection(false);
}

void ScmManageWidget::loadInventoryData(){
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "SCM Manage: 데이터베이스가 열려있지 않습니다.";
        return;
    }

    // inventory 테이블 쿼리 실행 (MariaDB 소문자 주의)
    QSqlQuery query("SELECT id, item_code, item_name, current_stock, location FROM inventory", db);

    if (!query.exec()) {
        qDebug() << "조회 에러:" << query.lastError().text();
        return;
    }

    // 테이블 초기화 후 데이터 삽입
    ui->tableStockStatus->setRowCount(0);
    int row = 0;
    while (query.next()) {
        ui->tableStockStatus->insertRow(row);
        ui->tableStockStatus->setItem(row, 0, new QTableWidgetItem(query.value("item_code").toString()));
        ui->tableStockStatus->setItem(row, 1, new QTableWidgetItem(query.value("item_name").toString()));
        ui->tableStockStatus->setItem(row, 2, new QTableWidgetItem(query.value("current_stock").toString()));
        ui->tableStockStatus->setItem(row, 3, new QTableWidgetItem(query.value("location").toString()));
        row++;
    }
    qDebug() << "총" << row << "개의 재고 항목을 불러왔습니다.";
}

void ScmManageWidget::loadInventoryOrderData(){
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "SCM Manage: 데이터베이스가 열려있지 않습니다.";
        return;
    }

    // 정렬순서 create 최신순으로 설정
    QSqlQuery query("SELECT id, user_id, item_id, item_code, item_name, stock, status, created_at, receive_at, updated_at FROM inventory_order_logs ORDER BY created_at DESC", db);

    if (!query.exec()) {
        qDebug() << "주문 로그 조회 에러:" << query.lastError().text();
        return;
    }

    ui->tableStockOrder->setRowCount(0);
    int row = 0;
    while (query.next()) {
        ui->tableStockOrder->insertRow(row);
        // ui->tableStockOrder->setItem(row, 0, new QTableWidgetItem(query.value("item_code").toString()));
        // ui->tableStockOrder->setItem(row, 1, new QTableWidgetItem(query.value("item_name").toString()));
        // ui->tableStockOrder->setItem(row, 2, new QTableWidgetItem(query.value("stock").toString()));
        // ui->tableStockOrder->setItem(row, 3, new QTableWidgetItem(query.value("status").toString()));
        ui->tableStockOrder->setItem(row, 0, createCenteredItem(query.value("user_id").toString()));
        ui->tableStockOrder->setItem(row, 1, createCenteredItem(
                                                 QString("[%1] %2").arg(query.value("item_code").toString(), query.value("item_name").toString())
                                                 ));
        ui->tableStockOrder->setItem(row, 2, createCenteredItem(query.value("stock").toString()));
        ui->tableStockOrder->setItem(row, 3, createCenteredItem(query.value("status").toString()));
        ui->tableStockOrder->setItem(row, 4, new QTableWidgetItem(query.value("created_at").toString()));
        ui->tableStockOrder->setItem(row, 5, new QTableWidgetItem(query.value("receive_at").toString()));
        ui->tableStockOrder->setItem(row, 6, new QTableWidgetItem(query.value("updated_at").toString()));
        // 숨겨진 id 컬럼에 저장 (Cancel Order 기능에 사용)
        ui->tableStockOrder->setItem(row, 7, new QTableWidgetItem(query.value("id").toString()));
        row++;
    }
    qDebug() << "총" << row << "개의 주문 로그를 불러왔습니다.";
}

void ScmManageWidget::on_create_order_button_clicked()
{
    // 입고 스케줄 생성 팝업 호출
    LogisticsScheduleDialog *dialog = new LogisticsScheduleDialog(this);

    // 팝업 닫힌 후 테이블 새로고침
    if (dialog->exec() == QDialog::Accepted) {
        loadInventoryOrderData();
    }
}

void ScmManageWidget::on_cancel_order_button_clicked()
{
    // 선택된 행 확인
    int row = ui->tableStockOrder->currentRow();
    if (row < 0) {
        qDebug() << "취소할 항목을 선택해주세요.";
        return;
    }

    // 완료된 항목은 취소 불가
    QString status = ui->tableStockOrder->item(row, 3)->text();
    if (status == "DONE") {
        qDebug() << "이미 완료된 항목은 취소할 수 없습니다.";
        return;
    }

    // 숨겨진 컬럼에서 id 가져오기
    QString selected_id = ui->tableStockOrder->item(row, 7)->text();

    // DB 연결 확인
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "DB 연결이 열려있지 않습니다.";
        return;
    }

    // DB에서 해당 row 삭제
    QSqlQuery query(db);
    query.prepare("DELETE FROM inventory_order_logs WHERE id = :id");
    query.bindValue(":id", selected_id);

    if (query.exec()) {
        qDebug() << "[스케줄 취소 성공]" << selected_id;
        loadInventoryOrderData(); // 테이블 새로고침
    } else {
        qDebug() << "[스케줄 취소 실패]" << query.lastError().text();
    }
}

ScmManageWidget::~ScmManageWidget()
{
    delete ui;
}

void ScmManageWidget::on_Back_btn_clicked()
{
    emit requestPageChange(PageType::Dashboard);
}
