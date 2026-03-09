#include "manufacture_widget.h"
#include "ui_manufacture_widget.h"
#include "manufacture_schedule_dialog.h"
#include "mainwindow.h"
#include "../services/opcua_service.h"
#include "../services/manufacture_service.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>

ManufactureWidget::ManufactureWidget(QWidget *parent)
    : BasePageWidget(parent)
    , ui(new Ui::ManufactureWidget)
{
    ui->setupUi(this);

    setupManufactureTableConfigs();
    setupScheduleTableConfigs();
    loadManufactureData();
    loadScheduleData();
}

ManufactureWidget::~ManufactureWidget()
{
    delete ui;
}



QTableWidgetItem* createManufactureCenteredItem(const QString &text) {
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

void ManufactureWidget::showEvent(QShowEvent *event)
{
    BasePageWidget::showEvent(event); // 부모 이벤트 호출

    // 페이지가 열릴 때마다 최신 DB 정보를 불러옴
    loadManufactureData();
    loadScheduleData();

    // OPC UA 바인딩 (처음 한 번만 실행됨)
    QTimer::singleShot(0, this, [this](){
        setupOpcBindings();
    });
}

void ManufactureWidget::setupManufactureTableConfigs()
{
    QStringList headers = {"Code", "Name", "Stock", "Description"};
    ui->manufacture_table->setColumnCount(5); // 4 + 1 숨김(id)
    ui->manufacture_table->setHorizontalHeaderLabels(headers);
    ui->manufacture_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->manufacture_table->setColumnHidden(4, true); // id 숨김

    // 세로 스크롤바가 필요할 때만 나타나도록 설정
    ui->manufacture_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 마우스 휠 한 번에 이동하는 픽셀 수 조절 (부드러운 스크롤)
    ui->manufacture_table->verticalHeader()->setDefaultSectionSize(35); // 행 높이 고정

    // 테이블 끝에 도달했을 때 빈 공간mfgStatusUpdated이 생기지 않도록 설정
    ui->manufacture_table->verticalHeader()->setStretchLastSection(false);
}

void ManufactureWidget::setupScheduleTableConfigs()
{
    QStringList headers = {"Product", "Order Count", "Motor Speed", "Status", "Created at", "Deadline at", "Updated at"};
    ui->schedule_table->setColumnCount(8); // 7 + 1 숨김(id)
    ui->schedule_table->setHorizontalHeaderLabels(headers);
    ui->schedule_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->schedule_table->setColumnHidden(7, true); // id 숨김

    // 세로 스크롤바가 필요할 때만 나타나도록 설정
    ui->schedule_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 마우스 휠 한 번에 이동하는 픽셀 수 조절 (부드러운 스크롤)
    ui->schedule_table->verticalHeader()->setDefaultSectionSize(35); // 행 높이 고정

    // 테이블 끝에 도달했을 때 빈 공간이 생기지 않도록 설정
    ui->schedule_table->verticalHeader()->setStretchLastSection(false);
}

void ManufactureWidget::loadManufactureData()
{
    auto products = ManufactureService::getProducts();

    product_id_list.clear();
    ui->manufacture_table->setRowCount(0);

    for (int i = 0; i < products.size(); ++i) {
        ui->manufacture_table->insertRow(i);
        ui->manufacture_table->setItem(i, 0, new QTableWidgetItem(products[i].code));
        ui->manufacture_table->setItem(i, 1, new QTableWidgetItem(products[i].name));
        ui->manufacture_table->setItem(i, 2, new QTableWidgetItem(QString::number(products[i].stock)));
        ui->manufacture_table->setItem(i, 3, new QTableWidgetItem(products[i].description));
        ui->manufacture_table->setItem(i, 4, new QTableWidgetItem(products[i].id)); // 숨김 id
        product_id_list.append(products[i].id);
    }
    qDebug() << "총" << products.size() << "개의 제품을 불러왔습니다.";
}

void ManufactureWidget::loadScheduleData()
{
    // 1. 서비스에 데이터 요청 (API 호출 느낌)
    auto schedules = ManufactureService::getSchedules();

    ui->schedule_table->setRowCount(0);
    for (int i = 0; i < schedules.size(); ++i) {
        ui->schedule_table->insertRow(i);

        // 데이터 매핑
        QString productDisplayName = QString("[%1] %2").arg(schedules[i].productCode, schedules[i].productName);

        ui->schedule_table->setItem(i, 0, createManufactureCenteredItem(productDisplayName));
        ui->schedule_table->setItem(i, 1, createManufactureCenteredItem(QString::number(schedules[i].orderCount)));
        ui->schedule_table->setItem(i, 2, createManufactureCenteredItem(QString::number(schedules[i].motorSpeed)));
        ui->schedule_table->setItem(i, 3, createManufactureCenteredItem(schedules[i].status));
        ui->schedule_table->setItem(i, 4, new QTableWidgetItem(schedules[i].createdAt));
        ui->schedule_table->setItem(i, 5, new QTableWidgetItem(schedules[i].deadlineAt));
        ui->schedule_table->setItem(i, 6, new QTableWidgetItem(schedules[i].updatedAt));
        ui->schedule_table->setItem(i, 7, new QTableWidgetItem(schedules[i].id)); // 숨김 ID
    }
    qDebug() << "총" << schedules.size() << "개의 스케줄을 불러왔습니다.";
}

void ManufactureWidget::on_edit_stock_button_clicked()
{
    // 선택된 행 확인
    int row = ui->manufacture_table->currentRow();
    if (row < 0) {
        qDebug() << "수정할 항목을 선택해주세요.";
        return;
    }

    QString product_name = ui->manufacture_table->item(row, 1)->text();
    int current_stock    = ui->manufacture_table->item(row, 2)->text().toInt();
    QString product_id   = product_id_list[row];

    // 입력 다이얼로그
    bool ok;
    int new_stock = QInputDialog::getInt(
        this,
        "Edit Stock",
        product_name + " 재고 수정:",
        current_stock, // 기본값
        0,             // 최솟값
        99999,         // 최댓값
        1,
        &ok
        );

    if (ok) {
        if (ManufactureService::updateProductStock(product_id, new_stock)) {
            loadManufactureData(); // 테이블 새로고침
        }
    }
}

void ManufactureWidget::on_create_schedule_button_clicked()
{
    // 제조 스케줄 생성 팝업 호출
    ManufactureScheduleDialog *dialog = new ManufactureScheduleDialog(this);
    if (dialog->exec() == QDialog::Accepted) {
        loadScheduleData(); // 테이블 새로고침
    }
}

void ManufactureWidget::on_cancel_schedule_button_clicked()
{
    int row = ui->schedule_table->currentRow();
    if (row < 0) return;

    QString status = ui->schedule_table->item(row, 3)->text();
    if (status == "DONE") {
        QMessageBox::warning(this, "취소 불가", "이미 완료된 항목은 취소할 수 없습니다.");
        return;
    }

    QString selected_id = ui->schedule_table->item(row, 7)->text();

    // 2. 서비스에 삭제 요청
    if (ManufactureService::deleteSchedule(selected_id)) {
        loadScheduleData(); // 성공 시 새로고침
    } else {
        QMessageBox::critical(this, "에러", "스케줄 취소에 실패했습니다.");
    }
}

void ManufactureWidget::on_Back_btn_clicked()
{
    emit requestPageChange(PageType::Dashboard);
}

void ManufactureWidget::setupOpcBindings()
{
    if (m_opcBound) return; // 이미 연결되었다면 중단

    auto *mw = qobject_cast<MainWindow*>(window());
    if (!mw) return;

    auto *ua = mw->opcUaService();
    if (!ua) return;

    m_opcBound = true;

    // 1. 생산량이 변했을 때 (실시간 Stock 갱신 등)
    connect(ua, &OpcUaService::mfgProdCountUpdated, this, [this](quint64 count){
        // 현재 이 위젯이 화면에 보이고 있을 때만 UI 갱신 (DB 부하 감소)
        if (!this->isVisible()) return;

        qDebug() << "[MFG UI] Production count updated:" << count;
        loadManufactureData(); // 제품 재고 테이블 갱신
        loadScheduleData();    // 스케줄 진행률(Status) 갱신
    });

    // 2. 공정 상태가 변했을 때 (PENDING -> INPROC -> DONE 등)
    connect(ua, &OpcUaService::mfgStatusUpdated, this, [this](const QString &status){
        loadScheduleData();

        // 오토 모드이고 장비 상태가 'IDLE' 혹은 'Done'으로 변했다면 실행
        if (m_isAutoMode && (status.compare("Idle", Qt::CaseInsensitive) == 0 || 
                             status.compare("Done", Qt::CaseInsensitive) == 0)) {
            qDebug() << "[AUTO] Machine is Idle. Searching for next task...";
            tryProcessNextAutoTask();
        }
    });
    
}

void ManufactureWidget::on_auto_button_clicked()
{
    m_isAutoMode = !m_isAutoMode;

    if (m_isAutoMode) {
        ui->auto_button->setText("AutoMode : ON");
        ui->auto_button->setStyleSheet("background-color: #2ecc71; color: white;"); // 녹색 강조
        // 모드 켜자마자 장비가 놀고 있다면 바로 시작 시도
        tryProcessNextAutoTask();
    } else {
        ui->auto_button->setText("AutoMode : OFF");
        ui->auto_button->setStyleSheet(""); // 기본 스타일
    }
}


void ManufactureWidget::tryProcessNextAutoTask()
{
    if (!m_isAutoMode) return;

    auto *mw = qobject_cast<MainWindow*>(window());
    auto *ua = mw ? mw->opcUaService() : nullptr;

    if (!ua || !ua->isMfgConnected()) {
        qDebug() << "[AUTO] Error: OPC UA Server is disconnected.";
        on_auto_button_clicked();
        QMessageBox::critical(this, "통신 오류", "OPC UA 서버와 연결되어 있지 않아 자동 모드를 종료합니다.");
        return;
    }

    auto task = ManufactureService::getNextAutoPendingOrder();

    if (!task.valid) {
        qDebug() << "[AUTO] No pending orders found.";
        if (m_isAutoMode) on_auto_button_clicked();
        QMessageBox::information(this, "자동 모드 종료", "대기 중인(PENDING) 생산 데이터가 없습니다.");
        return;
    }

    auto orderInfo = ManufactureService::getProductionOrderById(task.orderId);
    if (!orderInfo.valid) {
        qDebug() << "[AUTO] getProductionOrderById failed:" << task.orderId;
        return;
    }

    if (orderInfo.recipe.trimmed().isEmpty()) {
        qDebug() << "[AUTO] Task" << orderInfo.orderId << "has no recipe. Skipping.";
        if (m_isAutoMode) on_auto_button_clicked();
        QMessageBox::information(this, "자동 모드 종료", "생산하고자 하는 제품의 레시피가 없습니다.");
        return;
    }

    emit productionOrderStarted(orderInfo.orderId, orderInfo.productId, orderInfo.recipe);

    ua->mfgWriteSpeed(orderInfo.motorSpeed <= 0 ? 100.0 : orderInfo.motorSpeed);
    ua->mfgStartOrder(orderInfo.orderId,
                      static_cast<quint16>(orderInfo.productNo),
                      static_cast<quint32>(orderInfo.orderCount));

    if (!ManufactureService::markProductionOrderInProc(orderInfo.orderId)) {
        qDebug() << "[AUTO] Failed to update status to INPROC for:" << orderInfo.orderId;
        return;
    }

    if (!ManufactureService::createProductLog(orderInfo)) {
        qDebug() << "[AUTO] Failed to create product log for:" << orderInfo.orderId;
    }

    qDebug() << "[AUTO] Successfully started order:" << orderInfo.orderId;
    loadScheduleData();
}
