#pragma once  // 중복 포함 방지
#include <QString> // 이게 빠져서 'QString' does not name a type 에러가 난 겁니다!

struct CompanyInfo {
    QString id;
    QString name;
    QString address;
    QString contact;
};