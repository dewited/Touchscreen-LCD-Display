/**
  ******************************************************************************
  * EECE 437 Spring 2014
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "stm32f429i_discovery_l3gd20.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


#define         B1_XMIN         2
#define         B1_XMAX         64
#define         B1_YMIN         228
#define         B1_YMAX         290

#define         B2_XMIN         89
#define         B2_XMAX         153
#define         B2_YMIN         228
#define         B2_YMAX         290

#define         B3_XMIN         176
#define         B3_XMAX         238
#define         B3_YMIN         228
#define         B3_YMAX         290

#define         A1_YMIN         54
#define         A1_YMAX         130

#define         A2_YMIN         140
#define         A2_YMAX         210

extern uint32_t CurrentFrameBuffer;

/* Private macro -------------------------------------------------------------*/

#define ABS(x)                     (x < 0) ? (-x) : x
#define L3G_Sensitivity_250dps     (float)114.285f        /*!< gyroscope sensitivity with 250 dps full scale [LSB/dps]  */
#define L3G_Sensitivity_500dps     (float)57.1429f        /*!< gyroscope sensitivity with 500 dps full scale [LSB/dps]  */
#define L3G_Sensitivity_2000dps    (float)14.285f         /*!< gyroscope sensitivity with 2000 dps full scale [LSB/dps] */
  
/* Private variables ---------------------------------------------------------*/
float Buffer[6];
float Gyro[3];
float X_BiasError, Y_BiasError, Z_BiasError = 0.0;
uint8_t Xval, Yval = 0x00;
static __IO uint32_t TimingDelay;
RCC_ClocksTypeDef RCC_Clocks;

/* Private variables ---------------------------------------------------------*/

int x_index = 2;


/* Private function prototypes -----------------------------------------------*/
static void TP_Config(void);

void LCD_ClearSection(uint16_t, uint16_t);

static void Demo_GyroConfig(void);
static void Demo_GyroReadAngRate (float* pfData);
static void Gyro_SimpleCalibration(float* GyroData);

//Function s for x and y graph
void y_value(int);
void x_value(int);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{

  static TP_STATE* TP_State; 
    
  /*!< At this stage the microcontroller clock setting is already configured, 
  this is done through SystemInit() function which is called from startup
  file (startup_stm32f429_439xx.s) before to branch to application main.
  To reconfigure the default setting of SystemInit() function, refer to
  system_stm32f4xx.c file
  */      

  
    /* SysTick end of count event each 10ms */
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);
  
  /* LCD initialization */
  LCD_Init();
  
  /* LCD Layer initialization */
  LCD_LayerInit();
    
  /* Enable the LTDC */
  LTDC_Cmd(ENABLE);
  
  /* Set LCD foreground layer */
  LCD_SetLayer(LCD_FOREGROUND_LAYER);
  
   /* Gyroscope configuration */
  Demo_GyroConfig();

  /* Gyroscope calibration */
  Gyro_SimpleCalibration(Gyro); 
  
  /* Touch Panel configuration */
  TP_Config();
  
  
// Your code changes go here...........................................
    //intigers for button presses
  int blue_button=0;
  int green_button=0;
  // sensitivity is initially divided by 5
  int sensitivity =5;
    
  while (1)
  {
 
      TP_State = IOE_TP_GetState();
      //if touch detected in the button region
      if((TP_State->TouchDetected) && ((TP_State->Y < B1_YMAX) && (TP_State->Y >= B1_YMIN))) 
          //if the touch was detected on the first button we clear the lcd and start over
          if((TP_State->X >= B1_XMIN) && (TP_State->X < B1_XMAX)) 
          {   
                 LCD_ClearSection(A1_YMIN, A1_YMAX);
                 LCD_ClearSection(A2_YMIN, A2_YMAX);
                 x_index = 2; 
                 
          } 
          // if the button in the 2nd region was detected we will increase or decreasde the sensitivity
          else if ((TP_State->TouchDetected) && (TP_State->X >= B2_XMIN) && (TP_State->X < B2_XMAX)) 
          {
            
              // code to decrease or increase sensitivityh
            switch (blue_button)
            {
            case 0:
              blue_button = 1;
              sensitivity = 15;
              break;
            case 1:
              blue_button =0;
              sensitivity = 5;
              break;
            default:
                blue_button = 0;
            }
            if (blue_button > 1)
            {
              blue_button =0;
            }
          } 
      // if the button was pushed in the third region we increase or decrease the speed of the line
          else if ((TP_State->TouchDetected) && (TP_State->X >= B3_XMIN) && (TP_State->X < B3_XMAX)) 
          {
            switch (green_button)
            {
            case 0:
              // i did this by altering the systick to either increase by a factor of 2 ( 200 hz)
              SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);
              green_button++;
              break;
            case 1:
              //or decrease to 100 hz
              SysTick_Config(RCC_Clocks.HCLK_Frequency / 50);
              green_button++;
              break; 
            default:
              green_button = 0;
            }
            if (green_button>1)
            {
              green_button = 0;
            }
          }
            //waits for it to not detect a touch 
            while (TP_State ->TouchDetected)
            {
              TP_State = IOE_TP_GetState();
            }
      // incriments the x axis
      x_index++;     
      if (x_index >= 238)
      {
          x_index = 2;          
          LCD_ClearSection(138, 212);
          LCD_ClearSection(52, 132);
      }
           
      LCD_SetTextColor(LCD_COLOR_BLACK);
      
      /* Read Gyro Angular data */
      
      Demo_GyroReadAngRate(Buffer);
      // functions to draw y values for the gyroscope
      y_value(sensitivity);
      x_value(sensitivity); 
    
      Delay(1);
    }
}
void x_value(int sensitivity)
{
  //sets up the x-axis baseline
  int y2_index = A1_YMIN + ((A1_YMAX - A1_YMIN)/2);
  //finds acceleration of the gyroscope
  Buffer[1] = (int8_t)Buffer[1] - (int8_t)Gyro[1];
  // intiger to write to the lcd
   int y_comp=y2_index+(Buffer[1]/sensitivity);
   // if the y value is in the allowed region
   if (y_comp>=A1_YMIN && y_comp <= A1_YMAX)
   {
      LCD_DrawFullCircle(x_index, y_comp, 1);
   }
  // if the y value is under the allowed region
   if (y_comp < A1_YMIN)
   {
      LCD_DrawFullCircle(x_index, A1_YMIN, 1);
   }
  //if the y value is over the allowed region
   if (y_comp > A1_YMAX)
      LCD_DrawFullCircle(x_index, A1_YMAX, 1);
}

// function to write for the y axis of the gyroscope
void y_value(int sensitivity)
{
  //sets up y axis baseline
  int y_index = A2_YMIN + ((A2_YMAX - A2_YMIN)/2);
  //reads in the acceleration of the gyroscope
  Buffer[0] = (int8_t)Buffer[0] - (int8_t)Gyro[0];
  // adds te acceleration and the baseline
  int y_comp = y_index+(Buffer[0]/sensitivity);
  // if it s in the allowed values
  if(y_comp >=A2_YMIN && y_comp <=A2_YMAX)
  {
    LCD_DrawFullCircle(x_index, y_comp, 1);  
  }
  // if it is under the allowed values
  if (y_comp < A2_YMIN)
  {
    LCD_DrawFullCircle(x_index, A2_YMIN, 1);
  }
  // if it is over the max value
  if (y_comp>A2_YMAX)
  { LCD_DrawFullCircle(x_index, A2_YMAX, 1);}
}
// ................................................................







//----------------------------------------------------------------------------
// Supplied Functions - Do not change ......

void LCD_ClearSection(uint16_t YStart, uint16_t YEnd)
{
    int x = 0;
    
    LCD_SetTextColor(LCD_COLOR_WHITE);
    for (x=0; x<=240; x++) 
      LCD_DrawLine(x, YStart, (YEnd - YStart), LCD_DIR_VERTICAL);
    
}


/**
* @brief  Configure the IO Expander and the Touch Panel.
* @param  None
* @retval None
*/
static void TP_Config(void)
{
  /* Clear the LCD */ 
  LCD_Clear(LCD_COLOR_WHITE);
  
  /* Configure the IO Expander */
  if (IOE_Config() == IOE_OK)
  { 
      LCD_SetFont(&Font16x24);
      LCD_DisplayStringLine(LINE(1), (uint8_t*)" EECE 437 S14 ");
      LCD_DrawLine(0, 15, 240, LCD_DIR_HORIZONTAL);
      LCD_DrawLine(0, 50, 240, LCD_DIR_HORIZONTAL);
   
      
      LCD_DrawLine(0, 133, 240, LCD_DIR_HORIZONTAL);
      LCD_DrawLine(0, 217, 240, LCD_DIR_HORIZONTAL);
      LCD_DrawLine(0, 300, 240, LCD_DIR_HORIZONTAL);
      
      LCD_SetTextColor(LCD_COLOR_BLACK); 
      LCD_DrawFullRect(2, 228, 62, 62);
      LCD_SetTextColor(LCD_COLOR_RED); 
      LCD_DrawFullRect(6, 232, 54, 54);    
    
      LCD_SetTextColor(LCD_COLOR_BLACK); 
      LCD_DrawFullRect(89, 228, 62, 62);
      LCD_SetTextColor(LCD_COLOR_BLUE); 
      LCD_DrawFullRect(93, 232, 54, 54);
      
      LCD_SetTextColor(LCD_COLOR_BLACK); 
      LCD_DrawFullRect(176, 228, 62, 62);
      LCD_SetTextColor(LCD_COLOR_GREEN); 
      LCD_DrawFullRect(180, 232, 54, 54);    
 
   
    //LCD_SetFont(&Font16x24);
    //LCD_DisplayChar(LCD_LINE_12, 195, 0x43);
    //LCD_DrawLine(0, 248, 240, LCD_DIR_HORIZONTAL);
    
  }  
  else
  {
    LCD_Clear(LCD_COLOR_RED);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"   IOE NOT OK      ");
    LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"Reset the board   ");
    LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"and try again     ");
  }
}



/**
* @brief  Configure the Mems to gyroscope application.
* @param  None
* @retval None
*/
static void Demo_GyroConfig(void)
{
  L3GD20_InitTypeDef L3GD20_InitStructure;
  L3GD20_FilterConfigTypeDef L3GD20_FilterStructure;

  /* Configure Mems L3GD20 */
  L3GD20_InitStructure.Power_Mode = L3GD20_MODE_ACTIVE;
  L3GD20_InitStructure.Output_DataRate = L3GD20_OUTPUT_DATARATE_1;
  L3GD20_InitStructure.Axes_Enable = L3GD20_AXES_ENABLE;
  L3GD20_InitStructure.Band_Width = L3GD20_BANDWIDTH_4;
  L3GD20_InitStructure.BlockData_Update = L3GD20_BlockDataUpdate_Continous;
  L3GD20_InitStructure.Endianness = L3GD20_BLE_LSB;
  L3GD20_InitStructure.Full_Scale = L3GD20_FULLSCALE_500; 
  L3GD20_Init(&L3GD20_InitStructure);
  
  L3GD20_FilterStructure.HighPassFilter_Mode_Selection =L3GD20_HPM_NORMAL_MODE_RES;
  L3GD20_FilterStructure.HighPassFilter_CutOff_Frequency = L3GD20_HPFCF_0;
  L3GD20_FilterConfig(&L3GD20_FilterStructure) ;
  
  L3GD20_FilterCmd(L3GD20_HIGHPASSFILTER_ENABLE);
}

/**
* @brief  Calculate the angular Data rate Gyroscope.
* @param  pfData : Data out pointer
* @retval None
*/
static void Demo_GyroReadAngRate (float* pfData)
{
  uint8_t tmpbuffer[6] ={0};
  int16_t RawData[3] = {0};
  uint8_t tmpreg = 0;
  float sensitivity = 0;
  int i =0;
  
  L3GD20_Read(&tmpreg,L3GD20_CTRL_REG4_ADDR,1);
  
  L3GD20_Read(tmpbuffer,L3GD20_OUT_X_L_ADDR,6);
  
  /* check in the control register 4 the data alignment (Big Endian or Little Endian)*/
  if(!(tmpreg & 0x40))
  {
    for(i=0; i<3; i++)
    {
      RawData[i]=(int16_t)(((uint16_t)tmpbuffer[2*i+1] << 8) + tmpbuffer[2*i]);
    }
  }
  else
  {
    for(i=0; i<3; i++)
    {
      RawData[i]=(int16_t)(((uint16_t)tmpbuffer[2*i] << 8) + tmpbuffer[2*i+1]);
    }
  }
  
  /* Switch the sensitivity value set in the CRTL4 */
  switch(tmpreg & 0x30)
  {
  case 0x00:
    sensitivity=L3G_Sensitivity_250dps;
    break;
    
  case 0x10:
    sensitivity=L3G_Sensitivity_500dps;
    break;
    
  case 0x20:
    sensitivity=L3G_Sensitivity_2000dps;
    break;
  }
  /* divide by sensitivity */
  for(i=0; i<3; i++)
  {
  pfData[i]=(float)RawData[i]/sensitivity;
  }
}

/**
* @brief  Calculate offset of the angular Data rate Gyroscope.
* @param  GyroData : Data out pointer
* @retval None
*/
static void Gyro_SimpleCalibration(float* GyroData)
{
  uint32_t BiasErrorSplNbr = 500;
  int i = 0;
  
  for (i = 0; i < BiasErrorSplNbr; i++)
  {
    Demo_GyroReadAngRate(GyroData);
    X_BiasError += GyroData[0];
    Y_BiasError += GyroData[1];
    Z_BiasError += GyroData[2];
  }
  /* Set bias errors */
  X_BiasError /= BiasErrorSplNbr;
  Y_BiasError /= BiasErrorSplNbr;
  Z_BiasError /= BiasErrorSplNbr;
  
  /* Get offset value on X, Y and Z */
  GyroData[0] = X_BiasError;
  GyroData[1] = Y_BiasError;
  GyroData[2] = Z_BiasError;
}


/**
* @brief  Basic management of the timeout situation.
* @param  None.
* @retval None.
*/
uint32_t L3GD20_TIMEOUT_UserCallback(void)
{
  return 0;
}


/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}



#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
