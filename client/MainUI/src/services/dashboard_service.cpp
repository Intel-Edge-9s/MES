#include "dashboard_service.h"
#include <QSqlQuery>
#include <QMap>

QList<InventoryInfo> DashboardService::getStorageCharts() {
    QList<InventoryInfo> list;
    QSqlQuery query("SELECT id, company_id, item_code, item_name, current_stock, "
                    "min_stock_level, max_stock_level, unit, location "
                    "FROM inventory");
    while (query.next()) {
        InventoryInfo info;

        // QString 필드 매핑
        info.id         = query.value("id").toString();
        info.company_id = query.value("company_id").toString();
        info.item_code  = query.value("item_code").toString();
        info.item_name  = query.value("item_name").toString();

        // int 필드 매핑 (DB 컬럼명과 구조체 변수명 매칭)
        info.current_stock   = query.value("current_stock").toInt();
        info.min_stock_level = query.value("min_stock_level").toInt();
        info.max_stock_level = query.value("max_stock_level").toInt();

        // 나머지 QString 필드
        info.unit       = query.value("unit").toString();
        info.location   = query.value("location").toString();
        list.append(info);
    }
    return list;
}

QList<LocationInfo> DashboardService::getLocations() {
    QList<LocationInfo> list;
    QSqlQuery query("SELECT DISTINCT location "
                    "FROM inventory");
    while (query.next()) {
        LocationInfo info;

        info.location = query.value("location").toString();
        list.append(info);
    }
    return list;
}

QList<ProductionChartInfo> DashboardService::getProductionChart() {
    // 더미 데이터 (DB 데이터 부족으로 임시 사용)
    QList<ProductionChartInfo> list;

    QMap<QString, QList<int>> dummyData = {
        {"PRD-ALU-001", {5, 4, 8, 9, 7, 6, 9}},
        {"PRD-WIR-001", {4, 3, 6, 7, 8, 5, 10}},
        {"PRD-SEN-001", {2, 1, 3, 4, 3, 2, 3}}
    };

    QStringList dates = {"03-03", "03-04", "03-05", "03-06", "03-07", "03-08", "03-09"};

    for (const QString &product : dummyData.keys()) {
        const QList<int> &counts = dummyData[product];
        for (int i = 0; i < dates.size(); ++i) {
            ProductionChartInfo info;
            info.product_name = product;
            info.date         = dates[i];
            info.prod_count   = counts[i];
            list.append(info);
        }
    }

    return list;
}
