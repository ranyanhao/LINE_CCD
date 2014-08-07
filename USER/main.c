#include "common.h"
#include "gpio.h"
#include "uart.h"
#include "adc.h"
#include "math.h"

#define  CCD_SI(data)    GPIO_WriteBit(HW_GPIOB,2,data);
#define  CCD_CLK(data)   GPIO_WriteBit(HW_GPIOB,4,data);

void CCD_Restet(void);
void CCD(void);
uint8_t MAX_2(uint8_t a,uint8_t b);
uint8_t MAX_5(uint8_t data_0,uint8_t data_1,uint8_t data_2,uint8_t data_3,uint8_t data_4);
uint8_t MIN_2(uint8_t a,uint8_t b);
uint8_t MIN_5(uint8_t data_0,uint8_t data_1,uint8_t data_2,uint8_t data_3,uint8_t data_4);
uint8_t Data_sort(uint8_t data[5]);
void Data_binarization(uint8_t average);
uint8_t averaging(void);
void CCD_Filtering(void);
void CCD_gather(void);
uint8_t Track_Midline(void);


uint8_t CCD_original_data[128+4]={0};
uint8_t CCD_filtering_data[128]={0};


int main()
{
	
	uint8_t i,Track_Midline_value,A;
	DelayInit();
	GPIO_QuickInit(HW_GPIOE,0,kGPIO_Mode_OPP);
	GPIO_WriteBit(HW_GPIOE,0,0);
	

	GPIO_QuickInit(HW_GPIOB,2,kGPIO_Mode_OPP);
	GPIO_QuickInit(HW_GPIOB,4,kGPIO_Mode_OPP);
	CCD_SI(0);
  CCD_CLK(0);
	
	UART_QuickInit(UART0_RX_PA15_TX_PA14, 115200);

	ADC_QuickInit(ADC0_SE8_PB0,kADC_SingleDiff8or9);

	CCD_Restet();
  while(1)
	{
		  //CCD();
		  CCD_gather(); 
		  CCD_Filtering();

		  Data_binarization(averaging());
		

			for(i=0;i<128;i++)
		{
      if(CCD_filtering_data[127]==0xff)
			{
        CCD_filtering_data[127]=0xfe;
      }
			UART_WriteByte(HW_UART0,CCD_filtering_data[i]);
    }
		UART_WriteByte(HW_UART0,0xff);
		
		
	
//		Track_Midline_value = Track_Midline();
//		A = Track_Midline_value;
//		printf("Track_Midline_value = %d\n",A);
  }
}

void CCD_gather(void)
{
	uint8_t i;
	
//采集上次曝光的得到的图像
	CCD_CLK(0);
	__nop();
	CCD_SI(1);  //开始SI
	__nop();
	CCD_CLK(1);
	__nop();
	CCD_SI(0);
	__nop();
	
	
	for(i=0;i<128;i++)   //采集1-128个像素点
	{
	  CCD_CLK(0);
	  __nop();
		CCD_original_data[i+2] = ADC_QuickReadValue(ADC0_SE8_PB0);
		CCD_CLK(1);
		__nop();
	}
	
	__nop();   //发送第129个CLK
	CCD_CLK(1);
	__nop();
	CCD_CLK(0);
	__nop();
	
	DelayMs(30);	//曝光时间
}


//void CCD_sharpen(void)
//{
//	uint8_t i,j,k;
//	uint8_t temp[5];
//	for(i=0;i<128;i++)
//	{
//    switch (i)
//		{
//			case 0:
//				temp[0] = CCD_filtering_data[0];
//			  temp[1] = CCD_filtering_data[1];
//				for(j=2;j<5;j++)
//			  {
//			    temp[j] = CCD_filtering_data[j-2];
//			  }
//				break;
//			case 1:
//				temp[0] = CCD_filtering_data[1];
//			  for(j=1;j<5;j++)
//			  {
//				  temp[j] = CCD_filtering_data[j-1];
//				}
//				break;
//			case 126:
//				temp[4] = CCD_filtering_data[127];
//			  for(j=0;j<4;j++)
//			  {
//				  temp[j] = CCD_filtering_data[j+124];
//				}
//				break;
//			case 127:
//				temp[3] = CCD_filtering_data[126];
//			  temp[4] = CCD_filtering_data[127];
//			  for(j=0;j<3;j++)
//			  {
//				  temp[j] = CCD_filtering_data[j+125];
//				}
//				break;
//			default :
//				for(j=0,k=i;j<5;j++)
//			  {
//					k = i+j;
//			    temp[j] = CCD_filtering_data[k-2];
//			  }
//		}
//		CCD_sharpen_data[i] = Laplace(temp);
//	}
//}	

//uint8_t Laplace(uint8_t data[5])
//{
//	uint8_t i;
//	int sum;
//  for(i=0;i<5;i++)
//	{
//	  if(i < 2)
//		{
//		  sum = sum + data[i]*(-1);
//		}
//		else if(i > 2)
//		{
//		  sum = sum + data[i]*(-1);
//		}
//		else
//		{
//		  sum = sum +data[2]*4;
//		}
//	}
////	printf("sum = %d",abs(sum/5));
//	return (abs(sum/5));
//}


void CCD_Filtering(void)      //数据滤波
{
	uint8_t i,j,k,MAX,MIN;
	uint8_t temp[5];
	for(i=0;i<128;i++)
	{
	  for(j=0,k=i;j<5;j++)
		{
			k = i+j;
		  temp[j] = CCD_original_data[k];
		}
		MAX = MAX_5(temp[0],temp[1],temp[2],temp[3],temp[4]);
		MIN = MIN_5(temp[0],temp[1],temp[2],temp[3],temp[4]);
		if(MAX == CCD_original_data[i+2] || MIN == CCD_original_data[i+2])
		{
		  CCD_filtering_data[i] =  Data_sort(temp);
		}
		else
		{
		  CCD_filtering_data[i] = CCD_original_data[i+2];
		}
	}
}


void Data_binarization(uint8_t average)
{
  uint8_t i;
	for(i=0;i<128;i++)
	{
	  if(CCD_filtering_data[i] >= average)
		{
		 CCD_filtering_data[i] = 0XFF;
		}
		else
		{
		 CCD_filtering_data[i] = 0X00;
		}
	}
}	


uint8_t Data_sort(uint8_t data[5])
{
  uint8_t i,j,temp;
	for(j=0;j<5;j++)
	{
	  for(i=0;i<5-j;i++)
		{
		  if(data[i] > data[i+1])
			{
			  temp = data[i];
				data[i] = data[i+1];
				data[i+1] = temp;
			}
		}
	}
	return (data[3]);
}


uint8_t averaging(void)
{
  uint8_t i,MIN,MAX;
	for(i=0;i<128;i++)
	{
	  MAX = MAX_2(MAX,CCD_filtering_data[i]);
		MIN = MIN_2(MIN,CCD_filtering_data[i]);
	}
//	printf("MAX = %d\n", MAX);
//	printf("MIN = %d\n", MIN);
//	printf("avr = %d\n", (MAX + MIN)/2);
	return ((MAX + MIN)/2);
}	





uint8_t MAX_5(uint8_t data_0,uint8_t data_1,uint8_t data_2,uint8_t data_3,uint8_t data_4)
{
 return MAX_2(MAX_2(MAX_2(MAX_2(data_0,data_1),data_2),data_3),data_4);
}

uint8_t MIN_5(uint8_t data_0,uint8_t data_1,uint8_t data_2,uint8_t data_3,uint8_t data_4)
{
 return MIN_2(MIN_2(MIN_2(MIN_2(data_0,data_1),data_2),data_3),data_4);
}

uint8_t MAX_2(uint8_t a,uint8_t b)
{
	return (a >= b)? a:b; 
}

uint8_t MIN_2(uint8_t a,uint8_t b)
{
 return (a <= b)? a:b;
}

////uint8_t Track_Midline(void)
////{
////	  uint8_t i;
////	  uint8_t Left_Side,Right_Side,Track_Midline_value,A,B;
////		for(i=0;i<64;i++)
////	  {
////	   if(CCD_filtering_data[64-i]==0)
////		 {
////		   Left_Side = 64 - i;
////			 break;
////		 }
////	  	else
////			{
////			  Left_Side = 0;
////			}				
////	  }
////		
////		for(i=0;i<64;i++)
////	  {
////	   if(CCD_filtering_data[64+i]==0)
////		 {
////		   Right_Side = 64 + i;
////			 break;
////		 }
////	  	else
////			{
////			  Right_Side = 127;
////			}				
////	  }
////		A = Left_Side;
////		B = Right_Side;
////		printf("Left_Side = %d\n",A);
////		printf("Right_Side = %d\n",B);
////		Track_Midline_value = Left_Side+((Right_Side - Left_Side)/2);
////		return (Track_Midline_value);
////}



void CCD_Restet(void)
{
	uint8_t i=0;
  CCD_CLK(0);
	__nop();
	CCD_SI(1);
	__nop();
	CCD_CLK(1);//发送第一个CLK
	__nop();
	CCD_SI(0);  
	__nop();  
	
	for(i=1;i<129;i++)   //发送第2--129个CLK
	{
		CCD_CLK(0);
	  __nop();
    CCD_CLK(1);
		__nop();
	}
}




//void CCD (void)
//{
//	uint8_t i=0;
//	uint8_t CCD_original_data[128]={0};

//	//采集上次曝光的得到的图像
//	CCD_CLK(0);
//	__nop();
//	CCD_SI(1);  //开始SI
//	__nop();
//	CCD_CLK(1);
//	__nop();
//	CCD_SI(0);
//	__nop();
//	
//	
//	for(i=0;i<128;i++)   //采集1-128个像素点
//	{
//	  CCD_CLK(0);
//	  __nop();
//		CCD_original_data[i] = ADC_QuickReadValue(ADC0_SE8_PB0);
//		CCD_CLK(1);
//		__nop();
//	}
//	
//	__nop();   //发送第129个CLK
//	CCD_CLK(1);
//	__nop();
//	CCD_CLK(0);
//	__nop();


//	for(i=0;i<128;i++)                                      
//		{
//      if(CCD_original_data[127]==0xff)
//			{
//        CCD_original_data[127]=0xfe;
//      }
//			UART_WriteByte(HW_UART0,CCD_original_data[i]);
//    }
//		UART_WriteByte(HW_UART0,0xff);
//		
//		
//		DelayMs(30);  //加上出口数据输出，和DelayMs(30)时间，为曝光时间
//}




//asd 