#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include "base/base_page_widget.h"
#include "../services/opcua_service.h"
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QChart>
#include <QShowEvent>
#include <QLabel>
#include <QGridLayout>

namespace Ui {
class DashboardWidget;
}

class DashboardWidget : public BasePageWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

    void set_opcua_service(OpcUaService *service);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_CompanyListBtn_clicked();
    void on_ScmManageBtn_clicked();
    void on_DeliveryBtn_clicked();
    void on_ProcessBtn_clicked();
    void on_ManufactureBtn_clicked();
    void on_ErrorLogBtn_clicked();

    void update_mfg_temp(double temp);
    void update_mfg_hum(double hum);
    void update_log_temp(double temp);
    void update_log_hum(double hum);

private:
    Ui::DashboardWidget *ui;
    void initStorageCharts();
    void initProductionChart();
    void initSensorWidget();
    void clearLayout(QLayout *layout);

    OpcUaService *m_opcua_service = nullptr;

    QLabel *mfg_temp_value = nullptr;
    QLabel *mfg_hum_value  = nullptr;
    QLabel *log_temp_value = nullptr;
    QLabel *log_hum_value  = nullptr;

    QLabel *machine_status[4] = {nullptr, nullptr, nullptr, nullptr};

    void initMachineStatusWidget();
    void update_log_conveyor(int idx, bool loading);
    void update_mfg_conveyor(const QString &status);
    void setMachineLamp(int idx, bool running);
};

#endif
