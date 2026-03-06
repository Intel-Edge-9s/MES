#include "scm_manage_service.h"
#include "../core/user_session.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>


QList<InventoryInfo> ScmManageService::getInventoryStatus(){
    QList<InventoryInfo> list;
    // 재고 현황을 가져오는 쿼리
    QSqlQuery query("SELECT id, item_code, item_name, current_stock, location FROM inventory");

    while (query.next()) {
        InventoryInfo info;
        info.id = query.value("id").toString();
        info.item_code = query.value("item_code").toString();
        info.item_name = query.value("item_name").toString();
        info.current_stock = query.value("current_stock").toInt();
        info.location = query.value("location").toString();
        list.append(info);
    }
    return list;
}
QList<OrderInfo> ScmManageService::getOrderLogs(){
    QList<OrderInfo> list;

    // 💡 JOIN을 사용하여 user 테이블의 user_name을 가져옵니다.
    // l은 inventory_order_logs의 별칭, u는 user의 별칭입니다.
    QSqlQuery query("SELECT l.id, u.user_name, l.item_name, l.stock, l.status, "
                    "l.created_at, l.receive_at, l.updated_at "
                    "FROM inventory_order_logs l "
                    "JOIN user u ON l.user_id = u.id "
                    "ORDER BY l.created_at DESC");

    while (query.next()) {
        OrderInfo info;
        info.id = query.value("id").toString();
        // 💡 이제 user_id 대신 JOIN으로 가져온 user_name을 사용합니다.
        info.userName = query.value("user_name").toString();
        info.itemName = query.value("item_name").toString();
        info.stock = query.value("stock").toInt();
        info.status = query.value("status").toString();
        info.createdAt = query.value("created_at").toString();
        info.receiveAt = query.value("receive_at").toString();
        info.updatedAt = query.value("updated_at").toString();

        list.append(info);
    }
    return list;
}

bool ScmManageService::addOrder(const QString& userName, const QString& itemCode, int amount, const QString& dueDate) {
    QSqlQuery query;
    query.prepare("INSERT INTO inventory_order_logs (id, user_id, item_id, item_code, item_name, stock, status, created_at, receive_at) "
                  "SELECT UUID_v4(), :userId, "
                  "id, item_code, item_name, :amount, 'PENDING', NOW(), :dueDate "
                  "FROM inventory WHERE item_code = :code");


    query.bindValue(":userId", UserSession::instance().userId());
    query.bindValue(":amount", amount);
    query.bindValue(":code", itemCode);
    query.bindValue(":dueDate", dueDate);

    return query.exec();
}

bool ScmManageService::cancelOrder(const QString& orderId) {
    QSqlQuery query;
    query.prepare("DELETE FROM inventory_order_logs WHERE id = :id");
    query.bindValue(":id", orderId);
    return query.exec();
}
