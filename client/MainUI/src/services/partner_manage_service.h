#include "../models/company_model.h"
#include <QList>

class PartnerService {
public:
    // API 호출처럼 데이터를 리스트로 반환하는 함수들
    static QList<CompanyInfo> getSuppliers();
    static QList<CompanyInfo> getCustomers();
};