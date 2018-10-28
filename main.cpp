#define DEBUG

#include "main.h"
#include "string.h"
using namespace std;

pthread_t   collector_thread;
void        *collector_handle(void *arg);

pthread_t   kplv_thread;
void        *kplv_thread_handle(void *arg);
int num=0;
int fd;// uart file descriptor
char* result;//global variable for get the result from collector
char *del_more = "---- More ----";
char* pos,pos2;
int num1;
int num2;
int str1;
int flag = 0;
int flag1 = 0;
int flag2 = 0;
int flag3 = 0;
int flag4 = 0;
int flag5 = 0;
int flag6 = 0;
void clear_string(char str[],int num )
{
    int len=strlen(str);
    int j=len-num-2;
    int i=len-1;
    for(; i>=j; i--)
    {
        result[i]=' ';
    }
}
static int vlan_number(char* result)
{
    char strFirst[] = "The total number of vlans is :";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNumµÄ³€¶È
    char *p1 = strstr(result,strFirst);
    char *p2 = strstr(result,strFirst);
    if(p1 != NULL)
    {
        int num = 0;
        for(; num < 10; num++)
        {
            if(*(p2 +strFirstlength + num) == '-')
            {
                int i = 0;
                for(; i < num; i++)
                {
                    strResult[i] = *(p1 +strFirstlength + i);
                }
                for( i= 0; i < num; i++)
                {
                    printf("%c",strResult[i]);
                }
                return 1;
            }
        }


    }
    else
    {
        return 0;
    }
}

static int port_speed(char* result)
{
    char strFirst[] = "Speed :";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNumµÄ³€¶È
    char *p1 = strstr(result,strFirst);
    char *p2 = strstr(result,strFirst);
    if(p1 != NULL)
    {
        int num = 0;
        for(; num < 10; num++)
        {
            if(*(p2 +strFirstlength + num) == ',')
            {
                int i = 0;
                for(; i < num; i++)
                {
                    strResult[i] = *(p1 +strFirstlength + i);
                }
                for( i= 0; i < num; i++)
                {
                    printf("%c",strResult[i]);
                }
                return 1;
            }
        }


    }
    else
    {
        return 0;
    }
}

static int input_rate(char* result)
{
    char strFirst[] = "Last 300 seconds input rate";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNumµÄ³€¶È
    char *p1 = strstr(result,strFirst);
    char *p2 = strstr(result,strFirst);
    if(p1 != NULL)
    {
        int num = 0;
        for(; num < 10; num++)
        {
            if(*(p2 +strFirstlength + num) == 'b')
            {
                int i = 0;
                for(; i < num; i++)
                {
                    strResult[i] = *(p1 +strFirstlength + i);
                }
                for( i= 0; i < num; i++)
                {
                    printf("%c",strResult[i]);
                }
                return 1;
            }
        }


    }
    else
    {
        return 0;
    }
}
static int output_rate(char* result)
{
    char strFirst[] = "Last 300 seconds output rate";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNumµÄ³€¶È
    char *p1 = strstr(result,strFirst);
    char *p2 = strstr(result,strFirst);
    if(p1 != NULL)
    {
        int num = 0;
        for(; num < 10; num++)
        {
            if(*(p2 +strFirstlength + num) == 'b')
            {
                int i = 0;
                for(; i < num; i++)
                {
                    strResult[i] = *(p1 +strFirstlength + i);
                }
                for( i= 0; i < num; i++)
                {
                    printf("%c",strResult[i]);
                }
                return 1;
            }
        }


    }
    else
    {
        return 0;
    }
}
static int input_peakrate(char* result)
{
    char strFirst[] = "Input peak rate";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNumµÄ³€¶È
    char *p1 = strstr(result,strFirst);
    char *p2 = strstr(result,strFirst);
    if(p1 != NULL)
    {
        int num = 0;
        for(; num < 10; num++)
        {
            if(*(p2 +strFirstlength + num) == 'b')
            {
                int i = 0;
                for(; i < num; i++)
                {
                    strResult[i] = *(p1 +strFirstlength + i);
                }
                for( i= 0; i < num; i++)
                {
                    printf("%c",strResult[i]);
                }
                return 1;
            }
        }


    }
    else
    {
        return 0;
    }
}
static int output_peakrate(char* result)
{
    char strFirst[] = "Output peak rate";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNumµÄ³€¶È
    char *p1 = strstr(result,strFirst);
    char *p2 = strstr(result,strFirst);
    if(p1 != NULL)
    {
        int num = 0;
        for(; num < 10; num++)
        {
            if(*(p2 +strFirstlength + num) == 'b')
            {
                int i = 0;
                for(; i < num; i++)
                {
                    strResult[i] = *(p1 +strFirstlength + i);
                }
                for( i= 0; i < num; i++)
                {
                    printf("%c",strResult[i]);
                }
                return 1;
            }
        }


    }
    else
    {
        return 0;
    }
}
int main()
{
    /*uart initialize*/
    if( ( fd = open_port() ) && set_com_config(fd, 9600 , 8, 'N', 1) < 0)
    {
        perror("!Error: port initialize error");
        return -1;
    }


//	int cnt_write;
    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid();
    if (fcntl(fd, F_SETLKW, &lock) != 0 )
        printf("error in fcntl\n");


    /*timer initialize*/
    if( timerInit() < 0 )
    {
        perror("!Error: timer initialize error");
        return -1;
    }


    /*start collector
     *collector: a thread keeps collecting uart response pieces by pieces and adds them up
     *the timer timely sends up the result and refresh it*/
    if(pthread_create(&collector_thread, NULL, collector_handle, (void*)NULL) != 0  )
    {
        perror("!Error: collector_thread creation failed\n");
        return -1;
    }


    /*for interactive testing*/
    while(1)
    {
        char buff[MAX_BUF_SIZE];
        memset(buff, 0, MAX_BUF_SIZE);
        if (fgets(buff, MAX_BUF_SIZE, stdin) == NULL)
        {
            perror("fgets");
            break;
        }
        timer_settime(five_sc_timerid, 0, &five_sc_value, NULL);
        int test = strlen(buff);
        write(fd, buff, strlen(buff));

    }


    /*socket initialize*/
    socket_init();
    send(socketfd,DEV_SEQUENCE,DEV_SEQUENCE_LEN,0);
    printf("send B1 frame.\n");
    if(pthread_create(&kplv_thread, NULL, kplv_thread_handle, (void*)NULL) != 0  )
    {
        perror("!Error: kplv thread creation failed\n");
        return -1;
    }


    /*command mapping initialize*/
    commands_init();


    /*start: get frame*/
    while(1)
    {
        int lenRecvFrame = recv(socketfd, recv_buf, MAX_BUF_SIZE, len_inet);
        if( lenRecvFrame == -1 && errno != EINTR)
        {
            perror("!Error: receive error.\n");
        }
#ifdef DEBUG
        else
        {
            printf("received %d bytes: ",lenRecvFrame);
            for(int i = 0; i<lenRecvFrame; i++)
                printf("%x ", (int) *(recv_buf+i) );
            printf("\n");
        }
#endif


        /*parse frame: generate header, type, opcode, valueNum, values*/
        config_frm  *p_config_frm = (config_frm*)recv_buf;
        string      header(p_config_frm->MsgHeader, p_config_frm->len_MsgHeader);
        string      type(p_config_frm->MsgType, p_config_frm->len_MsgType);
        string      opcode(p_config_frm->Opcode, p_config_frm->len_Opcode);
        int         valueNum = atoi( (string(p_config_frm->valueNum, p_config_frm->len_valueNum)).c_str() );
        string      *pValue = new string[valueNum];
        for(int i = 0, cursor = 32; i < valueNum; i++, cursor += 34)
        {
            int _value_length = atoi( string(recv_buf + cursor, 4).c_str() );
            int _value_cursor = cursor + 4;
            pValue[i].assign( recv_buf + _value_cursor, _value_length );
        }
#ifdef DEBUG
        cout<<"Parsed frame head: "<<header<<" "<<type<<" "<<opcode<<" "<<valueNum<<endl;
#endif


        /*assemble command*/
        map<string, string>::iterator it_command = commands.find(opcode);
        if ( it_command == commands.end() )
            cout<<"!Error: mapping not found."<<endl;//            continue;
        string final_command = it_command->second;
#ifdef DEBUG
        cout<<"command mapping: "<<final_command<<endl;
#endif
        for(int i = 0; i < valueNum; i++)
        {
            final_command.append(" ");
            final_command.append(pValue[i]);
        }
        final_command.append("\r");
        delete []pValue;
        cout<<endl
            <<">>>>>>>final command"<<endl
            <<final_command<<endl
            <<">>>>>>>final command ends"<<endl
            <<endl;


        /*write down command*/
        timer_settime(five_sc_timerid, 0, &five_sc_value, NULL);
        if ( write(fd, final_command.c_str(), final_command.length() ) < 0)
            cout<<"!Error: write error"<<endl;

    }


    getchar();
    getchar();
    getchar();
// fclose(fp);
    close(fd);
//    fclose(fp);
    return 0;
}

void *kplv_thread_handle(void *arg)
{
    while(1)
    {
        sleep(10);
#ifdef DEBUG
        printf("send a keepalive frame.\n");
#endif
        send(socketfd,"B2",2,0);

    }
    return NULL;
}

void *collector_handle(void *arg)
{

    while(1)
    {
        memset(snd_buf, 0, MAX_BUF_SIZE);
        if (read(fd, snd_buf, MAX_BUF_SIZE) > 0)
        {
            /*
             result.append(snd_buf);//8 or 7 charactors one piece
             if(result.rfind(del_more)!=-1)//find --More--
             {

                 int length=result.length();//get result length
                 result.erase(length-16,length);//delete --More--
                 //  result.append("        ");
                 */
            strcat(result,snd_buf);
            char *p;
            p= strstr(result,del_more);
            if(p)//find --More--
            {
                clear_string(result,16);

                /*
                int length=strlen(result);//get result length
                Series_substring_delete(result);//delete --More--
                */
                char buff[MAX_BUF_SIZE];//send " "
                memset(buff,0,MAX_BUF_SIZE);
                buff[0]='\x20';
                write(fd, buff, strlen(buff));
            }
            if(flag == 0 )
            {
                flag = vlan_number(result);
            }
            if(flag1 == 0)
            {
                flag1 = port_speed(result);
            }
            if(flag2 == 0)
            {
                flag2 = input_rate(result);
            }
            if(flag3 == 0)
            {
                flag3 = output_rate(result);
            }
            if(flag4 == 0)
            {
                flag4 = input_peakrate(result);
            }
            if(flag5 == 0)
            {
                flag5 = output_peakrate(result);
            }
        }


    }



}

//return NULL;


void timer_handler(int signo, siginfo_t *info, void *context)
{
    /* sleep(3);
     //result.insert(result.begin(), '3');
     //result.insert(result.begin(), 'B');

    #ifdef DEBUG
     cout<<endl
         <<"<<<<<<<result:"<<endl
        // <<result<<endl;
         //write(fd,"\r\n",3);
         cout<<"<<<<<<<result end"<<endl
         <<endl;
    #endif

    //information extraction
     int begin;
     int end;
     string strVlanNum = "The total number of vlans is :"; //Vlan number
     begin = result.find(strVlanNum);
     if(begin != -1)
     {
         string strtemp = result.substr((begin + strVlanNum.length()),3);
         cout<<"**********"<<strtemp<<endl;
     }
     string strGE = "GE0/0/"; //port number
     string strUp = "(U)";
     begin = result.find(strGE);
     end = result.find(strUp);
     if(begin != -1 && end != -1)
     {
         string strtemp = result.substr(begin,(end - begin));
         cout<<"**********"<<strtemp<<endl;
     }
     string strSpeed = "Speed :"; //Speed
     string strSpeednext = ",    Loopback:";
     begin = result.find(strSpeed);
     end = result.find(strSpeednext);
     if(begin != -1 && end != -1)
     {   begin += strSpeed.length();
    end = end - begin;
         string strtemp = result.substr(begin,end);
         cout<<"**********"<<strtemp<<endl;
     }
     string strInputRate = "Last 300 seconds input rate"; //InputRate
     string strInputRatenext = "Last 300 seconds output rate";
     begin = result.find(strInputRate);
     end = result.find(strInputRatenext);
     if(begin != -1 && end != -1)
     {
         begin += strInputRate.length();
    end -= begin;
         string strtemp = result.substr(begin,end);
         end = strtemp.find("bits/sec");
    string strtemp1 = strtemp.substr(0,end);
         cout<<"**********"<<strtemp1<<endl;
     }
     string strOutputRate = "Last 300 seconds output rate"; //OutputRate
     string strOutputRatenext = "Input peak rate";
     begin = result.find(strOutputRate);
     end = result.find(strOutputRatenext);
     if(begin != -1 )
     {
         begin += strOutputRate.length();
    end -= begin;
         string strtemp = result.substr(begin,end);
    end = strtemp.find("bits/sec");
    string strtemp1 = strtemp.substr(0,end);
         cout<<"**********"<<strtemp1<<endl;
     }
     string strInputPeak = "Input peak rate"; //InputPeak
     string strInputPeaknext = "Output peak rate";
     begin = result.find(strInputPeak);
     end = result.find(strInputPeaknext);
     if(begin != -1 )
     {
         begin += strInputPeak.length();
    end -= begin;
         string strtemp = result.substr(begin,end);
         end = strtemp.find("bits/sec");
    string strtemp1 = strtemp.substr(0,end);
         cout<<"**********"<<strtemp1<<endl;
     }
     string strOutputPeak = "Output peak rate"; //OutputPeak
     string strOutputPeaknext = "Input: ";
     begin = result.find(strOutputPeak);
     end = result.find(strOutputPeaknext);
     if(begin != -1 )
     {
         begin += strOutputPeak.length();
    end -= begin;
         string strtemp = result.substr(begin,end);
         end = strtemp.find("bits/sec");
    string strtemp1 = strtemp.substr(0,end);
         cout<<"**********"<<strtemp1<<endl;
     }
     send(socketfd,result.c_str(),result.length(),0);
     result.clear();
     */
}

