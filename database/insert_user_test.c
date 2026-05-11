#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mariadb/mysql.h>
#include <uuid/uuid.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define DB_HOST "localhost"
#define DB_USER "admin"      // 본인의 DB 계정으로 변경
#define DB_PASS "pw1234"  // 본인의 DB 암호로 변경
#define DB_NAME "Smart_MES_Core"

// UUID 생성 함수 (v4 기반)
void generate_uuid(char *uuid_str) {
    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, uuid_str);
}

// SHA512 해싱 함수 (Salt 적용)
void sha512_hash(const char *password, const char *salt, char *output) {
    
    // 1. 결과물을 임시로 저장할 바이너리 버퍼
    // EVP_MAX_MD_SIZE는 OpenSSL에서 정의한 최대 해시 크기(64바이트 이상)
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;
    
    // 2. 비밀번호와 솔트를 하나로 합치기 (Salted Password)
    // "password" + "salt" => "passwordsalt" 형태로 만듦
    char salted_pw[256];
    snprintf(salted_pw, sizeof(salted_pw), "%s%s", password, salt);

    // 3. OpenSSL 해싱 컨텍스트 생성
    // 컨텍스트(Context)는 해싱 계산을 위한 '작업장'이라고 생각
    EVP_MD_CTX *context = EVP_MD_CTX_new();
    
    // 4. 해시 알고리즘 초기화 (SHA-512 사용 설정)
    EVP_DigestInit_ex(context, EVP_sha512(), NULL);
    
    // 5. 실제 데이터 주입 (작업장에서 계산 시작)
    // 위에서 합친 '비밀번호+솔트' 문자열을 알고리즘에 통과
    EVP_DigestUpdate(context, salted_pw, strlen(salted_pw));
    
    // 6. 해싱 완료 및 결과 추출
    // 계산된 바이너리 데이터(64바이트)를 hash 배열에 담고, 길이를 lengthOfHash에 저장
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    
    // 7. 작업장(컨텍스트) 메모리 해제
    EVP_MD_CTX_free(context);

    // 8. 바이너리 결과값을 16진수 문자열로 변환
    // 64바이트의 숫자를 128글자의 텍스트로 바꿈
    // %02x는 1바이트를 2글자의 16진수로 출력
    for(unsigned int i = 0; i < lengthOfHash; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return 1;
    }

    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        return 1;
    }

    // 데이터 준비
    char user_id[37], pw_id[37], salt[17], hash_res[129];
    const char *user_name = "test";
    const char *raw_password = "1234";
    
    generate_uuid(user_id); // User ID 생성
    generate_uuid(pw_id);   // User_Password 테이블용 PK 생성

    // 16바이트 랜덤 Salt 생성

    // 우선 char (1바이트) 8개짜리 배열 선언
    unsigned char salt_bin[8];
    // openssl의 RAND_bytes함수를 호출해 배열에 0~255 사이의 무작위 암호값 생성 후 삽입
    RAND_bytes(salt_bin, sizeof(salt_bin));
    // 배열의 값들을 가져와 1바이트(8비트)를 4비트씩 쪼개 0~f 문자를 총 16개 생성, salt 배열에 삽입
    // salt의 배열이 17인 이유는 널문자 때문
    for(int i=0; i<8; i++) sprintf(salt + (i*2), "%02x", salt_bin[i]);

    // 해싱
    sha512_hash(raw_password, salt, hash_res);

    // 트랜잭션 시작
    mysql_autocommit(conn, 0);

    char query[1024];
    
    // 1. User 테이블 삽입
    sprintf(query, "INSERT INTO user (id, user_name, role) VALUES ('%s', '%s', 'SYS_ADMIN')", user_id, user_name);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "User insert failed: %s\n", mysql_error(conn));
        mysql_rollback(conn);
        return 1;
    }

    // 2. User_Password 테이블 삽입
    sprintf(query, "INSERT INTO user_password (id, user_id, password_hash, salt) VALUES ('%s', '%s', '%s', '%s')",
            pw_id, user_id, hash_res, salt);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Password insert failed: %s\n", mysql_error(conn));
        mysql_rollback(conn);
        return 1;
    }

    // 커밋
    if (mysql_commit(conn)) {
        fprintf(stderr, "Commit failed\n");
    } else {
        printf("성공: 유저 '%s'가 생성되었습니다.\nID: %s\n", user_name, user_id);
    }

    mysql_close(conn);
    return 0;
}
