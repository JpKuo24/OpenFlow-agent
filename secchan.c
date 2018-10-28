/* Copyright (c) 2008, 2009 The Board of Trustees of The Leland Stanford
 * Junior University
 *
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

#include <config.h>
#include "secchan.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "command-line.h"
#include "compiler.h"
#include "daemon.h"
#include "dirs.h"
#include "discovery.h"
#include "failover.h"
#include "fault.h"
#include "in-band.h"
#include "leak-checker.h"
#include "list.h"
#include "ofp.h"
#include "ofpbuf.h"
#include "openflow/openflow.h"
#include "packets.h"
#include "port-watcher.h"
#include "poll-loop.h"
#include "ratelimit.h"
#include "rconn.h"
#include "stp-secchan.h"
#include "status.h"
#include "timeval.h"
#include "util.h"
#include "vconn-ssl.h"
#include "vconn.h"
#include "vlog-socket.h"
#include "vlog.h"

/****G/S*****/
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <time.h>
#include <unistd.h>
#include <aio.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

/*****zsz added for test*******/
#include "../oflib/ofl-messages.h"
#include "../oflib/ofl-utils.h"
#include "inttypes.h"

#define LOG_MODULE VLM_secchan

/*********zsz added for test. *******2015-11-26*****/
#define Default_Fixed_Pad_Len 2
#define OF_Hearder_Len 8
#define OF_Buffer_ID_Len 4
#define OF_IN_Port_Len 4
#define MAX_BUF_SIZE 2048

/*********zsz added for test. Packet_Out_Define.*******2015-11-26*****/
#define OF_Action_Len 2
#define OF_Pad_Len 6

#define ARM_COM "/dev/ttyUSB0"/*G/S*/

/*zsz added for test:
*Usage: test_print( "%s, %s", s,s);
*/
void test_print( const char *format, ...)
{
#ifdef zsztest
	va_list args;

	fprintf(stderr, "%s: ", program_name);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
#endif
}
/*********zsz added for test. *******2015-11-26*****/
/*Current OLT paramaters, will be replaced later. zsz*/
struct OLT_Paramaters
{
	uint32_t OLT_ID;
	uint32_t ONU_Num;
	uint32_t Port_Speed_Up;
	uint32_t Port_Speed_Down;
	uint32_t Vlan_Total_Num;
	uint32_t OLT_Paramater_Len;
	char * OLT_Name;
};
struct OLT_Paramaters Global_OLT_Data;	/*Using this global variable to record the OLT parameters got from Serial or Telnet. */

/**zsz added for test. Should be modified later.**/
struct Default_OLT_Match
{
	uint16_t type;
	uint16_t length;
	uint16_t OLT_Class; /*Default  value 0x8000. */
	uint8_t OLT_Field_HM; /*Default field 0, HM0, 0x00*/
	uint8_t OLT_OXM_Len; /*Default 8 bytes.*/
	uint32_t OLT_OXM_Data; /*Default 0x00000000.*/
};



struct hook {
    const struct hook_class *class;
    void *aux;
};

struct secchan {
    struct hook *hooks;
    size_t n_hooks, allocated_hooks;
};
/****************G/S*********/
struct sigaction    timer_act;
timer_t				five_sc_timerid;
struct itimerspec 	five_sc_value;
struct sigevent  	five_sc_timer_evp;
char snd_buf[MAX_BUF_SIZE]="\0";
char end_buf[MAX_BUF_SIZE]="\0";
char recv_buf[MAX_BUF_SIZE]="\0";
char *result;//global variable for get the result from collector
char  *del_more;
char *pos,*pos2;
char *vlan_n;
/*************ld**************/
int flag = 0;
int flag1 = 0;
int flag2 = 0;
int flag3 = 0;
int flag4 = 0;
int flag5 = 0;
int flag6 = 0;


/****************G/S*********/




static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(60, 60);

static void parse_options(int argc, char *argv[], struct settings *);
static void usage(void); /*zsz*/

static char *vconn_name_without_subscription(const char *);
static struct pvconn *open_passive_vconn(const char *name);
static struct vconn *accept_vconn(struct pvconn *pvconn);

static struct relay *relay_create(struct rconn *async,
                                  struct rconn *local, struct rconn *remote,
                                  bool is_mgmt_conn);
static struct relay *relay_accept(const struct settings *, struct pvconn *);
static void relay_run(struct relay *, struct secchan *);
static void relay_wait(struct relay *);
static void relay_destroy(struct relay *);
int		Series_init();/*G/S*/
int S_time_init();/*G/S*/
int open_port();/*G/S*/
int set_com_config(int fd,int baud_rate, int data_bits, char parity, int stop_bits);/*G/S*/
int s_fd;
void *collector_handle(void *arg);
void Series_substring_delete(char* p);
/*InformationExtract functions.ld*/
static int vlan_number(char* result)
{
    char strFirst[] = "The total number of vlans is :";
    char strResult[30];
    int  strFirstlength = strlen(strFirst); //strVlanNum¦Ì?3€?¨¨
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
				vlan_n=&strResult;
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
    int  strFirstlength = strlen(strFirst); //strVlanNum¦Ì?3€?¨¨
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
    int  strFirstlength = strlen(strFirst); //strVlanNum¦Ì?3€?¨¨
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
    int  strFirstlength = strlen(strFirst); //strVlanNum¦Ì?3€?¨¨
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
    int  strFirstlength = strlen(strFirst); //strVlanNum¦Ì?3€?¨¨
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
    int  strFirstlength = strlen(strFirst); //strVlanNum¦Ì?3€?¨¨
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




/*Serial Port Timer_handler.zsz*/
void timer_handler(int signo, siginfo_t *info, void *context)
{
#if 0
    sleep(3);
    /*result.insert(result.begin(), '3');
    result.insert(result.begin(), 'B');

#ifdef DEBUG
    cout<<endl
        <<"<<<<<<<result:"<<endl
        <<result<<endl;
        //write(fd,"\r\n",3);
        cout<<"<<<<<<<result end"<<endl
        <<endl;
#endif
*/
//information extraction
    int begin; 
    int end;
    char* strVlanNum = "The total number of vlans is :"; //Vlan number
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
	#endif
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


/*Extract_Rcvd_OLT_Pkt_Out(struct ofpbuf *buffer)
**Extract data information from the ofl_packet_out.
**data information is at the end of ofl_packet_out.
**
*Author: zsz
*/
bool Extract_Rcvd_OLT_Pkt_Out(struct ofpbuf *buffer)
{
	struct ofp_header *oh = buffer->data;
	struct ofp_packet_out *Rcvd_OLT_Pkt_Out;
	struct ofl_msg_packet_out *Local_OLT_Pkt_Out;
	uint16_t of_packet_len,action_len,off_set_of_action,data_len;
	uint8_t *ptr;
	int i;
	int cnt=0;
	uint8_t original_data[200]; /*Maximum received data length.*/
	uint8_t handled_data[196]; /*Maximum received data length without '===='.*/

/*Data structure of ofl_packet_out:
*||ofp_header(8bytes)||buffer_id(4bytes)||in_port(4bytes)||action_len(2bytes)||
*||action(action_len bytes)||pad(6bytes)||data-------------------------------||
**/
	/*1. Conver data into ofp_packet_out struct.*/
	Rcvd_OLT_Pkt_Out = (struct ofp_packet_out *)buffer->data;

	/*2. Get action length.*/
	action_len=ntohs(Rcvd_OLT_Pkt_Out->actions_len);
	printf(">>>>>>>>>>>>>>>Extract_Rcvd_OLT_Pkt_Out action_len %x \n",action_len);
	
	/*3. Get total packet_out length.*/
	of_packet_len=ntohs(Rcvd_OLT_Pkt_Out->header.length);
	printf(">>>>>>>>>>>>>>>Extract_Rcvd_OLT_Pkt_Out of_packet_len %x \n",of_packet_len);

	/*4. Calculate the length of data.*/
	data_len=of_packet_len-OF_Hearder_Len-OF_Buffer_ID_Len-OF_IN_Port_Len-OF_Action_Len-OF_Pad_Len-action_len;
	printf(">>>>>>>>>>>>>>>Extract_Rcvd_OLT_Pkt_Out data_len %x \n",data_len);

	/*5. Get the pointer point ot data.*/
	off_set_of_action=OF_Hearder_Len+OF_Buffer_ID_Len+OF_IN_Port_Len+OF_Action_Len+OF_Pad_Len+action_len;
	printf(">>>>>>>>>>>>>>>Extract_Rcvd_OLT_Pkt_Out off_set_of_action %x \n",off_set_of_action);
	ptr= (uint8_t*)buffer->data;
	
	/*6. Get the data info.*/
	memcpy(original_data,ptr+off_set_of_action,data_len);

	/*7. Remove the identifer '==' from the original data. And the data shoulde be used to send to Serial Port.*/
	for(i=0;i<data_len;i++)
	{
		if(cnt==2)
		{
			i=i-2;
			break;
		}
		if( original_data[i]=='=')
		{
			cnt++;
		}
	}
	memcpy(handled_data,original_data,i);
	printf("Config Command from RYU is :\n",handled_data);
	printf("%s",handled_data);/*Print the real config command.*/
	printf("\n",handled_data);
	timer_settime(five_sc_timerid, 0, &five_sc_value, NULL);
    write(s_fd, handled_data, strlen(handled_data));
	return true;

}

/*Should be modified later.*/
bool Extract_Rcvd_OLT_Flow_Mod(struct ofpbuf *buffer)
{
#if 0
	struct ofp_header *oh = buffer->data;

	struct ofp_flow_mod *Rcvd_OLT_Pkt_Flow_Mod;
	struct ofl_msg_flow_mod *Local_OLT_Pkt_Flow_Mod;
	size_t len;

	len=oh->length;
	if (len < (sizeof(struct ofp_flow_mod) - sizeof(struct ofp_match))) 
	{
		printf( "Received FLOW_MOD message has invalid length (%zu).", len);
		return false;
	}

	len -= (sizeof(struct ofp_flow_mod) - sizeof(struct ofp_match));

	Rcvd_OLT_Pkt_Flow_Mod= (struct ofp_flow_mod *)buffer->data;
	Local_OLT_Pkt_Flow_Mod = (struct ofl_msg_flow_mod *)malloc(sizeof(struct ofl_msg_flow_mod));

	if (Rcvd_OLT_Pkt_Flow_Mod->table_id >= PIPELINE_TABLES && ((Rcvd_OLT_Pkt_Flow_Mod->command != OFPFC_DELETE
		|| Rcvd_OLT_Pkt_Flow_Mod->command != OFPFC_DELETE_STRICT) && Rcvd_OLT_Pkt_Flow_Mod->table_id != OFPTT_ALL)) 
	{
		printf( "Received FLOW_MOD message has invalid table id (%d).", Rcvd_OLT_Pkt_Flow_Mod->table_id );
		return false;
	} 

	Local_OLT_Pkt_Flow_Mod->cookie =	   ntoh64(Rcvd_OLT_Pkt_Flow_Mod->cookie);
	Local_OLT_Pkt_Flow_Mod->cookie_mask =  ntoh64(Rcvd_OLT_Pkt_Flow_Mod->cookie_mask);
	Local_OLT_Pkt_Flow_Mod->table_id =			  Rcvd_OLT_Pkt_Flow_Mod->table_id;
	Local_OLT_Pkt_Flow_Mod->command =			  (enum ofp_flow_mod_command)Rcvd_OLT_Pkt_Flow_Mod->command;
	Local_OLT_Pkt_Flow_Mod->idle_timeout = ntohs( Rcvd_OLT_Pkt_Flow_Mod->idle_timeout);
	Local_OLT_Pkt_Flow_Mod->hard_timeout = ntohs( Rcvd_OLT_Pkt_Flow_Mod->hard_timeout);
	Local_OLT_Pkt_Flow_Mod->priority =	   ntohs( Rcvd_OLT_Pkt_Flow_Mod->priority);
	Local_OLT_Pkt_Flow_Mod->buffer_id =    ntohl( Rcvd_OLT_Pkt_Flow_Mod->buffer_id);
	Local_OLT_Pkt_Flow_Mod->out_port =	   ntohl( Rcvd_OLT_Pkt_Flow_Mod->out_port);
	Local_OLT_Pkt_Flow_Mod->out_group =    ntohl( Rcvd_OLT_Pkt_Flow_Mod->out_group);
	Local_OLT_Pkt_Flow_Mod->flags = 	   ntohs( Rcvd_OLT_Pkt_Flow_Mod->flags);
	/*char *bs = ofl_buffer_to_string(Rcvd_OLT_Pkt_Flow_Mod);*/
	/*printf( "Received PACKET_OUT message with data and buffer_id (%s).", bs);
	free(bs);*/
#endif
	return false;

}
/*Check_OLT_Pkt_Out(struct ofpbuf *buffer)
*Check if the received packetes are OLT-related packets.
*
**OLT-related message must be:Secondly
**buffer_id==0xffffffff
**
**
*Author: zsz
*/
bool Check_OLT_Pkt_Out(struct ofpbuf *buffer)
{
	struct ofp_header *oh = buffer->data;
	struct ofp_packet_out *OLT_Pkt_Out;

	size_t len=oh->length;
	uint32_t buffer_id;
	uint32_t in_port;
	OLT_Pkt_Out = (struct ofp_packet_out *)buffer->data;
	
	/*1. Check if default buffer_id == 0x00ffffff. zsz*/
	printf("\nCheck_If_OLT_Related_Packets>>>>>>Check_OLT_Pkt_Out>>>>ERROR-Buff_ID %x\n",ntohl(OLT_Pkt_Out->buffer_id));
	if (ntohl(OLT_Pkt_Out->buffer_id) != 0x00ffffff) 
	{
		printf("\nCheck_If_OLT_Related_Packets>>>>>>Check_OLT_Pkt_Out>>>>ERROR-Buff_ID\n",oh->type);
		return false;
	}
	return true;
		
}
/*Should be modified later.*/
bool Check_OLT_Flow_Mod(struct ofpbuf *buffer)
{
	struct ofp_header *oh = buffer->data;
	struct ofp_flow_mod *OLT_Pkt_Flow_Mod;
	size_t len;

	len=oh->length;
	
	if (len < (sizeof(struct ofp_flow_mod) - sizeof(struct ofp_match))) 
	{
		printf( "Received FLOW_MOD message has invalid length (%zu).", len);
		return false;
	}

	OLT_Pkt_Flow_Mod= (struct ofp_flow_mod *)buffer->data;

	if (OLT_Pkt_Flow_Mod->table_id == 0xff) 
	{
		printf( "Received FLOW_MOD message has invalid table id (%d).", OLT_Pkt_Flow_Mod->table_id );
		return true;
	} 
	return false;

}

/*Check_If_OLT_Related_Packets(struct ofpbuf *buffer)
*Check if the received packetes are OLT-related packets.
*
**OLT-related message must be: firstly
**xid==0xffffffff
**
*Author: zsz
*/
bool Check_If_OLT_Related_Packets(struct ofpbuf *buffer)
{
	struct ofp_header *oh = buffer->data;
	switch(oh->type) 
	{/*Check specific data info*/ /*Modified according ofl_msg_unpack by zsz.*/
		case OFPT_HELLO :
		case OFPT_ECHO_REQUEST :
		case OFPT_ECHO_REPLY :
		case OFPT_PACKET_OUT: 
			{
				printf("\nCheck_If_OLT_Related_Packets>>>>>>>>>>>>>recved pack out message%x\n",oh->type);
				if(oh->xid==0xffffffff )
				{
					return Check_OLT_Pkt_Out( buffer);
				}
				else
					return false;
			}
		case OFPT_FLOW_MOD:
			{
				return Check_OLT_Flow_Mod( buffer);
			}
		default: 
			{
				return false;
			}
	}
}

/*Controller_To_OLT_Msg_Process(struct ofpbuf *buffer) 
*Handle all the OLT-related message.
*
**INPUT:
**ofpbuf *ofpbuffer: memory address which stores whole packed openflow packet.
**
*Author: zsz
*/
int Controller_To_OLT_Msg_Process(struct ofpbuf *buffer)
{
	struct ofp_header *oh = buffer->data;
	printf(">>>>>>>>>>>>>>>go into controller to olt msg process\n");
	switch(oh->type) 
	{/*Check specific data info*/ /*Modified according ofl_msg_unpack by zsz.*/
		case OFPT_HELLO:/*for test. zsz*/
		case OFPT_PACKET_OUT: 
			{
				Extract_Rcvd_OLT_Pkt_Out(buffer);
				break;
			}
		case OFPT_FLOW_MOD:
			{
				Extract_Rcvd_OLT_Flow_Mod( buffer);
				break;
			}
		default: 
			{				
				return 0;
			}
	}
	return 0;
}

/*Do_Send_OLT_Buffer( struct ofpbuf *ofpbuffer,struct rconn* r) 
* Send whole openflow packet to txq.
*
**INPUT:
**ofpbuf *ofpbuffer: memory address which stores whole packed openflow packet.
**
*Author: zsz
*/
int Do_Send_OLT_Buffer( struct ofpbuf *ofpbuffer,struct rconn* r)
{
	int retval;
	int n_queued;
	retval = rconn_send(r, ofpbuffer, &n_queued);
	if (retval)
	{
		printf(">>>>>>>>>>>>>>>>>>>>>>>>Send TO Controller Failed\n");
		VLOG_WARN_RL(LOG_MODULE, &rl, "send to %s failed: %s",
			rconn_get_name(r), strerror(retval));
		ofpbuf_delete(ofpbuffer);
	}
	
	printf(">>>>>>>>>>>>>>>>>>>>>>>>Send TO Controller Succeed\n");
	return retval;
}

/*OLT_Msg_Pack_Packet_In(struct ofl_msg_packet_in *msg, uint8_t **buf, size_t *buf_len) 
* Calculte length of total OF packet, malloc and copy data into it.
*
**INPUT:
**ofl_msg_packet_in *msg:
**uint8_t **buf: used to store the memory address which will be malloced in this function.
**usize_t *buf_len: length of total OF packet.
**
*Author: zsz
*/
int OLT_Msg_Pack_Packet_In	(struct ofl_msg_packet_in *msg, uint8_t **buf, size_t *buf_len) 
{
	struct ofp_packet_in *packet_in;
	uint16_t match_length; 
	
	uint16_t net_match_length; 
	uint8_t *ptr;
	
	/*1.Calculate match length with pad length. Set match final length to multiple 0f 8. But the real length is still final_len-pad_len.zsz*/
	match_length=msg->match->length+((msg->match->length+7)/8*8-msg->match->length);	/*real_len + pad_len*/
	printf( "|||||||||||||||>>>>>>>OLT_Msg_Pack_Packet_In---match_length %x\n",match_length);
	
	/*2.Calculate total buf_len=len of (header-buffer_id-total_len-reason-table_id-cookie) + (match) +(data).*/
	*buf_len = sizeof(struct ofp_packet_in) -sizeof(struct ofp_match)+ match_length+msg->data_length+Default_Fixed_Pad_Len;
	printf( "|||||||||||||||>>>>>>>OLT_Msg_Pack_Packet_In--------------buf_len %d\n",*buf_len);

	/*3. Malloc buf_len and prepare to copy.*/
	*buf	 = (uint8_t *)malloc(*buf_len);
	if(*buf==NULL)
	{
		printf( "\n|||||MALLOC ERROR ERROR ERROR ERROR ERROR ERROR||||||\n");
		return 0;
	}
	memset(*buf,0,*buf_len);
	printf( "|||||||||||||||OLT_Pkt_Send_To_Queue--->>OLT_Msg_Pack --->>OLT_Msg_Pack_Packet_In buf_len -->>Malloc\n");

	/*4.Put Header and other info in Packet_in.*/
	packet_in = (struct ofp_packet_in *)(*buf);
	packet_in->buffer_id	= htonl(msg->buffer_id);
	packet_in->total_len	= htons(msg->total_len);
	packet_in->reason	=	   msg->reason;
	packet_in->table_id	=	   msg->table_id;
	packet_in->cookie	= hton64(msg->cookie);
	printf( "|||||||||||||||OLT_Pkt_Send_To_Queue--->>OLT_Msg_Pack --->>OLT_Msg_Pack_Packet_In buf_len -->>Malloc>>cookie %x\n",msg->match->length);

	/*5.Copy match just behind cookie. Type and Length should be htonl modified before.*/
	ptr = (*buf) + (sizeof(struct ofp_packet_in) - sizeof(struct ofp_match));/*point to packet_in.match. zsz*/
	memcpy(ptr,msg->match,msg->match->length);
	/*5.1 Set match_length to network-endian. Then copy it into buffer.*/
	net_match_length=htons(msg->match->length);
	memcpy(ptr+2,&net_match_length,2);
	
	/*6. Copy data  behind match+Default_Fixed_Pad_Len.*/
	//match_length=ntohs(match_length);
	ptr = (*buf) + (sizeof(struct ofp_packet_in) -sizeof(struct ofp_match)+match_length+Default_Fixed_Pad_Len);
	/* Ethernet frame */
	if (msg->data_length > 0) 
	{
		memcpy(ptr, msg->data, msg->total_len);
	}
	printf( "|||||||||||||||Malloc>>cookie Ethernet frame\n");
	
	return 0;
}

/*OLT_Msg_Pack(struct ofl_msg_header *msg, uint32_t xid, uint8_t **buf, size_t *buf_size)
* Calculate buf_size, Malloc for buf, fill important memeber of ofl_msg_hearder.
*
**INPUT:
**ofl_msg_header *msg: msg can be any ofl_msg, which means all ofl_msg can be located by msg.
**struct rconn *r: remote connection r
**uint32_t xid: 
**uint8_t **buf:
**size_t *buf_size:
*
*unsigned char *data: data that be sent to the queue of r.
*
*/
int OLT_Msg_Pack(struct ofl_msg_header *msg, uint32_t xid, uint8_t **buf, size_t *buf_size)
{/* learn from int ofl_msg_pack(struct ofl_msg_header *msg, uint32_t xid, uint8_t **buf, size_t *buf_len, struct ofl_exp *exp)*/
	int error;
	switch (msg->type) 
	{
		case OFPT_HELLO: 
		{
			/*error = ofl_msg_pack_empty(msg, buf, buf_size);*/
			break;
		}
		case OFPT_ECHO_REQUEST:
		case OFPT_ECHO_REPLY: 
		{
			/*error = ofl_msg_pack_echo((struct ofl_msg_echo *)msg, buf, buf_size);*/
			break;
		}
		/* Asynchronous messages. */
		case OFPT_PACKET_IN: 
		case OFPT_FLOW_REMOVED:
		{
			error= OLT_Msg_Pack_Packet_In((struct ofl_msg_packet_in *)msg, buf, buf_size);
			break;
		}
#if 0	
		case OFPT_FLOW_REMOVED: 
		{
			//error = ofl_msg_pack_flow_removed((struct ofl_msg_flow_removed *)msg, buf, buf_size, exp);
			break;
		}
	/* Statistics messages. */
		case OFPT_MULTIPART_REQUEST: 
		{
			//error = ofl_msg_pack_multipart_request((struct ofl_msg_multipart_request_header *)msg, buf, buf_size, exp);
			break;
		}
		case OFPT_MULTIPART_REPLY: 
		{
			//error = ofl_msg_pack_multipart_reply((struct ofl_msg_multipart_reply_header *)msg, buf, buf_size, exp);
			break;
		}
#endif
		default: 
		{
			/*OFL_LOG_WARN(LOG_MODULE, "Trying to pack unknown message type.");*/
			error = -1;
		}
	}
	return 0;
}

/*OLT_Pkt_Send_To_Queue(enum ofp_type Msg_type, struct rconn *r, unsigned char *data) 
* Send two types of OpenFlow Message: OFPT_PACKET_IN && OFPT_FLOW_REMOVED
*
**INPUT:
**enum ofp_type Msg_type: OFPT_PACKET_IN or OFPT_FLOW_REMOVED
**struct rconn *r: remote connection r
**unsigned char *data: data that be sent to the queue of r.
**USAGE:OLT_Pkt_Send_To_Queue(OFPT_PACKET_IN,RCONN);
*
*Note>>>>> All the OLT parameters got from Sertial or Telnet should be stored in GLOBAL VARIABLE "Global_OLT_Data",
*Then line 597-605 shoule be used. And this function can be used anywhere anytime.
*Author: zsz
*/
static int
OLT_Pkt_Send_To_Queue(enum ofp_type Msg_type, struct rconn *r/*, OLT_Paramaters*data*/) 
{/*learn from dp_handle_role_request. zsz*/
	struct ofp_header * oh;
	struct ofpbuf *ofpbuf;
	struct ofl_msg_header *msg;
	struct Default_OLT_Match OLT_Match; /*A Default OLT OXM Entry.*/
	struct OLT_Paramaters pdata;
	uint32_t xid=0xffffffff;		/*Default OLT xid. zsz*/
	uint8_t *buf;
	size_t buf_size;
	int error;
	uint16_t oxm_data_len;
	unsigned char test[]="TEST";

	

	/*1. Set OLT OXM. zsz*/
	/*1.1 Set A Default OLT OXM Entry.*/
	OLT_Match.OLT_Class=htons(0x8000);	/*Same with OF 1.3 specification. zsz*/
	OLT_Match.OLT_Field_HM=0x00;		/*Same with OF 1.3 specification. Default. zsz*/
	OLT_Match.OLT_OXM_Data=0xffffffff;/*Default. zsz*/
	OLT_Match.OLT_OXM_Len=0x04;		/*length of this whole OLT OXM entry. zsz*/
	/*1.2 Fill type and length for ofp_match A Default OLT OXM Entry for test*/
	OLT_Match.type=htons(0x0001);
	OLT_Match.length=0x000c; 	/*length=OLT_OXM_LEN+type+length. Will be "htons" later in OLT_Msg_Pack.*/
	
	/*2. Set OLT Data. zsz*/ /*Should be modified later. zsz*/
	pdata.OLT_ID=0xffffffff;
	pdata.ONU_Num=0xffffffff;
	pdata.Port_Speed_Up=0xffffffff;;
	pdata.Port_Speed_Down=0xffffffff;
	pdata.Vlan_Total_Num=vlan_n;
	pdata.OLT_Name=test;
	oxm_data_len= 0x0018;
#if 0
	pdata.OLT_ID=Global_OLT_Data.OLT_ID;
	pdata.ONU_Num=Global_OLT_Data.ONU_Num;
	pdata.Port_Speed_Up=Global_OLT_Data.Port_Speed_Up;
	pdata.Port_Speed_Down=Global_OLT_Data.Port_Speed_Down;
	pdata.Vlan_Total_Num=Global_OLT_Data.Vlan_Total_Num;
	pdata.OLT_Name=test;
	oxm_data_len= Global_OLT_Data.OLT_Paramater_Len;
#endif
	printf( "|||||||||||||||OLT_Pkt_Send_To_Queue PACK DATA and MATCH\n" );

	/*learn from dp_actions_output_port and send_packet_to_controller. zsz*/
	/*3. Set ofp_packet_in message. zsz*/
	struct ofl_msg_packet_in OLT_Pkt_In =
	{{.type = OFPT_PACKET_IN},
		.reason=0x03, 			/*New defined ofp_packet_in_reason. zsz*/
		.total_len	= oxm_data_len, 	/*length of frame, i.e. the length of data.*/
		.table_id = 0x00, 			/*Default OLT table_id. zsz*/
		.buffer_id = 0x00ffffff,	/*Default OLT buffer_id. zsz*/
		.data_length = oxm_data_len,	/*Only used for store the total_len. zsz*/
		.data		= (uint8_t *)&pdata,
		.cookie = 0x1122334455667788,				/*Default OLT cookie. zsz*/
		.match = (struct ofl_match_header*) &OLT_Match}; 	/*dp_send_message .zsz*/
	msg=(struct ofl_msg_header *)&OLT_Pkt_In;
	/*3.1 Malloc buffer for ofp_packet_in, copy message into buffer and return the address of buffer and packet length. zsz*/
	error = OLT_Msg_Pack(msg, xid, &buf, &buf_size);
	printf( "|||||||||||||||>>>>buf address %x >>>>buf size %x\n", buf,buf_size);
	printf( "|||||||||||||||OLT_Pkt_Send_To_Queue-->>OLT_Msg_PackOLT_Msg_Packoh->xid 	= htonl(xid);\n" );
	if (error)
	{
		return error;
	}
	oh = (struct ofp_header *)(buf);
	oh->version =  OFP_VERSION;
	oh->type	=msg->type;
	oh->length= htons(buf_size);
	oh->xid 	= htonl(xid);
	printf( "|||||||||||||||OLT_Pkt_Send_To_Queue-->>OLT_Msg_PackOLT_Msg_Packoh->xid 	= htonl(xid);\n" );
	
	/*4. Prepare ofpbuffer. zsz*/
	ofpbuf = ofpbuf_new(0);
	ofpbuf_use(ofpbuf, buf, buf_size);
	ofpbuf_put_uninit(ofpbuf, buf_size);
	ofpbuf->conn_id = 1;			/*Default. zsz*/

	/*5. Do send to txbuffer. zsz*/
	Do_Send_OLT_Buffer(ofpbuf, r); /*No need to free when failed. zsz*/
	return 0;
}

int
main(int argc, char *argv[])
{
	struct settings s;

	struct list relays = LIST_INITIALIZER(&relays);

	struct secchan secchan;

	struct pvconn *monitor;

	struct pvconn *listeners[MAX_MGMT];
	size_t n_listeners;

	char *local_rconn_name;
	struct rconn *async_rconn, *local_rconn, *remote_rconn;
	struct relay *controller_relay;
	struct discovery *discovery;
	struct switch_status *switch_status;
	struct port_watcher *pw;
	int i;
	int retval;
	set_program_name(argv[0]);
	register_fault_handlers();
	time_init();
	vlog_init();
	Series_init();
	
	parse_options(argc, argv, &s);
	signal(SIGPIPE, SIG_IGN);

	secchan.hooks = NULL;
	secchan.n_hooks = 0;
	secchan.allocated_hooks = 0;

	/* Start listening for management and monitoring connections. */ /*No Listener.zsz*/
	n_listeners = 0;
	for (i = 0; i < s.n_listeners; i++) 
	{
		listeners[n_listeners++] = open_passive_vconn(s.listener_names[i]);
	}
	monitor = s.monitor_name ? open_passive_vconn(s.monitor_name) : NULL; /*No Monitor.zsz*/

	/* Initialize switch status hook. */
	switch_status_start(&secchan, &s, &switch_status);

	die_if_already_running();
	daemonize();

	/* Start listening for vlogconf requests. */
	retval = vlog_server_listen(NULL, NULL);
	if (retval) 
	{
		ofp_fatal(retval, "Could not listen for vlog connections");
	}

	VLOG_INFO(LOG_MODULE, "OpenFlow reference implementation version %s", VERSION BUILDNR);
	VLOG_INFO(LOG_MODULE, "OpenFlow protocol version 0x%02x", OFP_VERSION);

	/* Check datapath name, to try to catch command-line invocation errors. */
	if (strncmp(s.dp_name, "nl:", 3) && strncmp(s.dp_name, "unix:", 5)
		&& !s.controller_names[0]) 
	{
		VLOG_WARN(LOG_MODULE, "Controller not specified and datapath is not nl: or "
		"unix:.  (Did you forget to specify the datapath?)");
	}

	if (!strncmp(s.dp_name, "nl:", 3)) 
	{
		/* Connect to datapath with a subscription for asynchronous events.  By
		 * separating the connection for asynchronous events from that for
		 * request and replies we prevent the socket receive buffer from being
		 * filled up by received packet data, which in turn would prevent
		 * getting replies to any Netlink messages we send to the kernel. */
		async_rconn = rconn_create(0, s.max_backoff);
		rconn_connect(async_rconn, s.dp_name);
		switch_status_register_category(switch_status, "async", rconn_status_cb, async_rconn);
//		test_print("******** %s asynchronous connection create\n", s.dp_name);
	} 
	else 
	{
		/* No need for a separate asynchronous connection: we must be connected
		 * to the user datapath, which is smart enough to discard packet events
		 * instead of message replies.  In fact, having a second connection
		 * would work against us since we'd get double copies of asynchronous
		 * event messages (the user datapath provides no way to turn off
		 * asynchronous events). */
		async_rconn = NULL;
	}

	/* Connect to datapath without a subscription, for requests and replies. */
	local_rconn_name = vconn_name_without_subscription(s.dp_name);
//	test_print("******** %s loacl_rconn_name used for without subscription\n",local_rconn_name);
	local_rconn = rconn_create(0, s.max_backoff);
	rconn_connect(local_rconn, local_rconn_name);
	free(local_rconn_name);
	switch_status_register_category(switch_status, "local", rconn_status_cb, local_rconn);

	/* Connect to controller. */
	remote_rconn = rconn_create(s.probe_interval, s.max_backoff);
	if (s.controller_names[0]) 
	{
		retval = rconn_connect(remote_rconn, s.controller_names[0]);
		if (retval == EAFNOSUPPORT) 
		{
			ofp_fatal(0, "No support for %s vconn", s.controller_names[0]);
		}
	}
	switch_status_register_category(switch_status, "remote", rconn_status_cb, remote_rconn);

	/* Start relaying. */
	controller_relay = relay_create(async_rconn, local_rconn, remote_rconn, false);
	list_push_back(&relays, &controller_relay->node);

	/* Set up hooks. */
	port_watcher_start(&secchan, local_rconn, remote_rconn, &pw);
	discovery = s.discovery ? discovery_init(&s, pw, switch_status) : NULL;
	if (s.enable_stp) 
	{
		stp_start(&secchan, pw, local_rconn, remote_rconn);
	}
	if (s.in_band) 
	{
		in_band_start(&secchan, &s, switch_status, pw, remote_rconn);
	}
	if (s.num_controllers > 1) 
	{
		failover_start(&secchan, &s, switch_status, remote_rconn);
	}
	if (s.rate_limit) 
	{
		rate_limit_start(&secchan, &s, switch_status, remote_rconn);
	}

	while (s.discovery || rconn_is_alive(remote_rconn)) 
	{
		struct relay *r, *n;
		size_t i;

		/* Do work. */
		LIST_FOR_EACH_SAFE (r, n, struct relay, node, &relays) /*Travesal all relay in relays. zsz*/
		{
			relay_run(r, &secchan);
		}
		for (i = 0; i < n_listeners; i++) /*currently, n_listeners is 0. zsz*/
		{
			for (;;) 
			{
				struct relay *r = relay_accept(&s, listeners[i]);
				if (!r) 
				{
					break;
				}
				list_push_back(&relays, &r->node);
			}
		}
#if 1
		printf( "|||||||||||||||OLT_Pkt_Send_To_Queue\n" );
//		OLT_Pkt_Send_To_Queue(OFPT_PACKET_IN,remote_rconn);
#endif
		if (monitor) 
		{
			struct vconn *new = accept_vconn(monitor);
			if (new)
			{
				/* XXX should monitor async_rconn too but rconn_add_monitor()
				* takes ownership of the vconn passed in. */
				rconn_add_monitor(local_rconn, new);
			}
		}
		for (i = 0; i < secchan.n_hooks; i++) 
		{
			if (secchan.hooks[i].class->periodic_cb) 
			{
				secchan.hooks[i].class->periodic_cb(secchan.hooks[i].aux);
			}
		}
		if (s.discovery) 
		{
			char *controller_name;
			if (rconn_is_connectivity_questionable(remote_rconn)) 
			{
				discovery_question_connectivity(discovery);
			}
			if (discovery_run(discovery, &controller_name)) 
			{
				if (controller_name) 
				{
					rconn_connect(remote_rconn, controller_name);
				} 
				else
				{
					rconn_disconnect(remote_rconn);
				}
			}
		}

		/* Wait for something to happen. */
		LIST_FOR_EACH (r, struct relay, node, &relays) 
		{
			relay_wait(r);
		}
		for (i = 0; i < n_listeners; i++) 
		{
			pvconn_wait(listeners[i]);
		}
		if (monitor) 
		{
			pvconn_wait(monitor);
		}
		for (i = 0; i < secchan.n_hooks; i++) 
		{
			if (secchan.hooks[i].class->wait_cb)
			{
				secchan.hooks[i].class->wait_cb(secchan.hooks[i].aux);
			}
		}
		if (discovery) 
		{
			discovery_wait(discovery);
		}
		poll_block();
	}

	return 0;
}

static struct pvconn *
open_passive_vconn(const char *name)
{
	struct pvconn *pvconn;
	int retval;

	retval = pvconn_open(name, &pvconn);
	if (retval && retval != EAGAIN) 
	{
		ofp_fatal(retval, "opening %s", name);
	}
	return pvconn;
}

static struct vconn *
accept_vconn(struct pvconn *pvconn)
{
	struct vconn *new;
	int retval;

	retval = pvconn_accept(pvconn, OFP_VERSION, &new);
	if (retval && retval != EAGAIN) 
	{
		VLOG_WARN_RL(LOG_MODULE, &rl, "accept failed (%s)", strerror(retval));
	}
	return new;
}

void
add_hook(struct secchan *secchan, const struct hook_class *class, void *aux)
{
	struct hook *hook;

	if (secchan->n_hooks >= secchan->allocated_hooks) 
	{
		secchan->hooks = x2nrealloc(secchan->hooks, &secchan->allocated_hooks,
		                    sizeof *secchan->hooks);
	}
	hook = &secchan->hooks[secchan->n_hooks++];
	hook->class = class;
	hook->aux = aux;
}

struct ofp_packet_in *
get_ofp_packet_in(struct relay *r)
{
	struct ofpbuf *msg = r->halves[HALF_LOCAL].rxbuf;
	struct ofp_header *oh = msg->data;
	if (oh->type == OFPT_PACKET_IN) 
	{
		//if (msg->size >= offsetof (struct ofp_packet_in, data)) {
		return msg->data;
		//} else {
		//  VLOG_WARN(LOG_MODULE, "packet too short (%zu bytes) for packet_in",
		//          msg->size);
		//}
	}
	return NULL;
}

/* Need to adapt 1.2 packet-in changes */
bool
get_ofp_packet_eth_header(struct relay *r, struct ofp_packet_in **opip,
                          struct eth_header **ethp)
{
	const int min_len = 0; //offsetof(struct ofp_packet_in, data) + ETH_HEADER_LEN;
	struct ofp_packet_in *opi = get_ofp_packet_in(r);
	if (opi && ntohs(opi->header.length) >= min_len) 
	{
		*opip = opi;
		//*ethp = (void *) opi->data;
		return true;
	}
	return false;
}

/* OpenFlow message relaying. */

/* Returns a malloc'd string containing a copy of 'vconn_name' modified not to
 * subscribe to asynchronous messages such as 'ofp_packet_in' events (if
 * possible). */
static char *
vconn_name_without_subscription(const char *vconn_name)
{
	int nl_index;
	if (sscanf(vconn_name, "nl:%d", &nl_index) == 1) 
	{
		/* nl:123 or nl:123:1 opens a netlink connection to local datapath 123.
		 * nl:123:0 opens a netlink connection to local datapath 123 without
		 * obtaining a subscription for ofp_packet_in or ofp_flow_removed
		 * messages. */
		return xasprintf("nl:%d:0", nl_index);
	} 
	else
	{
		/* We don't have a way to specify not to subscribe to those messages
		 * for other transports.  (That's a defect: really this should be in
		 * the OpenFlow protocol, not the Netlink transport). */
		VLOG_WARN_RL(LOG_MODULE, &rl, "new management connection will receive "
		             "asynchronous messages");
		return xstrdup(vconn_name);
	}
}

static struct relay *
relay_accept(const struct settings *s, struct pvconn *pvconn)
{
	struct vconn *new_remote, *new_local;
	struct rconn *r1, *r2;
	char *vconn_name;
	int retval;

	new_remote = accept_vconn(pvconn);
	if (!new_remote) 
	{
		return NULL;
	}

	vconn_name = vconn_name_without_subscription(s->dp_name);
	retval = vconn_open(vconn_name, OFP_VERSION, &new_local);
	if (retval) 
	{
		VLOG_ERR_RL(LOG_MODULE, &rl, "could not connect to %s (%s)",
		        vconn_name, strerror(retval));
		vconn_close(new_remote);
		free(vconn_name);
		return NULL;
	}

	/* Create and return relay. */
	r1 = rconn_create(0, 0);
	rconn_connect_unreliably(r1, vconn_name, new_local);
	free(vconn_name);

	r2 = rconn_create(0, 0);
	rconn_connect_unreliably(r2, "passive", new_remote);

	return relay_create(NULL, r1, r2, true);
}

static struct relay *
relay_create(struct rconn *async, struct rconn *local, struct rconn *remote,
             bool is_mgmt_conn)
{
	struct relay *r = xcalloc(1, sizeof *r);
	r->halves[HALF_LOCAL].rconn = local;
	r->halves[HALF_REMOTE].rconn = remote;
	r->is_mgmt_conn = is_mgmt_conn;
	r->async_rconn = async;
	return r;
}

static bool
call_local_packet_cbs(struct secchan *secchan, struct relay *r)
{
	const struct hook *h;
	for (h = secchan->hooks; h < &secchan->hooks[secchan->n_hooks]; h++) 
	{
		bool (*cb)(struct relay *, void *aux) = h->class->local_packet_cb;
		if (cb && (cb)(r, h->aux)) 
		{
			return true;
		}
	}
	return false;
}

static bool
call_remote_packet_cbs(struct secchan *secchan, struct relay *r)
{
	const struct hook *h;
	for (h = secchan->hooks; h < &secchan->hooks[secchan->n_hooks]; h++)
	{
		bool (*cb)(struct relay *, void *aux) = h->class->remote_packet_cb;
		if (cb && (cb)(r, h->aux)) 
		{
			return true;
		}
	}
	return false;
}

static void
relay_run(struct relay *r, struct secchan *secchan)
{
	int iteration;
	int i;

	if (r->async_rconn) 
	{
		rconn_run(r->async_rconn);
	}
	for (i = 0; i < 2; i++) 
	{
		rconn_run(r->halves[i].rconn);
	}

	/* Limit the number of iterations to prevent other tasks from starving. */
	for (iteration = 0; iteration < 50; iteration++) 
	{
		bool progress = false;
		for (i = 0; i < 2; i++) 
		{
			struct half *this = &r->halves[i];
			struct half *peer = &r->halves[!i];

			if (!this->rxbuf) 
			{
				this->rxbuf = rconn_recv(this->rconn);
				if (!this->rxbuf && i == HALF_LOCAL && r->async_rconn) 
				{
					this->rxbuf = rconn_recv(r->async_rconn);
				}
/*zsz added here to process OLT-related "packet-out" or "Flow-mod" message. 2015-11-22.*/
/*
*No matter async_rconn or locak or remote_rconn, this->rxbuf should NOT be NULL now.
*
*/
#if 1
				if (this->rxbuf && (i == HALF_REMOTE) )
				{
					if(Check_If_OLT_Related_Packets(this->rxbuf))
					{
						Controller_To_OLT_Msg_Process(this->rxbuf);		
						ofpbuf_delete(this->rxbuf);
						this->rxbuf = NULL;
						progress = true;
						break;
					}
					if(iteration==1)
					{   
						printf("|||||||||||||----->>>>>>>Iteration1\n");
						OLT_Pkt_Send_To_Queue(OFPT_PACKET_IN,this->rconn);
					}
				}
#endif
/***********************zsz**************************************/				
				if (this->rxbuf && (i == HALF_REMOTE || !r->is_mgmt_conn)) 
				{
					if (i == HALF_LOCAL
						? call_local_packet_cbs(secchan, r)
							: call_remote_packet_cbs(secchan, r))
					{
						ofpbuf_delete(this->rxbuf);
						this->rxbuf = NULL;
						progress = true;
						break;
					}
				}
			}

			if (this->rxbuf && !this->n_txq) 
			{
				int retval = rconn_send(peer->rconn, this->rxbuf, &this->n_txq);
				if (retval != EAGAIN)
				{
					if (!retval) 
					{
						progress = true;
					}
					else
					{
						ofpbuf_delete(this->rxbuf);
					}
					this->rxbuf = NULL;
				}
			}
		}
		if (!progress) 
		{
			break;
		}
	}

	if (r->is_mgmt_conn) 
	{
		for (i = 0; i < 2; i++) 
		{
			struct half *this = &r->halves[i];
			if (!rconn_is_alive(this->rconn)) 
			{
				relay_destroy(r);
				return;
			}
		}
	}
}

static void
relay_wait(struct relay *r)
{
	int i;

	if (r->async_rconn) 
	{
		rconn_run_wait(r->async_rconn);
	}
	for (i = 0; i < 2; i++) 
	{
		struct half *this = &r->halves[i];

		rconn_run_wait(this->rconn);
		if (!this->rxbuf) 
		{
			rconn_recv_wait(this->rconn);
			if (i == HALF_LOCAL && r->async_rconn) 
			{
				rconn_recv_wait(r->async_rconn);
			}
		}
	}
}

static void
relay_destroy(struct relay *r)
{
	int i;

	list_remove(&r->node);
	rconn_destroy(r->async_rconn);
	for (i = 0; i < 2; i++) 
	{
		struct half *this = &r->halves[i];
		rconn_destroy(this->rconn);
		ofpbuf_delete(this->rxbuf);
	}
	free(r);
}

/* User interface. */

static void
parse_options(int argc, char *argv[], struct settings *s)
{
	enum {
	    OPT_ACCEPT_VCONN = UCHAR_MAX + 1,
	    OPT_NO_RESOLV_CONF,
	    OPT_INACTIVITY_PROBE,
	    OPT_MAX_IDLE,
	    OPT_MAX_BACKOFF,
	    OPT_RATE_LIMIT,
	    OPT_BURST_LIMIT,
	    OPT_BOOTSTRAP_CA_CERT,
	    OPT_STP,
	    OPT_NO_STP,
	    OPT_OUT_OF_BAND,
	    OPT_IN_BAND,
	    VLOG_OPTION_ENUMS,
	    LEAK_CHECKER_OPTION_ENUMS
	};
	static struct option long_options[] = {
	    {"accept-vconn", required_argument, 0, OPT_ACCEPT_VCONN},
	    {"no-resolv-conf", no_argument, 0, OPT_NO_RESOLV_CONF},
	    {"fail",        required_argument, 0, 'F'},
	    {"inactivity-probe", required_argument, 0, OPT_INACTIVITY_PROBE},
	    {"max-idle",    required_argument, 0, OPT_MAX_IDLE},
	    {"max-backoff", required_argument, 0, OPT_MAX_BACKOFF},
	    {"listen",      required_argument, 0, 'l'},
	    {"monitor",     required_argument, 0, 'm'},
	    {"rate-limit",  optional_argument, 0, OPT_RATE_LIMIT},
	    {"burst-limit", required_argument, 0, OPT_BURST_LIMIT},
	    {"stp",         no_argument, 0, OPT_STP},
	    {"no-stp",      no_argument, 0, OPT_NO_STP},
	    {"out-of-band", no_argument, 0, OPT_OUT_OF_BAND},
	    {"in-band",     no_argument, 0, OPT_IN_BAND},
	    {"verbose",     optional_argument, 0, 'v'},
	    {"help",        no_argument, 0, 'h'},
	    {"version",     no_argument, 0, 'V'},
	    DAEMON_LONG_OPTIONS,
	    VLOG_LONG_OPTIONS,
	    LEAK_CHECKER_LONG_OPTIONS,
#ifdef HAVE_OPENSSL
	    VCONN_SSL_LONG_OPTIONS
	    {"bootstrap-ca-cert", required_argument, 0, OPT_BOOTSTRAP_CA_CERT},
#endif
	    {0, 0, 0, 0},
	};
	char *short_options = long_options_to_short_options(long_options);
	char *accept_re = NULL;
	int retval;

	/* Set defaults that we can figure out before parsing options. */
	s->n_listeners = 0;
	s->monitor_name = NULL;
	s->max_idle = 15;
	s->probe_interval = 15;
	s->max_backoff = 4;
	s->update_resolv_conf = true;
	s->rate_limit = 0;
	s->burst_limit = 0;
	s->enable_stp = false;
	s->in_band = true;
	for (;;) 
	{
		int c;

		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1) 
		{
			break;
		}

		switch (c) 
		{
			case OPT_ACCEPT_VCONN:
			accept_re = optarg[0] == '^' ? optarg : xasprintf("^%s", optarg);
			break;

			case OPT_NO_RESOLV_CONF:
			s->update_resolv_conf = false;
			break;

			case OPT_INACTIVITY_PROBE:
			s->probe_interval = atoi(optarg);
			if (s->probe_interval < 1) 
			{
			    ofp_fatal(0, "--inactivity-probe argument must be at least 1");
			}
			break;

			case OPT_MAX_IDLE:
			if (!strcmp(optarg, "permanent")) {
			    s->max_idle = OFP_FLOW_PERMANENT;
			} else {
			    s->max_idle = atoi(optarg);
			    if (s->max_idle < 1 || s->max_idle > 65535) {
			        ofp_fatal(0, "--max-idle argument must be between 1 and "
			                  "65535 or the word 'permanent'");
			    }
			}
			break;

			case OPT_MAX_BACKOFF:
			s->max_backoff = atoi(optarg);
			if (s->max_backoff < 1) 
			{
			    ofp_fatal(0, "--max-backoff argument must be at least 1");
			} 
			else if (s->max_backoff > 3600) 
			{
			    s->max_backoff = 3600;
			}
			break;

			case OPT_RATE_LIMIT:
			if (optarg) 
			{
			    s->rate_limit = atoi(optarg);
				if (s->rate_limit < 1) 
				{
					ofp_fatal(0, "--rate-limit argument must be at least 1");
				}
			} 
			else 
			{
			    s->rate_limit = 1000;
			}
			break;

			case OPT_BURST_LIMIT:
			s->burst_limit = atoi(optarg);
			if (s->burst_limit < 1)
			{
			    ofp_fatal(0, "--burst-limit argument must be at least 1");
			}
			break;

			case OPT_STP:
			s->enable_stp = true;
			break;

			case OPT_NO_STP:
			s->enable_stp = false;
			break;

			case OPT_OUT_OF_BAND:
			s->in_band = false;
			break;

			case 'F':
			break;   

			case OPT_IN_BAND:
			s->in_band = true;
			break;

			case 'l':
			if (s->n_listeners >= MAX_MGMT) 
			{
			    ofp_fatal(0, "-l or --listen may be specified at most %d times",
			              MAX_MGMT);
			}
			s->listener_names[s->n_listeners++] = optarg;
			break;

			case 'm':
			if (s->monitor_name) 
			{
			    ofp_fatal(0, "-m or --monitor may only be specified once");
			}
			s->monitor_name = optarg;
			break;

			case 'h':
			usage();

			case 'V':
			printf("%s %s compiled "__DATE__" "__TIME__"\n",
			       program_name, VERSION BUILDNR);
			exit(EXIT_SUCCESS);

			DAEMON_OPTION_HANDLERS

			VLOG_OPTION_HANDLERS

			LEAK_CHECKER_OPTION_HANDLERS

#ifdef HAVE_OPENSSL
			VCONN_SSL_OPTION_HANDLERS

			case OPT_BOOTSTRAP_CA_CERT:
			vconn_ssl_set_ca_cert_file(optarg, true);
			break;
#endif

			case '?':
			exit(EXIT_FAILURE);

			default:
			abort();
		}
	}
	free(short_options);

	argc -= optind;
	argv += optind;
	if (argc < 1 || argc > 2) 
	{
	    ofp_fatal(0, "need one or two non-option arguments; "
	              "use --help for usage");
	}

	/* Local and remote vconns. */
	s->dp_name = argv[0];
	{
		char *curr;
		char *save;
		int i;

		s->num_controllers = 0;
		for (i = 0; i < MAX_CONTROLLERS; ++i)
		    s->controller_names[i] = NULL;
		if (argc > 1) 
		{
			for (curr = strtok_r(argv[1], ",,", &save), i = 0;
			 curr && i < MAX_CONTROLLERS;
			 curr = strtok_r(NULL, ",,", &save), ++i) 
			{
			s->controller_names[i] = xstrdup(curr);
			++s->num_controllers;
			}
		}
	}

	/* Set accept_controller_regex. */
	if (!accept_re) 
	{
	    accept_re = vconn_ssl_is_configured() ? "^ssl:.*" : ".*";
	}
	retval = regcomp(&s->accept_controller_regex, accept_re,
	                 REG_NOSUB | REG_EXTENDED);
	if (retval)
	{
	    size_t length = regerror(retval, &s->accept_controller_regex, NULL, 0);
	    char *buffer = xmalloc(length);
	    regerror(retval, &s->accept_controller_regex, buffer, length);
	    ofp_fatal(0, "%s: %s", accept_re, buffer);
	}
	s->accept_controller_re = accept_re;

	/* Mode of operation. */
	s->discovery = s->controller_names[0] == NULL;
	if (s->discovery && !s->in_band) 
	{
	    ofp_fatal(0, "Cannot perform discovery with out-of-band control");
	}

	/* Rate limiting. */
	if (s->rate_limit) 
	{
		if (s->rate_limit < 100) 
		{
			VLOG_WARN(LOG_MODULE, "Rate limit set to unusually low value %d",
			s->rate_limit);
		}
		if (!s->burst_limit) 
		{
			s->burst_limit = s->rate_limit / 4;
		}
		s->burst_limit = MAX(s->burst_limit, 1);
		s->burst_limit = MIN(s->burst_limit, INT_MAX / 1000);
	}
/*zsz added for test.*/
#if 0
	printf("Discovery: %d,  in_band: %d, dp_name %s, Controller Number : %d, Ctrl_1 name: %s, Listener: %d,\
	Lsnr_1 name: %s, Mntr name: %s, max idle: %d, probe_interval: %d, max backoff: %d, rate limit %d, burst limit: %d ", s->discovery,\
	s->in_band,s->dp_name,s->num_controllers,s->controller_names[1],s->n_listeners,s->listener_names[1],s->monitor_name,s->max_idle,\
	s->probe_interval,s->max_backoff,s->rate_limit,s->burst_limit);
#endif
	test_print("Discovery: %d,  in_band: %d, dp_name %s, Controller Number : %d, Ctrl_1 name: %s, Listener: %d \n",\
	s->discovery,s->in_band,s->dp_name, s->num_controllers,s->controller_names[0],s->n_listeners);
	test_print("Lsnr_1 name: %s, Mntr name: %s, max idle: %d, probe_interval: %d, max backoff: %d, rate limit %d, burst limit: %d \n",\
	s->listener_names[0],s->monitor_name,s->max_idle,	s->probe_interval,s->max_backoff,s->rate_limit,s->burst_limit);

}

static void
usage(void)
{
    printf("%s: secure channel, a relay for OpenFlow messages.\n"
           "usage: %s [OPTIONS] DATAPATH [CONTROLLER]\n"
           "DATAPATH is an active connection method to a local datapath.\n"
           "CONTROLLER is an active OpenFlow connection method; if it is\n"
           "omitted, then secchan performs controller discovery.\n",
           program_name, program_name);
    vconn_usage(true, true, true);
    printf("\nController discovery options:\n"
           "  --accept-vconn=REGEX    accept matching discovered controllers\n"
           "  --no-resolv-conf        do not update /etc/resolv.conf\n"
           "\nNetworking options:\n"
           "  --inactivity-probe=SECS time between inactivity probes\n"
           "  --max-idle=SECS         max idle for flows set up by secchan\n"
           "  --max-backoff=SECS      max time between controller connection\n"
           "                          attempts (default: 15 seconds)\n"
           "  -l, --listen=METHOD     allow management connections on METHOD\n"
           "                          (a passive OpenFlow connection method)\n"
           "  -m, --monitor=METHOD    copy traffic to/from kernel to METHOD\n"
           "                          (a passive OpenFlow connection method)\n"
           "  --out-of-band           controller connection is out-of-band\n"
           "  --stp                   enable 802.1D Spanning Tree Protocol\n"
           "  --no-stp                disable 802.1D Spanning Tree Protocol\n"
           "\nRate-limiting of \"packet-in\" messages to the controller:\n"
           "  --rate-limit[=PACKETS]  max rate, in packets/s (default: 1000)\n"
           "  --burst-limit=BURST     limit on packet credit for idle time\n");
    daemon_usage();
    vlog_usage();
    printf("\nOther options:\n"
           "  -h, --help              display this help message\n"
           "  -V, --version           display version information\n");
    leak_checker_usage();
    exit(EXIT_SUCCESS);
}

/*****G/S******/
int Series_init()
{   pthread_t collector_thread; 
	printf("wait\n");
	if( ( s_fd = open_port() ) && set_com_config(s_fd, 115200, 8, 'N', 1) < 0) 
	{
        	perror("!Error: port initialize error");    
		return -1;
    	}
	
	printf("wait1\n"); 
	static struct flock lock;
   	lock.l_type = F_WRLCK;
    	lock.l_start = 0;
    	lock.l_whence = SEEK_SET;
    	lock.l_len = 0;
    	lock.l_pid = getpid();
	if (fcntl(s_fd, F_SETLKW, &lock) != 0 )
        	printf("error in fcntl\n");
	//printf("ssss\n");
    	/*timer initialize*/
	if( S_time_init() < 0 ) 
	{
        	perror("!Error: timer initialize error"); 
		return -1;
    	}
	printf("ssss\n");
    	/*start collector.
     	collector: a thread keeps collecting uart response pieces by pieces and adds them up.
     	the timer timely sends up the result and refresh it.*/
    if(pthread_create(&collector_thread, NULL, collector_handle, (void*)NULL) != 0  )
	{
        	perror("!Error: collector_thread creation failed\n");
        	return -1;
    	}
	printf("wait3\n");

	return 0;
}
/****inital the timer(timer is used to print the data when it time out)****/
int S_time_init(){
    void timer_handler(int signo, siginfo_t *info, void *context);
    /***Timer_handler init***/
    timer_act.sa_flags = SA_RESTART;
    timer_act.sa_sigaction = timer_handler;
    if ((sigemptyset(&timer_act.sa_mask) == -1) || (sigaction(SIGALRM, &timer_act, NULL) == -1))
        return -1;
    /*Timer init: five second for receiving, one second for sending and for waiting timeout */
    five_sc_value.it_interval.tv_sec=0;
    five_sc_value.it_interval.tv_nsec=0;
    five_sc_value.it_value.tv_sec=0;
    five_sc_value.it_value.tv_nsec=200000000;
    five_sc_timer_evp.sigev_notify = SIGEV_SIGNAL;
    five_sc_timer_evp.sigev_signo = SIGALRM;
    if (timer_create(CLOCK_REALTIME, &five_sc_timer_evp, &five_sc_timerid) == -1)
        return -1;
    return 0;
}

int open_port()
{
    int fd;
    fd = open(ARM_COM, O_RDWR|O_NOCTTY|O_NDELAY);
    if (fd < 0)
    {
        perror("open serial port\n");
        return(-1);
    }
    if (fcntl(fd, F_SETFL, 0) < 0) /* \u6062\u590d\u4e32\u53e3\u4e3a\u963b\u585e\u72b6\u6001*/
    {
        perror("fcntl F_SETFL\n");
    }
    if (isatty(fd) == 0) /* \u6d4b\u8bd5\u6253\u5f00\u7684\u6587\u4ef6\u662f\u5426\u4e3a\u7ec8\u7aef\u8bbe\u5907*/
    {
        perror("This is not a terminal device\n");
    }
    return fd;
}
/****set the serials configure*****/
int set_com_config(int fd,int baud_rate, int data_bits, char parity, int stop_bits)
    {
        struct termios new_cfg,old_cfg;
        int speed;
        if (tcgetattr(fd, &old_cfg) != 0)
        {
            perror("tcgetattr");
            return -1;
        }
        new_cfg = old_cfg;
        cfmakeraw(&new_cfg); 
        new_cfg.c_cflag &= ~CSIZE;
        switch (baud_rate)
        {
            case 2400:
            {
                speed = B2400;
            }
            break;
            case 4800:
            {
                speed = B4800;
            }
            break;
            case 9600:
            {
                speed = B9600;
            }
            break;
            case 19200:
            {
                speed = B19200;
            }
            break;
            case 38400:
            {
                speed = B38400;
            }
            break;

            default:
            case 115200:
            {
                speed = B115200;
            }
            break;
        }
        cfsetispeed(&new_cfg, speed);
        cfsetospeed(&new_cfg, speed);

        switch (data_bits)
        {
            case 7:
            {
                new_cfg.c_cflag |= CS7;
            }
            break;

            default:
            case 8:
            {
                new_cfg.c_cflag |= CS8
;             }
            break;
        }

        switch (parity) 
        {
            default:
            case 'n':
            case 'N':
            {
                new_cfg.c_cflag &= ~PARENB;
                new_cfg.c_iflag &= ~INPCK;
            }
            break;

            case 'o':
            case 'O':
            {
                new_cfg.c_cflag |= (PARODD | PARENB);
                new_cfg.c_iflag |= INPCK;
            }
            break;

            case 'e':
            case 'E':
            {
                new_cfg.c_cflag |= PARENB;
                new_cfg.c_cflag &= ~PARODD;
                new_cfg.c_iflag |= INPCK;
            }
            break;

            case 's': 
            case 'S':
            {
                new_cfg.c_cflag &= ~PARENB;
                new_cfg.c_cflag &= ~CSTOPB;
            }
            break;
        }

        switch (stop_bits)
        {
            default:
            case 1:
            {
                new_cfg.c_cflag &= ~CSTOPB;
            }
            break;

            case 2:
            {
                new_cfg.c_cflag |= CSTOPB;
            }
        }

        
        new_cfg.c_cc[VTIME] = 0;
        new_cfg.c_cc[VMIN] = 1;
        tcflush(fd, TCIFLUSH); 
        if ((tcsetattr(fd, TCSANOW, &new_cfg)) != 0) 
        {
            perror("tcsetattr\n");
            return -1;
        }
        return 0;
    }
/****read the data from the OLT******/
	void *collector_handle(void *arg)
	{
	  *del_more="	---- More ----";
		while(1)
		{
			memset(snd_buf, 0, MAX_BUF_SIZE);
			if (read(s_fd, snd_buf, MAX_BUF_SIZE) > 0)
			{	
				strcat(*result,snd_buf);
				if(strstr(*result,*del_more))//find --More--
				{
	
					int length=strlen(*result);//get result length
					Series_substring_delete(result);//delete --More--
					char buff[MAX_BUF_SIZE];//send " "
					memset(buff,0,MAX_BUF_SIZE);
					buff[0]='\x20';
					write(s_fd, buff, strlen(buff));
				}
	
	
			}
	
	
	
		}
	
		return NULL;
	}

void Series_substring_delete(char* p)
{
  pos = strstr(*p,*del_more);
 if(NULL != pos) {
 	pos2 = pos+strlen(*del_more);
	while('\0'!= pos2)
		{
		  pos=pos++;
		pos=pos2++;
		}
	pos = pos2;
 }
}
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

/***************G/S*******/
