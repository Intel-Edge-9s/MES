#include "delivery_widget.h"
#include "ui_delivery_widget.h"
#include "../services/delivery_service.h"
#include <QMessageBox>

DeliveryWidget::DeliveryWidget(QWidget *parent)
    : BasePageWidget(parent)
    , ui(new Ui::DeliveryWidget)
{
    ui->setupUi(this);

    setupTableConfigs();
    loadDeliveryData();
}

DeliveryWidget::~DeliveryWidget()
{
    delete ui;
}

void DeliveryWidget::setupTableConfigs()
{
    QStringList headers = {"회사명", "제품명", "수량", "상태", "주문시간", "납품시간"};
    ui->delivery_table->setColumnCount(7); // 6 + 1 숨김(id)
    ui->delivery_table->setHorizontalHeaderLabels(headers);
    ui->delivery_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->delivery_table->setColumnHidden(6, true); // id 숨김

    ui->delivery_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->delivery_table->verticalHeader()->setDefaultSectionSize(35);
    ui->delivery_table->verticalHeader()->setStretchLastSection(false);
}

void DeliveryWidget::loadDeliveryData()
{
    auto deliveries = DeliveryService::getDeliveries();

    ui->delivery_table->setRowCount(0);
    int row = 0;
    for (const auto &d : deliveries) {
        ui->delivery_table->insertRow(row);
        ui->delivery_table->setItem(row, 0, new QTableWidgetItem(d.company_name));
        ui->delivery_table->setItem(row, 1, new QTableWidgetItem(d.product_name));
        ui->delivery_table->setItem(row, 2, new QTableWidgetItem(QString::number(d.delivery_stock)));
        ui->delivery_table->setItem(row, 3, new QTableWidgetItem(d.status));
        ui->delivery_table->setItem(row, 4, new QTableWidgetItem(d.created_at));
        ui->delivery_table->setItem(row, 5, new QTableWidgetItem(d.updated_at));
        // 숨겨진 id 컬럼에 저장 (Complete 기능에 사용)
        ui->delivery_table->setItem(row, 6, new QTableWidgetItem(d.id));
        row++;
    }
    qDebug() << "총" << row << "개의 납품 항목을 불러왔습니다.";
}

void DeliveryWidget::on_create_delivery_button_clicked()
{
    // 납품 지시 팝업 호출
    DeliveryDialog *dialog = new DeliveryDialog(this);

    // 팝업 닫힌 후 테이블 새로고침
    if (dialog->exec() == QDialog::Accepted) {
        loadDeliveryData();
    }
}

void DeliveryWidget::on_complete_delivery_button_clicked()
{
    int row = ui->delivery_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "납품 불가", "완료할 항목을 선택해주세요.");
        return;
    }

    QString selected_id = ui->delivery_table->item(row, 6)->text();
    const auto result = DeliveryService::completeDelivery(selected_id);

    switch (result) {
    case DeliveryCompleteResult::Success:
        QMessageBox::information(this, "납품 완료", "납품이 완료되었습니다.");
        loadDeliveryData();
        break;

    case DeliveryCompleteResult::AlreadyDone:
        QMessageBox::warning(this, "납품 불가", "이미 납품을 완료했습니다.");
        loadDeliveryData();
        break;

    case DeliveryCompleteResult::NotEnoughStock:
        QMessageBox::warning(this, "납품 불가", "생산품 재고가 부족하여 납품할 수 없습니다.");
        break;

    case DeliveryCompleteResult::NotFound:
        QMessageBox::warning(this, "납품 불가", "선택한 납품 정보를 찾을 수 없습니다.");
        loadDeliveryData();
        break;

    case DeliveryCompleteResult::DbError:
    default:
        QMessageBox::critical(this, "오류", "납품 처리 중 오류가 발생했습니다.");
        break;
    }
}



void DeliveryWidget::on_Back_btn_clicked()
{
    emit requestPageChange(PageType::Dashboard);
}
