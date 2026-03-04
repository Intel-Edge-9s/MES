// log_server_hardened.c  (Hardened OPC UA Server for open62541 v1.5.2)
// 목표: MFG와 동일 보안 구조
//  1) Discovery는 None-only 허용 (securityPolicyNoneDiscoveryOnly = true)
//  2) 실제 통신은 Sign/SignAndEncrypt + Basic256Sha256로만
//  3) Anonymous 금지(Username/Password만) + endpoint token에서도 Anonymous 제거
//  4) trusted client cert 검증(trust_mes.der) 유지
//
// Build:
//   gcc -Wall -Wextra -O2 log_server_hardened.c -o log_server \
//     $(pkg-config --cflags open62541) \
//     -L/usr/local/lib -lopen62541 \
//     -lpthread -lm -lrt -lssl -lcrypto
//
// Run:
//   ./log_server

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdbool.h>
#include <modbus/modbus.h>


#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/accesscontrol_default.h>

static volatile sig_atomic_t g_running = 1;
static void on_sigint(int sig) { (void)sig; g_running = 0; }

/* ---- persistent users (MUST be static, not stack) ---- */
static UA_UsernamePasswordLogin g_users[1];
static UA_Boolean g_users_inited = false;
static void init_users_once(void) {
    if(g_users_inited) return;
    g_users[0].username = UA_STRING("mes");
    g_users[0].password = UA_STRING("mespw_change_me");
    g_users_inited = true;
}

/* ---------- file loader (robust) ---------- */
static UA_ByteString loadFile(const char *path) {
    UA_ByteString bs = UA_BYTESTRING_NULL;
    FILE *f = fopen(path, "rb");
    if(!f) {
        fprintf(stderr, "[LOG] cannot open file: %s\n", path);
        return bs;
    }
    if(fseek(f, 0, SEEK_END) != 0) { fclose(f); return bs; }
    long sz = ftell(f);
    if(sz <= 0) { fclose(f); return bs; }
    rewind(f);

    bs.data = (UA_Byte*)UA_malloc((size_t)sz);
    if(!bs.data) { fclose(f); return UA_BYTESTRING_NULL; }
    bs.length = (size_t)sz;

    size_t n = fread(bs.data, 1, (size_t)sz, f);
    fclose(f);

    if(n != (size_t)sz) {
        UA_ByteString_clear(&bs);
        return UA_BYTESTRING_NULL;
    }
    return bs;
}
/* ---------- NodeIds ---------- */
static UA_NodeId STATUS_ID;
static UA_NodeId STOCK_ID;
static UA_NodeId TEMP_ID;
static UA_NodeId HUM_ID;
static UA_NodeId SPEED1_ID;
static UA_NodeId SPEED2_ID;
static UA_NodeId SPEED3_ID;




/*----------------- Modbus --------------*/
static UA_NodeId SCM_RUN1_ID;
static UA_NodeId SCM_RUN2_ID;
static UA_NodeId SCM_RUN3_ID;



static uint16_t g_max1 = 100;
static uint16_t g_max2 = 200;
static uint16_t g_max3 = 300;

static uint16_t g_spd1 = 4;
static uint16_t g_spd2 = 6;
static uint16_t g_spd3 = 7;


enum SCM_COIL_ADDR {
    SCM_QX_START = 0,  // %QX0.0
    SCM_QX_STOP  = 1,  // %QX0.1
    SCM_QX_AUTO  = 2,  // %QX0.2

    SCM_QX_RUN1  = 4,  // %QX0.4 (Motor out 등, 너가 쓰겠다는 주소 유지)
    SCM_QX_RUN2  = 12, // %QX1.4
    SCM_QX_RUN3  = 20  // %QX2.4
};

enum SCM_REG_ADDR {
    SCM_MW_COUNT1 = 1025, // %MW1
    SCM_MW_COUNT2 = 1026, // %MW2
    SCM_MW_COUNT3 = 1027, // %MW3

    SCM_MW_MAX1   = 1028, // %MW4
    SCM_MW_MAX2   = 1029, // %MW5
    SCM_MW_MAX3   = 1030, // %MW6

    SCM_MW_SPEED1 = 1031, // %MW7 (Supply Speed1)
    SCM_MW_SPEED2 = 1032, // %MW8
    SCM_MW_SPEED3 = 1033  // %MW9
};


/*------------------------------------------------------*/

/*---------------------- Modbus Handler ----------------------------------------*/



static modbus_t *g_mb = NULL;
static int g_mb_connected = 0;

static int modbus_connect_once(void) {
    if(!g_mb) return -1;
    if(modbus_connect(g_mb) == -1) {
        fprintf(stderr, "[MFG][MODBUS] connect failed: %s\n", modbus_strerror(errno));
        g_mb_connected = 0;
        return -1;
    }
    g_mb_connected = 1;
    printf("[MFG][MODBUS] connected to OpenPLC (127.0.0.1:502)\n");
    return 0;
}

static void modbus_reconnect(void) {
    if(!g_mb) return;
    modbus_close(g_mb);
    usleep(200 * 1000);
    (void)modbus_connect_once();
}

static int modbus_init(void) {
    /* 이미 만들어져 있으면 재사용 */
    if(g_mb)
        return 0;

    g_mb = modbus_new_tcp("127.0.0.1", 502);
    if(!g_mb) {
        fprintf(stderr, "[MFG][MODBUS] modbus_new_tcp failed\n");
        return -1;
    }

    /* OpenPLC Slave ID (일반적으로 1, 네 환경에 맞춰 조정) */
    modbus_set_slave(g_mb, 1);

    /* 응답 대기 시간 (너무 길면 서버가 멈춘 것처럼 보임) */
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 300 * 1000; /* 300ms */
    modbus_set_response_timeout(g_mb, tv.tv_sec, tv.tv_usec);

    return modbus_connect_once();
}

static void modbus_cleanup(void) {
    if(!g_mb) return;
    modbus_close(g_mb);
    modbus_free(g_mb);
    g_mb = NULL;
    g_mb_connected = 0;
}

// reconnect 이후 1회 재시도 헬퍼 추가
static int mb_write_bit_retry(int addr, int value) {
    if(!g_mb) return -1;
    int rc = modbus_write_bit(g_mb, addr, value);
    if(rc == 1) return 0;

    fprintf(stderr, "[MFG][MODBUS] write_bit addr=%d fail: %s (reconnect)\n",
            addr, modbus_strerror(errno));
    modbus_reconnect();

    rc = modbus_write_bit(g_mb, addr, value);
    if(rc == 1) return 0;

    fprintf(stderr, "[MFG][MODBUS] write_bit addr=%d retry fail: %s\n",
            addr, modbus_strerror(errno));
    return -1;
}



static int mb_write_reg_u16_retry(int addr, uint16_t value) {
    if(!g_mb) return -1;

    int rc = modbus_write_register(g_mb, addr, value);
    if(rc == 1) return 0;

    fprintf(stderr, "[MFG][MODBUS] write_reg addr=%d fail: %s (reconnect)\n",
            addr, modbus_strerror(errno));
    modbus_reconnect();

    rc = modbus_write_register(g_mb, addr, value);
    if(rc == 1) return 0;

    fprintf(stderr, "[MFG][MODBUS] write_reg addr=%d retry fail: %s\n",
            addr, modbus_strerror(errno));
    return -1;
}


static int mb_read_bit_retry(int addr, uint8_t *out) {
    if(!g_mb || !out) return -1;
    int rc = modbus_read_bits(g_mb, addr, 1, out);
    if(rc == 1) return 0;

    fprintf(stderr, "[MFG][MODBUS] read_bits addr=%d fail: %s (reconnect)\n",
            addr, modbus_strerror(errno));
    modbus_reconnect();

    rc = modbus_read_bits(g_mb, addr, 1, out);
    if(rc == 1) return 0;

    fprintf(stderr, "[MFG][MODBUS] read_bits addr=%d retry fail: %s\n",
            addr, modbus_strerror(errno));
    return -1;
}

static int mb_read_reg_u16_retry(int addr, uint16_t *out) {
    if(!g_mb || !out) return -1;
    uint16_t tmp = 0;
    int rc = modbus_read_registers(g_mb, addr, 1, &tmp);
    if(rc == 1) { *out = tmp; return 0; }

    fprintf(stderr, "[MFG][MODBUS] read_regs addr=%d fail: %s (reconnect)\n",
            addr, modbus_strerror(errno));
    modbus_reconnect();

    rc = modbus_read_registers(g_mb, addr, 1, &tmp);
    if(rc == 1) { *out = tmp; return 0; }

    fprintf(stderr, "[MFG][MODBUS] read_regs addr=%d retry fail: %s\n",
            addr, modbus_strerror(errno));
    return -1;
}


static void mb_pulse_coil(int addr, int pulse_ms) {
    /* 연결 안돼도 서버가 죽진 않게 */
    if(!g_mb || !g_mb_connected) {
        fprintf(stderr, "[MFG][MODBUS] pulse skipped (not connected) addr=%d\n", addr);
        return;
    }

    /* ON */
    if(mb_write_bit_retry(addr, 1) != 0) return;

    /* 유지 시간 */
    if(pulse_ms > 0)
        usleep((useconds_t)pulse_ms * 1000u);

    /* OFF */
    (void)mb_write_bit_retry(addr, 0);
}

static void mb_set_coil(int addr, int value) {
    if(!g_mb || !g_mb_connected) {
        fprintf(stderr, "[MFG][MODBUS] set skipped (not connected) addr=%d\n", addr);
        return;
    }

    int rc = mb_write_bit_retry(addr, value ? 1 : 0);
    fprintf(stdout, "[MFG][MODBUS] set addr=%d <= %d (rc=%d)\n",
            addr, value ? 1 : 0, rc);

    /* 바로 읽어서 실제 값 확인 (PLC가 즉시 덮어쓰는지도 잡힘) */
    uint8_t b=0;
    int rr = mb_read_bit_retry(addr, &b);
    fprintf(stdout, "[MFG][MODBUS] readback addr=%d => %d (rr=%d)\n",
            addr, (int)b, rr);
}


static void modbus_poll_cb(UA_Server *server, void *data) {
    (void)data;

    uint8_t b = 0;
    uint16_t r = 0;

    /* RUN1~3 (coil) */
    if(mb_read_bit_retry(SCM_QX_RUN1, &b) == 0) {
        printf("[MODBUS] RUN1 = %u\n", b);   // ✅ ADD

        UA_Boolean v = b ? UA_TRUE : UA_FALSE;
        UA_Variant vv; 
        UA_Variant_setScalar(&vv, &v, &UA_TYPES[UA_TYPES_BOOLEAN]);
        (void)UA_Server_writeValue(server, SCM_RUN1_ID, vv);
    }

    if(mb_read_bit_retry(SCM_QX_RUN2, &b) == 0) {
        printf("[MODBUS] RUN2 = %u\n", b);   // ✅ ADD

        UA_Boolean v = b ? UA_TRUE : UA_FALSE;
        UA_Variant vv; 
        UA_Variant_setScalar(&vv, &v, &UA_TYPES[UA_TYPES_BOOLEAN]);
        (void)UA_Server_writeValue(server, SCM_RUN2_ID, vv);
    }

    if(mb_read_bit_retry(SCM_QX_RUN3, &b) == 0) {
        printf("[MODBUS] RUN3 = %u\n", b);   // ✅ ADD

        UA_Boolean v = b ? UA_TRUE : UA_FALSE;
        UA_Variant vv; 
        UA_Variant_setScalar(&vv, &v, &UA_TYPES[UA_TYPES_BOOLEAN]);
        (void)UA_Server_writeValue(server, SCM_RUN3_ID, vv);
    }
}




/* ===================== ADD: Modbus RUN1~3 OPC UA 변수 노드 생성 ===================== */
static void add_bool_ro_node(UA_Server *server, UA_NodeId nid,
                            const char *disp, const char *qname) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char*)disp);

    UA_Boolean init = UA_FALSE;
    UA_Variant_setScalar(&attr.value, &init, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* ✅ 클라이언트는 읽기만 (READ ONLY) */
    attr.accessLevel     = UA_ACCESSLEVELMASK_READ;
    attr.userAccessLevel = UA_ACCESSLEVELMASK_READ;

    UA_StatusCode rc = UA_Server_addVariableNode(server,
        nid,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, (char*)qname),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    if(rc != UA_STATUSCODE_GOOD)
        printf("[LOG] add %s failed: 0x%08x\n", disp, (unsigned)rc);
}





/* ---------- Security helpers (same idea as MFG) ---------- */

static UA_String* findPolicyUri(UA_ServerConfig *cfg, const char *uri) {
    if(!cfg || !cfg->securityPolicies || cfg->securityPoliciesSize == 0)
        return NULL;
    UA_String target = UA_STRING((char*)uri);
    for(size_t i=0; i<cfg->securityPoliciesSize; i++) {
        if(UA_String_equal(&cfg->securityPolicies[i].policyUri, &target))
            return &cfg->securityPolicies[i].policyUri;
    }
    return NULL;
}

static void filter_user_tokens_no_anonymous(UA_EndpointDescription *ep) {
    if(!ep->userIdentityTokens || ep->userIdentityTokensSize == 0)
        return;

    size_t keep = 0;
    for(size_t i=0; i<ep->userIdentityTokensSize; i++) {
        if(ep->userIdentityTokens[i].tokenType != UA_USERTOKENTYPE_ANONYMOUS)
            keep++;
    }

    UA_UserTokenPolicy *newArr = NULL;
    if(keep > 0) {
        newArr = (UA_UserTokenPolicy*)UA_Array_new(keep, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
        size_t w = 0;
        for(size_t i=0; i<ep->userIdentityTokensSize; i++) {
            if(ep->userIdentityTokens[i].tokenType == UA_USERTOKENTYPE_ANONYMOUS)
                continue;
            UA_UserTokenPolicy_copy(&ep->userIdentityTokens[i], &newArr[w++]);
        }
    }

    UA_Array_delete(ep->userIdentityTokens, ep->userIdentityTokensSize,
                    &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

    ep->userIdentityTokens = newArr;
    ep->userIdentityTokensSize = keep;
}

/* ✅ endpoint 배열 재구성 X, 토큰만 정리 (내부와 충돌 최소) */
static void harden_endpoints_tokens_only(UA_ServerConfig *cfg) {
    if(!cfg || !cfg->endpoints || cfg->endpointsSize == 0)
        return;
    for(size_t i=0; i<cfg->endpointsSize; i++)
        filter_user_tokens_no_anonymous(&cfg->endpoints[i]);
}

/* ✅ ActivateSession 단계에서 Anonymous 2중 차단 */
static UA_StatusCode
activateSession_cb(UA_Server *server,
                   UA_AccessControl *ac,
                   const UA_EndpointDescription *endpointDescription,
                   const UA_ByteString *secureChannelRemoteCertificate,
                   const UA_NodeId *sessionId,
                   const UA_ExtensionObject *userIdentityToken,
                   void **sessionContext) {
    (void)server; (void)ac; (void)endpointDescription;
    (void)secureChannelRemoteCertificate; (void)sessionId; (void)sessionContext;

    if(userIdentityToken &&
       userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

/* ✅ “정확히 mes / mespw_change_me”만 허용 (MFG strict와 동일) */
static UA_StatusCode
activateSession_strict_cb(UA_Server *server,
                          UA_AccessControl *ac,
                          const UA_EndpointDescription *endpointDescription,
                          const UA_ByteString *secureChannelRemoteCertificate,
                          const UA_NodeId *sessionId,
                          const UA_ExtensionObject *userIdentityToken,
                          void **sessionContext) {
    (void)server; (void)ac; (void)endpointDescription;
    (void)secureChannelRemoteCertificate; (void)sessionId; (void)sessionContext;

    if(userIdentityToken &&
       userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    if(!userIdentityToken ||
       userIdentityToken->content.decoded.type != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    UA_UserNameIdentityToken *tok =
        (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;

    UA_String expectUser = UA_STRING("mes");
    if(!UA_String_equal(&tok->userName, &expectUser))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    const char *expectPw = "mespw_change_me";
    size_t expectLen = strlen(expectPw);

    if(tok->password.length != expectLen ||
       memcmp(tok->password.data, expectPw, expectLen) != 0) {
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    return UA_STATUSCODE_GOOD;
}




static void configure_accesscontrol_no_anonymous(UA_ServerConfig *cfg) {
    init_users_once();

    UA_String *pwPolicyUri = findPolicyUri(cfg,
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    if(cfg->accessControl.clear)
        cfg->accessControl.clear(&cfg->accessControl);

    /* allowAnonymous=false (username/password만) */
    UA_AccessControl_default(cfg,
        false,        /* allowAnonymous */
        pwPolicyUri,  /* policyUri for encrypting credentials (can be NULL) */
        1, g_users);

    /* 1차 차단 */
    cfg->accessControl.activateSession = activateSession_cb;
}

/* ---------- DHT11(IIO) helper ---------- */
static int find_dht_iio_device(char *outPath, size_t outLen) {
    const char *base = "/sys/bus/iio/devices";
    DIR *d = opendir(base);
    if(!d) return -1;

    struct dirent *ent;
    while((ent = readdir(d))) {
        if(strncmp(ent->d_name, "iio:device", 10) != 0) continue;

        char namePath[512];
        int n = snprintf(namePath, sizeof(namePath), "%s/%s/name", base, ent->d_name);
        if(n < 0 || (size_t)n >= sizeof(namePath)) continue;

        FILE *f = fopen(namePath, "r");
        if(!f) continue;

        char name[128] = {0};
        if(fgets(name, sizeof(name), f)) {
            name[strcspn(name, "\r\n")] = 0;
            if(strstr(name, "dht") || strstr(name, "DHT")) {
                int m = snprintf(outPath, outLen, "%s/%s", base, ent->d_name);
                fclose(f);
                closedir(d);
                if(m < 0 || (size_t)m >= outLen) return -1;
                return 0;
            }
        }
        fclose(f);
    }
    closedir(d);
    return -1;
}

static int read_long_file(const char *path, long *out) {
    FILE *f = fopen(path, "r");
    if(!f) return -1;
    char buf[64];
    if(!fgets(buf, sizeof(buf), f)) { fclose(f); return -1; }
    fclose(f);
    errno = 0;
    long v = strtol(buf, NULL, 10);
    if(errno) return -1;
    *out = v;
    return 0;
}

static int read_dht11_iio(double *tempC, double *humPct) {
    char dev[512];
    if(find_dht_iio_device(dev, sizeof(dev)) != 0) return -1;

    char tPath[600], hPath[600];
    int nt = snprintf(tPath, sizeof(tPath), "%s/in_temp_input", dev);
    int nh = snprintf(hPath, sizeof(hPath), "%s/in_humidityrelative_input", dev);
    if(nt < 0 || (size_t)nt >= sizeof(tPath)) return -1;
    if(nh < 0 || (size_t)nh >= sizeof(hPath)) return -1;

    long tRaw=0, hRaw=0;
    if(read_long_file(tPath, &tRaw) != 0) return -1;
    if(read_long_file(hPath, &hRaw) != 0) return -1;

    *tempC = (double)tRaw / 1000.0;
    *humPct = (double)hRaw / 1000.0;
    return 0;
}

static void dht_update_cb(UA_Server *server, void *data) {
    (void)data;
    double t=0.0, h=0.0;
    if(read_dht11_iio(&t, &h) == 0) {
        UA_Variant vT, vH;
        UA_Variant_setScalar(&vT, &t, &UA_TYPES[UA_TYPES_DOUBLE]);
        UA_Variant_setScalar(&vH, &h, &UA_TYPES[UA_TYPES_DOUBLE]);
        (void)UA_Server_writeValue(server, TEMP_ID, vT);
        (void)UA_Server_writeValue(server, HUM_ID, vH);
    }
}




/* -----------------------Modbus Helper ------------------------------------ */
static UA_UInt16 read_u16_or_default(UA_Server *server, UA_NodeId id, UA_UInt16 defval) {
    UA_Variant v; UA_Variant_init(&v);
    UA_StatusCode rc = UA_Server_readValue(server, id, &v);
    if(rc == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT16])) {
        UA_UInt16 out = *(UA_UInt16*)v.data;
        UA_Variant_clear(&v);
        return out;
    }
    UA_Variant_clear(&v);
    return defval;
}

/* ------------------------------------------------------------------------- */

static UA_StatusCode
Move_cb(UA_Server *server,
        const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *methodId, void *methodContext,
        const UA_NodeId *objectId, void *objectContext,
        size_t inputSize, const UA_Variant *input,
        size_t outputSize, UA_Variant *output) {
    (void)server; (void)sessionId; (void)sessionContext;
    (void)methodId; (void)methodContext; (void)objectId; (void)objectContext;
    (void)outputSize; (void)output;


    if(inputSize != 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_UInt32 qty = *(UA_UInt32*)input[0].data;
    printf("[LOG] Move called qty=%u\n", (unsigned)qty);


    /*-----------------------------------------ModBus----------------------------------------*/

    /* ✅ ADD: Move_cb 실행될 때 PLC(Modbus) 제어 값 세팅 (필요한 것만 추가) */
    {
        uint16_t max1 = g_max1;
        uint16_t max2 = g_max2;
        uint16_t max3 = g_max3;
        uint16_t spd1 = g_spd1;
        uint16_t spd2 = g_spd2;
        uint16_t spd3 = g_spd3;

        /* MAX1~3 -> MW4~MW6 */
        (void)mb_write_reg_u16_retry(SCM_MW_MAX1,   max1);  
        (void)mb_write_reg_u16_retry(SCM_MW_MAX2,   max2);  
        (void)mb_write_reg_u16_retry(SCM_MW_MAX3,   max3);  

        /* SPEED1~3 -> MW7~MW9 */
        (void)mb_write_reg_u16_retry(SCM_MW_SPEED1, spd1);  
        (void)mb_write_reg_u16_retry(SCM_MW_SPEED2, spd2);  
        (void)mb_write_reg_u16_retry(SCM_MW_SPEED3, spd3);  

        /* START/AUTO 유지, STOP 해제 */
        mb_set_coil(SCM_QX_STOP,  0);   
        mb_set_coil(SCM_QX_START, 1);   
        mb_set_coil(SCM_QX_AUTO,  1);   
    }


    /* 현재 컨베이어 속도 읽어서 출력 */
    {
        UA_Variant v; UA_Variant_init(&v);
        UA_StatusCode rc = UA_Server_readValue(server, SPEED1_ID, &v);
        if(rc == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_DOUBLE])) {
            double spd = *(double*)v.data;
            printf("[LOG] Conveyor1Speed = %.1f %%\n", spd);
        } else {
            printf("[LOG] Conveyor1Speed read failed: 0x%08x\n", (unsigned)rc);
        }
        UA_Variant_clear(&v);
    }

    {
        UA_Variant v; UA_Variant_init(&v);
        UA_StatusCode rc = UA_Server_readValue(server, SPEED2_ID, &v);
        if(rc == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_DOUBLE])) {
            double spd = *(double*)v.data;
            printf("[LOG] Conveyor2Speed = %.1f %%\n", spd);
        } else {
            printf("[LOG] Conveyor2Speed read failed: 0x%08x\n", (unsigned)rc);
        }
        UA_Variant_clear(&v);
    }

    {
        UA_Variant v; UA_Variant_init(&v);
        UA_StatusCode rc = UA_Server_readValue(server, SPEED3_ID, &v);
        if(rc == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_DOUBLE])) {
            double spd = *(double*)v.data;
            printf("[LOG] Conveyor3Speed = %.1f %%\n", spd);
        } else {
            printf("[LOG] Conveyor3Speed read failed: 0x%08x\n", (unsigned)rc);
        }
        UA_Variant_clear(&v);
    }
    UA_Variant cur;
    UA_Variant_init(&cur);
    UA_StatusCode rc = UA_Server_readValue(server, STOCK_ID, &cur);
    if(rc != UA_STATUSCODE_GOOD || !UA_Variant_hasScalarType(&cur, &UA_TYPES[UA_TYPES_UINT32])) {
        UA_Variant_clear(&cur);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    UA_UInt32 stock = *(UA_UInt32*)cur.data;
    UA_Variant_clear(&cur);

    if(qty > stock) qty = stock;
    stock -= qty;

    UA_Variant vStock;
    UA_Variant_setScalar(&vStock, &stock, &UA_TYPES[UA_TYPES_UINT32]);
    (void)UA_Server_writeValue(server, STOCK_ID, vStock);

    UA_String moving = UA_STRING("Moving");
    UA_Variant vStatus;
    UA_Variant_setScalar(&vStatus, &moving, &UA_TYPES[UA_TYPES_STRING]);
    (void)UA_Server_writeValue(server, STATUS_ID, vStatus);

    return UA_STATUSCODE_GOOD;
}

/* ---------- Method callback: StopMove ---------- */
static UA_StatusCode
StopMove_cb(UA_Server *server,
            const UA_NodeId *sessionId, void *sessionContext,
            const UA_NodeId *methodId, void *methodContext,
            const UA_NodeId *objectId, void *objectContext,
            size_t inputSize, const UA_Variant *input,
            size_t outputSize, UA_Variant *output) {
    (void)server; (void)sessionId; (void)sessionContext;
    (void)methodId; (void)methodContext; (void)objectId; (void)objectContext;
    (void)inputSize; (void)input; (void)outputSize; (void)output;

    printf("[LOG] StopMove called\n");

    /* STOP 래치 ON, START/AUTO OFF */
    mb_set_coil(SCM_QX_START, 0);
    mb_set_coil(SCM_QX_AUTO,  0);
    mb_set_coil(SCM_QX_STOP,  1);

    UA_String stopped = UA_STRING("Stopped");
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setScalar(&v, &stopped, &UA_TYPES[UA_TYPES_STRING]);
    (void)UA_Server_writeValue(server, STATUS_ID, v);

    return UA_STATUSCODE_GOOD;
}

int main(void) {
    if(modbus_init() != 0) {
        fprintf(stderr, "[LOG][MODBUS] init failed (is OpenPLC Modbus-TCP running on 127.0.0.1:502?)\n");
        // 데모면 계속 진행 가능, 확실히 하려면 return 1;
        // return 1;
    }

    signal(SIGINT, on_sigint);

    /* --- Security material --- */
    UA_ByteString serverCert = loadFile("/home/pi240/opcua_prac/certs/log/cert.der");
    UA_ByteString serverKey  = loadFile("/home/pi240/opcua_prac/certs/log/key.der");
    UA_ByteString trustMes   = loadFile("/home/pi240/opcua_prac/certs/log/trust_mes.der");

    if(serverCert.length == 0 || serverKey.length == 0 || trustMes.length == 0) {
        fprintf(stderr, "[LOG] cert/key/trust load failed (check file paths)\n");
        UA_ByteString_clear(&serverCert);
        UA_ByteString_clear(&serverKey);
        UA_ByteString_clear(&trustMes);
        return 1;
    }

    const UA_ByteString trustList[1] = { trustMes };

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *cfg = UA_Server_getConfig(server);

    UA_StatusCode rc = UA_ServerConfig_setDefaultWithSecurityPolicies(
        cfg,
        4841,
        &serverCert,
        &serverKey,
        trustList, 1,
        NULL, 0,
        NULL, 0
    );


    if(rc != UA_STATUSCODE_GOOD) {
        printf("[LOG] Security config failed: 0x%08x\n", (unsigned)rc);
        UA_Server_delete(server);
        UA_ByteString_clear(&serverCert);
        UA_ByteString_clear(&serverKey);
        UA_ByteString_clear(&trustMes);
        return 1;
    }


    /* ✅ MFG와 동일: Discovery는 None-only */
    cfg->securityPolicyNoneDiscoveryOnly = true;
    /* ✅ None에서 password 평문 허용 금지 */
    cfg->allowNonePolicyPassword = false;

    /* ✅ Anonymous 금지 + endpoint token에서도 Anonymous 제거 */
    configure_accesscontrol_no_anonymous(cfg);
    harden_endpoints_tokens_only(cfg);

    /* ✅ 최종: strict username/password 검사 (MFG와 동일) */
    cfg->accessControl.activateSession = activateSession_strict_cb;

    /* --- Nodes --- */
    STATUS_ID = UA_NODEID_STRING(1, "log/status");
    STOCK_ID  = UA_NODEID_STRING(1, "log/stock_raw");
    TEMP_ID = UA_NODEID_STRING(1, "log/temp");
    HUM_ID  = UA_NODEID_STRING(1, "log/hum");



    /*------------------------Modbus--------------------------*/
    SCM_RUN1_ID   = UA_NODEID_STRING(1, "log/run1");
    SCM_RUN2_ID   = UA_NODEID_STRING(1, "log/run2");
    SCM_RUN3_ID   = UA_NODEID_STRING(1, "log/run3");


    /* main() 안에서, SCM_RUNx_ID 설정한 뒤에 호출 */
    add_bool_ro_node(server, SCM_RUN1_ID, "SCM_RUN1", "SCM_RUN1");
    add_bool_ro_node(server, SCM_RUN2_ID, "SCM_RUN2", "SCM_RUN2");
    add_bool_ro_node(server, SCM_RUN3_ID, "SCM_RUN3", "SCM_RUN3");


    /* Status */
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_Status");
        UA_String init = UA_STRING("Idle");
        UA_Variant_setScalar(&attr.value, &init, &UA_TYPES[UA_TYPES_STRING]);
        (void)UA_Server_addVariableNode(server,
            STATUS_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_Status"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);
    }

    /* Stock */
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Stock_Raw");
        UA_UInt32 stock = 100;
        UA_Variant_setScalar(&attr.value, &stock, &UA_TYPES[UA_TYPES_UINT32]);
        (void)UA_Server_addVariableNode(server,
            STOCK_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Stock_Raw"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);
    }
    /* Temp node */
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_Temp");
        double t = 20.0;
        UA_Variant_setScalar(&attr.value, &t, &UA_TYPES[UA_TYPES_DOUBLE]);

        UA_StatusCode rc2 = UA_Server_addVariableNode(server,
            TEMP_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_Temp"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add temp failed: 0x%08x\n", (unsigned)rc2);
    }

    /* Hum node */
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_Humidity");
        double h = 50.0;
        UA_Variant_setScalar(&attr.value, &h, &UA_TYPES[UA_TYPES_DOUBLE]);

        UA_StatusCode rc2 = UA_Server_addVariableNode(server,
            HUM_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_Humidity"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add hum failed: 0x%08x\n", (unsigned)rc2);
    }

    /* Method: Move(qty:UInt32) */
    {
        UA_Argument inArg;
        UA_Argument_init(&inArg);
        inArg.name = UA_STRING("qty");
        inArg.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
        inArg.valueRank = -1;

        UA_MethodAttributes ma = UA_MethodAttributes_default;
        ma.displayName = UA_LOCALIZEDTEXT("en-US", "Move");
        ma.executable = true;
        ma.userExecutable = true;

        (void)UA_Server_addMethodNode(server,
            UA_NODEID_STRING(1, "log/Move"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, "Move"),
            ma,
            &Move_cb,
            1, &inArg,
            0, NULL,
            NULL, NULL);
    }
    /* Conveyor(1~3) Speed (0~100 %) */
    {
        SPEED1_ID = UA_NODEID_STRING(1, "log/conveyor_speed1");

        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_ConveyorSpeed1");

        double speed = 0.0;
        UA_Variant_setScalar(&attr.value, &speed, &UA_TYPES[UA_TYPES_DOUBLE]);

        attr.accessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.userAccessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

        UA_StatusCode rc2 = UA_Server_addVariableNode(server,
            SPEED1_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_ConveyorSpeed1"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add speed1 failed: 0x%08x\n", (unsigned)rc2);
    }
    {
        SPEED2_ID = UA_NODEID_STRING(1, "log/conveyor_speed2");

        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_ConveyorSpeed2");

        double speed = 0.0;
        UA_Variant_setScalar(&attr.value, &speed, &UA_TYPES[UA_TYPES_DOUBLE]);

        attr.accessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.userAccessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

        UA_StatusCode rc2 = UA_Server_addVariableNode(server,
            SPEED2_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_ConveyorSpeed2"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add speed2 failed: 0x%08x\n", (unsigned)rc2);
    }
    {
        SPEED3_ID = UA_NODEID_STRING(1, "log/conveyor_speed3");

        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_ConveyorSpeed3");

        double speed = 0.0;
        UA_Variant_setScalar(&attr.value, &speed, &UA_TYPES[UA_TYPES_DOUBLE]);

        attr.accessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.userAccessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

        UA_StatusCode rc2 = UA_Server_addVariableNode(server,
            SPEED3_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_ConveyorSpeed3"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add speed3 failed: 0x%08x\n", (unsigned)rc2);
    }
    /*=====================================================================*/
    {
        UA_MethodAttributes ma = UA_MethodAttributes_default;
        ma.displayName = UA_LOCALIZEDTEXT("en-US", "StopMove");
        ma.executable = true;
        ma.userExecutable = true;

        UA_StatusCode rc2 = UA_Server_addMethodNode(server,
            UA_NODEID_STRING(1, "log/StopMove"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, "StopMove"),
            ma,
            &StopMove_cb,
            0, NULL,
            0, NULL,
            NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add StopMove failed: 0x%08x\n", (unsigned)rc2);
    }

    /*----------------------------------  ModBus ------------------------------------------*/
    /* ---- Modbus RUN1~3 nodes (Boolean) ---- */
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "SCM_RUN1");
        UA_Boolean init = UA_FALSE;
        UA_Variant_setScalar(&attr.value, &init, &UA_TYPES[UA_TYPES_BOOLEAN]);
        (void)UA_Server_addVariableNode(server,
            SCM_RUN1_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "SCM_RUN1"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);
    }
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "SCM_RUN2");
        UA_Boolean init = UA_FALSE;
        UA_Variant_setScalar(&attr.value, &init, &UA_TYPES[UA_TYPES_BOOLEAN]);
        (void)UA_Server_addVariableNode(server,
            SCM_RUN2_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "SCM_RUN2"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);
    }
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "SCM_RUN3");
        UA_Boolean init = UA_FALSE;
        UA_Variant_setScalar(&attr.value, &init, &UA_TYPES[UA_TYPES_BOOLEAN]);
        (void)UA_Server_addVariableNode(server,
            SCM_RUN3_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "SCM_RUN3"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);
    }





    printf("[LOG] Ready: discovery(None-only) + session(encrypted), no Anonymous\n");

    rc = UA_Server_run_startup(server);
    printf("[LOG] startup rc=0x%08x\n", (unsigned)rc);
    UA_UInt64 cbId = 0;
    UA_StatusCode rc3 = UA_Server_addRepeatedCallback(server, dht_update_cb, NULL, 1200 /*ms*/, &cbId);
    printf("[LOG] addRepeatedCallback(DHT) rc=0x%08x\n", (unsigned)rc3);
    fflush(stdout);

    /*----------------------------ModBus----------------------------------------*/
    UA_UInt64 mbCbId = 0;
    UA_StatusCode rcMb = UA_Server_addRepeatedCallback(server, modbus_poll_cb, NULL, 200 /*ms*/, &mbCbId);
    printf("[MFG] modbus poll cb rc=0x%08x\n", (unsigned)rcMb);
    /*----------------------------------------------------------------------------------------*/

    if(rc != UA_STATUSCODE_GOOD) {
        printf("[LOG] STARTUP FAILED -> NO LISTEN\n");
        UA_Server_delete(server);
        UA_ByteString_clear(&serverCert);
        UA_ByteString_clear(&serverKey);
        UA_ByteString_clear(&trustMes);
        return 1;
    }

    printf("[LOG] running... Ctrl+C to stop\n");
    while(g_running) {
        UA_Server_run_iterate(server, true);
    }

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    UA_ByteString_clear(&serverCert);
    UA_ByteString_clear(&serverKey);
    UA_ByteString_clear(&trustMes);
    modbus_cleanup();

    printf("[LOG] shutdown complete\n");
    return 0;
}