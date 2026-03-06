#pragma once
#include <QString>

class UserSession {
public:
    static UserSession& instance() {
        static UserSession _instance;
        return _instance;
    }

    void login(const QString& uuid, const QString& name) {
        m_userId = uuid;
        m_userName = name;
        m_isLoggedIn = true;
    }

    void logout() {
        m_userId = "";
        m_userName = "";
        m_isLoggedIn = false;
    }

    // Getter 함수들
    QString userId() const { return m_userId; }
    QString userName() const { return m_userName; }
    bool isLoggedIn() const { return m_isLoggedIn; }

private:
    UserSession() : m_isLoggedIn(false) {}

    QString m_userId;
    QString m_userName;
    bool m_isLoggedIn;
};
