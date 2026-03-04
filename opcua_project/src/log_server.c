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
static UA_NodeId SPEED_ID;

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


static UA_StatusCode
Move_cb(UA_Server *server,
        const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *methodId, void *methodContext,
        const UA_NodeId *objectId, void *objectContext,
        size_t inputSize, const UA_Variant *input,
        size_t outputSize, UA_Variant *output) {
    (void)sessionId; (void)sessionContext; (void)methodId; (void)methodContext;
    (void)objectId; (void)objectContext; (void)outputSize; (void)output;

    if(inputSize != 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_UInt32 qty = *(UA_UInt32*)input[0].data;
    printf("[LOG] Move called qty=%u\n", qty);
    /* 현재 컨베이어 속도 읽어서 출력 */
    {
        UA_Variant v; UA_Variant_init(&v);
        UA_StatusCode rc = UA_Server_readValue(server, SPEED_ID, &v);
        if(rc == UA_STATUSCODE_GOOD &&
        UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_DOUBLE])) {
            double spd = *(double*)v.data;
            printf("[LOG] ConveyorSpeed = %.1f %%\n", spd);
        } else {
            printf("[LOG] ConveyorSpeed read failed: 0x%08x\n", (unsigned)rc);
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
    (void)sessionId; (void)sessionContext; (void)methodId; (void)methodContext;
    (void)objectId; (void)objectContext; (void)inputSize; (void)input;
    (void)outputSize; (void)output;

    printf("[LOG] StopMove called\n");

    UA_String stopped = UA_STRING("Stopped");
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setScalar(&v, &stopped, &UA_TYPES[UA_TYPES_STRING]);
    (void)UA_Server_writeValue(server, STATUS_ID, v);

    return UA_STATUSCODE_GOOD;
}

int main(void) {
    signal(SIGINT, on_sigint);

    /* --- Security material --- */
    UA_ByteString serverCert = loadFile("/home/pi/opcua_project/certs/log/cert.der");
    UA_ByteString serverKey  = loadFile("/home/pi/opcua_project/certs/log/key.der");
    UA_ByteString trustMes   = loadFile("/home/pi/opcua_project/certs/log/trust_mes.der");

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
    /* Conveyor Speed (0~100 %) */
    {
        SPEED_ID = UA_NODEID_STRING(1, "log/conveyor_speed");

        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "LOG_ConveyorSpeed");

        double speed = 0.0;
        UA_Variant_setScalar(&attr.value, &speed, &UA_TYPES[UA_TYPES_DOUBLE]);

        attr.accessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.userAccessLevel =
            UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

        UA_StatusCode rc2 = UA_Server_addVariableNode(server,
            SPEED_ID,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LOG_ConveyorSpeed"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr, NULL, NULL);

        if(rc2 != UA_STATUSCODE_GOOD)
            printf("[LOG] add speed failed: 0x%08x\n", (unsigned)rc2);
    }
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

    printf("[LOG] Ready: discovery(None-only) + session(encrypted), no Anonymous\n");

    rc = UA_Server_run_startup(server);
    printf("[LOG] startup rc=0x%08x\n", (unsigned)rc);
    fflush(stdout);

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

    printf("[LOG] shutdown complete\n");
    return 0;
}