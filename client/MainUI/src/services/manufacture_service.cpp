#include "manufacture_service.h"
#include "../core/user_session.h"
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

namespace {
    static QList<RecipeItem> buildRecipeItemsFromString(const QString &recipe)
    {
        QList<RecipeItem> list;

        const QStringList tokens = recipe.split(',', Qt::KeepEmptyParts);
        for (int i = 0; i < 4; ++i) {
            RecipeItem item;
            item.itemCode = QString("s%1").arg(i + 1);
            item.itemName = item.itemCode;

            const QString token = (i < tokens.size()) ? tokens[i].trimmed() : QString();
            bool ok = false;
            const int qty = token.toInt(&ok);
            item.quantityRequired = (ok && qty > 0) ? qty : 0;

            list.append(item);
        }

        return list;
    }
}

QList<RecipeItem> ManufactureService::parseRecipeString(const QString &recipe)
{
    return buildRecipeItemsFromString(recipe);
}

bool ManufactureService::consumeRecipeItems(const QString &productId, int producedDelta)
{
    Q_UNUSED(productId);
    Q_UNUSED(producedDelta);
    // 원재료 DB 반영은 LOG 서버 item stock 노드를 구독한 클라이언트가 수행한다.
    // 직접 DB 차감은 하지 않는다.
    return true;
}

// ====================
// SELECT
// ====================

QList<ManufactureInfo> ManufactureService::getProducts() {
    QList<ManufactureInfo> list;
    QSqlQuery query(
        "SELECT id, product_code, product_name, product_stock, description "
        "FROM product");

    while (query.next()) {
        ManufactureInfo info;
        info.id = query.value("id").toString();
        info.code = query.value("product_code").toString();
        info.name = query.value("product_name").toString();
        info.stock = query.value("product_stock").toInt();
        info.description = query.value("description").toString();
        list.append(info);
    }
    return list;
}

ProductionOrderTask ManufactureService::getProductionOrderById(const QString &orderId)
{
    ProductionOrderTask task;

    QSqlQuery query;
    query.prepare(
        "SELECT o.id, o.product_id, o.order_count, o.motor_speed, o.status, "
        "p.product_code, p.product_name, p.recipe "
        "FROM product_order_logs o "
        "JOIN product p ON o.product_id = p.id "
        "WHERE o.id = :id "
        "LIMIT 1");
    query.bindValue(":id", orderId);

    if (!query.exec()) {
        qDebug() << "getProductionOrderById failed:" << query.lastError().text();
        return task;
    }

    if (!query.next())
        return task;

    task.valid = true;
    task.orderId = query.value("id").toString();
    task.productId = query.value("product_id").toString();
    task.productCode = query.value("product_code").toString();
    task.productName = query.value("product_name").toString();
    task.recipe = query.value("recipe").toString();
    task.orderCount = query.value("order_count").toInt();
    task.motorSpeed = query.value("motor_speed").toInt();
    task.status = query.value("status").toString();

    QRegularExpression re("(\\d+)$");
    auto match = re.match(task.productCode);
    task.productNo = match.hasMatch() ? match.captured(1).toInt() : 1;
    if (task.productNo <= 0)
        task.productNo = 1;

    return task;
}


QList<RecipeItem> ManufactureService::getRecipeItemsByProductId(const QString &productId)
{
    QSqlQuery query;
    query.prepare(
        "SELECT recipe "
        "FROM product "
        "WHERE id = :productId "
        "LIMIT 1");
    query.bindValue(":productId", productId);

    if (!query.exec()) {
        qDebug() << "getRecipeItemsByProductId failed:" << query.lastError().text();
        return {};
    }

    if (!query.next())
        return {};

    return buildRecipeItemsFromString(query.value("recipe").toString());
}

QList<ManufactureScheduleInfo> ManufactureService::getSchedules() {
    QList<ManufactureScheduleInfo> list;
    QSqlQuery query(
        "SELECT p.product_code, p.product_name, o.id, o.order_count, o.motor_speed, "
        "o.status, o.created_at, o.deadline_at, o.updated_at "
        "FROM product_order_logs o "
        "LEFT JOIN product p ON o.product_id = p.id "
        "ORDER BY o.created_at DESC");

    while (query.next()) {
        ManufactureScheduleInfo info;
        info.id = query.value("id").toString();
        info.productCode = query.value("product_code").toString();
        info.productName = query.value("product_name").toString();
        info.orderCount = query.value("order_count").toInt();
        info.motorSpeed = query.value("motor_speed").toInt();
        info.status = query.value("status").toString();
        info.createdAt = query.value("created_at").toString();
        info.deadlineAt = query.value("deadline_at").toString();
        info.updatedAt = query.value("updated_at").toString();
        list.append(info);
    }
    return list;
}

ProductionOrderTask ManufactureService::getNextAutoPendingOrder() {
    ProductionOrderTask task;
    task.valid = false;

    QSqlQuery query;
    // 7일 이내(EDD) + 그 외 수량 적은 순(SPT) 정렬
    QString sql = 
        "SELECT l.id, l.order_count, l.motor_speed, l.status, p.product_code, p.recipe "
        "FROM product_order_logs l "
        "JOIN product p ON l.product_id = p.id "
        "WHERE l.status = 'PENDING' "
        "ORDER BY "
        "  (l.deadline_at <= DATE_ADD(NOW(), INTERVAL 7 DAY)) DESC, " // 1순위: 7일 이내 여부
        "  CASE WHEN l.deadline_at <= DATE_ADD(NOW(), INTERVAL 7 DAY) THEN l.deadline_at END ASC, " // 2순위: 급한 날짜
        "  l.order_count ASC, " // 3순위: 적은 수량
        "  l.created_at ASC "   // 4순위: 먼저 생성된 순
        "LIMIT 1";

    if (query.exec(sql) && query.next()) {
        task.valid = true;
        task.orderId = query.value("id").toString();
        task.orderCount = query.value("order_count").toInt();
        task.motorSpeed = query.value("motor_speed").toInt();
        task.status = query.value("status").toString();
        task.productCode = query.value("product_code").toString();
        task.recipe = query.value("recipe").toString();

        QRegularExpression re("(\\d+)$");
        auto match = re.match(task.productCode);
        task.productNo = match.hasMatch() ? match.captured(1).toInt() : 1;
    }
    return task;
}

// ====================
// UPDATE
// ====================

bool ManufactureService::updateProductStock(const QString &product_id, int new_stock) {
    QSqlQuery query;
    query.prepare(
        "UPDATE product "
        "SET product_stock = :stock "
        "WHERE id = :id");
    query.bindValue(":stock", new_stock);
    query.bindValue(":id", product_id);

    if (query.exec()) {
        qDebug() << "[재고 수정 성공]" << product_id << "→" << new_stock;
        return true;
    }

    qDebug() << "[재고 수정 실패]" << query.lastError().text();
    return false;
}

bool ManufactureService::markProductionOrderInProc(const QString &orderId)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE product_order_logs "
        "SET status = 'INPROC', updated_at = NOW() "
        "WHERE id = :id AND status = 'PENDING'");
    query.bindValue(":id", orderId);

    if (!query.exec()) {
        qDebug() << "markProductionOrderInProc failed:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool ManufactureService::markProductionOrderDone(const QString &orderId)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE product_order_logs "
        "SET status = 'DONE', updated_at = NOW() "
        "WHERE id = :id");
    query.bindValue(":id", orderId);

    if (!query.exec()) {
        qDebug() << "markProductionOrderDone failed:" << query.lastError().text();
        return false;
    }

    return true;
}

bool ManufactureService::markProductionOrderWaitMaterial(const QString &orderId)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE product_order_logs "
        "SET status = 'INPROC', updated_at = NOW() "
        "WHERE id = :id");
    query.bindValue(":id", orderId);

    if (!query.exec()) {
        qDebug() << "markProductionOrderWaitMaterial failed:" << query.lastError().text();
        return false;
    }

    return true;
}
bool ManufactureService::markProductionOrderError(const QString &orderId)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE product_order_logs "
        "SET status = 'ERROR', updated_at = NOW() "
        "WHERE id = :id"
        );
    query.bindValue(":id", orderId);

    if (!query.exec()) {
        qDebug() << "markProductionOrderError failed:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool ManufactureService::updateProductLogProgress(const QString &orderId, int prodCount, int defectCount, const QString &status)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE product_logs "
        "SET prod_count = :prodCount, defect_count = :defectCount, status = :status, "
        "ended_at = CASE WHEN :status = 'DONE' THEN NOW() ELSE ended_at END "
        "WHERE order_id = :orderId");
    query.bindValue(":prodCount", prodCount);
    query.bindValue(":defectCount", defectCount);
    query.bindValue(":status", status);
    query.bindValue(":orderId", orderId);

    if (!query.exec()) {
        qDebug() << "updateProductLogProgress failed:" << query.lastError().text();
        return false;
    }

    return true;
}

bool ManufactureService::increaseProductStock(const QString &productId, int delta)
{
    if (delta <= 0) return true;

    QSqlQuery query;
    query.prepare(
        "UPDATE product "
        "SET product_stock = COALESCE(product_stock, 0) + :delta "
        "WHERE id = :id");
    query.bindValue(":delta", delta);
    query.bindValue(":id", productId);

    if (!query.exec()) {
        qDebug() << "increaseProductStock failed:" << query.lastError().text();
        return false;
    }

    return true;
}

// ====================
// INSERT
// ====================

bool ManufactureService::createProductLog(const ProductionOrderTask &task)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO product_logs "
        "(id, order_id, user_id, assignment_part, motor_speed, prod_count, defect_count, status, started_at) "
        "VALUES (UUID(), :orderId, :userId, 'MFG', :motorSpeed, 0, 0, 'INPROC', NOW())");
    query.bindValue(":orderId", task.orderId);
    query.bindValue(":userId", UserSession::instance().userId().isEmpty() 
                ? QVariant(QMetaType(QMetaType::QString)) // 명시적 NULL QString 타입
                : QVariant(UserSession::instance().userId()));
    query.bindValue(":motorSpeed", task.motorSpeed);

    if (!query.exec()) {
        qDebug() << "createProductLog failed:" << query.lastError().text();
        return false;
    }

    return true;
}

// ====================
// DELETE
// ====================

bool ManufactureService::deleteSchedule(const QString &orderId) {
    QSqlQuery query;
    query.prepare("DELETE FROM product_order_logs WHERE id = :id AND status != 'DONE'");
    query.bindValue(":id", orderId);

    if (query.exec()) {
        return query.numRowsAffected() > 0;
    }
    qDebug() << "Delete Schedule Error:" << query.lastError().text();
    return false;
}
