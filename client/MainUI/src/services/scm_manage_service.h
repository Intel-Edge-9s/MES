#pragma once
#include "../models/inventory_model.h"
#include "../models/inventory_order_logs_model.h"
#include <QString>
#include <QList>

struct InboundOrderTask {
    bool valid = false;
    QString id;
    QString itemId;
    QString itemCode;
    QString itemName;
    int stock = 0;
    QString status;
    QString location;
    int warehouseNo = 0;
};

struct RawMaterialStockSnapshot {
    quint32 s1 = 0;
    quint32 s2 = 0;
    quint32 s3 = 0;
    quint32 s4 = 0;
};

class ScmManageService
{
public:
    static QList<InventoryInfo> getInventoryStatus();
    static QList<OrderInfo> getOrderLogs();
    static bool addOrder(const QString& userName, const QString& itemCode, int amount, const QString& dueDate);
    static bool cancelOrder(const QString& orderId);

    static InboundOrderTask getInboundOrderTaskById(const QString& orderId);
    static int warehouseNoFromLocation(const QString& location);

    static bool markOrderInProc(const QString& orderId);
    static bool markOrderDone(const QString& orderId);
    static bool markOrderError(const QString& orderId);
    static bool increaseInventoryByOrderId(const QString& orderId, int delta);

    static RawMaterialStockSnapshot getRawMaterialStockSnapshot();
    static bool updateInventoryStockByItemCode(const QString &itemCode, int newStock);
};
