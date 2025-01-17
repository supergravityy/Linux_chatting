#include "clnt.h"

bool session_down = false;
bool user_down = false;
pthread_mutex_t mtx;

int main(int argsNum, char *args[])
{
    /*-------------------------------------*/
    // 1. 통신에 필요한 변수들 선언 +  ip검사
    /*-------------------------------------*/

    int serv_socket;
    struct sockaddr_in serv_addr;
    char serv_ip[16] = {0};
    bool login_success = false;
    int DB_Err = 0;
    int choose;
    char userID[IDSIZE];
    int serverAccepted = 0;
    pthread_t recv_thread;
    pthread_t send_thread;
    Args send_args = { 0, NULL, {0}}; // 스레드 동작함수에 넘길 파라미터
    Args recv_args = { 0, NULL, {0}};

    if(argsNum < 2) // 먼저 메인함수에 파라미터가 들어왔는지 검사
    {
        printf("Input server IP address correctly!\n");
        return 0;
    }
    else
    {
        strncpy(serv_ip, args[1], IDSIZE-1);
        serv_ip[sizeof(serv_ip) - 1] = '\0';

        if(!is_correctIP(serv_ip, strlen(serv_ip)))
        {
                printf("\033[0;32mSYS : Input server IP address correctly!\n\033[0m");
                return 0;
        }
        else
                printf("start communication with the appropriate ip you entered : %s\n",serv_ip);
    }

    /*-------------------------------------*/
    // 2. 서버와 통신할 소켓생성
    /*-------------------------------------*/

return_menu:    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
    serv_addr.sin_port = htons(PORT);

    printf("\033[0;32mSYS : socket is creating for server...\n\033[0m");

    serv_socket = socket(PF_INET, SOCK_STREAM, 0); // TCP용
    if (serv_socket == -1)
    {
        printf("\033[0;31mSYS : socket creation error!\n\033[0m");
        return 0;
    }

    /*-------------------------------------*/
    // 3. connect 절차
    /*-------------------------------------*/

    printf("\033[0;32msocket is trying to connect with server...\n\033[0m");

    if (connect(serv_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
	    printf("\033[0;31mSYS : connection error!\n\033[0m");
	    return 0;
    }
    else
    {
	    printf("\033[0;31mSYS : connecting completed, but server not Accepted!\n\033[0m");
	    printf("\033[0;31mSYS : waiting for server answer...\033[0m\n");
	    int length = read(serv_socket,&serverAccepted,sizeof(int));
	    if(length > 0)
        	{
                	if(serverAccepted) // 서버로부터 연결을 승인받음
                        	printf("\033[0;31mSYS : server Allowed connection!\n\033[0m");
			else // 수신오류
                	{
				printf("\033[0;31mSYS : read Err!\n\033[0m");
				return 0;
                	}
        	}
	    else 
	    {
		    printf("\033[0;31mSYS : server closed the connection\n\033[0m");
		    return 0;
	    }
    }
    printf("\033[0;32mSYS : connecting success!\033[0m\n");

    /*-------------------------------------*/
    // 4. 메인메뉴 화면 띄우기
    /*-------------------------------------*/

    memset(userID, 0, IDSIZE);

    while (!login_success)
    {
        //system("clear"); // 화면 전부 지우기

        printf("-----Penguin version 1.0----\n");
        printf("1. login\n");
        printf("2. sign up\n");
        printf("3. quit\n");
        printf("----------------------------\n");
        printf("input : _\b");

        scanf("%d", &choose);        // 입력받기
        while (getchar() != '\n');   // 엔터비우기
        putchar('\n');

        switch (choose)
        {
        case 1:
            login_success = login_process(serv_socket, &DB_Err, userID);     // id와 pw를 입 력받아서 서버로 넘기기 -> 유효하면 success = 1
            break;

        case 2:
            login_success = signup_process(serv_socket, &DB_Err, userID);    // id와 pw를 입 력받아서 서버로 넘겨서 유효한지 확인
            break;

        case 3:
            printf("\033[0;31mSYS : quit sequence\033[0m\n");
            //clean_process();                                     /* --- 혹시 있을 스레드들 삭제 + 통신프로세스 종료 --- */
            return 0;
            break;

        default:
            printf("\033[0;31mSYS : incorrect input, try again\n\033[0m");
            break;
        }

        printf("\033[0;31mDB errorCode : %d\033[0m\n", DB_Err);
    }

    /*-------------------------------------*/
    // 5. 통신에 필요한 스레드 + 파라미터 구조체 + 뮤텍스 만들기
    /*-------------------------------------*/

    pthread_mutex_init(&mtx, NULL); // 이후로 오류가 생길때마다 무조건 뮤텍스 파괴함수호출   

    //system("clear");
    static int commuErr = 0;  // 메인메뉴로 돌아오는 과정에서 또 선언되는걸 방지 -> 어차피 commu.c에서 뮤텍스로 보호됨

    send_args.serv_socket = serv_socket; // 파라미터 구조체 초기화
    recv_args.serv_socket = serv_socket;
    send_args.errCode = &commuErr;
    recv_args.errCode = &commuErr;
    strcpy(send_args.userID, userID);
    strcpy(recv_args.userID, userID);

    /*--- 수정 : 스레드 함수에 전달할 인자는 하나밖에 전달 못한다 ---*/

    pthread_create(&recv_thread, NULL, recv_msg, (void*)&recv_args); // 수신용 스레드        
    pthread_create(&send_thread, NULL, send_msg, (void*)&send_args); // 송신용 스레드        
    printf("\033[0;31mSYS : threads were made for communication right now\033[0m\n");

    /*-------------------------------------*/
    // 6. 에러확인 + 리소스 반환
    /*-------------------------------------*/

    while(!session_down && !user_down) usleep(500);

    // 뭐가됬던 플래그만 받고 바로 종료시켜버리기
    pthread_cancel(recv_thread);
    pthread_join(recv_thread,NULL);
    printf("\033[0;31mrecv_thread : down\n\033[0m");
    
    pthread_cancel(send_thread);
    pthread_join(send_thread,NULL);
    printf("\033[0;31msend_thread : down\033[0m\n");

    pthread_mutex_destroy(&mtx);
    printf("\033[0;31mSYS : threads and mutex were terminated!\033[0m\n");

    if (commuErr == 1)
    {
        login_success = 0;
        DB_Err = 0;
        commuErr = 0;
	session_down = false;
	user_down = false;
        goto return_menu;
    }
    else if (2 <= commuErr && commuErr <= 4)
        printf("\033[0;31mcommunication errorCode : %d\033[0m\n", commuErr);
    
    close(serv_socket); // 완전히 종료
    printf("\033[0;31mSYS : good bye~!\033[0m\n");
    return 0;
}
