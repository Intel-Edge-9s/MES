// mes_client_final.c  (open62541 v1.5.2)
// - MFG/LOG 각각 시작/정지 플래그 분리 (UI 버튼 연결용)
// - PKI( cert/key/trust ) 전역 로드로 수명 안정화
// - 자동 StartOrder/Move 호출 제거 (버튼 트리거로만 실행)
// - Ctrl+C로 정상 종료
//
// Build:
//   gcc -Wall -Wextra -O2 mes_client_final.c -o mes_client \
//     $(pkg-config --cflags open62541) $(pkg-config --libs open62541) \
//     -lssl -lcrypto -lpthread -lm -lrt
//
// Run:
//   ./mes_client

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <open62541/types.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static int stdin_has_key(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0;
}

static int read_key_nonblock(void) {
    if(!stdin_has_key()) return -1;
    char c;
    if(read(STDIN_FILENO, &c, 1) == 1) return (int)c;
    return -1;
}

/* ----------------- UI flags (connect later) ----------------- */
static atomic_int g_mfg_start = 0;
static atomic_int g_mfg_stop  = 0;
static atomic_int g_log_start = 0;
static atomic_int g_log_stop  = 0;

void ui_mfg_start_pressed(void){ atomic_store(&g_mfg_start, 1); }
void ui_mfg_stop_pressed(void) { atomic_store(&g_mfg_stop,  1); }
void ui_log_start_pressed(void){ atomic_store(&g_log_start, 1); }
void ui_log_stop_pressed(void) { atomic_store(&g_log_stop,  1); }

/* ----------------- graceful shutdown ----------------- */
static volatile sig_atomic_t g_running = 1;
static void on_sigint(int sig){ (void)sig; g_running = 0; }

/* ----------------- robust file loader ----------------- */
static UA_ByteString loadFile(const char *path) {
    UA_ByteString bs = UA_BYTESTRING_NULL;
    FILE *f = fopen(path, "rb");
    if(!f) {
        fprintf(stderr, "[MES] cannot open file: %s\n", path);
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

/* ----------------- PKI lifetime: global ----------------- */
static UA_ByteString g_clientCert = {0, NULL};
static UA_ByteString g_clientKey  = {0, NULL};
static UA_ByteString g_trustMfg   = {0, NULL};
static UA_ByteString g_trustLog   = {0, NULL};
static UA_Boolean    g_pki_loaded = UA_FALSE;

static UA_Boolean load_pki_once(void) {
    if(g_pki_loaded) return true;

    g_clientCert = loadFile("/home/pi/opcua_project/certs/mes/cert.der");
    g_clientKey  = loadFile("/home/pi/opcua_project/certs/mes/key.der");
    g_trustMfg   = loadFile("/home/pi/opcua_project/certs/mes/trust_mfg.der");
    g_trustLog   = loadFile("/home/pi/opcua_project/certs/mes/trust_log.der");

    g_pki_loaded = (g_clientCert.length > 0 &&
                    g_clientKey.length  > 0 &&
                    g_trustMfg.length   > 0 &&
                    g_trustLog.length   > 0);

    if(!g_pki_loaded) {
        fprintf(stderr, "[MES] PKI load failed (check paths)\n");
        UA_ByteString_clear(&g_clientCert);
        UA_ByteString_clear(&g_clientKey);
        UA_ByteString_clear(&g_trustMfg);
        UA_ByteString_clear(&g_trustLog);
    }

    return g_pki_loaded;
}

static void clear_pki(void) {
    UA_ByteString_clear(&g_clientCert);
    UA_ByteString_clear(&g_clientKey);
    UA_ByteString_clear(&g_trustMfg);
    UA_ByteString_clear(&g_trustLog);
    g_pki_loaded = false;
}

/* ----------------- subscription callback ----------------- */
static void onDataChange(UA_Client *client, UA_UInt32 subId, void *subCtx,
                         UA_UInt32 monId, void *monCtx, UA_DataValue *value) {
    (void)client; (void)subId; (void)subCtx; (void)monId;

    const char *tag = (const char*)monCtx;
    if(!value || !value->hasValue) return;

    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DOUBLE])) {
        UA_Double d = *(UA_Double*)value->value.data;
        printf("[MES][SUB] %s = %.2f\n", tag, d);
    }
}

/* ----------------- call output cleanup ----------------- */
static void freeCallOutput(size_t outSize, UA_Variant *out) {
    if(!out || outSize == 0) return;
    UA_Array_delete(out, outSize, &UA_TYPES[UA_TYPES_VARIANT]);
}

/* ----------------- secure client factory ----------------- */
static UA_Client* makeSecureClient(void) {
    if(!load_pki_once())
        return NULL;

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    UA_StatusCode rc = UA_ClientConfig_setDefault(cc);
    if(rc != UA_STATUSCODE_GOOD) {
        printf("[MES] Client default config failed: 0x%08x\n", (unsigned)rc);
        UA_Client_delete(client);
        return NULL;
    }

    const UA_ByteString trustList[2] = { g_trustMfg, g_trustLog };

    rc = UA_ClientConfig_setDefaultEncryption(
        cc,
        g_clientCert,
        g_clientKey,
        trustList, 2,
        NULL, 0
    );
    if(rc != UA_STATUSCODE_GOOD) {
        printf("[MES] Client encryption config failed: 0x%08x\n", (unsigned)rc);
        UA_Client_delete(client);
        return NULL;
    }

    /* server 정책과 100% 일치 강제 */
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_String_clear(&cc->securityPolicyUri);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:opcua:mes");

    return client;
}

/* ----------------- command helpers ----------------- */
static UA_StatusCode call_mfg_start(UA_Client *mfg, const char *orderId) {
    UA_Variant in; UA_Variant_init(&in);
    UA_String order = UA_STRING((char*)orderId);
    UA_Variant_setScalar(&in, &order, &UA_TYPES[UA_TYPES_STRING]);

    size_t outSize = 0; UA_Variant *out = NULL;
    UA_StatusCode rc = UA_Client_call(
        mfg,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_STRING(1, "mfg/StartOrder"),
        1, &in,
        &outSize, &out
    );
    freeCallOutput(outSize, out);
    return rc;
}

static UA_StatusCode call_mfg_stop(UA_Client *mfg) {
    size_t outSize = 0; UA_Variant *out = NULL;
    UA_StatusCode rc = UA_Client_call(
        mfg,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_STRING(1, "mfg/StopOrder"), /* 서버에 추가 필요 */
        0, NULL,
        &outSize, &out
    );
    freeCallOutput(outSize, out);
    return rc;
}

static UA_StatusCode call_log_move(UA_Client *log, UA_UInt32 qty) {
    UA_Variant in; UA_Variant_init(&in);
    UA_Variant_setScalar(&in, &qty, &UA_TYPES[UA_TYPES_UINT32]);

    size_t outSize = 0; UA_Variant *out = NULL;
    UA_StatusCode rc = UA_Client_call(
        log,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_STRING(1, "log/Move"),
        1, &in,
        &outSize, &out
    );
    freeCallOutput(outSize, out);
    return rc;
}

static UA_StatusCode call_log_stop(UA_Client *log) {
    size_t outSize = 0; UA_Variant *out = NULL;
    UA_StatusCode rc = UA_Client_call(
        log,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_STRING(1, "log/StopMove"), /* 서버에 추가 필요 */
        0, NULL,
        &outSize, &out
    );
    freeCallOutput(outSize, out);
    return rc;
}
static UA_StatusCode write_speed(UA_Client *c,
                                 const char *nodeStr,
                                 double spd)
{
    if(spd < 0.0) spd = 0.0;
    if(spd > 100.0) spd = 100.0;

    UA_Variant v; UA_Variant_init(&v);
    UA_Variant_setScalar(&v, &spd, &UA_TYPES[UA_TYPES_DOUBLE]);

    return UA_Client_writeValueAttribute(
        c,
        UA_NODEID_STRING(1, (char*)nodeStr),
        &v);
}
/* ----------------- main ----------------- */
int main(void) {
    signal(SIGINT, on_sigint);

    UA_Client *mfg = makeSecureClient();
    UA_Client *log = makeSecureClient();
    if(!mfg || !log) {
        UA_Client_delete(mfg);
        UA_Client_delete(log);
        clear_pki();
        return 1;
    }

    UA_StatusCode rc;

    rc = UA_Client_connectUsername(mfg, "opc.tcp://10.10.16.208:4850", "mes", "mespw_change_me");
    if(rc != UA_STATUSCODE_GOOD) {
        printf("[MES] connect MFG fail 0x%08x\n", (unsigned)rc);
        goto cleanup;
    }

    rc = UA_Client_connectUsername(log, "opc.tcp://localhost:4841", "mes", "mespw_change_me");
    if(rc != UA_STATUSCODE_GOOD) {
        printf("[MES] connect LOG fail 0x%08x\n", (unsigned)rc);
        goto cleanup;
    }

    /* ---- Subscription (MFG temp/hum) ---- */
    UA_CreateSubscriptionRequest req = UA_CreateSubscriptionRequest_default();
    req.requestedPublishingInterval = 1000.0;

    UA_CreateSubscriptionResponse resp =
        UA_Client_Subscriptions_create(mfg, req, NULL, NULL, NULL);

    if(resp.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        printf("[MES] subscription create fail 0x%08x\n",
               (unsigned)resp.responseHeader.serviceResult);
        goto cleanup;
    }

    UA_UInt32 subId = resp.subscriptionId;

    UA_NodeId tempId = UA_NODEID_STRING(1, "mfg/temp");
    UA_NodeId humId  = UA_NODEID_STRING(1, "mfg/hum");

    UA_MonitoredItemCreateRequest monT = UA_MonitoredItemCreateRequest_default(tempId);
    UA_Client_MonitoredItems_createDataChange(
        mfg, subId, UA_TIMESTAMPSTORETURN_BOTH,
        monT, (void*)"Temp", onDataChange, NULL);

    UA_MonitoredItemCreateRequest monH = UA_MonitoredItemCreateRequest_default(humId);
    UA_Client_MonitoredItems_createDataChange(
        mfg, subId, UA_TIMESTAMPSTORETURN_BOTH,
        monH, (void*)"Humidity", onDataChange, NULL);

    printf("[MES] Ready. (UI flags: mfg/log start/stop)\n");
    printf("      Test without UI: call ui_mfg_start_pressed() etc in code.\n");

    while(g_running) {
        UA_Client_run_iterate(mfg, 100);
        UA_Client_run_iterate(log, 0);
        int k = read_key_nonblock();
        if(k == 'q') { ui_mfg_stop_pressed(); ui_log_stop_pressed(); }
        if(k == 'm') { ui_mfg_stop_pressed(); }
        if(k == 'l') { ui_log_stop_pressed(); }
        if(k == '1') { ui_mfg_start_pressed(); }   // 테스트: 제조 시작
        if(k == '2') { ui_log_start_pressed(); }   // 테스트: 물류 시작
        if(atomic_exchange(&g_mfg_start, 0)) {
            write_speed(mfg, "mfg/conveyor_speed", 75.0);
            UA_StatusCode r = call_mfg_start(mfg, "ORD-001");
            printf("[MES] MFG START rc=0x%08x\n", (unsigned)r);
        }

        if(atomic_exchange(&g_log_start, 0)) {
            write_speed(log, "log/conveyor_speed", 60.0);
            UA_StatusCode r = call_log_move(log, 10);
            printf("[MES] LOG START(Move) rc=0x%08x\n", (unsigned)r);
        }

        if(atomic_exchange(&g_mfg_stop, 0)) {
            write_speed(mfg, "mfg/conveyor_speed", 0.0);
            UA_StatusCode r = call_mfg_stop(mfg);
            printf("[MES] MFG STOP rc=0x%08x (needs server StopOrder)\n", (unsigned)r);
        }

        if(atomic_exchange(&g_log_stop, 0)) {
            write_speed(log, "log/conveyor_speed", 0.0); 
            UA_StatusCode r = call_log_stop(log);
            printf("[MES] LOG STOP rc=0x%08x (needs server StopMove)\n", (unsigned)r);
        }
    }

cleanup:
    UA_Client_disconnect(mfg);
    UA_Client_disconnect(log);
    UA_Client_delete(mfg);
    UA_Client_delete(log);
    clear_pki();
    printf("[MES] shutdown\n");
    return 0;
}