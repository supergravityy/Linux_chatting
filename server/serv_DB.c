// serv_DB.c
#include"serv.h"
extern char clnt_idList[CLNT_MAX][IDSIZE];
extern int clnt_socketList[CLNT_MAX];
extern int clnt_cnt;
extern pthread_mutex_t mtx;

#ifdef DEBUG // 디버그용

void login_ID(int clnt_socket, char packet[])
// txt 파일에서 ID가 있는지 확인
{
    FILE* fp;
    char info[INFOSIZE] = { 0 };
    char clnt_id[IDSIZE] = { 0 };   // 로그인 요청 클라이언트 아이디
    char temp_id[IDSIZE] = { 0 };   // txt 파일에서의 아이디 목록
    int seek_result = 0;

    // 1. 파일열기

    fp = fopen("testDB.txt","r");

    if(fp == NULL)
    {
        printf("cannot open file!\n");
        return;
    }

    // 2. 패킷에서 id 추출
    for(int idx = 3; packet[idx] != '\0'; idx++)
    // 아이디 로그인 프로세스 패킷구성 -> id sungsu
    {
        clnt_id[idx-3] = packet[idx];
    }

    // 3. id 리스트에서 해당 id가 존재하는지 확인
    pthread_mutex_lock(&mtx);
    for(int idx = 0; idx < clnt_cnt; idx++)
    {
        if(!strcmp(clnt_id,clnt_idList[idx]))
	{
	    pthread_mutex_unlock(&mtx);
            goto already_connected;
	}
    }
    pthread_mutex_unlock(&mtx);

    // 4. 이미 접속된 아이디가 아니라면, txt에서 한줄씩 읽고 그 id와 수신받은 id를 검사
    int str_cursor;

    while(fgets(info, sizeof(info), fp) != NULL)
    // 파일을 한줄씩 EOF까지 읽어서 거기서 id만 추출해서 검사
    {
        memset(temp_id, 0, sizeof(temp_id)); // 초기화

        for(str_cursor = 0; info[str_cursor] != ' '; str_cursor++)
        {
            temp_id[str_cursor] = info[str_cursor];
            // 그 한줄에서 id만 추출
        }
        temp_id[str_cursor] = '\0';

        if(!strcmp(temp_id,clnt_id))
        {
            printf("[%d]'s ID found!\n",clnt_socket);
            seek_result = 1;
            break;
        }
    }

already_connected:
    // 4. 해당결과를 클라에게 반환
    write(clnt_socket, &seek_result, sizeof(int));

    fclose(fp);
}

void login_PW(int clnt_socket, char packet[])
/* --- 그냥 클라이언트가 ID와 PW를 전송하게 하자 안그러면 너무 복잡 --- */
/* --- 생각해보니 클라가 ID와 PW에 띄어쓰기를 넣으면 안됨 --- */
{
    FILE* fp;
    char info[INFOSIZE] = { 0 };
    char clnt_account[INFOSIZE] = {0};
    int seek_result = 0;

    // 1. 파일열기

    fp = fopen("testDB.txt","r");

    if(fp == NULL)
    {
        printf("cannot open file!\n");
        return;
    }

    // 2. 패킷에서 id와 pw만 추출

    int idx;
    for( idx = 3; idx < strlen(packet);idx++)
    {
        clnt_account[idx-3] = packet[idx];
    }
    clnt_account[idx] = 0;

    // 3. txt에서 한줄씩 읽고 패킷자체와 그 한줄을 검사

    while(fgets(info, sizeof(info), fp) != NULL)
    // 패킷구성 -> "id" "pw" == "id" "pw" (txt의 한줄)
    {
        info[strcspn(info, "\n")] = 0; // 개행문자 제거

        if(!strcmp(info,clnt_account))
        {
            printf("client[%d] is Allowed!\n", clnt_socket);
            seek_result = 1;
            break;
        }
    }

    // 4. 클라이언트 id만 추출해서, id 리스트에 저장

    char userID[IDSIZE] = {0};
    DB_extractID(info,userID);
    add_idList(clnt_socket, userID);

    // 5. 해당결과를 클라에게 반환

    write(clnt_socket, &seek_result, sizeof(int));

    fclose(fp);
}

void unique_ID(int clnt_socket, char packet[])
{
    FILE* fp;
    char info[INFOSIZE] = { 0 };
    char clnt_id[IDSIZE] = { 0 };   // 로그인 요청 클라이언트 아이디
    char temp_id[IDSIZE] = { 0 };   // txt 파일에서의 아이디 목록
    int seek_result = 1;            // 기본적으로 유니크하다고 가정

    // 1. 파일열기

    fp = fopen("testDB.txt","r");

    if(fp == NULL)
    {
        printf("cannot open file!\n");
        return;
    }

    // 2. 패킷에서 id 추출
    for(int idx = 3; packet[idx] != '\0'; idx++)
    // 아이디 고유확인 프로세스 패킷구성 -> id sungsu
    {
        clnt_id[idx-3] = packet[idx];
    }
    clnt_id[strlen(packet) - 3] = '\0'; // clnt_id를 끝내줄것

    // 3. txt에서 한줄씩 읽고 그 id와 수신받은 id를 검사
    int str_cursor;

    while(fgets(info, sizeof(info), fp) != NULL)
    // 파일을 한줄씩 EOF까지 읽어서 거기서 id만 추출해서 검사
    {
        memset(temp_id, 0, sizeof(temp_id)); // 초기화

        for(str_cursor = 0; info[str_cursor] != ' '; str_cursor++)
        {
            temp_id[str_cursor] = info[str_cursor];
            // 그 한줄에서 id만 추출
        }
        temp_id[str_cursor] = '\0';
        /*디버그용*/
        printf("temp_id : %s\n",temp_id);

        if(!strcmp(temp_id,clnt_id))
        {
            printf("[%d]'s new ID [%s] is NOT Unique!\n",clnt_socket,clnt_id);
            seek_result = 0;
        }

    }

    if(seek_result == 1)
        printf("[%d]'s new ID [%s] is Unique!\n",clnt_socket,clnt_id);

    // 4. 해당결과를 클라에게 반환
    write(clnt_socket, &seek_result, sizeof(int));

    fclose(fp);
}

void save_Info(int clnt_socket, char packet[])
{
    FILE *fp;
    char info[INFOSIZE] = {0};
    int result = 0;

    // 1. 파일열기
    fp = fopen("testDB.txt","a");
    if(fp == NULL)
    {
        printf("cannot open file!\n");
        return;
    }

    // 2. 패킷에서 정보만 추출
    int idx;
    for(idx = 4; packet[idx] != '\0'; idx++)
    // 패킷구조 -> new "ID" "PW"
    {
        info[idx-4] = packet[idx];
    }
    info[idx-4] = '\0';

    // 3. id
    fprintf(fp, "%s\n", info);
    printf("[%d]'s account info has been written at DB!\n",clnt_socket);
    printf("saveInfo : %s\n",info);
    fclose(fp);

    // 4. 클라이언트 id만 추출해서, id 리스트에 저장

    char userID[IDSIZE] = {0};
    DB_extractID(info,userID);
    add_idList(clnt_socket, userID);

    // 5. 작성되었다고 알리기
    write(clnt_socket, &result, sizeof(int));
}

#else // 릴리스용

void login_ID(int clnt_socket, char packet[])
{
    char info[INFOSIZE] = {0};    
    int seek_result = 0;
    char clnt_id[IDSIZE] = {0};

    // 1. 파일열기
    MYSQL *con = mysql_init(NULL);
   
    // MYSQL API 연동 (MYSQL, host, id, pw, db명, port번호, NULL, 0)
    mysql_real_connect(con, "127.0.0.1", "in", "in", "Test2_db", 3333, NULL, 0); 

    
    // 2. 패킷에서 id 추출
    for (int idx = 3; packet[idx] != '\0'; idx++) 
    {
        clnt_id[idx - 3] = packet[idx];
    }

    // 3. id 리스트에서 해당 id가 존재하는지 확인
    for(int idx = 0; idx < clnt_cnt; idx++)
    {
        if(!strcmp(clnt_id,clnt_idList[idx]))
            goto already_connected;
    }
        
    snprintf(info, sizeof(info), "SELECT id FROM users WHERE id='%s'", clnt_id);

    // info값 얻어오기
    mysql_query(con, info);

    // info의 결과로 리턴되는 ROW들을 한꺼번에 얻어옴(mysql_store_result)
    MYSQL_RES *result = mysql_store_result(con);
    

    MYSQL_ROW row;
    // result에 있는 ROW들에서 한개의 ROW를 얻어옴 한 개의 ROW에서 각각 배열형태로 들어 있음
    while ((row = mysql_fetch_row(result))) {
        if (strcmp(clnt_id, row[0]) == 0) {
            printf("[%d]'s ID found!\n", clnt_socket);
            seek_result = 1;
            break;
        }
    }

already_connected:
    // 클라이언트에게 결과 반환
    write(clnt_socket, &seek_result, sizeof(int));

    mysql_free_result(result);
    mysql_close(con);
}

void login_PW(int clnt_socket, char packet[])
{
    char clnt_id[IDSIZE] = {0};
    char clnt_pw[IDSIZE] = {0};
    int seek_result = 0;
    char info[INFOSIZE] = {0}; // text문자열 한줄 뜯어오기
    char clnt_account[INFOSIZE]={0}; // id랑 pw 저장

    // 1. 파일열기
    MYSQL *con = mysql_init(NULL);


    // MYSQL API 연동 (MYSQL, host, id, pw, db명, port번호, NULL, 0)
    mysql_real_connect(con, "127.0.0.1", "in", "in", "Test2_db", 3333, NULL, 0);

 
    // 2. 패킷에서 id와 pw만 추출

    int idx;
    for( idx = 3; idx < strlen(packet);idx++)
    {
        clnt_account[idx-3] = packet[idx];
    }
    clnt_account[idx] = '\0';

    // MYSQL에서 id와 pw 조회

    // strok 첫번째 문자열 출력
    char *token;
    token = strtok(clnt_account, " ");
    
    strcpy(clnt_id, token);
    
    // 두번쨰 문자열 NULL 다음부터 탐색
    token = strtok(NULL, " "); 
    strcpy(clnt_pw, token);

    
    char query[100];

    snprintf(query, sizeof(query), "SELECT * FROM users WHERE id='%s'", clnt_id);



    mysql_query(con, query);

    // query의 결과로 리턴되는 ROW들을 한꺼번에 얻어옴(mysql_store_result)
    MYSQL_RES *result = mysql_store_result(con);


    // result에 있는 ROW들에서 한개의 ROW를 얻어옴 한 개의 ROW에서 각각 배열형태로 들어 있음
    
    bool login_success = false;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
	

	if(strcmp(row[0], clnt_id) == 0 && strcmp(row[1], clnt_pw) == 0)
	{
	   login_success = true;
	   seek_result = 1;
	   break;
	}

        if (strcmp(row[0], clnt_id) == 0) {
            printf("client[%d] is Allowed!\n", clnt_socket);
            seek_result = 1;
            break;
        }
    }

    if(!login_success)
    {
	seek_result = 0;
    }

    // 4. 클라이언트 id만 추출해서, id 리스트에 저장

    add_idList(clnt_socket, clnt_id);

    // 클라이언트에게 결과 반환
    write(clnt_socket, &seek_result, sizeof(int));

    mysql_free_result(result);
    mysql_close(con);
}

void unique_ID(int clnt_socket, char packet[]) {
	
    char info[INFOSIZE]={0};
    char clnt_id[IDSIZE] = {0};
    int seek_result = 1; // 기본적으로 유니크하다고 가정

    MYSQL *con = mysql_init(NULL);
    
    mysql_real_connect(con, "127.0.0.1", "in", "in", "Test2_db", 3333, NULL, 0);


    // 패킷에서 ID 추출
    for (int idx = 3; packet[idx] != '\0'; idx++) {
        clnt_id[idx - 3] = packet[idx];
    }
    clnt_id[strlen(packet) - 3] = '\0'; // clnt_id를 끝내줄 것

    char* token;
    token = strtok(clnt_id," ");
    strcpy(clnt_id, token);
    

    snprintf(info, sizeof(info), "SELECT * FROM users WHERE id='%s'", clnt_id);

    
    mysql_query(con, info);
    

    MYSQL_RES *result = mysql_store_result(con);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        if (strcmp(clnt_id, row[0]) == 0) {
            printf("[%d]'s new ID [%s] is NOT Unique!\n", clnt_socket, clnt_id);
            seek_result = 0;
            break;
        }
    }

    if (seek_result == 1) {
        printf("[%d]'s new ID [%s] is Unique!\n", clnt_socket, clnt_id);
    }

    // 클라이언트에게 결과 반환
    write(clnt_socket, &seek_result, sizeof(int));
    mysql_free_result(result);
    mysql_close(con);
}

void save_Info(int clnt_socket, char packet[]) {

    char info[INFOSIZE]={0};
    char clnt_id[IDSIZE] = {0};
    char clnt_pw[IDSIZE] = {0};
    int result = 0;

    MYSQL *con = mysql_init(NULL);
    
    mysql_real_connect(con, "127.0.0.1", "in", "in", "Test2_db", 3333, NULL, 0);


    // 2. 패킷에서 정보만 추출
    int idx;
    for(idx = 4; packet[idx] != '\0'; idx++)
    // 패킷구조 -> new "ID" "PW"
    {
        info[idx-4] = packet[idx];
    }
    info[idx-4] = '\0';

	
    // 3. info에서 ID와 PW 추출
    for(idx = 0; info[idx] != ' '; idx++)
    {
	    clnt_id[idx] = info[idx];
    }
    clnt_id[idx] = 0;

    idx++;

    for(int i = 0; info[idx] != '\0'; idx++)
    {
	    clnt_pw[i++] = info[idx];
    }
    clnt_pw[idx] = 0;


    snprintf(info, sizeof(info), "INSERT INTO users (id, password) VALUES('%s', '%s')", clnt_id, clnt_pw);

    mysql_query(con, info);

    printf("[%d]'s account info has been written at DB!\n", clnt_socket);
    
    // 4. 클라이언트 id만 추출해서, id 리스트에 저장

    add_idList(clnt_socket, clnt_id);

    // 클라이언트에게 결과 반환
    write(clnt_socket, &result, sizeof(int));

    mysql_close(con);
}
// 정인영이 채울것

#endif

void add_idList(int clnt_socket, char clnt_id[])
// id 리스트에 추가하기
{
    int clnt_idx;

    pthread_mutex_lock(&mtx);
    for(clnt_idx = 0; clnt_idx < clnt_cnt; clnt_idx++) // 소켓배열을 통하여, idx찾기
        if(clnt_socket == clnt_socketList[clnt_idx])
            break;
    
    strcpy(clnt_idList[clnt_idx], clnt_id); // 클라 id 리스트에 입력받았던 id 복사하기
    pthread_mutex_unlock(&mtx);

    return;
}

void DB_extractID(char info[], char clnt_id[])
{
    int i=0;
    printf("packet : %s\n",info);

    while(info[i] != ' ')
    {
	    clnt_id[i] = info[i];
	    i++;
    }

    clnt_id[i+1] = '\0'; // 널 추가
    printf("\nnew clnt : %s\n",clnt_id);
    return ;
}

