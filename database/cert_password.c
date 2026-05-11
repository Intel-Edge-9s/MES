// DB 연동 확인용 테스트 코드 입니다. 실제 프로젝트 로직에 사용되지 않습니다.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mariadb/mysql.h>  // 헤더 경로 수정됨
#include <openssl/evp.h>

#define DB_HOST "localhost"
#define DB_USER "admin"
#define DB_PASS "pw1234"
#define DB_NAME "Smart_MES_Core"

// SHA512 해싱 함수 (기존 코드와 동일)
void sha512_hash(const char *password, const char *salt, char *output) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;
    char salted_pw[256];
    snprintf(salted_pw, sizeof(salted_pw), "%s%s", password, salt);

    EVP_MD_CTX *context = EVP_MD_CTX_new();
    EVP_DigestInit_ex(context, EVP_sha512(), NULL);
    EVP_DigestUpdate(context, salted_pw, strlen(salted_pw));
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);

    for(unsigned int i = 0; i < lengthOfHash; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0)) {
        fprintf(stderr, "DB Connection Error: %s\n", mysql_error(conn));
        return 1;
    }

    // 1. 로그인 시도 정보 (사용자 입력 가정)
    const char *input_user_name = "test_user";
    const char *input_password = "testpw1234";

    char query[1024];
    // JOIN을 사용하여 user_name으로 password_hash와 salt를 한 번에 가져옵니다.
    sprintf(query, 
        "SELECT p.password_hash, p.salt "
        "FROM user u JOIN user_password p ON u.id = p.user_id "
        "WHERE u.user_name = '%s'", input_user_name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL) {
        printf("사용자를 찾을 수 없음\n");
    } else {
        const char *db_hash = row[0]; // DB에 저장된 해시
        const char *db_salt = row[1]; // DB에 저장된 솔트
        char challenge_hash[129];      // 입력된 비번으로 계산할 해시

        // 2. 입력받은 비번 + DB에서 가져온 Salt로 해싱 시도
        sha512_hash(input_password, db_salt, challenge_hash);

        // 3. 결과 비교
        if (strcmp(db_hash, challenge_hash) == 0) {
            printf("비밀번호가 같음. 성공 %s님.\n", input_user_name);
        } else {
            printf("비밀번호가 일치하지 않음\n");
        }
    }

    mysql_free_result(res);
    mysql_close(conn);
    return 0;
}
