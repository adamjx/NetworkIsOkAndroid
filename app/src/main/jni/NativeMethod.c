//
// Created by GourdBoy on 2017/7/13.
//
#include "com_demo_gourdboy_netisoklinux_NativeMethod.h"
#define LOG "NETISOK-JNI"
#ifdef DEBUG
#if DEBUG>0
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG,__VA_ARGS__)
#else
#define LOGI(...)
#define LOGE(...)
#endif
#endif
#define MAX_WAIT_TIME   3
#define MAX_NO_PACKETS  1
#define ICMP_HEADSIZE 8
#define PACKET_SIZE     4096
struct timeval tvsend,tvrecv;
struct sockaddr_in dest_addr,recv_addr;
int sockfd;
pid_t pid;
char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
unsigned short cal_chksum(unsigned short *addr,int len);
int pack(int pkt_no,char *sendpacket);
int send_packet(int pkt_no,char *sendpacket);
int recv_packet(int pkt_no,char *recvpacket);
int unpack(int cur_seq,char *buf,int len);
jstring CStr2Jstring( JNIEnv* env, const char* pat );
void _CloseSocket();
static volatile int isThreadExit = 0 ;
static volatile int isIpCorrect = 0 ;
JavaVM * j_VM;
jobject jObj;
int uid;
JNIEXPORT void JNICALL Java_com_demo_gourdboy_netisoklinux_NativeMethod_netOkInit(JNIEnv *env, jobject jcls)
{
    (*env)->GetJavaVM(env,&j_VM);
    jObj = (*env)->NewGlobalRef(env,jcls);
    uid = getuid();
    setuid(uid);
}
JNIEXPORT void JNICALL Java_com_demo_gourdboy_netisoklinux_NativeMethod_setIP(JNIEnv *env, jobject jcls,jstring j_str)
{
    const char *c_str = (*env)->GetStringUTFChars(env, j_str, NULL);
    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if(c_str != NULL&&inet_pton(AF_INET, c_str, &(dest_addr.sin_addr)) > 0)
    {
        isIpCorrect=1;
    }
    else
    {
        isIpCorrect = 0;
        LOGE("inet_pton errno %d %s\n", errno, strerror(errno));
    }
    LOGI("got ip is %s",c_str);
    (*env)->ReleaseStringUTFChars(env, j_str,c_str);
}
JNIEXPORT void JNICALL Java_com_demo_gourdboy_netisoklinux_NativeMethod_netOkThreadStart(JNIEnv *env, jobject jcls)
{
    isThreadExit = 0;
    pthread_t threadId;
    if(pthread_create(&threadId,NULL,thread_isNetOk_exec,NULL)!=0)
    {
        LOGE("native thread start failed!");
    }
}
JNIEXPORT void JNICALL Java_com_demo_gourdboy_netisoklinux_NativeMethod_netOkThreadStop(JNIEnv *env, jobject jcls)
{
    isThreadExit = 1;
    LOGI("native thread stop successed!");
}
static void* thread_isNetOk_exec(void *p)
{
     LOGE("isNetOk thread started");
     JNIEnv *env;
     if ((*j_VM)->AttachCurrentThread(j_VM, &env, NULL) != 0)
     {
        LOGE("AttachCurrentThread failed!");
        (*j_VM)->DetachCurrentThread(j_VM);
        return NULL;
     }
     jclass javaClass = (*env)->GetObjectClass(env, jObj);
     if (javaClass == 0)
     {
             LOGE("Unable to find class");
             (*j_VM)->DetachCurrentThread(j_VM);
             return NULL;
     }
     jmethodID javaCallbackId = (*env)->GetMethodID(env, javaClass,"isNetOkCallBack", "(ZLjava/lang/String;)V");
     if (javaCallbackId == NULL)
     {
             LOGE("Unable to find method:isNetOkCallBack");
             (*j_VM)->DetachCurrentThread(j_VM);
             return NULL;
     }
     int i;
     while(!isThreadExit)
     {
        sleep(5);
        if(isIpCorrect)
        {
            if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP))==-1)
            {
                LOGE("create SOCK_DGRAM failed ,sockfd is %d,errno is %d",sockfd,errno);
                continue;
            }
            pid=getpid();
            LOGI("pid %d",pid);
            for(i=0;i<MAX_NO_PACKETS;i++)
            {
                if(send_packet(i,sendpacket)<0)
                {
                    LOGE("send packet failed!");
                    (*env)->CallVoidMethod(env, jObj, javaCallbackId,JNI_FALSE,(*env)->NewStringUTF(env,"-"));
                    break;
                }
                if(recv_packet(i,recvpacket)>0)
                {
                    float delayTime = (float)((1000000*(tvrecv.tv_sec-tvsend.tv_sec)+(tvrecv.tv_usec-tvsend.tv_usec))/1000);
                    LOGI("net is ok! time=%.2fms",delayTime);
                    char str_time[30];
                    sprintf(str_time, "%.2f", delayTime);
                    (*env)->CallVoidMethod(env, jObj, javaCallbackId,JNI_TRUE,CStr2Jstring(env,str_time));
                    break;
                }
                else
                {
                    (*env)->CallVoidMethod(env, jObj, javaCallbackId,JNI_FALSE,(*env)->NewStringUTF(env,"-"));
                }
            }
            _CloseSocket();
            LOGI("close socket");
        }
        else
        {
            (*env)->CallVoidMethod(env, jObj, javaCallbackId,JNI_FALSE,(*env)->NewStringUTF(env,"-"));
        }
     }
      LOGI("isNetOk thread loop exit");
     (*env)->DeleteGlobalRef(env,jObj);
     (*j_VM)->DetachCurrentThread(j_VM);
     env = NULL;
     return NULL;
}
int send_packet(int pkt_no,char *sendpacket)
{
    int packetsize;
    packetsize=pack(pkt_no,sendpacket);
    gettimeofday(&tvsend,NULL);
    int code = sendto(sockfd,sendpacket,packetsize,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr));
    if(code<0)
    {
        LOGE("error : sendto error %d, errno %d  %s",code,errno,strerror(errno));
        return -1;
    }
    return 1;
}
int recv_packet(int pkt_no,char *recvpacket)
{
    int rc;
    socklen_t fromlen;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd,&rfds);
    fromlen=sizeof(recv_addr);
    struct timeval timeout = {MAX_WAIT_TIME, 0};
    rc=select(sockfd+1, &rfds, NULL, NULL, &timeout);
    if (rc == 0)
    {
        LOGE("no reply in 3 second\n");
        return -1;
    }
    else if (rc < 0)
    {
        LOGE("select errno %d %s\n", errno, strerror(errno));
        return -1;
    }
    rc=recvfrom(sockfd,recvpacket,PACKET_SIZE,0,(struct sockaddr *)&recv_addr,&fromlen);
    if( rc <=0)
    {
        LOGE("revcfrom errno %d %s\n", errno, strerror(errno));
        return -1;
    }
    if(unpack(pkt_no,recvpacket,rc)==-1)
        return -1;
    return 1;
}
int pack(int pkt_no,char*sendpacket)
{
    int i,packsize;
    struct icmp *icmp;
    struct timeval *tval;
    icmp=(struct icmp*)sendpacket;
    icmp->icmp_type=ICMP_ECHO;   //设置类型为ICMP请求报文
    icmp->icmp_code=0;
    icmp->icmp_cksum=0;
    icmp->icmp_seq=pkt_no;
    icmp->icmp_id=pid;           //设置当前进程ID为ICMP标示符
    packsize=ICMP_HEADSIZE+sizeof(struct timeval);
    tval= (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);
    icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize);
    return packsize;
}
int unpack(int cur_seq,char *buf,int len)
{
    //LOGI("unpack packet");
    int iphdrlen;
    struct ip *ip;
    struct icmp *icmp;
    ip=(struct ip *)buf;
    iphdrlen=ip->ip_hl<<2;     //求ip报头长度,即ip报头的长度标志乘4
    icmp=(struct icmp *)(buf+iphdrlen);     //越过ip报头,指向ICMP报头
    len-=iphdrlen;      //ICMP报头及ICMP数据报的总长度
    gettimeofday(&tvrecv,NULL);
    if( len<8)
    {
        LOGE("receive packet lenth < 8");
        return -1;
    }
    if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_seq==cur_seq))
    {
         //LOGI("unpack packet success");
        return 0;
    }
    LOGE("unpack packet failed,current sequence %d,icmp_seq %d",cur_seq,icmp->icmp_seq);
    return -1;
}
unsigned short cal_chksum(unsigned short *addr,int len)
{
    int nleft=len;
    int sum=0;
    unsigned short *w=addr;
    unsigned short answer=0;
    while(nleft>1)       //把ICMP报头二进制数据以2字节为单位累加起来
    {
        sum+=*w++;
        nleft-=2;
    }
    if( nleft==1)       //若ICMP报头为奇数个字节,会剩下最后一字节.把最后一个字节视为一个2字节数据的高字节,这个2字节数据的低字节为0,继续累加
    {
        *(unsigned char *)(&answer)=*(unsigned char *)w;
        sum+=answer;
    }
    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    answer=~sum;
    return answer;
}
void _CloseSocket()
{
    close(sockfd);
    sockfd = 0;
}
jstring CStr2Jstring( JNIEnv* env, const char* c )
 {
   jclass strClass = (*env)->FindClass(env,"java/lang/String");
   jmethodID ctorID = (*env)->GetMethodID(env,strClass, "<init>", "([BLjava/lang/String;)V");
   jbyteArray bytes = (*env)->NewByteArray(env,(jsize)strlen(c));
   (*env)->SetByteArrayRegion(env,bytes, 0, (jsize)strlen(c), (jbyte*)c);
   jstring encoding = (*env)->NewStringUTF(env,"UTF-8");
   return (jstring)(*env)->NewObject(env,strClass, ctorID, bytes, encoding);
 }