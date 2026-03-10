#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "base/base_page_widget.h"
#include "login_widget.h"
#include "dashboard_widget.h"
#include "partner_manage_widget.h"
#include "process_widget.h"
#include "manufacture_widget.h"
#include "scm_manage_widget.h"
#include "../core/database_manager.h"
#include "../services/manufacture_service.h"
#include "../services/scm_manage_service.h"
#include "../services/environment_logs_service.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QFile>
const double kFireTestThreshold = 50.0;



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ua = new OpcUaService(this);
    auto* manufacture = qobject_cast<ManufactureWidget*>(ui->manufacturePage);
    auto* process = qobject_cast<ProcessWidget*>(ui->processPage);
    auto* scmManage = qobject_cast<ScmManageWidget*>(ui->ScmManagePage);
    if (process) {
        process->setOpcUaService(ua);

        connect(process, &ProcessWidget::productionOrderStarted,
                this, [this](const QString &orderId, const QString &productId, const QString &recipe){
                    m_activeProdOrderId = orderId;
                    m_activeProductId = productId;
                    m_activeRecipe = recipe;
                    m_activeRecipeItems = ManufactureService::parseRecipeString(recipe);
                    m_lastProdCount = 0;
                    m_lastAttemptCount = 0;
                    m_lastDefectCount = 0;
                    m_materialStopRequested = false;
                });
        connect(process, &ProcessWidget::manualProcessStopRequested,
                this, [this](const QString &processName){
                    qDebug() << "[MAIN] manualProcessStopRequested =" << processName;

                    if (!ua)
                        return;

                    // 제조 수동 정지
                    if (processName == "제조 컨테이너 1") {
                        if (!m_activeProdOrderId.isEmpty()) {

                            ManufactureService::markProductionOrderError(m_activeProdOrderId);
                            ManufactureService::updateProductLogProgress(
                                m_activeProdOrderId,
                                m_lastProdCount,
                                m_lastDefectCount,
                                "ERROR"
                                );
                        }

                        ua->mfgStopOrder();
                        clearActiveProduction();
                        return;
                    }

                    // 물류 수동 정지

                    int wh = 0;
                    if (processName == "컨베이어 벨트 1") wh = 1;
                    else if (processName == "컨베이어 벨트 2") wh = 2;
                    else if (processName == "컨베이어 벨트 3") wh = 3;

                    if (wh > 0) {
                        const QString orderId = m_activeInboundOrderIdByWh.value(wh);



                        if (!orderId.isEmpty()) {
                            ScmManageService::markOrderError(orderId);
                            m_activeInboundOrderIdByWh.remove(wh);
                        }

                        ua->logStopMove(static_cast<quint16>(wh));
                        return;
                    }
                });


    }
    if (manufacture) {
        connect(manufacture, &ManufactureWidget::productionOrderStarted,
                this, [this](const QString &orderId, const QString &productId, const QString &recipe){
                    m_activeProdOrderId = orderId;
                    m_activeProductId = productId;
                    m_activeRecipe = recipe;
                    m_activeRecipeItems = ManufactureService::parseRecipeString(recipe);
                    m_lastProdCount = 0;
                    m_lastAttemptCount = 0;
                    m_lastDefectCount = 0;
                    m_materialStopRequested = false;

                    qDebug() << "[MAIN] productionOrderStarted from manufacture:"
                             << orderId << productId << recipe;
                });
    }
    auto* dashboard = qobject_cast<DashboardWidget*>(ui->dashBoardPage);
    if (dashboard) {
        dashboard->set_opcua_service(ua);
    }

    if (scmManage) {
        connect(scmManage, &ScmManageWidget::activeInboundOrderChanged,
                this, [this](int wh, const QString &orderId){
                    if (orderId.isEmpty()) {
                        m_activeInboundOrderIdByWh.remove(wh);
                    } else {
                        m_activeInboundOrderIdByWh[wh] = orderId;
                    }

                    qDebug() << "[MAIN] active inbound order changed:"
                             << "wh =" << wh
                             << "orderId =" << orderId;
                });
    }

    connect(ua, &OpcUaService::mfgTempUpdated, this, [this](double temp){
        if (temp >= kFireTestThreshold) {
            if (!m_mfgFireTriggered) {
                m_mfgFireTriggered = true;
                requestEmergencyStopAll("MFG", temp);
            }
        } else {
            m_mfgFireTriggered = false;
        }
    });
    connect(ua, &OpcUaService::logTempUpdated, this, [this](double temp){
        if (temp >= kFireTestThreshold) {
            if (!m_logFireTriggered) {
                m_logFireTriggered = true;
                requestEmergencyStopAll("LOG", temp);
            }
        } else {
            m_logFireTriggered = false;
        }
    });


    connect(ua, &OpcUaService::mfgAuthRequestReceived, this,
            [this](const QString &id, const QString &pw){
                const bool ok = m_authService.checkServerAccount("MFG", id, pw);
                qDebug() << "[MES] MFG auth request:" << "id =" << id << "result =" << ok;
                ua->mfgSendAuthResult(ok);
            });


    connect(ua, &OpcUaService::logAuthRequestReceived, this,
            [this](const QString &id, const QString &pw){
                const bool ok = m_authService.checkServerAccount("LOG", id, pw);
                qDebug() << "[MES] LOG auth request:" << "id =" << id << "result =" << ok;
                ua->logSendAuthResult(ok);

                if (ok) {
                    m_logStockSeeded = true;

                    QTimer::singleShot(300, this, [this]() {
                        const RawMaterialStockSnapshot snap = ScmManageService::getRawMaterialStockSnapshot();
                        qDebug() << "[MES] LOG seed stocks from DB:"
                                 << snap.s1 << snap.s2 << snap.s3 << snap.s4;
                        ua->logInitItemStocks(snap.s1, snap.s2, snap.s3, snap.s4);
                    });
                }
            });


    connect(ua, &OpcUaService::logConnectedChanged, this, [this](bool connected){
        if (!connected) {
            m_logStockSeeded = false;
            return;
        }
    });

    connect(ua, &OpcUaService::mfgProdCountUpdated, this, [this](quint64 v) {
        if (m_activeProdOrderId.isEmpty() || m_activeProductId.isEmpty())
            return;

        const int prodCount = static_cast<int>(v);
        const int delta = prodCount - m_lastProdCount;
        if (delta <= 0)
            return;

        m_lastProdCount = prodCount;
        ManufactureService::increaseProductStock(m_activeProductId, delta);
        ManufactureService::updateProductLogProgress(m_activeProdOrderId, m_lastProdCount, m_lastDefectCount, "INPROC");
    });

    connect(ua, &OpcUaService::mfgDefectCountUpdated, this, [this](quint64 v) {
        if (m_activeProdOrderId.isEmpty())
            return;

        m_lastDefectCount = static_cast<int>(v);
        ManufactureService::updateProductLogProgress(m_activeProdOrderId, m_lastProdCount, m_lastDefectCount, "INPROC");
    });

    connect(ua, &OpcUaService::mfgAttemptCountUpdated, this, [this](quint64 v) {
        if (m_activeProdOrderId.isEmpty() || m_activeProductId.isEmpty())
            return;

        const int attemptCount = static_cast<int>(v);
        const int delta = attemptCount - m_lastAttemptCount;
        if (delta <= 0)
            return;

        m_lastAttemptCount = attemptCount;

        if (m_activeRecipeItems.isEmpty())
            m_activeRecipeItems = ManufactureService::parseRecipeString(m_activeRecipe);

        for (int n = 0; n < delta; ++n) {
            for (const auto &item : m_activeRecipeItems) {
                if (item.itemCode.isEmpty() || item.quantityRequired <= 0)
                    continue;
                ua->logConsumeItem(item.itemCode, static_cast<quint32>(item.quantityRequired));
            }
        }

        ManufactureService::updateProductLogProgress(m_activeProdOrderId, m_lastProdCount, m_lastDefectCount, "INPROC");
    });


    connect(ua, &OpcUaService::logItemStockUpdated, this,
            [this](const QString &itemCode, quint32 stock){
                const QString key = itemCode.trimmed().toLower();
                m_itemStocks[key] = static_cast<int>(stock);

                if (!m_logStockSeeded) {
                    qDebug() << "[MES] ignore pre-seed stock update:" << itemCode << stock;
                    return;
                }

                ScmManageService::updateInventoryStockByItemCode(itemCode, static_cast<int>(stock));

                if (!m_activeProdOrderId.isEmpty() && stock <= 5)
                    requestMaterialStop(QString("low stock: %1=%2").arg(itemCode).arg(stock));
            });

    connect(ua, &OpcUaService::logItemLowStockUpdated, this,
            [this](const QString &itemCode, bool low){
                m_itemLowFlags[itemCode.toLower()] = low;
                if (!m_activeProdOrderId.isEmpty() && low)
                    requestMaterialStop(QString("low_stock node true: %1").arg(itemCode));
            });



    connect(ua, &OpcUaService::errorOccurred, this,
            [this](const QString &where, const QString &msg){
                if (!m_activeProdOrderId.isEmpty() && where == "logConsumeItem")
                    requestMaterialStop(QString("consume failed: %1").arg(msg));
            });

    connect(ua, &OpcUaService::mfgStatusUpdated, this, [this](const QString &status) {
        if (m_activeProdOrderId.isEmpty())
            return;

        if (status.compare("Done", Qt::CaseInsensitive) == 0) {
            ManufactureService::markProductionOrderDone(m_activeProdOrderId);
            ManufactureService::updateProductLogProgress(m_activeProdOrderId, m_lastProdCount, m_lastDefectCount, "DONE");
            clearActiveProduction();
        }
    });



    setupNavigation();
    moveToPage(PageType::Login);

    auto* login = qobject_cast<LoginWidget*>(ui->loginPage);
    if (login) {
        connect(login, &LoginWidget::loginSuccess, this, [this](){
            startOpcUaOnce();
        });

        connect(ua, &OpcUaService::mfgSpeedUpdated, this, [](double speed){
            qDebug() << "MFG SPEED =" << speed;
        });

        connect(ua, &OpcUaService::mfgProdCountUpdated, this, [](quint64 v){
            qDebug() << "MFG PRODCOUNT =" << v;
        });
    }
}

MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::requestMaterialStop(const QString &reason)
{
    if (m_activeProdOrderId.isEmpty() || m_materialStopRequested)
        return;

    qDebug() << "[MES] requestMaterialStop reason =" << reason;
    m_materialStopRequested = true;


    ManufactureService::markProductionOrderError(m_activeProdOrderId);
    ManufactureService::updateProductLogProgress(
        m_activeProdOrderId,
        m_lastProdCount,
        m_lastDefectCount,
        "ERROR"
        );

    if (ua)
        ua->mfgStopOrder();

    clearActiveProduction();
}


void MainWindow::clearActiveProduction()
{
    m_activeProdOrderId.clear();
    m_activeProductId.clear();
    m_activeRecipe.clear();
    m_activeRecipeItems.clear();
    m_lastProdCount = 0;
    m_lastDefectCount = 0;
    m_lastAttemptCount = 0;
    m_materialStopRequested = false;
}

void MainWindow::startOpcUaOnce()
{
    qDebug() << QFile::exists("/home/pi/MES/servers/certs/mfg/cert.der");
    if (uaStarted) return;
    uaStarted = true;

    ua->connectMfg(
        "opc.tcp://10.10.16.208:4850",
        "mes","mespw_change_me",
        "/home/pi/MES/servers/certs/mes/cert.der",
        "/home/pi/MES/servers/certs/mes/key.der",
        "/home/pi/MES/servers/certs/mes/trust_mfg.der"
    );

    ua->connectLog(
        "opc.tcp://10.10.16.210:4841",
        "mes","mespw_change_me",
        "/home/pi/MES/servers/certs/mes/cert.der",
        "/home/pi/MES/servers/certs/mes/key.der",
        "/home/pi/MES/servers/certs/mes/trust_log.der"
    );
}

void MainWindow::requestEmergencyStopAll(const QString &source, double temp)
{
    if (m_globalEmergencyStop)
        return;

    m_globalEmergencyStop = true;

    qDebug() << "[MES] EMERGENCY STOP ALL:"
             << "source =" << source
             << "temp =" << temp;

    m_environmentLogsService.insertLog(
        source == "MFG" ? 2 : 1,
        "TEMP",
        QString::number(temp, 'f', 1),
        QString("FIRE DETECTED: %1 temp=%2")
            .arg(source)
            .arg(QString::number(temp, 'f', 1))
        );

    if (ua) {
        QMessageBox::critical(this, "화재 경고",
                              QString("%1 온도가 %2°C로 감지되어 전체 공정을 정지했습니다.")
                                  .arg(source)
                                  .arg(QString::number(temp, 'f', 1)));
        ua->mfgStopOrder();
        ua->logStopMove(1);
        ua->logStopMove(2);
        ua->logStopMove(3);

    }
    clearActiveProduction();
}

void MainWindow::setupNavigation() {
    auto* login = qobject_cast<BasePageWidget*>(ui->loginPage);
    auto* dashboard = qobject_cast<BasePageWidget*>(ui->dashBoardPage);
    auto* partnerManage = qobject_cast<BasePageWidget*>(ui->partnerManagePage);
    auto* scmManage = qobject_cast<BasePageWidget*>(ui->ScmManagePage);
    auto* delivery = qobject_cast<BasePageWidget*>(ui->deliveryPage);
    auto* process = qobject_cast<BasePageWidget*>(ui->processPage);
    auto* manufacture = qobject_cast<BasePageWidget*>(ui->manufacturePage);
    auto* enviromentLogs = qobject_cast<BasePageWidget*>(ui->environmentLogsPage);

    QList<BasePageWidget*> pages = {login, dashboard, partnerManage, scmManage, delivery, process, manufacture, enviromentLogs};

    for (BasePageWidget* page : pages) {
        if (page) {
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
    case PageType::Delivery:
        ui->stackedWidget->setCurrentWidget(ui->deliveryPage);
        break;
    case PageType::Process:
        ui->stackedWidget->setCurrentWidget(ui->processPage);
        break;
    case PageType::Manufacture:
        ui->stackedWidget->setCurrentWidget(ui->manufacturePage);
        break;
    case PageType::EnvironmentLogs:
        ui->stackedWidget->setCurrentWidget(ui->environmentLogsPage);
        break;
    }
}
