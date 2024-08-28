// ��C�ǿ�t�v�]�m��9600�A�L�P��
// ����11.0592MHz�A�o�e�M�����ϥΪ��榡�ۦP
#include<reg52.h>  
#include <stdio.h>
#include <string.h>                       
#define MAX 20
#define DataPort P0 // �K�ӤC�q��ܾ�
#define KeyPort  P1 // �x�}��L 

unsigned char TempData[10]; // �s�x��ܭȪ������ܼ�
sbit LATCH1=P2^2; // �w�q��s�ϯ�ݤf �q��s
sbit LATCH2=P2^3; //                 ����s
sbit RELAY1=P2^7;
unsigned char code dofly_DuanMa[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c}; // ��ܬq�X��0~9,A,B (�@�� '1'�|�G)
unsigned char code dofly_WeiMa[]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};

typedef unsigned char byte; // 8 bits 
typedef unsigned int  word; // 16 bits

byte buf[MAX];
byte head = 0;

byte get_0d = 0; // �T���榡: 0d0a \r\n 
byte rec_flag = 0; // �O�_������T�� 

// �禡�ŧi
void SendStr(unsigned char *s);
unsigned char KeyScan(void);
unsigned char KeyPro(void);

// ��f��l��
void InitUART (void) {
    SCON  = 0x50;		        // SCON: �Ҧ� 1, 8-bit UART, �ϯ౵��  
    TMOD |= 0x21;               // TMOD: timer 1, mode 2, 8-bit ����  timer 2, mode 1�A16�줸�p�ɾ�
    TH1   = 0xFD;               // TH1:  ���˭� 9600 ��C�ǿ�t�v ���� 11.0592MHz  
    TR1   = 1;                  // TR1:  timer 1 ���}    
    
    ET0 = 1;           //�p�ɾ����_���}
    TF0 = 0; // TF0: timer 0 ���_����
	TR0 = 1; // TR0: timer 0 ���} 
                         
    EA    = 1;                  //���}�`���_
   // ES    = 1;                //���}��f���_
}                            

void DelayUs2x(unsigned char t){   
    while(--t);
}

void DelayMs(unsigned char t){  
    while(t--){
        //�j�P����1mS
        DelayUs2x(245);
        DelayUs2x(245);
    }
}

void Display(unsigned char FirstBit,unsigned char Num){
    static unsigned char i=0;
	
	DataPort=0;   //�M�Ÿ�ơA���������v
    LATCH1=1;     //�q��s
    LATCH1=0;

    DataPort=dofly_WeiMa[i+FirstBit]; //����X 
    LATCH2=1;     //����s
    LATCH2=0;

    DataPort=TempData[i]; //����ܸ�ơA�q�X
    LATCH1=1;     //�q��s
    LATCH1=0;
       
	i++;
    if(i==Num)
	    i=0;
}

// ��s�C�q��ܾ� 
void Timer0_ISR() interrupt 1 {
    TF0 = 0;
    TH0=(65536-2000)/256; //���s��� 2ms
    TL0=(65536-2000)%256;
    Display(0,8); // �եμƽX�ޱ��y
}


void main (void){
    unsigned char j, num;
    unsigned char pos=0; // �K�X��ܦ�m 
    unsigned char temp[8]; // �Ȧs�q�X 
	unsigned char temp_pw[8]; // �Ȧs�K�X(�Ʀr) 
	unsigned char password[4]; // 8051�ϥΪ̳]���K�X
	unsigned char guess[4]; // ESP32�ǿ�L�Ӫ��K�X 
	
    InitUART();
    SendStr("UART test");
    
    ES = 1; // ���}��f���_
	while (1) {
	    num = KeyPro();
		if (rec_flag == 1) { 
		    unsigned char cnt_num = 0; // �O�_������|�ӼƦr 
			buf[head] = '\0'; // C������r�굲�� 
			
			for(j=0;j<4;j++)
			    if(((buf[j] - '0') >= 0) && ((buf[j] - '0') <= 9))
			        cnt_num += 1;
			// ��l�� �i���s�ǰe��� 
			rec_flag = 0;
			head = 0;
		    if(cnt_num == 4){
		        for(j=0;j<4;j++)
		          guess[j] = buf[j] - '0';
            }
            if(cnt_num == 2){
                TempData[0]=dofly_DuanMa[buf[0] - '0']; 
                TempData[1]=dofly_DuanMa[10]; 
                TempData[2]=dofly_DuanMa[buf[2] - '0'];
                TempData[3]=dofly_DuanMa[11]; 
            } 
		}
        if(num!=0xff){ // ���U���s  
            if(num >= 0 && num <=9){ // ��J�K�X 
                if(pos<4){  // ��ܥ|��K�X 
                    temp[pos]=dofly_DuanMa[num];
                    temp_pw[pos]=num;
        	        for(j=0;j<=pos;j++)
                        TempData[3-pos+j]=temp[j];
                }
        	    pos++;
        	    if(pos==5){ // �K�X�W�L�|�쭫�s�]�w
        	       pos=0;
        	  	    for(j=0;j<4;j++)
                        TempData[j]=0;
                }
            } 
            if(num == 10){ // �]�w�K�X 
                for(j=0;j<4;j++) {
                    password[j] = temp_pw[j];
                    TempData[j] = 0x00;
                    TempData[j+4] = dofly_DuanMa[temp_pw[j]];
                }
            }
            if(num == 11){ // ���K�X 
                // for(j=0;j<4;j++) // ����: �K�X�O�_���T����(ESP32 UART) 
                    // TempData[j+4]=dofly_DuanMa[guess[j]];
                unsigned char hint[4];
                unsigned int a = 0, b = 0;
                unsigned int passwordCount[10], guessCount[10];
                // ��l�� 
                for(j=0;j<10;j++){
                    passwordCount[j] = 0;
                    guessCount[j] = 0;
                }
                // �������T�K�X 
                for(j=0;j<4;++j) {
                    if (password[j] == guess[j]) {
                        a++;
                    } else {
                        passwordCount[password[j]]++;
                        guessCount[guess[j]]++;
                    }
                }
                // ������m���~�K�X 
                for (j=0;j<10;++j) 
                    b += passwordCount[j] < guessCount[j] ? passwordCount[j] : guessCount[j];
                // �榡�ƿ�X�r�� 
                hint[0] = a + '0';
                hint[1] = 'A';
                hint[2] = b + '0';
                hint[3] = 'B';
                hint[4] = '\0'; // �]�w�r�굲����
                TempData[0]=dofly_DuanMa[a]; 
                TempData[1]=dofly_DuanMa[10]; 
                TempData[2]=dofly_DuanMa[b];
                TempData[3]=dofly_DuanMa[11]; 
                if(a != 4){
                    RELAY1 = !RELAY1; // �K�X���~���ܭ�
                    DelayUs2x(250);
                    RELAY1 = !RELAY1;
                    DelayUs2x(250);
                } 
                SendStr(hint);
            }
            if(num==12){ // �ǰe�K�X�� ESP32
                unsigned char pw2uart[6];
                for(j=0;j<4;j++) {
                    password[j] = temp_pw[j];
                    TempData[j] = 0x00;
                    TempData[j+4] = dofly_DuanMa[temp_pw[j]];
                }
                // �榡�ƿ�X�r��
                pw2uart[0] = 'C'; 
                pw2uart[1] = 'K';
                pw2uart[2] = temp_pw[0] + '0';
                pw2uart[3] = temp_pw[1] + '0';
                pw2uart[4] = temp_pw[2] + '0';
                pw2uart[5] = temp_pw[3] + '0';
                pw2uart[6] = '\0'; // �]�w�r�굲����
                SendStr(pw2uart);
            }
        }	
	}
}

// �o�e�@�Ӧ줸��
void SendByte(unsigned char dat){
    SBUF = dat;
    while(!TI);
    TI = 0;
}

// �o�e�@�Ӧr��
void SendStr(unsigned char *s){
    while(*s!='\0') {// \0 ��ܦr�굲���лx�A�q�L�˴��O�_�r�꥽��
        SendByte(*s);
        s++;
    }
}

// ��f���_�{��
void UART_SER (void) interrupt 4 { //��C���_�A�ȵ{��
    unsigned char tmp;             //�w�q�{���ܼ� 
    if(RI){                        //�P�_�O�������_����
		RI=0;                      //�лx�줸�M�s
		tmp=SBUF;                  //Ū�J�w�İϪ���
		if (get_0d == 0){
			if (tmp == 0x0d) get_0d = 1;
			else{
				buf[head]=tmp;              
				head++;
				if (head == MAX) head = 0;	
			}				     
		}
		else if (get_0d == 1){
		  if (tmp != 0x0a){
				head = 0;
				get_0d = 0;		
			}
			else {
				rec_flag = 1; // �w�����짹���� 
				get_0d = 0;
			}
		}	
		//	SBUF=tmp;                 //�Ⱶ���쪺�ȦA�o�^�q����
	 }
//   if(TI)                        //�p�G�O�o�e�лx�줸�A�M�s
//     TI=0;
}


// ��L 
unsigned char KeyScan(void){  //��L���˨�ơA�ϥΦ�C�v�ű��˪k
    unsigned char Val;
    KeyPort=0xf0;//���|��m���A�C�|��ԧC
    if(KeyPort!=0xf0){ //��ܦ�������U
        DelayMs(10); //�h��
        if(KeyPort!=0xf0){ //��ܦ�������U         
            KeyPort=0xfe; //�˴��Ĥ@��
            /* ���ͲզX�X */
            if(KeyPort!=0xfe){
                /* KeyPort�e4bits */
                Val=KeyPort&0xf0;
                Val+=0x0e;
                while(KeyPort!=0xfe);
                DelayMs(10); //�h��
                while(KeyPort!=0xfe);
                return Val;
            }
            KeyPort=0xfd; //�˴��ĤG��
            if(KeyPort!=0xfd){
                Val=KeyPort&0xf0;
                Val+=0x0d;
                while(KeyPort!=0xfd);
                DelayMs(10); //�h��
                while(KeyPort!=0xfd);
                return Val;
            }
            KeyPort=0xfb; //�˴��ĤT��
            if(KeyPort!=0xfb){
                Val=KeyPort&0xf0;
                Val+=0x0b;
                while(KeyPort!=0xfb);
                DelayMs(10); //�h��
                while(KeyPort!=0xfb);
                return Val;
            }
            KeyPort=0xf7; //�˴��ĥ|��
            if(KeyPort!=0xf7){
                Val=KeyPort&0xf0;
                Val+=0x07;
                while(KeyPort!=0xf7);
                DelayMs(10); //�h��
                while(KeyPort!=0xf7);
                return Val;
            }
        }
    }
    return 0xff;
}

unsigned char KeyPro(void){
    switch(KeyScan()){
        case 0x7e:return 0;break;//0 ���U����������ܬ۹������X��
        case 0x7d:return 1;break;//1
        case 0x7b:return 2;break;//2
        case 0x77:return 3;break;//3
        case 0xbe:return 4;break;//4
        case 0xbd:return 5;break;//5
        case 0xbb:return 6;break;//6
        case 0xb7:return 7;break;//7
        case 0xde:return 8;break;//8
        case 0xdd:return 9;break;//9
        case 0xdb:return 10;break;//a
        case 0xd7:return 11;break;//b
        case 0xee:return 12;break;//c
        case 0xed:return 13;break;//d
        case 0xeb:return 14;break;//e
        case 0xe7:return 15;break;//f
        default:return 0xff;break;
    }
}