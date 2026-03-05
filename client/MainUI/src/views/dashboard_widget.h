#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QLineSeries>
#include <QChart>
#include <QValueAxis>

namespace Ui { class DashboardWidget; }

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();
    void initChart(); // 차트를 초기화하는 함수

private:
    Ui::DashboardWidget *ui;

protected:
    // QWidget의 showEvent를 재정의합니다.
    void showEvent(QShowEvent *event) override;
};

#endif
