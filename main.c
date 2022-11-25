#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/types.h>

// check ICMP checksum
unsigned short checksum(unsigned short *buf, int bufsz){
    unsigned long sum = 0xffff;
    while(bufsz > 1){
        sum += *buf;
        buf++;
        bufsz -= 2;
    }
    if(bufsz == 1)
        sum += *(unsigned char*)buf;
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}

int main(int argc , char *argv[])
{
	struct sockaddr_in IP_recv; // receive IP
	struct sockaddr_in IPheader_send; // send IP header
	struct icmphdr ICMPheader_send; // send ICMP header
	struct icmphdr *ICMPheader_recv; // receive ICMP header
    struct iphdr *IPheader_recv; // receive IP header

	int sockfd_recv = 0, sockfd_send = 0;
	int time=1;
	int IP_recv_size=sizeof(IP_recv);

	char buffer[1024];

    //check root權限
	if(geteuid() != 0){
		printf("ERROR: You must be root to use this tool!\n");
		exit(1);
	}

    //open send socket
	if((sockfd_send = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	{
		perror("open send socket error");
		exit(1);
	}

    //open receive socket
	if((sockfd_recv = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)	{
		perror("open recv socket error");
		exit(1);
	}

    //check argv[2] is ip address?
    if (argc==3 && strlen(argv[2])>=7 && strlen(argv[2])<=15){

        //送封包設定
        //初始化ip header
        memset(&IPheader_send, 0, sizeof(IPheader_send));

        //設定ip header
		IPheader_send.sin_family = AF_INET;
		IPheader_send.sin_addr.s_addr = inet_addr(argv[2]);

        //設定ip header的TTL欄位
		setsockopt(sockfd_send, IPPROTO_IP, IP_TTL, (char *)&time, sizeof(IP_TTL));

		//初始化ICMP header
		memset(&ICMPheader_send, 0, sizeof(ICMPheader_send));

        //設定ICMP header
		ICMPheader_send.type = 8; //echo request
		ICMPheader_send.code = 0;
		ICMPheader_send.un.echo.id = 0;
		ICMPheader_send.un.echo.sequence = 0;
		ICMPheader_send.checksum = checksum((unsigned short*)&ICMPheader_send, sizeof(ICMPheader_send));

		//送ICMP echo request
		if(sendto(sockfd_send, (char*)&ICMPheader_send, sizeof(ICMPheader_send), 0, (struct sockaddr*)&IPheader_send, sizeof(IPheader_send))<0){
			perror("sendto");
			exit(1);
		}
		else{
			printf("%d) Send an ICMP echo request packet to %s\n", time, argv[2]);
		}

        while(1){
			//初始buffer
			memset(buffer, 0, sizeof(buffer));

            //收ICMP封包
			if(recvfrom(sockfd_recv,buffer,sizeof(buffer),0,(struct sockaddr *)&IP_recv,&IP_recv_size)<0){
				perror("recv");
				exit(1);
			}

            //取出ip header
			IPheader_recv = (struct iphdr*)buffer;
			//取出ICMP header
            ICMPheader_recv = (struct icmphdr*)(buffer+(IPheader_recv->ihl)*4);

            //ICMP type 0 echo reply
			if(ICMPheader_recv->type == 0 && ICMPheader_recv->code == 0){
				printf("TTL = %d\n",time);
				printf("The host %s is alive!\n",argv[2]);
				break;
			}
            //ICMP type 11 time exceeded
			else if(ICMPheader_recv->type == 11 && ICMPheader_recv->code == 0){
				printf("TTL = %d\n",time);
				printf("The router ip is %s\n", inet_ntoa(IP_recv.sin_addr));
                printf("The packet send to %s is Time Exceeded!\n",argv[2]);
			}
            else{
                printf("Other type!\n");
				printf("The ICMP type is %d\n", ICMPheader_recv->type);
				printf("The ICMP code is %d\n", ICMPheader_recv->code);
            }

			//check是否大於設定的TTL
			if(time>atoi(argv[1])){
				break;
			}
			else{
				//更新TTL
				time = time + 1;

                //初始化ip header
                memset(&IPheader_send, 0, sizeof(IPheader_send));

                //設定ip header
                IPheader_send.sin_family = AF_INET;
                IPheader_send.sin_addr.s_addr = inet_addr(argv[2]);

                //設定ip header的TTL欄位
                setsockopt(sockfd_send, IPPROTO_IP, IP_TTL, (char *)&time, sizeof(IP_TTL));

                //初始化ICMP header
                memset(&ICMPheader_send, 0, sizeof(ICMPheader_send));

                //設定ICMP header
                ICMPheader_send.type = 8; //echo request
                ICMPheader_send.code = 0;
                ICMPheader_send.un.echo.id = 0;
                ICMPheader_send.un.echo.sequence = 0;
                ICMPheader_send.checksum = checksum((unsigned short*)&ICMPheader_send, sizeof(ICMPheader_send));

                //送ICMP echo request
                if(sendto(sockfd_send, (char*)&ICMPheader_send, sizeof(ICMPheader_send), 0, (struct sockaddr*)&IPheader_send, sizeof(IPheader_send))<0){
                    perror("sendto");
                    exit(1);
                }
                else{
                    printf("%d) Send an ICMP echo request packet to %s\n", time, argv[2]);
                }
			}


        }

    }
    else{
        printf("Command error!\n");
    }
	return 0;
}
