#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "base/base_page_widget.h"
#include "login_widget.h"
#include "dashboard_widget.h"
#include "partner_manage_widget.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    //===========================================
    //
    //===========================================
    ua = new OpcUaService(this);
    //===========================================
    //
    //===========================================

    setupNavigation();
    moveToPage(PageType::Login);
    //===========================================
    //
    //===========================================
    auto* login = qobject_cast<LoginWidget*>(ui->loginPage);
    if (login) {
        // ✅ 로그인 성공 “딱 한번”만 통신 시작
        connect(login, &LoginWidget::loginSuccess, this, [this](){
            startOpcUaOnce();
        });
        connect(ua, &OpcUaService::mfgTempUpdated, this, [](double t){
            qDebug() << "MFG TEMP =" << t;
        });

        connect(ua, &OpcUaService::logTempUpdated, this, [](double t){
            qDebug() << "LOG TEMP =" << t;
        });

    }

    //===========================================
    //
    //===========================================

}

MainWindow::~MainWindow() {
    delete ui;
}
//===========================================
//
//===========================================
void MainWindow::startOpcUaOnce()
{
    if (uaStarted) return;
    uaStarted = true;

    ua->connectMfg(
        "opc.tcp://10.10.16.208:4850",
        "mes","mespw_change_me",
        "/home/pi/opcua_project/certs/mes/cert.der",
        "/home/pi/opcua_project/certs/mes/key.der",
        "/home/pi/opcua_project/certs/mes/trust_mfg.der"
        );

    ua->connectLog(
        "opc.tcp://10.10.16.210:4841",
        "mes","mespw_change_me",
        "/home/pi/opcua_project/certs/mes/cert.der",
        "/home/pi/opcua_project/certs/mes/key.der",
        "/home/pi/opcua_project/certs/mes/trust_log.der"
        );


}
//===========================================
//
//===========================================

void MainWindow::setupNavigation() {
    // 위젯 캐스팅
    auto* login = qobject_cast<BasePageWidget*>(ui->loginPage);
    auto* dashboard = qobject_cast<BasePageWidget*>(ui->dashBoardPage);
    auto* partnerManage = qobject_cast<BasePageWidget*>(ui->partnerManagePage);

    // 모든 위젯을 리스트에 담아 한 번에 연결 (중복 코드 방지)
    QList<BasePageWidget*> pages = {login, dashboard, partnerManage};

    for (BasePageWidget* page : pages) {
        if (page) {
            // Signal: requestPageChange(PageType)
            // Slot: moveToPage(PageType) -> 타입이 일치해야 합니다!
            connect(page, &BasePageWidget::requestPageChange, this, &MainWindow::moveToPage);
        }
    }
}

void MainWindow::moveToPage(PageType type) {
    switch (type) {
    case PageType::Login:
        ui->stackedWidget->setCurrentWidget(ui->loginPage);
        break;
    case PageType::Dashboard:
        ui->stackedWidget->setCurrentWidget(ui->dashBoardPage);
        break;
    case PageType::PartnerManage:
        ui->stackedWidget->setCurrentWidget(ui->partnerManagePage);
        break;
    case PageType::ScmManage:
        ui->stackedWidget->setCurrentWidget(ui->ScmManagePage);
        break;
    }
}
