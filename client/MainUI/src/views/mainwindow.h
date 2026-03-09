#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QList>
#include "base/page_types.h"
#include "opcua_service.h"
#include "auth_service.h"
#include "../models/manufacture_model.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    OpcUaService* opcUaService() const { return ua; }

private:
    Ui::MainWindow *ui;

    OpcUaService* ua = nullptr;
    AuthService m_authService;
    bool uaStarted = false;
    bool m_logStockSeeded = false;

    QString m_activeProdOrderId;
    QString m_activeProductId;
    QString m_activeRecipe;
    QList<RecipeItem> m_activeRecipeItems;

    int m_lastProdCount = 0;
    int m_lastDefectCount = 0;
    int m_lastAttemptCount = 0;

    bool m_materialStopRequested = false;

    QMap<QString, int> m_itemStocks;
    QMap<QString, bool> m_itemLowFlags;

    void startOpcUaOnce();
    void setupNavigation();
    void moveToPage(PageType type);
    void requestMaterialStop(const QString &reason);
    void clearActiveProduction();
};

#endif
