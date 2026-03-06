#include "partner_manage_service.h"
#include <QSqlQuery>

QList<CompanyInfo> PartnerService::getSuppliers() {
    QList<CompanyInfo> list;
    QSqlQuery query("SELECT id, company_name, company_address, company_number FROM supp_company");
    
    while (query.next()) {
        CompanyInfo info;
        info.id = query.value("id").toString();
        info.name = query.value("company_name").toString();
        info.address = query.value("company_address").toString();
        info.contact = query.value("company_number").toString();
        list.append(info);
    }
    return list;
}

QList<CompanyInfo> PartnerService::getCustomers() {
    QList<CompanyInfo> list;
    QSqlQuery query("SELECT id, company_name, company_address, company_number FROM cust_company");
    
    while (query.next()) {
        CompanyInfo info;
        info.id = query.value("id").toString();
        info.name = query.value("company_name").toString();
        info.address = query.value("company_address").toString();
        info.contact = query.value("company_number").toString();
        list.append(info);
    }
    return list;
}