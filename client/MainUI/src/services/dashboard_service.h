#pragma once
#include "../models/inventory_model.h"
#include <QList>

class DashboardService {
public:
    static QList<InventoryInfo> getStorageCharts();
    static QList<LocationInfo> getLocations();
    static QList<ProductionChartInfo> getProductionChart();
};
