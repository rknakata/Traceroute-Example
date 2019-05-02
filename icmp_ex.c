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

#include "checksum.h" //my checksum library

#define BUFSIZE 1500 //1500 MTU (so within one frame in layer 2)
#define PROTO_ICMP 1
#define NI_MAXHOST 1025 // http://www.microhowto.info/howto/convert_an_ip_address_to_the_corresponding_domain_name_in_c.html

char hostname[NI_MAXHOST] = "";
int icmpReqCount = 0; //this needs to be done 3 times for each ttl
//https://stackoverflow.com/questions/6970224/providing-passing-argument-to-signal-handler

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


/*
need to test this program without a nat nat blocks icmp time exceeded reply from router

I need to find out where the packet send and recive sigaalarm caller should go
do i use sigalarm to call the packet function as well as use it to determing a 3 second timeout on recieve?

i need toincrement the ttl incrementer

i do not need sigint when i ctrl c on real traceroute nothing is printed

I need to answer the questions in the readme
*/


int main(int argc, char * argv[]){
  char sendbuf[BUFSIZE], recvbuf[BUFSIZE], controlbuf[BUFSIZE];
  struct icmp * icmp;
  struct ip * ip;
  int sockfd;
  int packet_len, recv_len, ip_len, data_len;
  struct addrinfo * ai;
  struct iovec iov;
  struct msghdr msg;


  //process addr info
  // http://man7.org/linux/man-pages/man3/gai_strerror.3.html
  // this gets information about ip
  //getaddrinfo(argv[1], NULL, NULL, &ai);

  /* resolve the domain name into a list of addresses */
  error = getaddrinfo(argv[1], NULL, NULL, &ai);
  if (error != 0)
  {
      fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
      return EXIT_FAILURE;
  }

  //fix this later
  /* loop over all returned results and do inverse lookup */
  for (res = ai; res != NULL; res = res->ai_next)
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

  printf("%d bytes from %s (%s): icmp_req=%d ttl=%d time=%.1f ms\n", data_len, hostname, inet_ntoa(ip->ip_src), icmpReqCount, reply_ttl, delayFloat);
  if(firstRun == 0){
    firstRun = 1; // this message wont print again
  }


// sample output
//sudo traceroute -I example.com
//traceroute to example.com (158.130.69.89), 30 hops max, 60 byte packets

  //process destination address
  printf("Dest: %s\n", ai->ai_canonname ? ai->ai_canonname : argv[1]);

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

  //compute checksum
  icmp->icmp_cksum = checksum((unsigned short *) icmp, sizeof(struct icmp));

  packet_len = sizeof(struct icmp);

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

  //recv the reply
  // according to pdf i need to set a 3 second timer i also think i need to look for icmp timeout response packet_len
  if((recv_len = recvmsg(sockfd, &msg, 0)) < 0){ //could get interupted ??
    perror("recvmsg");
    exit(1);
  }

//  printf("%d\n",recv_len);

  ip = (struct ip*) recvbuf;
  ip_len = ip->ip_hl << 2; //length of ip header

  icmp = (struct icmp *) (recvbuf + ip_len);
  data_len = (recv_len - ip_len);
/*
EXAMPLE output
ttl? ip          time of 3 icmp requests made with the same corresponding ttl
1 130.58.68.1 0.193 ms 0.197 ms 0.205 ms
*/
  printf("%d bytes from %s\n", data_len, inet_ntoa(ip->ip_src));

// end of move this to new sig alarm function (sig alarm does this make a new thread?)


  return 0;
}
