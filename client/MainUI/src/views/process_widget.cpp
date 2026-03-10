#include "process_widget.h"
#include "ui_process_widget.h"
#include "../services/opcua_service.h"
#include "../services/manufacture_service.h"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

ProcessWidget::ProcessWidget(QWidget *parent)
    : BasePageWidget(parent)
    , ui(new Ui::ProcessWidget)
{
    ui->setupUi(this);
    ui->title_label->setText("Process Monitor");
    setup_process_tree();
}

ProcessWidget::~ProcessWidget()
{
    delete ui;
}

void ProcessWidget::setOpcUaService(OpcUaService *uaService) {
    m_ua = uaService;
}

void ProcessWidget::setup_process_tree()
{
    ui->process_tree_widget->setColumnCount(2);
    ui->process_tree_widget->setHeaderLabels({"공정명", "제어"});
    ui->process_tree_widget->setColumnWidth(0, 600);
    ui->process_tree_widget->setColumnWidth(1, 200);

    QTreeWidgetItem *logistics = new QTreeWidgetItem(ui->process_tree_widget);
    logistics->setText(0, "[물류]");
    add_process_item(logistics, "컨베이어 벨트 1");
    add_process_item(logistics, "컨베이어 벨트 2");
    add_process_item(logistics, "컨베이어 벨트 3");

    QTreeWidgetItem *manufacturing = new QTreeWidgetItem(ui->process_tree_widget);
    manufacturing->setText(0, "[제조]");
    add_process_item(manufacturing, "제조 컨테이너 1");

    ui->process_tree_widget->expandAll();
}

void ProcessWidget::add_process_item(QTreeWidgetItem *parent_item, const QString &process_name)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent_item);
    item->setText(0, process_name);

    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);


    QPushButton *stop_btn = new QPushButton("정지");
    stop_btn->setFixedWidth(80);
    connect(stop_btn, &QPushButton::clicked, this, [this, process_name]() {
        on_stop_clicked(process_name);
    });

    layout->addWidget(stop_btn);
    layout->addStretch();

    ui->process_tree_widget->setItemWidget(item, 1, container);
}



void ProcessWidget::on_stop_clicked(const QString &process_name)
{
    qDebug() << "[정지 신호]" << process_name;

    if (!m_ua)
        return;

    if (process_name == "제조 컨테이너 1") {
        m_ua->mfgStopOrder();
    }
    else if (process_name == "컨베이어 벨트 1") {
        m_ua->logStopMove(1);
    }
    else if (process_name == "컨베이어 벨트 2") {
        m_ua->logStopMove(2);
    }
    else if (process_name == "컨베이어 벨트 3") {
        m_ua->logStopMove(3);
    }
}

void ProcessWidget::on_Back_btn_clicked()
{
    emit requestPageChange(PageType::Dashboard);
}
