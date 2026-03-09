#include "delivery_service.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>

QList<ProductInfo> DeliveryService::getProducts() {
    QList<ProductInfo> list;
    QSqlQuery query("SELECT id, product_code, product_name, product_stock FROM product");
    while (query.next()) {
        ProductInfo info;
        info.id    = query.value("id").toString();
        info.code  = query.value("product_code").toString();
        info.name  = query.value("product_name").toString();
        info.stock = query.value("product_stock").toInt();
        list.append(info);
    }
    return list;
}

QList<CompanyInfo> DeliveryService::getCustomers() {
    QList<CompanyInfo> list;
    QSqlQuery query("SELECT id, company_name, company_address, company_number FROM cust_company");
    while (query.next()) {
        CompanyInfo info;
        info.id      = query.value("id").toString();
        info.name    = query.value("company_name").toString();
        info.address = query.value("company_address").toString();
        info.contact = query.value("company_number").toString();
        list.append(info);
    }
    return list;
}

QList<DeliveryInfo> DeliveryService::getDeliveries() {
    QList<DeliveryInfo> list;
    QSqlQuery query(
        "SELECT d.id, c.company_name, p.product_name, d.delivery_stock, "
        "d.status, d.created_at, d.updated_at "
        "FROM product_deli_logs d "
        "LEFT JOIN cust_company c ON d.company_id = c.id "
        "LEFT JOIN product p ON d.product_id = p.id "
        "ORDER BY d.created_at DESC"
        );

    while (query.next()) {
        DeliveryInfo info;
        info.id             = query.value("id").toString();
        info.company_name   = query.value("company_name").toString();
        info.product_name   = query.value("product_name").toString();
        info.delivery_stock = query.value("delivery_stock").toInt();
        info.status         = query.value("status").toString();
        info.created_at     = query.value("created_at").toString();
        info.updated_at     = query.value("updated_at").toString();
        list.append(info);
    }
    return list;
}



DeliveryCompleteResult DeliveryService::completeDelivery(const QString &id) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qWarning() << "[납품 완료 실패] DB not open";
        return DeliveryCompleteResult::DbError;
    }

    QSqlQuery query(db);

    // 1) 납품 주문 조회
    query.prepare(
        "SELECT d.status, d.product_id, d.delivery_stock, p.product_stock "
        "FROM product_deli_logs d "
        "LEFT JOIN product p ON d.product_id = p.id "
        "WHERE d.id = :id "
        "LIMIT 1"
        );
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "[납품 조회 실패]" << query.lastError().text();
        return DeliveryCompleteResult::DbError;
    }

    if (!query.next()) {
        qWarning() << "[납품 조회 실패] not found:" << id;
        return DeliveryCompleteResult::NotFound;
    }

    const QString status = query.value(0).toString().trimmed().toUpper();
    const QString productId = query.value(1).toString();
    const int deliveryQty = query.value(2).toInt();
    const int productStock = query.value(3).toInt();

    // 2) 이미 완료 여부
    if (status == "DONE") {
        qDebug() << "[납품 완료 불가] 이미 완료:" << id;
        return DeliveryCompleteResult::AlreadyDone;
    }

    // 3) 재고 부족 여부
    if (productId.isEmpty() || deliveryQty <= 0 || productStock < deliveryQty) {
        qDebug() << "[납품 완료 불가] 재고 부족 또는 잘못된 데이터:"
                 << "productId =" << productId
                 << "deliveryQty =" << deliveryQty
                 << "productStock =" << productStock;
        return DeliveryCompleteResult::NotEnoughStock;
    }

    // 4) 트랜잭션 시작
    if (!db.transaction()) {
        qWarning() << "[납품 완료 실패] transaction start failed:" << db.lastError().text();
        return DeliveryCompleteResult::DbError;
    }

    // 5) 제품 재고 차감
    QSqlQuery updateStock(db);
    updateStock.prepare(
        "UPDATE product "
        "SET product_stock = product_stock - :qty "
        "WHERE id = :productId AND product_stock >= :qty"
        );
    updateStock.bindValue(":qty", deliveryQty);
    updateStock.bindValue(":productId", productId);

    if (!updateStock.exec()) {
        qWarning() << "[제품 재고 차감 실패]" << updateStock.lastError().text();
        db.rollback();
        return DeliveryCompleteResult::DbError;
    }

    if (updateStock.numRowsAffected() <= 0) {
        qWarning() << "[제품 재고 차감 실패] affected rows 0";
        db.rollback();
        return DeliveryCompleteResult::NotEnoughStock;
    }

    // 6) 납품 완료 처리
    QSqlQuery updateDelivery(db);
    updateDelivery.prepare(
        "UPDATE product_deli_logs "
        "SET status = 'DONE', updated_at = NOW() "
        "WHERE id = :id AND status <> 'DONE'"
        );
    updateDelivery.bindValue(":id", id);

    if (!updateDelivery.exec()) {
        qWarning() << "[납품 상태 업데이트 실패]" << updateDelivery.lastError().text();
        db.rollback();
        return DeliveryCompleteResult::DbError;
    }

    if (updateDelivery.numRowsAffected() <= 0) {
        qWarning() << "[납품 상태 업데이트 실패] affected rows 0";
        db.rollback();
        return DeliveryCompleteResult::AlreadyDone;
    }

    if (!db.commit()) {
        qWarning() << "[납품 완료 실패] commit failed:" << db.lastError().text();
        db.rollback();
        return DeliveryCompleteResult::DbError;
    }

    qDebug() << "[납품 완료 성공]" << id << "qty =" << deliveryQty;
    return DeliveryCompleteResult::Success;
}
