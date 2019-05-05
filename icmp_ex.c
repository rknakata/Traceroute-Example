#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h> // added
#include <time.h> // added

#include "checksum.h" //my checksum library
#include <poll.h>

#define BUFSIZE 1500 //1500 MTU (so within one frame in layer 2)
#define PROTO_ICMP 1
#define NI_MAXHOST 1025 // http://www.microhowto.info/howto/convert_an_ip_address_to_the_corresponding_domain_name_in_c.html

char hostname[NI_MAXHOST] = "";
char original[NI_MAXHOST] = "";
//char hostname[NI_MAXHOST] = "";
int icmpReqCount = 0; //this needs to be done 3 times for each ttl //https://stackoverflow.com/questions/6970224/providing-passing-argument-to-signal-handler
int firstRun = 0; // change this to 1 after the first run finishes
int currentTTL = 1;
int maxTTL = 30;
/*
These vars probably need to be set according to the first print statement around line 86 variables
printf("%d bytes from %s (%s): icmp_req=%d ttl=%d time=%.1f ms\n", data_len, hostname, inet_ntoa(ip->ip_src), icmpReqCount, reply_ttl, delayFloat);
if(firstRun == 0){
  firstRun = 1; // this message wont print again
}
*/
//float delayFloat = delay; //probably need to implement otherwise
//int reply_ttl = ip->ip_ttl;  //probably need to implement this

/*DELETE ME*/
float delayFloat = 0; //change this and delete
int reply_ttl = 0; //change this and delete
int keepRunning = 1;

//get packet to return time
// char sentTimeBuffer[30]; // holds the string of delay
// char receivedTimeBuffer[30]; // holds the string of delay
struct timeval sent;
struct timeval received;
struct timeval programStart;// this is used in the console when sig int is received
struct timeval programEnd;// this is used in the console when sig int is received

time_t sentTime;
time_t receivedTime;
time_t programStartTime;
time_t programEndTime;
long delay = 0;
long totalRoundTripTime = 0;
long programRunTime; // this is printed in the console when sig int is received

//https://stackoverflow.com/questions/6970224/providing-passing-argument-to-signal-handler

float minDelayFloat = 0; // min
float avgDelayFloat = 0; // avg
float maxDelayFloat = 0; // max
float standardDeviation = 0; // mdev
float totalDelayFloat = 0;
float devArray[100];
int devCount = 0;
//make this dynamic if i work on this in the future

float tryTime[] = {0,0,0};
int currentTry3 = 0; // this can be 0 1 2

int timeout = 0;


/*
need to test this program without a nat nat blocks icmp time exceeded reply from router

I need to find out where the packet send and recive sigaalarm caller should go
do i use sigalarm to call the packet function as well as use it to determing a 3 second timeout on recieve?

i need toincrement the ttl incrementer

i do not need sigint when i ctrl c on real traceroute nothing is printed

I need to answer the questions in the readme

Traceroute sends out three packets per TTL increment. Each column corresponds to the time is took to get one packet back (round-trip-time).
*/

//this may not be needed check the recv operation there are two timeouts in place but i think this incrementiing it one of them is not needded
void handler(int signum) { // i should use this for time out maybe set global variable after a timer and cancel this by callign alarm(0)
  //https://stackoverflow.com/questions/12406915/using-signal-and-alarm-as-timeouts-in-c
   // printf("put the trace route function  here\n");
   // exit(1);
     timeout++;
       //printf("packet timed out delete\n");
}


int main(int argc, char * argv[]){
 struct addrinfo *result;
 struct addrinfo *res;
  //i think this is supposed to be used instead of doing first run like i did nvm i added this
 int error;

  char sendbuf[BUFSIZE], recvbuf[BUFSIZE], controlbuf[BUFSIZE];
  struct icmp * icmp;
  struct ip * ip;
  int sockfd;
  //int packet_len, recv_len, ip_len, data_len;
  int packet_len, recv_len, ip_len;
  struct addrinfo * ai;
  struct iovec iov;
  struct msghdr msg;
  struct sockaddr_in *addr; // this was in first run


  //process addr info
  // http://man7.org/linux/man-pages/man3/gai_strerror.3.html
  // this gets information about ip
  //getaddrinfo(argv[1], NULL, NULL, &ai);

  /* resolve the domain name into a list of addresses */
  error = getaddrinfo(argv[1], NULL, NULL, &result);
  if (error != 0)
  {
      fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
      return EXIT_FAILURE;
  }

  //fix this later
  /* loop over all returned results and do inverse lookup */
  for (res = result; res != NULL; res = res->ai_next)
  {
      // char hostname[NI_MAXHOST] = "";


      error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
      if (error != 0)
      {
          fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
          continue;
      }
      if (*hostname != '\0'){
          //hostname is sets

          // printf("hostname: %s\n", hostname);// char hostname[NI_MAXHOST] = "";
      }
  }


  freeaddrinfo(result);
  //signal(SIGALRM,handler);
while(keepRunning == 1){
// //  printf("%d bytes from %s (%s): icmp_req=%d ttl=%d time=%.1f ms\n", data_len, hostname, inet_ntoa(ip->ip_src), icmpReqCount, reply_ttl, delayFloat);
//   printf("hostname: %s", hostname);
while(currentTry3 < 3){
  getaddrinfo(argv[1], NULL, NULL, &ai);
  addr = (struct sockaddr_in *)ai->ai_addr;
//   if(firstRun == 0){
//     //process addr info
//     // getaddrinfo(argv[1], NULL, NULL, &ai);
//     // addr = (struct sockaddr_in *)ai->ai_addr;
//       //process destination address
//     printf("traceroute to %s (%s), (%d) hops max, 60 byte packets\n", ai->ai_canonname ? ai->ai_canonname : argv[1], inet_ntoa((struct in_addr)addr->sin_addr), maxTTL);
//     firstRun = 1; // this message wont print again
//   }
// strcpy(hostname, inet_ntoa((struct in_addr)addr->sin_addr));
if(firstRun == 0){
  //process addr info
  // getaddrinfo(argv[1], NULL, NULL, &ai);
  // addr = (struct sockaddr_in *)ai->ai_addr;
    //process destination address
    strcpy(original, inet_ntoa((struct in_addr)addr->sin_addr));
  printf("traceroute to %s (%s), (%d) hops max, 60 byte packets\n", ai->ai_canonname ? ai->ai_canonname : argv[1], inet_ntoa((struct in_addr)addr->sin_addr), maxTTL);
  firstRun = 1; // this message wont print again
}
freeaddrinfo(ai);

// alarm(0);
// signal(SIGALRM, handler);
// alarm(0);

// sample output
//sudo traceroute -I example.com
//traceroute to example.com (158.130.69.89), 30 hops max, 60 byte packets

// //process addr info
// getaddrinfo(argv[1], NULL, NULL, &ai);
//
//   //process destination address
//   printf("Dest: %s\n", ai->ai_canonname ? ai->ai_canonname : argv[1]);
//
// while(keepRunning == 1){
  //Initialize the socket
  if((sockfd = socket(AF_INET, SOCK_RAW, PROTO_ICMP)) < 0){
    perror("socket"); //check for errors
    exit(1);
  }




  //initiate ICMP header
  icmp = (struct icmp *) sendbuf; //map to get proper layout
  icmp->icmp_type = ICMP_ECHO; //Do an echoreply
  icmp->icmp_code = 0;
  icmp->icmp_id = 42;
  icmp->icmp_seq= 0;
  icmp->icmp_cksum = 0;
  icmp->icmp_seq = 1;

  //printf("test %d\n", icmp->icmp_seq);

  //compute checksum
  icmp->icmp_cksum = checksum((unsigned short *) icmp, sizeof(struct icmp));
  // int test = 1;
  packet_len = sizeof(struct icmp);
  //https://stackoverflow.com/questions/24590818/what-is-the-difference-between-ipproto-ip-and-ipproto-raw
  setsockopt(sockfd, IPPROTO_IP, IP_TTL, &currentTTL, sizeof(int));
  gettimeofday(&sent, NULL);
  sentTime = sent.tv_sec;

  //Setsockopt (sockfd, IPPROTO_IP, IP_TTL, &currentTTL, sizeof(int));
  //send the packet
  if( sendto(sockfd, sendbuf, packet_len, 0, ai->ai_addr, ai->ai_addrlen) < 0){
    perror("sendto");//error check
    exit(1);
  }

  //built msgheader structure for receiving reply
  iov.iov_base = recvbuf;
  iov.iov_len = BUFSIZE;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control=controlbuf;
  msg.msg_controllen=BUFSIZE;

  //alarm(3);
  // LINUX
  struct pollfd fd;
  int ret;

  fd.fd = sockfd; // your socket handler
  fd.events = POLLIN;
  ret = poll(&fd, 1, 3000); // 1 second for timeout
  switch (ret) {
      case -1:
          // Error
          break;
      case 0:
          timeout ++; // it timed out keep going
          //printf("There is a timeout\n");
          break;
      default:
          // recv(mySocket,buf,sizeof(buf), 0); // get your data
          if((recv_len = recvmsg(sockfd, &msg, 0)) < 0){ //could get interupted ??
            perror("recvmsg");
            exit(1);
          }
          break;
  }
  //setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&xd, sizeof xd);
  //recv the reply
  // according to pdf i need to set a 3 second timer i also think i need to look for icmp timeout response packet_len
  // if((recv_len = recvmsg(sockfd, &msg, 0)) < 0){ //could get interupted ??
  //   perror("recvmsg");
  //   exit(1);
  // }



  //receivedTime = received.tv_sec;
    gettimeofday(&received, NULL);
    receivedTime = received.tv_sec;
    // strftime(receivedTimeBuffer,30,"%m-%d-%Y  %T.",localtime(&receivedTime));
    // printf("%s%ld\n",receivedTimeBuffer,received.tv_usec);
// while(1){
//   //printf("delete");
// }
    delay = (received.tv_sec - sent.tv_sec) * 1000000 + received.tv_usec - sent.tv_usec; // useq is a milionth of a second ms is microsecond which is 1/1000th of a second
    //delay = received.tv_usec - sent.tv_usec; // useq is a milionth of a second ms is microsecond which is 1/1000th of a second
  //alarm(0);
/************************
look here make it run 4 times use array for each current ttl
****************************************/
    float delayFloat = delay;
      delayFloat = delayFloat / 1000;
//printf("delete delay %.3f ms ", delayFloat);
tryTime[currentTry3] = delayFloat;

if(currentTry3 <= 3){
 currentTry3 ++;
}

  delay = 0; // set global variable back to 0

//  printf("%d\n",recv_len);

  ip = (struct ip*) recvbuf;
  ip_len = ip->ip_hl << 2; //length of ip header

  icmp = (struct icmp *) (recvbuf + ip_len);
  // data_len = (recv_len - ip_len);

// printf("delete %d", data_len);
  //timeout = 0;
  // if data len is set cancel the sigalarm

/*
EXAMPLE output
ttl? ip          time of 3 icmp requests made with the same corresponding ttl
1 130.58.68.1 0.193 ms 0.197 ms 0.205 ms
*/
}
//inner while loop bracket

int same = 0;

// const char* ip1 = hostname;

// const char* ip1 = inet_ntoa((struct in_addr)addr->sin_addr);
const char* ip1 = original;
const char* ip2 = inet_ntoa(ip->ip_src);

//printf("%s,%s\n", original,inet_ntoa(ip->ip_src));

unsigned char s1, s2, s3, s4;
unsigned int uip1, uip2;

sscanf(ip1,"%hhu.%hhu.%hhu.%hhu",&s1,&s2,&s3,&s4);
uip1 = (s1<<24) | (s2<<16) | (s3<<8) | s4; //store all values in 32bits unsigned int

sscanf(ip2,"%hhu.%hhu.%hhu.%hhu",&s1,&s2,&s3,&s4);
uip2 = (s1<<24) | (s2<<16) | (s3<<8) | s4;

if (uip1 == uip2)
{
  same = 1;
}

// end of move this to new sig alarm function (sig alarm does this make a new thread?)
if(currentTTL == maxTTL ||  same == 1){ // if the max ttl is hit or the destination ip is reached stop running
  keepRunning = 0;
  if(same == 1){
    getaddrinfo(inet_ntoa(ip->ip_src), NULL, NULL, &ai);
    //printf("%d bytes from %s\n", data_len, inet_ntoa(ip->ip_src));
    printf(" %d %s (%s) %.3f ms %.3f ms %.3f ms\n",currentTTL ,ai->ai_canonname ? ai->ai_canonname : inet_ntoa(ip->ip_src), inet_ntoa(ip->ip_src) , tryTime[0], tryTime[1], tryTime[2]);
  }
}
else if(timeout == 0){
//  printf("timeoutval:%d", timeout);
  getaddrinfo(inet_ntoa(ip->ip_src), NULL, NULL, &ai);
  //printf("%d bytes from %s\n", data_len, inet_ntoa(ip->ip_src));
  printf(" %d %s (%s) %.3f ms %.3f ms %.3f ms\n",currentTTL ,ai->ai_canonname ? ai->ai_canonname : inet_ntoa(ip->ip_src), inet_ntoa(ip->ip_src) , tryTime[0], tryTime[1], tryTime[2]);
  currentTTL ++;

    currentTry3 = 0;
}
else{
  if(currentTTL < 10){
  printf(" %d ",currentTTL);
}
else {
    printf("%d ",currentTTL);
}
  while(timeout > 0){
  printf("* ");
  timeout--;
}
printf("\n");
currentTTL ++;
    currentTry3 = 0;
    timeout = 0;
}
}
  return 0;
}
