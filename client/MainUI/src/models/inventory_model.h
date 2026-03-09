#pragma once
#include <QString>

struct InventoryInfo {
    QString id;
    QString company_id;
    QString item_code;
    QString item_name;
    int current_stock;
    int min_stock_level;
    int max_stock_level;
    QString unit;
    QString location;
};

struct LocationInfo {
    QString location;
};

struct ProductionChartInfo {
    QString product_name;
    QString date;
    int prod_count;
};
