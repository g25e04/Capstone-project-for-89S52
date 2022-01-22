#include "8051.h"
#define uint unsigned int
#define uchar unsigned char
#define sda P1_0
#define scl P1_1
#define SLAVE 0x4E //LCM1602 address define

#define DO P2_0 /* 93c66's serial output pin*/
#define DI P2_1 /* 93c66's serial input pin*/
#define SK P2_2 /* 93c66's serial pulse wave input pin */
#define CS P2_3 /* 93c66's enable input pin */
#define BUZ P2_4 /* Buzzer */
#define LED P1_4 /* LED */
#define MAG P0_0
#define ON 0
#define OFF 1

#define SIZE 12 /* password length in 93c66 */
#define MINE 4
__sbit __at 0xa0 P2_0;
__sbit __at 0xa1 P2_1;
__sbit __at 0xa2 P2_2;
__sbit __at 0xa3 P2_3;
__sbit __at 0xa4 P2_4;
__sbit __at 0xa5 P2_5;
__sbit __at 0xa6 P2_6;
__sbit __at 0xa7 P2_7; 
__bit flag;
__bit error; /* compare password */
__bit checkTime; /* 1 is on*/
char ScanLine=0x08; /* keyboard scan */
char col=0;
char row=0;
char one=0; /* has released button been detected times */
char zero=0; /* has pushed button been detected times */
uint key=0; /* button value */
uint keyTemp=0xff; /*key value that not been debounce*/
uint keyData=0xff; /* key value that been debounce */
char secCnt=100; /* 1sec=100*10ms */
char HOUR=12; /* initial value */
char MIN=00;
char SEC=00;
char TIME[8]="00:00:00"; /* output string for time */
char AHOUR; /* a temp for change hour */
char AMIN;
char ASEC;
char ATIME[6];

char SHOUR=11;
char SMIN=00;

char EHOUR=13;
char EMIN=00;

char USER_CODE[MINE];
char INIT_CODE[SIZE]={1,2,3,4, 5,6,7,8, 5,1,3,6}; /* initial password */
char SYS_CODE[SIZE]; /* a temp for password */
unsigned char __code MSG1[]="F1:TIME   F2:ADJ"; /* length is 16 */
unsigned char __code MSG2[]="F3:INPUT  F4:SET";
unsigned char __code MSG3[]="Current Time"; /* 12 */
unsigned char __code MSG4[]="Adjust Time"; /* 11 */
unsigned char __code MSG5[]="Input Code"; /* 10 */
unsigned char __code MSG6[]="Exact";
unsigned char __code MSG7[]="Error";
unsigned char __code MSG8[]="Input old code"; /* 14 */
unsigned char __code MSG9[]="input new code";
unsigned char __code MSG10[]="Success!"; /* 8 */
unsigned char __code MSG11[]="SYS:0 MONITOR:1"; /* 15 */
unsigned char __code MSG12[]="Start Time:"; /* 11 */
unsigned char __code MSG13[]="End Time:"; /* 9 */

unsigned char FreqCount; /* Frequence */
unsigned char PeriodCount; /* Period(Cycle) */

void delay8us(unsigned char);

void delay(unsigned int dl);
void start(void);
void send_8bits(unsigned char strg);
void ack(void);
void stop(void);
void WriteInst4bits(unsigned char inst_4b);
void WriteInst(unsigned char inst);
void WriteData(unsigned char data_);
void WriteString(unsigned char count, unsigned char MSG[]);
void initial(void);


void Delay(void);
void KeyScan(void);


void ConvertTime(void);
void AdjustTime(void);

void  delay_time(unsigned int k);
void  clock(void);
void  address(unsigned  char  addr);
void  write(unsigned char address,char EEPROM_data); 
char  read(unsigned char address);
void  EWEN(void);
void  EWDS(void);
void  ERAL(void);

void speaker(void);

void main(){
	uchar i,j,z,w;  /*i,j is index, z is detect whether it's true, w means who number */
	IE=0x8a; /* enable timer 0,1 interrupt */
	TMOD=0x11; /* timer 0,1 work in mode 1 */
	TH0=(65536-10000)/256; /* set timer0's initial value ,10ms/1us=10000*/
	TL0=(65536-10000)%256;
	TH1=(65536-5000)/256; /* set timer1's initial value ,5ms/1us=5000*/
	TH1=(65536-5000)%256;
	TR0=1; /* start up timer0 */
	TR1=1;
	BUZ=1;
	
	initial(); /* LCD show function */
	LED=1;
	WriteInst(0x80); //set cursor to (0,0)
	WriteString(sizeof(MSG1)-1, MSG1);
	
	WriteInst(0xC0); //set cursor to (1,0)
	WriteString(sizeof(MSG2)-1, MSG2);
	
	EWEN();              
// 93c46 write enable
	delay_time(1000);
 	ERAL();              
// 93c46 clear all
	delay_time(1000);
 	EWDS();              
// 93c46 write disable
	//delay_time(1000);
       
       
 	EWEN();              //write enable
  	delay_time(1000);	
	//write
	for(i=0;i<SIZE;i++)
		write(i,INIT_CODE[i]);
	EWEN();		     //write disable
	delay_time(1000);
	
	
	for(i=0;i<SIZE;i++){
		SYS_CODE[i]=read(i);
		delay_time(30000);
		delay_time(30000);
	}
	error=1; /* default is fault */
	checkTime=1;
	while(1)
	{
		ConvertTime();
		if(SHOUR>EHOUR){
			if(HOUR>SHOUR || HOUR< EHOUR)
				checkTime=1;
			else if(HOUR == SHOUR && MIN>SMIN)
				checkTime=1;
			else if(HOUR == EHOUR && MIN<EMIN)
				checkTime=1;
			else
				checkTime=0;
		}
		else if(SHOUR<EHOUR){
			if(HOUR>SHOUR && HOUR< EHOUR)
				checkTime=1;
			else if(HOUR == SHOUR && MIN>SMIN)
				checkTime=1;
			else if(HOUR == EHOUR && MIN<EMIN)
				checkTime=1;
			else
				checkTime=0;
		}
		else{
			if(SMIN!=EMIN && (MIN>SMIN || MIN<EMIN))
				checkTime=1;
			else if(SMIN==EMIN)
				checkTime=1;
			else
				checkTime=0;
		}
				
		if(error && MAG && checkTime==1) /* if not true and door is open,ring the bell */
		{
			
			FreqCount=120;
			PeriodCount=100;
			LED=0;
			speaker();								
			delay_time(30000);
			LED=1;
			for(i=0;i<60;i++){
			BUZ=0;
			delay8us(229);
			BUZ=1;
			delay8us(229);
			BUZ=0;
			}
		}
		
		if(keyData!=0xff) /* detect whether button been pushed */
		{
			if(keyData==0x0b) /* push help */
			{
				keyData=0xff;				
				
				delay(300);
				WriteInst(0x80); //set cursor to (0,0)
				WriteString(sizeof(MSG1)-1, MSG1);
				WriteInst(0xC0); //set cursor to (1,0)
				WriteString(sizeof(MSG2)-1, MSG2);
			}/* end if ==0x0b */
			
			if(keyData==0x0c) /* push F1 */
			{
				keyData=0xff;
				delay(300);
				WriteInst(0x01); /* clear screen */
				WriteInst(0x82); /* set cursor to (0,2) */;
				WriteString(sizeof(MSG3)-1,MSG3); /* show "Current Time" on LCD*/
				while(keyData==0xff)
				{
					ConvertTime();
					WriteInst(0xc4); /* set cursor to (1,4) */
					WriteString(sizeof(TIME),TIME);
				}
				delay(300);
				WriteInst(0x80); //set cursor to (0,0)
				WriteString(sizeof(MSG1)-1, MSG1);
				WriteInst(0xC0); //set cursor to (1,0)
				WriteString(sizeof(MSG2)-1, MSG2);
			}/* end if == 0x0c */
			
			if(keyData==0x0d) /* push F2 */
			{
				i=0;
				keyData=0xff;
				delay(300);
				WriteInst(0x01); /* clear screen */
				WriteInst(0x80); /* set cursor to (0,0) */
				WriteString(sizeof(MSG11)-1,MSG11); /* show "SYS:0 Monitor:1" on LCD*/
				WriteInst(0xc5); /* set cursor to (1,5)*/
				while(keyData==0xff)
					;
				flag=1;
				while(flag)
				{
					if(keyData!=0 || keyData!=1) /* if keyData!=0 or 1 ,end loop*/
						flag=0;
					if(keyData==0) /* if keyData==0 */
					{
						i=0;
						keyData=0xff;
						delay(300);
						WriteInst(0x01); /* clear screen */
						WriteInst(0x82); /* set cursor to (0,2) */
						WriteString(sizeof(MSG4)-1,MSG4); /* show "Adjust Time" on LCD*/
						WriteInst(0xc5); /* set cursor to (1,5)*/
						while(keyData==0xff)
							;
						flag=1;
						while(flag)
						{
							if(keyData==0x0b || keyData==0x0c  || keyData==0x0d || keyData==0x0e  || keyData==0x0f) /* if keyData==function button ,end loop*/
								flag=0;
							if(keyData==0x0a) /* if keyData==set button  */
							{
								flag=0;
								keyData=0xff;
								AHOUR=ATIME[0]*10+ATIME[1];
								AMIN=ATIME[2]*10+ATIME[3];
								ASEC=ATIME[4]*10+ATIME[5];
								if(AHOUR<24 && AMIN<60 && ASEC<60)
								{
									HOUR = AHOUR; /* store time */
									MIN=AMIN;
									SEC=ASEC;
								}
								initial();
								
							}/* end if ==0x0a*/
							if(keyData>=0 && keyData<=9)
							{
								ATIME[i]=keyData; /* save into ATIME */
								WriteData(ATIME[i]+0x30); /* show time on LCD,0x30 for ascii */
								i++;
								if(i==6) /* xx:xx:xx -> 6 number*/
								{
									i=0;
									WriteInst(0xc5); /* set cursor to (1,5) */
								}/* end i==6 */
								keyData=0xff;
							}/* end if keyData is a number */ 
						}/* end while(flag) */	
					}/* end if == 0 */
					
					if(keyData==1) /* if keyData==1 */
					{
						i=0;
						keyData=0xff;
						delay(300);
						WriteInst(0x01); /* clear screen */
						WriteInst(0x82); /* set cursor to (0,2) */
						WriteString(sizeof(MSG12)-1,MSG12); /* show "Start Time:" on LCD*/
						WriteInst(0xc5); /* set cursor to (1,5)*/
						while(keyData==0xff)
							;
						flag=1;
						while(flag)
						{
							if(keyData==0x0b || keyData==0x0c  || keyData==0x0d || keyData==0x0e  || keyData==0x0f) /* if keyData==function button ,end loop*/
								flag=0;
							if(keyData==0x0a) /* if keyData==set button  */
							{
								flag=0;
								keyData=0xff;
								AHOUR=ATIME[0]*10+ATIME[1];
								AMIN=ATIME[2]*10+ATIME[3];
								if(AHOUR<24 && AMIN<60)
								{
									SHOUR = AHOUR; /* start time */
									SMIN=AMIN;
								}
								/*else
								{
									SHOUR = 0;
									SMIN = 0;
								}*/
								i=0;
								keyData=0xff;
								delay(300);
								WriteInst(0x01); /* clear screen */
								WriteInst(0x82); /* set cursor to (0,2) */
								WriteString(sizeof(MSG13)-1,MSG13); /* show "End Time:" on LCD*/
								WriteInst(0xc5); /* set cursor to (1,5)*/
								while(keyData==0xff)
									;
								flag=1;
								while(flag)
								{
									if(keyData==0x0b || keyData==0x0c  || keyData==0x0d || keyData==0x0e  || keyData==0x0f) /* if keyData==function button ,end loop*/
										flag=0;
									if(keyData==0x0a) /* if keyData==set button  */
									{
										flag=0;
										keyData=0xff;
										AHOUR=ATIME[0]*10+ATIME[1];
										AMIN=ATIME[2]*10+ATIME[3];
										
										if(AHOUR<24 && AMIN<60)
										{
											EHOUR = AHOUR; /* end time */
											EMIN=AMIN;
											
										}
										/*else
										{
											EHOUR = 12;
											EMIN = 0;
											
										}*/
										initial();
										
									}/* end if ==0x0a*/
									if(keyData>=0 && keyData<=9)
									{
										ATIME[i]=keyData; /* save into ATIME */
										WriteData(ATIME[i]+0x30); /* show time on LCD,0x30 for ascii */
										i++;
										if(i==4) /* xx:xx:xx -> 6 number*/
										{
											i=0;
											WriteInst(0xc5); /* set cursor to (1,5) */
										}/* end i==6 */
										keyData=0xff;
									}/* end if keyData is a number */ 
								}/* end while(flag) */
								
							}/* end if ==0x0a*/
							if(keyData>=0 && keyData<=9)
							{
								ATIME[i]=keyData; /* save into ATIME */
								WriteData(ATIME[i]+0x30); /* show time on LCD,0x30 for ascii */
								i++;
								if(i==4) /* xx:xx:xx -> 6 number*/
								{
									i=0;
									WriteInst(0xc5); /* set cursor to (1,5) */
								}/* end i==6 */
								keyData=0xff;
							}/* end if keyData is a number */ 
						}/* end while(flag) */
					}/* end if keyData is 1 */
					delay(300);
					WriteInst(0x80); //set cursor to (0,0)
					WriteString(sizeof(MSG1)-1, MSG1);
					WriteInst(0xC0); //set cursor to (1,0)
					WriteString(sizeof(MSG2)-1, MSG2); 
				}/* end while(flag) */				
			}/* end if == 0x0d */
			
			if(keyData==0x0e) /* push F3 */
			{
				i=0;
				j=0;
				keyData=0xff;
				delay(300);
				WriteInst(0x01); /* clear screen */
				WriteInst(0x83); /* set cursor to (0,3) */
				WriteString(sizeof(MSG5)-1,MSG5);
				WriteInst(0xc6); /* set cursor to (1,6) */
				while(keyData==0xff)
					;
				flag=1;
				while(flag)
				{
					if(keyData==0x0b || keyData==0x0c  || keyData==0x0d || keyData==0x0e  || keyData==0x0f)
						flag=0;
					if(keyData==0x0a) /* if keyData == set button*/
					{
						flag=0;
						keyData=0xff;
						error=0; /* 0:password is true		1:password is error*/						
						
						for(j=0;i+MINE*j<SIZE+1;j++)
						{
						z=0;
						for(i=0;i<MINE;i++)
							{
							if(USER_CODE[i]!=SYS_CODE[i+MINE*j])
								error=1;
							else
								{
								error=0;
								z++;
								}
							}
						if(error==0 && z==4)break;
						}
						
						if(error==0 && z==4)
						{
							WriteInst(0xc6); /* set cursor  to (1,6)*/
							WriteString(sizeof(MSG6)-1,MSG6); /* show "exact" on LCD */
							delay_time(30000);
							delay_time(30000);
						}
						else
						{
							WriteInst(0xc6); /* set cursor to (1,6) */
							WriteString(sizeof(MSG7)-1,MSG7);
							delay_time(30000);
							delay_time(30000);	
						}
					}/* end if == 0x0a */
					if(keyData>=0 && keyData<=9)
					{
						USER_CODE[i]=keyData; /* save the user input*/
						WriteData(USER_CODE[i]+0x30); /* 0x30 for ascii */
						i++;
						if(i==4)
						{
							i=0;
							WriteInst(0xc6); /* cursor return to start location(1,6) */
						}
						keyData=0xff;
					}/* end if keyData is a number */
				}/* end while(flag) */
				delay(300);
				WriteInst(0x80); //set cursor to (0,0)
				WriteString(sizeof(MSG1)-1, MSG1);
				WriteInst(0xC0); //set cursor to (1,0)
				WriteString(sizeof(MSG2)-1, MSG2);
			}/* end if == 0x0e */
			
			if(keyData==0x0f) /* push F4 */
			{
				i=0;
				keyData=0xff;
				delay(300);
				WriteInst(0x01); /* clear LCD screen */
				WriteInst(0x81); /* set cursor to (1,1) */
				WriteString(sizeof(MSG8)-1,MSG8); /* show "Input old code" on LCD */
				WriteInst(0xc6); /* set cursor to (1,6) */
				while(keyData==0xff)
					;
				flag=1;
				while(flag)
				{
					if(keyData==0x0b || keyData==0x0c  || keyData==0x0d || keyData==0x0e  || keyData==0x0f)
						flag=0;
					if(keyData==0x0a) /* if keyData == set button*/
					{
						flag=0;
						keyData=0xff;
						error=0;

						for(j=0,w=0;i+MINE*j<SIZE+1;j++,w++)
						{
						z=0;
						for(i=0;i<MINE;i++)
							{
							if(USER_CODE[i]!=SYS_CODE[i+MINE*j])
								error=1;
							else
								{
								error=0;
								z++;
								}
							}
						if(error==0 && z==4)break;
						}
						
						if(error==0 && z==4)
						{
							delay(300);
							WriteInst(0x01);
							WriteInst(0x81);
							WriteString(sizeof(MSG9)-1,MSG9); /* show "Input code" on LCD */
							WriteInst(0xc6);
							while(keyData==0xff)
								;
							flag=1;
							i=0;
							while(flag)
							{
								if(keyData==0x0b || keyData==0x0c  || keyData==0x0d || keyData==0x0e  || keyData==0x0f)
									flag=0;
								if(keyData==0x0a) /* if keyData == set button */
								{
									flag=0;
									keyData=0xff;
									error=0;
									EWEN();              // 93c46 write enable
       									delay_time(1000);	
	
									for(i=0;i<MINE;i++)
										write(w*MINE+i,USER_CODE[i]);
									EWEN();		     // 93c46 write disable
									delay_time(1000);
									
									for(i=0;i<SIZE;i++){
										SYS_CODE[i]=read(i);
										delay_time(30000);
										delay_time(30000);
									}
									delay(300);
									WriteInst(0x01);
									WriteInst(0x84); /* set cursor to (0,4) */
									WriteString(sizeof(MSG10)-1,MSG10);
								}/* end if == set button */
								
								if(keyData>=0 && keyData<=9)
								{
									USER_CODE[i]=keyData;
									WriteData(USER_CODE[i]+0x30);
									i++;
									if(i==SIZE)
									{
										i=0;
										WriteInst(0xc6);
									}
									keyData=0xff;
								}/* end if keyData is a number */
							}/* end while(flag) */
						}/* end if error == 0*/
						else
						{
							WriteInst(0xc6); /* set cursor to (1,6) */
							WriteString(sizeof(MSG7)-1,MSG7); /* show "Error" on LCD */
							delay_time(30000);
							delay_time(30000);
						}/* end if error==1*/
					}/* end if == set button */
					if(keyData>=0 && keyData<=9)
					{
						USER_CODE[i]=keyData;
						WriteData(USER_CODE[i]+0x30);
						i++;
						if(i==SIZE)
						{
							i=0;
							WriteInst(0xc6);
						}
						keyData=0xff;
					}/* end if keyData is a number*/
				}/* end while(flag) */
				delay(300);
				WriteInst(0x80); //set cursor to (0,0)
				WriteString(sizeof(MSG1)-1, MSG1);
				WriteInst(0xC0); //set cursor to (1,0)
				WriteString(sizeof(MSG2)-1, MSG2);
			}/* end if == 0x0f */
		}/* end if !=0xff */
	}/* end while(1) */
}/* end main */


void ConvertTime(void){
	TIME[0]=HOUR/10 + 0x30;
	TIME[1]=HOUR%10 + 0x30;
	TIME[3]=MIN/10 + 0x30;
	TIME[4]=MIN%10 + 0x30;
	TIME[6]=SEC/10 + 0x30;
	TIME[7]=SEC%10 + 0x30;
}


void T0_int(void) __interrupt 1{ /* T0 is system time */
	TH0=(65536-10000)/256;
	TL0=(65536-10000)%256;
	if(--secCnt==0)
	{
		secCnt=100;
		AdjustTime();
	}
}


void T1_int(void) __interrupt 3{ /* T1 is keyboard */
	TH1=(65536-5000)/256;
	TL0=(65536-5000)%256;
	KeyScan();
}

void delay8us(unsigned char x)                // 延遲函數開始 
{ unsigned char i,j;                                                // 宣告變數 
  for (i=0;i<x;i++)                                                // 外迴圈 
      for (j=0;j<1;j++);                                // 內迴圈 
}


void delay(unsigned int dl)
{
 while (dl>0)
  dl--;
}

void start()
{
 scl = 1;
 delay(5);
 sda = 1;
 delay(5);
 sda = 0;
 delay(4);
}

void send_8bits(unsigned char strg)
{
 unsigned char sf;
 
 for (sf=0; sf<8; sf++)
 {
  scl = 0;
  sda =(__bit)(strg & (0x80>>sf));
  delay(5);
  scl = 1;
  delay(4);
 }
 scl = 0;
 delay(5);
}

void ack(void)
{
 sda = 1; 
 
 if(sda == 0)
 {  
  scl = 1;
  delay(4);
  scl = 0;
  delay(5);
 }
}

void stop(void)
{
 sda = 0;
 scl = 1;
 delay(5);
 sda = 1;
}

void WriteInst4bits(unsigned char inst_4b)
{
 send_8bits(0x08);         //RS=0, RW=0
 ack();
 send_8bits(0x0C);         //EN=1
 ack();
 send_8bits((inst_4b&0xF0)+0x0C); //output D7-D4
 ack();
 send_8bits((inst_4b&0xF0)+0x08); //EN=0,read 4bits
 ack();
}

void WriteInst(unsigned char inst)
{ 
 send_8bits(0x08);         //RS=0, RW=0
 ack();
 send_8bits(0x0C);         //EN=1
 ack();
 send_8bits((inst&0xF0)+0x0C);   //high 4bits
 ack();
 send_8bits((inst&0xF0)+0x08);   //EN=0,read high 4bits
 ack();
 
 send_8bits(0x0C);         //EN=1
 ack();
 send_8bits((inst<<4)+0x0C);    //low 4bits
 ack();
 send_8bits((inst<<4)+0x08);    //EN=0,read low 4bits
 ack();
} 

void WriteData(unsigned char data_)
{
 send_8bits(0x09);         //RS=1, RW=0
 ack();
 send_8bits(0x0D);         //EN=1
 ack();
 send_8bits((data_&0xF0)+0x0D);  //high 4bits
 ack();
 send_8bits((data_&0xF0)+0x09);  //EN=0,read high 4bits
 ack();
 
 send_8bits(0x0D);         //EN=1
 ack();
 send_8bits((data_<<4)+0x0D);   //low 4bits
 ack();
 send_8bits((data_<<4)+0x09);   //EN=0,read low 4bits
 ack();
} 

void WriteString(unsigned char count, unsigned char MSG[])
{
 unsigned char sf;
 unsigned char move = 0;
 
 for (sf=0; sf<count; sf++)
   WriteData(MSG[sf]);
}

void initial(void)
{
 delay(15000);
 
 start();
 send_8bits(SLAVE);  //input LCM1602 address
 ack();
 
 WriteInst4bits(0x30); //input 0011 to D7-D4
 
 delay(4100);
 
 WriteInst4bits(0x30);
 
 delay(100);

 WriteInst4bits(0x30);
 WriteInst4bits(0x20); //input 0010 to D7-D4
  
 WriteInst(0x28); //function set, DL(DB4)=0(4bits transport), N(DB3)=1(2 rows), F(DB2)=0(5*7 resolution)
 WriteInst(0x08); //turn off screen
 WriteInst(0x01); //clear screen
 WriteInst(0x06); //entry mode
 WriteInst(0x0E); //turn on screen, D(DB2)=1(on), C(DB1)=1(cursor on), B(DB0)=0(flash off)
}







void AdjustTime(void){
	SEC++;
	if(SEC==60)
	{
		SEC=0;
		MIN++;
		if(MIN==60)
		{
			MIN=0;
			HOUR++;
			if(HOUR==24)
				HOUR=0;
		}
	}
}


void KeyScan(void){
	uint keyStatus;
	
	P3=~ScanLine; /* output keyboard scan */
	keyStatus=~P3; /* read button status */
	keyStatus&=0xf0; /* every line have 4 button */
	for(row=0;row<4;row++)
	{
		if(keyStatus==0x80)
		{
			one=0;
			if(keyTemp!=key) /* ensure that is not bounce ,when this time is not same as last time*/
			{
				keyTemp=key;
				zero=1;
			}
			else
			{
				if(zero!=5) /* for 5 times detected the same number? */
				{
					zero++;
					if(zero==5) /* not bounce,save number */
						keyData=keyTemp;
				}
			}
		}/* end if == 0x80 */
		key+=1;
		keyStatus<<=1; /* detect next button */
	}/* end for */
	ScanLine>>=1; /* detect next line */
	if(ScanLine==0) /* scan to 4 line? */
		ScanLine=0x08; /* reset */
	col++;
	if(col==4)
	{
		col=0;
		key=0;
		one++;
		if(one==5) /* button has not been push, and has been debounce */
		{
			zero=0;
			keyTemp=0xff;
			keyData=0xff;
		}
	}
}





void Delay(void){
	/* delay 10ms */
	int i;
	for(i=0;i<500;i++)
		;
	/* one time is 20us */
}


void  delay_time(unsigned int k)
{
	while (k>0)
	k--;
}
//-----------------------------------------------------
//   93c46 serial bit data synchronize clock
//-----------------------------------------------------
void clock(void)
{      
	SK=1;     //when sk from hi to low, w end
 	delay_time(5);   //93c46 write time
  	SK=0;
   	delay_time(5);
}
//-----------------------------------------------------
//    set 93c46 address
//-----------------------------------------------------
void  address(unsigned char  addr)
{
	char j;

	for (j=0;j<7;j++)     // A0 to A6,7 bits
 	{
  		if (addr & 0x40)  // if addr's highest bit is 1
			DI=1;       
		else  DI=0;       
        clock();         
	addr=addr<<1;     // 93c46 address LR 1 bit
	}
}
//-----------------------------------------------------
//  93c46 write data(data format=byte)
//  before write, need write enable,after enable, input 101 and address
//-----------------------------------------------------
void write(unsigned char addr,char EEPROM_data) 
{
	unsigned char i;
	CS=1; 
	delay_time(1000);  
	//  101:write
	DI=1;    
	clock();
	 
	DI=0;   
	clock();
	  
	DI=1;    
	clock();
	   
	address(addr);    //set address
 
// 93c46 write data(data format=byte)
	for (i=0;i<8;i++)      
	{
		if(EEPROM_data & 0x80)
		//if data bit is 1
			DI=1; 
		else   DI=0;
		clock();                  

		EEPROM_data=EEPROM_data<<1; 
		//data bit LR 1 bit
	}
	delay_time(10000);           // set delay time
	CS=0;       
}
//-----------------------------------------------------
//      93c46 read data(data format=byte)
//      read:110 and address
//-----------------------------------------------------
char read(unsigned char addr) 
{
	unsigned char i; 
	char   EEPROM_data=0x00;  

	CS=1;
	delay_time(1000); 
	// 110:read
	DI=1;    
	clock();
	
	DI=1;    
	clock();
	
	DI=0;   
	clock();
	 
	address(addr);        //set address 
	//   93c46 read data(data format=byte)   
 	for(i=0;i<8;i++)  
	{
		clock();                      

		EEPROM_data=EEPROM_data<<1;  
		//data bit LR 1 bit
		if(DO==1)  
			EEPROM_data=EEPROM_data | 0x01;
			//if DO is 1, data bit record 
	}      
	delay_time(1000);
	CS= 0;
	
	return(EEPROM_data);
}
//-----------------------------------------------------
//  93c46 write enable
//  input:100 and 11xxxx
//-----------------------------------------------------
void EWEN(void)   
{
	unsigned char addr=0x60;   //  address=11xx xxx?
      
	CS=1; 
	delay_time(1000); 
	//  write enable:100
	DI=1;     
	clock();
	  
	DI=0;    
	clock();
	  
	DI=0;    
	clock();  
  
	address(addr); 
	delay_time(1000);         
	CS=0; 
}
//-----------------------------------------------------
//  93c46 write disable
//  input:100 and 00xxxx
//-----------------------------------------------------
void EWDS(void)  
{
	unsigned char addr=0x00;  // address=00xx xxx?
       
	CS=1;     
	delay_time(1000); 
//  write disable:100
	DI=1;    
	clock();
	  
	DI=0;    
	clock();
	  
	DI=0;    
	clock();  
 
	address(addr); 
	delay_time(1000); 
	CS=0;  
} 
//----------------------------------------------------
//   93c46 clear all
//   input:100 and 10xxxx
//-----------------------------------------------------
void ERAL(void)   
{
	unsigned char addr=0x40;    // address=10xx xxx?
  
	CS=1; 
	delay_time(1000);  
	//  claer all:100
	DI=1;   
	clock();
	
	DI=0;    
	clock();
	  
	DI=0;    
	clock();  

	address(addr); 
	delay_time(1000); 
	CS=0;   
}
//------------------------------------------------------

void speaker(void)
{
	char i,j;
	for(i=0;i<PeriodCount;i++)
	{
		BUZ=0;
		for(j=0;j<FreqCount;j++);
		delay_time(1000);
		j=0;
		BUZ=1;
		for(j=0;j<FreqCount;j++);
	}
}
