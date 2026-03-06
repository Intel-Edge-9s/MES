#include "dashboard_service.h"
#include <QSqlQuery>

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
