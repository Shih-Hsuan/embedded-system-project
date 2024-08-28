// 串列傳輸速率設置為9600，無同位
// 晶振11.0592MHz，發送和接收使用的格式相同
#include<reg52.h>  
#include <stdio.h>
#include <string.h>                       
#define MAX 20
#define DataPort P0 // 八個七段顯示器
#define KeyPort  P1 // 矩陣鍵盤 

unsigned char TempData[10]; // 存儲顯示值的全域變數
sbit LATCH1=P2^2; // 定義鎖存使能端口 段鎖存
sbit LATCH2=P2^3; //                 位鎖存
sbit RELAY1=P2^7;
unsigned char code dofly_DuanMa[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c}; // 顯示段碼值0~9,A,B (共陰 '1'會亮)
unsigned char code dofly_WeiMa[]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};

typedef unsigned char byte; // 8 bits 
typedef unsigned int  word; // 16 bits

byte buf[MAX];
byte head = 0;

byte get_0d = 0; // 訊息格式: 0d0a \r\n 
byte rec_flag = 0; // 是否接收到訊息 

// 函式宣告
void SendStr(unsigned char *s);
unsigned char KeyScan(void);
unsigned char KeyPro(void);

// 串口初始化
void InitUART (void) {
    SCON  = 0x50;		        // SCON: 模式 1, 8-bit UART, 使能接收  
    TMOD |= 0x21;               // TMOD: timer 1, mode 2, 8-bit 重裝  timer 2, mode 1，16位元計時器
    TH1   = 0xFD;               // TH1:  重裝值 9600 串列傳輸速率 晶振 11.0592MHz  
    TR1   = 1;                  // TR1:  timer 1 打開    
    
    ET0 = 1;           //計時器中斷打開
    TF0 = 0; // TF0: timer 0 中斷關閉
	TR0 = 1; // TR0: timer 0 打開 
                         
    EA    = 1;                  //打開總中斷
   // ES    = 1;                //打開串口中斷
}                            

void DelayUs2x(unsigned char t){   
    while(--t);
}

void DelayMs(unsigned char t){  
    while(t--){
        //大致延時1mS
        DelayUs2x(245);
        DelayUs2x(245);
    }
}

void Display(unsigned char FirstBit,unsigned char Num){
    static unsigned char i=0;
	
	DataPort=0;   //清空資料，防止有交替重影
    LATCH1=1;     //段鎖存
    LATCH1=0;

    DataPort=dofly_WeiMa[i+FirstBit]; //取位碼 
    LATCH2=1;     //位鎖存
    LATCH2=0;

    DataPort=TempData[i]; //取顯示資料，段碼
    LATCH1=1;     //段鎖存
    LATCH1=0;
       
	i++;
    if(i==Num)
	    i=0;
}

// 更新七段顯示器 
void Timer0_ISR() interrupt 1 {
    TF0 = 0;
    TH0=(65536-2000)/256; //重新賦值 2ms
    TL0=(65536-2000)%256;
    Display(0,8); // 調用數碼管掃描
}


void main (void){
    unsigned char j, num;
    unsigned char pos=0; // 密碼顯示位置 
    unsigned char temp[8]; // 暫存段碼 
	unsigned char temp_pw[8]; // 暫存密碼(數字) 
	unsigned char password[4]; // 8051使用者設的密碼
	unsigned char guess[4]; // ESP32傳輸過來的密碼 
	
    InitUART();
    SendStr("UART test");
    
    ES = 1; // 打開串口中斷
	while (1) {
	    num = KeyPro();
		if (rec_flag == 1) { 
		    unsigned char cnt_num = 0; // 是否接收到四個數字 
			buf[head] = '\0'; // C的完整字串結尾 
			
			for(j=0;j<4;j++)
			    if(((buf[j] - '0') >= 0) && ((buf[j] - '0') <= 9))
			        cnt_num += 1;
			// 初始化 可重新傳送資料 
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
        if(num!=0xff){ // 按下按鈕  
            if(num >= 0 && num <=9){ // 輸入密碼 
                if(pos<4){  // 顯示四位密碼 
                    temp[pos]=dofly_DuanMa[num];
                    temp_pw[pos]=num;
        	        for(j=0;j<=pos;j++)
                        TempData[3-pos+j]=temp[j];
                }
        	    pos++;
        	    if(pos==5){ // 密碼超過四位重新設定
        	       pos=0;
        	  	    for(j=0;j<4;j++)
                        TempData[j]=0;
                }
            } 
            if(num == 10){ // 設定密碼 
                for(j=0;j<4;j++) {
                    password[j] = temp_pw[j];
                    TempData[j] = 0x00;
                    TempData[j+4] = dofly_DuanMa[temp_pw[j]];
                }
            }
            if(num == 11){ // 比對密碼 
                // for(j=0;j<4;j++) // 測試: 密碼是否正確接收(ESP32 UART) 
                    // TempData[j+4]=dofly_DuanMa[guess[j]];
                unsigned char hint[4];
                unsigned int a = 0, b = 0;
                unsigned int passwordCount[10], guessCount[10];
                // 初始化 
                for(j=0;j<10;j++){
                    passwordCount[j] = 0;
                    guessCount[j] = 0;
                }
                // 紀錄正確密碼 
                for(j=0;j<4;++j) {
                    if (password[j] == guess[j]) {
                        a++;
                    } else {
                        passwordCount[password[j]]++;
                        guessCount[guess[j]]++;
                    }
                }
                // 紀錄位置錯誤密碼 
                for (j=0;j<10;++j) 
                    b += passwordCount[j] < guessCount[j] ? passwordCount[j] : guessCount[j];
                // 格式化輸出字串 
                hint[0] = a + '0';
                hint[1] = 'A';
                hint[2] = b + '0';
                hint[3] = 'B';
                hint[4] = '\0'; // 設定字串結束符
                TempData[0]=dofly_DuanMa[a]; 
                TempData[1]=dofly_DuanMa[10]; 
                TempData[2]=dofly_DuanMa[b];
                TempData[3]=dofly_DuanMa[11]; 
                if(a != 4){
                    RELAY1 = !RELAY1; // 密碼錯誤提示音
                    DelayUs2x(250);
                    RELAY1 = !RELAY1;
                    DelayUs2x(250);
                } 
                SendStr(hint);
            }
            if(num==12){ // 傳送密碼給 ESP32
                unsigned char pw2uart[6];
                for(j=0;j<4;j++) {
                    password[j] = temp_pw[j];
                    TempData[j] = 0x00;
                    TempData[j+4] = dofly_DuanMa[temp_pw[j]];
                }
                // 格式化輸出字串
                pw2uart[0] = 'C'; 
                pw2uart[1] = 'K';
                pw2uart[2] = temp_pw[0] + '0';
                pw2uart[3] = temp_pw[1] + '0';
                pw2uart[4] = temp_pw[2] + '0';
                pw2uart[5] = temp_pw[3] + '0';
                pw2uart[6] = '\0'; // 設定字串結束符
                SendStr(pw2uart);
            }
        }	
	}
}

// 發送一個位元組
void SendByte(unsigned char dat){
    SBUF = dat;
    while(!TI);
    TI = 0;
}

// 發送一個字串
void SendStr(unsigned char *s){
    while(*s!='\0') {// \0 表示字串結束標誌，通過檢測是否字串末尾
        SendByte(*s);
        s++;
    }
}

// 串口中斷程式
void UART_SER (void) interrupt 4 { //串列中斷服務程式
    unsigned char tmp;             //定義臨時變數 
    if(RI){                        //判斷是接收中斷產生
		RI=0;                      //標誌位元清零
		tmp=SBUF;                  //讀入緩衝區的值
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
				rec_flag = 1; // 已接收到完整資料 
				get_0d = 0;
			}
		}	
		//	SBUF=tmp;                 //把接收到的值再發回電腦端
	 }
//   if(TI)                        //如果是發送標誌位元，清零
//     TI=0;
}


// 鍵盤 
unsigned char KeyScan(void){  //鍵盤掃瞄函數，使用行列逐級掃瞄法
    unsigned char Val;
    KeyPort=0xf0;//高四位置高，低四位拉低
    if(KeyPort!=0xf0){ //表示有按鍵按下
        DelayMs(10); //去抖
        if(KeyPort!=0xf0){ //表示有按鍵按下         
            KeyPort=0xfe; //檢測第一行
            /* 產生組合碼 */
            if(KeyPort!=0xfe){
                /* KeyPort前4bits */
                Val=KeyPort&0xf0;
                Val+=0x0e;
                while(KeyPort!=0xfe);
                DelayMs(10); //去抖
                while(KeyPort!=0xfe);
                return Val;
            }
            KeyPort=0xfd; //檢測第二行
            if(KeyPort!=0xfd){
                Val=KeyPort&0xf0;
                Val+=0x0d;
                while(KeyPort!=0xfd);
                DelayMs(10); //去抖
                while(KeyPort!=0xfd);
                return Val;
            }
            KeyPort=0xfb; //檢測第三行
            if(KeyPort!=0xfb){
                Val=KeyPort&0xf0;
                Val+=0x0b;
                while(KeyPort!=0xfb);
                DelayMs(10); //去抖
                while(KeyPort!=0xfb);
                return Val;
            }
            KeyPort=0xf7; //檢測第四行
            if(KeyPort!=0xf7){
                Val=KeyPort&0xf0;
                Val+=0x07;
                while(KeyPort!=0xf7);
                DelayMs(10); //去抖
                while(KeyPort!=0xf7);
                return Val;
            }
        }
    }
    return 0xff;
}

unsigned char KeyPro(void){
    switch(KeyScan()){
        case 0x7e:return 0;break;//0 按下相應的鍵顯示相對應的碼值
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