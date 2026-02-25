#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

/* 서버 실행 제어용 플래그 */
static UA_Boolean running = true;

/* Ctrl+C 누르면 서버 종료 */
static void stopHandler(int sig) {
    running = false;
}

int main(void) {
    /* 종료 시그널 등록 */
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    /* OPC UA 서버 생성 + 기본 설정 적용 */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /************ 1️⃣ ObjectType 정의 ************/

    /* MamalType 생성 (BaseObjectType 하위 타입) */
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MamalType");

    UA_Server_addObjectTypeNode(server,
                                UA_NODEID_NUMERIC(1, 10000),
                                UA_NS0ID(BASEOBJECTTYPE),
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "MamalType"),
                                otAttr, NULL, NULL);

    /* MamalType에 Age 변수 추가 */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_UInt32 ageVar = 0;
    UA_Variant_setScalar(&vAttr.value, &ageVar, &UA_TYPES[UA_TYPES_UINT32]);

    UA_Server_addVariableNode(server,
                              UA_NODEID_NUMERIC(1, 10001),
                              UA_NODEID_NUMERIC(1, 10000),
                              UA_NS0ID(HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Age"),
                              UA_NS0ID(BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    /* Age를 Mandatory로 설정 */
    UA_Server_addReference(server,
                           UA_NODEID_NUMERIC(1, 10001),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                           true);

    /************ 2️⃣ DogType 정의 (상속) ************/

    otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DogType");

    /* MamalType을 상속받는 DogType 생성 */
    UA_Server_addObjectTypeNode(server,
                                UA_NODEID_NUMERIC(1, 10002),
                                UA_NODEID_NUMERIC(1, 10000),
                                UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DogType"),
                                otAttr, NULL, NULL);

    /* DogType에 Name 변수 추가 */
    vAttr = UA_VariableAttributes_default;
    UA_String defaultName = UA_STRING("unnamed dog");
    UA_Variant_setScalar(&vAttr.value, &defaultName, &UA_TYPES[UA_TYPES_STRING]);

    UA_Server_addVariableNode(server,
                              UA_NODEID_NUMERIC(1, 10003),
                              UA_NODEID_NUMERIC(1, 10002),
                              UA_NS0ID(HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Name"),
                              UA_NS0ID(BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    /************ 3️⃣ 실제 객체 생성 ************/

    /* DogType 기반 Bello 객체 생성 */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Bello");

    UA_Server_addObjectNode(server,
                            UA_NODEID_NUMERIC(1, 0),
                            UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Bello"),
                            UA_NODEID_NUMERIC(1, 10002),
                            oAttr, NULL, NULL);

    /* DogType 기반 Doge 객체 생성 (수동 방식) */
    UA_NodeId dogeNodeId;
    oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Doge");

    UA_Server_addNode_begin(server,
                            UA_NODECLASS_OBJECT,
                            UA_NODEID_NUMERIC(1, 0),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Doge"),
                            UA_NODEID_NUMERIC(1, 10002),
                            &oAttr,
                            &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                            NULL,
                            &dogeNodeId);

    /* Doge에 Name 속성 직접 추가 */
    vAttr = UA_VariableAttributes_default;
    UA_String dogeName = UA_STRING("Doge");
    UA_Variant_setScalar(&vAttr.value, &dogeName, &UA_TYPES[UA_TYPES_STRING]);

    UA_Server_addVariableNode(server,
                              UA_NODEID_NUMERIC(1, 0),
                              dogeNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Name"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    UA_Server_addNode_finish(server, dogeNodeId);

    /************ 4️⃣ 서버 실행 ************/

    UA_StatusCode retval = UA_Server_run(server, &running);

    /* 서버 종료 및 메모리 정리 */
    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
