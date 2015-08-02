//
// HydroponicsController3
//
// Hydroponics controller
// Developed with [embedXcode](http://embedXcode.weebly.com)
//
// Author 		Dmitry Moshalkov
// 				Dmitry Moshalkov
//
// Date			28.07.15 19:36
// Version		<#version#>
//
// Copyright	© Dmitry Moshalkov, 2015
// Licence		<#license#>
//
// See         ReadMe.txt for references
//



#include "Arduino.h"
#include "U8glib.h"
#include <EEPROMex.h>
#include <CountUpDownTimer.h>



//Errors


typedef struct Errors_Type{
    uint8_t  uiErrorCode;
    char     cErrorText[30];
} Errors_Type;


Errors_Type Errors[8]={
    {1,"Liquid level low!"},
    {2,"Light does not work!"},
    {3,"Tank humidity high!"},
    {4,"Tank humidity low!"},
    {5,"Out humidity high!"},
    {6,"Out humidity low!"},
    {7,"Liquid temp high!"},
    {8,"Liquid temp low!"}
};

boolean bErrorPresent=false; // true if error exists
//uint8_t uiNumOfErrors=0; //количество необработанных ошибок
//uint8_t uiLastError=0;

typedef struct CurrentErrors_Type{
    uint8_t  uiErrorCode;
    boolean  uiErorState;
} CurrentErrors_Type;

CurrentErrors_Type CurrentErrors[8];



//Pins

#define KEYBOARD_PIN  3

//int value;
boolean bKeyPressed = false;


//Было изменение значений
boolean bFloodValuesChanged=false;
boolean bLightValuesChanged=false;

boolean bMainMenuPage2=false; //Отобразить второй основной информационный экран
boolean bErrorPage=false; //Отобразить экран ошибок

//Текущие значения параметров окружающей среды
int currentOuterHum;
int currentOuterTemp;
int currentTankHum;
int currentTankTemp;
int currentTopLightLevel;
int currentTopDownLevel;

//Текущие рабочие значения цикла затопления и освещения
uint8_t  uiCurrentProgram=0;
uint8_t  uiCurrentFloodCycleHours=0;
uint8_t  uiCurrentFloodCycleMinutes=0;
uint8_t  uiCurrentFloodTime=0;
uint8_t  uiCurrentLightStartHours=0;
uint8_t  uiCurrentLightStartMinutes=0;
uint8_t  uiCurrentLightTimeInHours=0;
uint8_t  uiCurrentLightTimeInMinutes=0;


typedef struct Program_Type{
    char   Name[10];
    uint8_t  uiFloodCycleHours;
    uint8_t  uiFloodCycleMinutes;
    uint8_t  uiFloodTime;
    uint8_t  uiLightStartHours;
    uint8_t  uiLightStartMinutes;
    uint8_t  uiLightTimeInHours;
    uint8_t  uiLightTimeInMinutes;
} Program_Type;


Program_Type Program[3]={
    {"SALAD",1,0,10,20,0,14,0 },
    {"TOMATO",1,0,10,20,0,14,0},
    {"MANUAL", 0,0,0,0,0,0,0} //manual mode always last
};

#define MODEMENU_ITEMS 3

uint8_t manual_settings_fcf_hours=0;
uint8_t manual_settings_fcf_minutes=0;
uint8_t manual_settings_fcf_floodtime=0;
boolean binside_fcf_edit_hours=false;
boolean binside_fcf_edit_minutes=false;
boolean binside_fcf_edit_floodtime=false;

uint8_t manual_settings_lt_hours=0;
uint8_t manual_settings_lt_minutes=0;
uint8_t manual_settings_lt_lighttime_hours=0;
uint8_t manual_settings_lt_lighttime_minutes=0;
boolean binside_lt_edit_hours=false;
boolean binside_lt_edit_minutes=false;
boolean binside_lt_edit_lighttime_hours=false;
boolean binside_lt_edit_lighttime_minutes=false;


uint8_t settings_date_day=0;
uint8_t settings_date_month=0;
unsigned int settings_date_year=2015;
uint8_t settings_date_hours=0;
uint8_t settings_date_minutes=0;
uint8_t settings_date_seconds=0;
boolean binside_date_edit_day=false;
boolean binside_date_edit_month=false;
boolean binside_date_edit_year=false;
boolean binside_date_edit_hours=false;
boolean binside_date_edit_minutes=false;
boolean binside_date_edit_seconds=false;



#define KEY_NONE     0
#define KEY_MENU     1
#define KEY_BACK     2
#define KEY_RIGHT    3
#define KEY_SELECT   4
#define KEY_LEFT     5

#define MMENU_ITEMS 5
char *mmenu_strings[MMENU_ITEMS] = { "Mode", "Manual settings", "Date & Time", "Liquid cycle On/Off", "Lamp On/Off" };



#define MANUALMENU_ITEMS 2
char *manualmenu_strings[MANUALMENU_ITEMS] = { "Flood cycle", "Light timings" };


uint8_t mmenu_current = 0;
uint8_t menu_redraw_required = 0;
uint8_t last_key_code = KEY_NONE;
uint8_t modemenu_current = 0;
uint8_t manualmenu_current = 0;

uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;

boolean binsideMainMenu=false;
boolean binsideModeMenu=false;
boolean binsideManualMenu=false;
boolean binsideFloodCycleFreqMenu=false;
boolean binsideLightSettingsMenu=false;
boolean binsideDateSettingsMenu=false;


boolean bFloodCycleRun=false; //Запущен таймер ожидания залива
boolean bFloodStayRun=false; //Запущен таймер нахождения в состоянии залива

U8GLIB_SSD1306_128X64 u8g(5, 4, 10, 2, 3);

CountUpDownTimer MainDisplayTimer(DOWN);

CountUpDownTimer FloodCycleTimer(DOWN);
CountUpDownTimer FloodStayTimer(DOWN);


/***************************************************************************************/
/*            Display error or warning and light error led                              */
/***************************************************************************************/


void displayError ( uint8_t uiErrorCode, uint8_t uiSeverity, boolean bAdd)
{
    // uiSeverity = 0 - no error, remove previous notification, uiSeverity = 1 - Warning, uiSeverity = 2 - Error
    // bAdd = true - add error, bAdd = false - remove error
    // Light status led on/off
    

    if ( bAdd )
    {
        uint8_t i=0;
        
        while ( CurrentErrors[i].uiErorState)
        {
            i++;
        }
        
            CurrentErrors[i].uiErrorCode=uiErrorCode;
            CurrentErrors[i].uiErorState=true;
            bErrorPresent = true;
    }
    
    

    
    
}


void clearErrors ( uint8_t uiErrorCode )
{
    //Очищать ошибки в функциях датчиков при переходе значений из значения в значение
    // По последней ошибке - перевести bErrorPresent в false
    
    uint8_t uiPos=0;
    
    bErrorPresent = false;
    
    for (int i=0; i < (sizeof(CurrentErrors)/sizeof(CurrentErrors[0])); i++)
    {
        if (CurrentErrors[i].uiErrorCode == uiErrorCode)
        {
            CurrentErrors[i].uiErorState = false;
        }

        if ( CurrentErrors[i].uiErorState )
        {
            bErrorPresent = true;
        }
        
    }
    
    if ( !bErrorPresent )
    {
        // Погасить светодиод ошибок
    }
    
    
    
}


/***************************************************************************************/
/*            Config saving & loading                                                  */
/***************************************************************************************/

void saveConfig ( void )
{
    
    EEPROM.update(0, uiCurrentProgram);
    EEPROM.updateBlock(sizeof(uint8_t)+1, Program[MODEMENU_ITEMS-1]);
    bFloodValuesChanged=false;
    bLightValuesChanged=false;
    
}


void loadConfig ( void )
{
    
    uiCurrentProgram=EEPROM.read(0);
    
    modemenu_current=uiCurrentProgram;
    
    
    
    EEPROM.readBlock(sizeof(uint8_t)+1, Program[MODEMENU_ITEMS-1]);
    
    manual_settings_fcf_hours=Program[MODEMENU_ITEMS-1].uiFloodCycleHours;
    manual_settings_fcf_minutes=Program[MODEMENU_ITEMS-1].uiFloodCycleMinutes;
    manual_settings_fcf_floodtime=Program[MODEMENU_ITEMS-1].uiFloodTime;
    manual_settings_lt_hours=Program[MODEMENU_ITEMS-1].uiLightStartHours;
    manual_settings_lt_minutes=Program[MODEMENU_ITEMS-1].uiLightStartMinutes;
    manual_settings_lt_lighttime_hours=Program[MODEMENU_ITEMS-1].uiLightTimeInHours;
    manual_settings_lt_lighttime_minutes=Program[MODEMENU_ITEMS-1].uiLightTimeInMinutes;
    
    
    uiCurrentFloodCycleHours=Program[uiCurrentProgram].uiFloodCycleHours;
    uiCurrentFloodCycleMinutes=Program[MODEMENU_ITEMS-1].uiFloodCycleMinutes;
    uiCurrentFloodTime=Program[MODEMENU_ITEMS-1].uiFloodTime;
    uiCurrentLightStartHours=Program[MODEMENU_ITEMS-1].uiLightStartHours;
    uiCurrentLightStartMinutes=Program[MODEMENU_ITEMS-1].uiLightStartMinutes;
    uiCurrentLightTimeInHours=Program[MODEMENU_ITEMS-1].uiLightTimeInHours;
    uiCurrentLightTimeInMinutes=Program[MODEMENU_ITEMS-1].uiLightTimeInMinutes;
    
}



/***************************************************************************************/
/*            OLED display Menu processing                                             */
/***************************************************************************************/

void drawMainPage(void)
{
    // graphic commands to redraw the complete screen should be placed here
    //u8g.setDefaultForegroundColor();
    //u8g.setDefaultBackgroundColor();
    
    //Serial.println(FloodCycleTimer.ShowMinutes());
    //Serial.println(FloodCycleTimer.ShowSeconds());
    
    
    char buffer[20];
    
    if ( bFloodCycleRun )
    {
        sprintf(buffer, "Waiting for  %02lu:%02lu:%02lu", FloodCycleTimer.ShowHours(),FloodCycleTimer.ShowMinutes(),FloodCycleTimer.ShowSeconds());
    } else if ( bFloodStayRun )
    {
        sprintf(buffer, "Flooding for %02lu:%02lu:%02lu", FloodStayTimer.ShowHours(),FloodStayTimer.ShowMinutes(),FloodStayTimer.ShowSeconds());
    }
    
    
    
    u8g.setFont(u8g_font_tpssr);
    //u8g.drawStr( 0, 15, "Salad");
    u8g.drawStr( 0, 15, Program[uiCurrentProgram].Name);
    
    
    u8g.setFont(u8g_font_5x7);
    u8g.drawStr( 80, 8, "5000 LUX");
    u8g.drawStr( 80, 18, "3500 LUX");
    
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr( 0, 48, "Outer T:25C HUM: 29%");
    //u8g.drawStr( 0, 39, "Tank  T:29C HUM: 95%");
    //u8g.drawStr( 0, 49, "Level: 95%");
    
    //u8g.setFont(u8g_font_5x7);
        u8g.setFont(u8g_font_6x10);
    u8g.drawStr( 0, 33, buffer);
    //u8g.drawStr( 80, 49, "00:20:15");
    
    u8g.setFont(u8g_font_6x10);
    u8g.drawStr( 0, 64, "Error string here.");
    
}


void drawMainPage2(void)
{

    
    u8g.setDefaultForegroundColor();
    u8g.drawBox(7, 0, 12, 60);
    u8g.setDefaultBackgroundColor();
    u8g.drawBox(8, 1, 10, 20);

    u8g.setDefaultForegroundColor();

    
    
    u8g.setFont(u8g_font_5x7);
    u8g.drawStr90(0, 0, "Liquid level");


    u8g.setFont(u8g_font_6x10);

    u8g.drawStr( 27, 8, "Plant tank");
    u8g.drawStr( 27, 18, "T: 29C HUM: 95%");

 
    u8g.drawStr( 27, 35, "Liquid T:   22C");
    
    u8g.drawStr( 27, 52, "Power usage 999W");

    
    
}



void drawErrorPage(void)
{
    
    uint8_t uiPos=20;
    
    u8g.setDefaultForegroundColor();
    u8g.drawBox(0, 0, 126, 10);

    u8g.setDefaultBackgroundColor();
    u8g.setFont(u8g_font_tpssr);
    u8g.drawStr( 48, 9, "ERROR");
    u8g.setDefaultForegroundColor();
    
    //u8g.setFont(u8g_font_6x10);
    u8g.setFont(u8g_font_6x10);
    
    for (uint8_t i=0; i<8; i++ )
    {
        if ( CurrentErrors[i].uiErorState )
        {
            u8g.drawStr( 0, uiPos, Errors[CurrentErrors[i].uiErrorCode-1].cErrorText);
            uiPos+=10;
        }

        
    }

//CurrentErrors
//Errors
//    u8g.drawStr( 0, 20, "Error 1");
//    u8g.drawStr( 0, 30, "Error 2");
//    u8g.drawStr( 0, 40, "Error 3");
//    u8g.drawStr( 0, 50, "Error 4");
//    u8g.drawStr( 0, 60, "Error 5");

    
}


//функция для определения номера нажатой кнопки
void get_key(void){
    
    int value = analogRead(KEYBOARD_PIN);
    //Serial.println(value);
    
    //Serial.println(bKeyPressed);
    if ( value > 100 && !bKeyPressed )
    {
        bKeyPressed=true;
        
        if ( value > 100 && value < 200 )
        {
            
            binsideMainMenu = true;
            uiKeyCode = KEY_MENU;
            menu_redraw_required = 1;
        } else if ( value > 300 && value < 400 )
        {
            
            if ( binsideMainMenu ) {
                binsideMainMenu=false; binsideModeMenu=false; binsideManualMenu=false; binsideFloodCycleFreqMenu=false; binsideLightSettingsMenu =false; binsideDateSettingsMenu=false; menu_redraw_required = 1;
                //Save config to NVRAM
                if ( bFloodValuesChanged || bLightValuesChanged )
                {
                    //Save config to NVRAM
                    saveConfig ();
                }
            }
            else
                if ( binsideModeMenu ) { binsideModeMenu=false; binsideMainMenu=true; binsideManualMenu=false; binsideFloodCycleFreqMenu=false; binsideLightSettingsMenu=false; binsideDateSettingsMenu=false; menu_redraw_required = 1; }
                else
                    if ( binsideManualMenu ) { binsideModeMenu=false; binsideMainMenu=true; binsideManualMenu=false; binsideFloodCycleFreqMenu=false; binsideLightSettingsMenu=false; binsideDateSettingsMenu=false; menu_redraw_required = 1; }
                    else
                        if ( binsideFloodCycleFreqMenu ) {
                            binsideModeMenu=false; binsideMainMenu=false; binsideManualMenu=true; binsideFloodCycleFreqMenu=false; binsideLightSettingsMenu=false; binsideDateSettingsMenu=false; menu_redraw_required = 1;
                            //save flood cycle settings on manual flood cycle menu exit
                            if ( uiCurrentProgram == MODEMENU_ITEMS-1 )
                            {
                                //set  value to current config values
                                uiCurrentFloodCycleHours=manual_settings_fcf_hours;
                                uiCurrentFloodCycleMinutes=manual_settings_fcf_minutes;
                                uiCurrentFloodTime=manual_settings_fcf_floodtime;
                            }
                            Program[MODEMENU_ITEMS-1].uiFloodCycleHours=manual_settings_fcf_hours;
                            Program[MODEMENU_ITEMS-1].uiFloodCycleMinutes=manual_settings_fcf_minutes;
                            Program[MODEMENU_ITEMS-1].uiFloodTime=manual_settings_fcf_floodtime;
                        }
                        else
                            if ( binsideLightSettingsMenu ) {
                                binsideModeMenu=false; binsideMainMenu=false; binsideManualMenu=true; binsideFloodCycleFreqMenu=false; binsideLightSettingsMenu=false; binsideDateSettingsMenu=false; menu_redraw_required = 1;
                                //save light settings on manual light menu exit
                                //save flood cycle settings on manual flood cycle menu exit
                                if ( uiCurrentProgram == MODEMENU_ITEMS-1 )
                                {
                                    //set  value to current config values
                                    uiCurrentLightStartHours=manual_settings_lt_hours;
                                    uiCurrentLightStartMinutes=manual_settings_lt_minutes;
                                    uiCurrentLightTimeInHours=manual_settings_lt_lighttime_hours;
                                    uiCurrentLightTimeInMinutes=manual_settings_lt_lighttime_minutes;
                                }
                                Program[MODEMENU_ITEMS-1].uiLightStartHours=manual_settings_lt_hours;
                                Program[MODEMENU_ITEMS-1].uiLightStartMinutes=manual_settings_lt_minutes;
                                Program[MODEMENU_ITEMS-1].uiLightTimeInHours=manual_settings_lt_lighttime_hours;
                                Program[MODEMENU_ITEMS-1].uiLightTimeInMinutes=manual_settings_lt_lighttime_minutes;
                                
                            }
                            else
                                if ( binsideDateSettingsMenu ) { binsideModeMenu=false; binsideMainMenu=true; binsideManualMenu=false; binsideFloodCycleFreqMenu=false; binsideLightSettingsMenu=false; binsideDateSettingsMenu=false; menu_redraw_required = 1; }
            uiKeyCode = KEY_BACK;
            
        }  else if ( value > 450 && value < 600)
        {
            if ( !binsideMainMenu && !binsideModeMenu && !binsideManualMenu && !binsideFloodCycleFreqMenu && !binsideFloodCycleFreqMenu && !binsideLightSettingsMenu && !binsideDateSettingsMenu)
            {
                
                if ( !bMainMenuPage2 && !bErrorPage ) { bMainMenuPage2=true; bErrorPage= false; } else if ( bMainMenuPage2 ) { bMainMenuPage2=false; bErrorPage=true; } else if ( bErrorPage ) { bErrorPage=false; bMainMenuPage2=false;}
                
            }
            
            uiKeyCode = KEY_RIGHT;
            menu_redraw_required = 1;
            //Serial.println(value);
        }  else if ( value > 600 && value < 750 )
        {
            uiKeyCode = KEY_SELECT;
            
            if ( binsideMainMenu ) {
                switch ( mmenu_current )
                {
                    case 0:
                        binsideMainMenu=false;
                        binsideManualMenu=false;
                        binsideModeMenu=true;
                        binsideFloodCycleFreqMenu=false;
                        binsideLightSettingsMenu=false;
                        binsideDateSettingsMenu=false;
                        break;
                    case 1:
                        binsideManualMenu=true;
                        binsideModeMenu=false;
                        binsideMainMenu=false;
                        binsideFloodCycleFreqMenu=false;
                        binsideLightSettingsMenu=false;
                        binsideDateSettingsMenu=false;
                        break;
                    case 2:
                        binsideManualMenu=false;
                        binsideModeMenu=false;
                        binsideMainMenu=false;
                        binsideFloodCycleFreqMenu=false;
                        binsideLightSettingsMenu=false;
                        binsideDateSettingsMenu=true;
                        uiKeyCode = KEY_NONE;
                        binside_date_edit_day=false;
                        binside_date_edit_month=false;
                        binside_date_edit_year=false;
                        binside_date_edit_hours=false;
                        binside_date_edit_minutes=false;
                        binside_date_edit_seconds=false;
                        break;
                }
                
                menu_redraw_required = 1;
            } else if (binsideManualMenu)
            {
                switch ( manualmenu_current )
                {
                    case 0:
                        binsideMainMenu=false;
                        binsideManualMenu=false;
                        binsideModeMenu=false;
                        binsideFloodCycleFreqMenu=true;
                        binsideLightSettingsMenu=false;
                        uiKeyCode = KEY_NONE;
                        binside_fcf_edit_hours=false;
                        binside_fcf_edit_minutes=false;
                        binside_fcf_edit_floodtime=false;
                        break;
                    case 1:
                        binsideLightSettingsMenu=true;
                        binsideMainMenu=false;
                        binsideManualMenu=false;
                        binsideModeMenu=false;
                        binsideFloodCycleFreqMenu=false;
                        uiKeyCode = KEY_NONE;
                        binside_lt_edit_hours=false;
                        binside_lt_edit_minutes=false;
                        binside_lt_edit_lighttime_hours=false;
                        binside_lt_edit_lighttime_minutes=false;
                        break;
                }
                menu_redraw_required = 1;
            }else if (binsideModeMenu)
            {
                uiCurrentProgram = modemenu_current;
                bFloodValuesChanged=true;
                
                //copy all settings from Program to current values
                uiCurrentFloodCycleHours=Program[uiCurrentProgram].uiFloodCycleHours;
                uiCurrentFloodCycleMinutes=Program[uiCurrentProgram].uiFloodCycleMinutes;
                uiCurrentFloodTime=Program[uiCurrentProgram].uiFloodTime;
                uiCurrentLightStartHours=Program[uiCurrentProgram].uiLightStartHours;
                uiCurrentLightStartMinutes=Program[uiCurrentProgram].uiLightStartMinutes;
                uiCurrentLightTimeInHours=Program[uiCurrentProgram].uiLightTimeInHours;
                uiCurrentLightTimeInMinutes=Program[uiCurrentProgram].uiLightTimeInMinutes;
                
                
            }
            
            
        }  else if ( value > 750 && value < 950)
        {
            if ( !binsideMainMenu && !binsideModeMenu && !binsideManualMenu && !binsideFloodCycleFreqMenu && !binsideFloodCycleFreqMenu && !binsideLightSettingsMenu && !binsideDateSettingsMenu)
            {
            
                if ( !bMainMenuPage2 && !bErrorPage) { bMainMenuPage2=false; bErrorPage=true; } else if (bErrorPage){ bMainMenuPage2=true; bErrorPage=false;} else if ( bMainMenuPage2 ) { bMainMenuPage2=false; bErrorPage=false;}
                   
            }
            
            uiKeyCode = KEY_LEFT;
            menu_redraw_required = 1;
        }
        
    } else if ( value < 100 && bKeyPressed )
    {
        bKeyPressed = false;
        uiKeyCode =  KEY_NONE;
    }
    
    
    
    
    
    
}



void drawmMenu(void) {
    uint8_t i, h;
    u8g_uint_t w, d;
    
    u8g.setFont(u8g_font_6x13);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();
    
    h = u8g.getFontAscent()-u8g.getFontDescent();
    w = u8g.getWidth();
    for( i = 0; i < MMENU_ITEMS; i++ ) {
        d = (w-u8g.getStrWidth(mmenu_strings[i]))/2;
        u8g.setDefaultForegroundColor();
        if ( i == mmenu_current ) {
            u8g.drawBox(0, i*h+1, w-2, h);
            u8g.setDefaultBackgroundColor();
        }
        u8g.drawStr(d, i*h, mmenu_strings[i]);
    }
}



void updatemMenu(void) {
    if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
        return;
    }
    last_key_code = uiKeyCode;
    
    switch ( uiKeyCode ) {
        case KEY_RIGHT:
            mmenu_current++;
            if ( mmenu_current >= MMENU_ITEMS )
                mmenu_current = 0;
            menu_redraw_required = 1;
            break;
        case KEY_LEFT:
            if ( mmenu_current == 0 )
                mmenu_current = MMENU_ITEMS;
            mmenu_current--;
            menu_redraw_required = 1;
            break;
    }
}





void drawmodeMenu(void) {
    uint8_t i, h;
    u8g_uint_t w, d;
    
    u8g.setFont(u8g_font_6x13);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();
    
    h = u8g.getFontAscent()-u8g.getFontDescent();
    w = u8g.getWidth();
    for( i = 0; i < MODEMENU_ITEMS; i++ ) {
        d = (w-u8g.getStrWidth(Program[i].Name))/2;
        u8g.setDefaultForegroundColor();
        if ( i == modemenu_current ) {
            u8g.drawBox(0, i*h+1, w-2, h);
            u8g.setDefaultBackgroundColor();
        }
        u8g.drawStr(d, i*h, Program[i].Name);
    }
}

void updatemodeMenu(void) {
    if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
        return;
    }
    last_key_code = uiKeyCode;
    
    switch ( uiKeyCode ) {
        case KEY_RIGHT:
            modemenu_current++;
            if ( modemenu_current >= MODEMENU_ITEMS )
                modemenu_current = 0;
            menu_redraw_required = 1;
            break;
        case KEY_LEFT:
            if ( modemenu_current == 0 )
                modemenu_current = MODEMENU_ITEMS;
            modemenu_current--;
            menu_redraw_required = 1;
            break;
    }
}



void drawmanualMenu(void) {
    uint8_t i, h;
    u8g_uint_t w, d;
    
    u8g.setFont(u8g_font_6x13);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();
    
    h = u8g.getFontAscent()-u8g.getFontDescent();
    w = u8g.getWidth();
    for( i = 0; i < MANUALMENU_ITEMS; i++ ) {
        d = (w-u8g.getStrWidth(manualmenu_strings[i]))/2;
        u8g.setDefaultForegroundColor();
        if ( i == manualmenu_current ) {
            u8g.drawBox(0, i*h+1, w-2, h);
            u8g.setDefaultBackgroundColor();
        }
        u8g.drawStr(d, i*h, manualmenu_strings[i]);
    }
}

void updatemanualMenu(void) {
    if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
        return;
    }
    last_key_code = uiKeyCode;
    
    switch ( uiKeyCode ) {
        case KEY_RIGHT:
            manualmenu_current++;
            if ( manualmenu_current >= MANUALMENU_ITEMS )
                manualmenu_current = 0;
            menu_redraw_required = 1;
            break;
        case KEY_LEFT:
            if ( manualmenu_current == 0 )
                manualmenu_current = MANUALMENU_ITEMS;
            manualmenu_current--;
            menu_redraw_required = 1;
            break;
    }
}


void drawFloodCycleFrequencyPage(void)
{
    // graphic commands to redraw the complete screen should be placed here
    //u8g.setDefaultForegroundColor();
    //u8g.setDefaultBackgroundColor();
    
    char charBufHours[3];
    char charBufMinutes[3];
    char charBufFloodTime[3];
    String strBufHours=String(manual_settings_fcf_hours, DEC);  //declaring string
    String strBufMinutes=String(manual_settings_fcf_minutes, DEC);  //declaring string
    String strBufFloodTime=String(manual_settings_fcf_floodtime, DEC);  //declaring string
    strBufHours.toCharArray(charBufHours,3);
    strBufMinutes.toCharArray(charBufMinutes,3);
    strBufFloodTime.toCharArray(charBufFloodTime,3);
    
    u8g.setFont(u8g_font_tpssr);
    u8g.setDefaultForegroundColor();
    u8g.drawStr( 25, 10, "FLOOD CYCLE");
    
    
    
    u8g.drawStr( 10, 25, "Hours:");
    
    u8g.drawStr( 10, 40, "Minutes:");
    
    u8g.drawStr( 10, 60, "Flood time:");
    
    u8g.drawStr( 80, 60, "min");
    
    if ( binside_fcf_edit_hours )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(58, 14, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 60, 25, charBufHours);
    
    if (  binside_fcf_edit_minutes )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(58, 29, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 60, 40, charBufMinutes);
    
    if (  binside_fcf_edit_floodtime )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(66, 49, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 68, 60, charBufFloodTime);
    
    
    //u8g.setDefaultForegroundColor();
    
}



void updateFloodCycleFrequencyMenu(void) {
    if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
        return;
    }
    last_key_code = uiKeyCode;
    
    switch ( uiKeyCode ) {
        case KEY_SELECT:
            if ( !binside_fcf_edit_hours && !binside_fcf_edit_minutes && !binside_fcf_edit_floodtime)
            {
                binside_fcf_edit_hours = true;
                binside_fcf_edit_minutes = false;
                binside_fcf_edit_floodtime=false;
            } else if ( binside_fcf_edit_hours && !binside_fcf_edit_minutes && !binside_fcf_edit_floodtime)
            {
                binside_fcf_edit_hours=false;
                binside_fcf_edit_floodtime=false;
                binside_fcf_edit_minutes = true;
                
            } else if ( !binside_fcf_edit_hours && binside_fcf_edit_minutes && !binside_fcf_edit_floodtime)
            {
                binside_fcf_edit_hours=false;
                binside_fcf_edit_minutes = false;
                binside_fcf_edit_floodtime=true;
            } else if ( !binside_fcf_edit_hours && !binside_fcf_edit_minutes && binside_fcf_edit_floodtime)
            {
                binside_fcf_edit_hours=false;
                binside_fcf_edit_minutes = false;
                binside_fcf_edit_floodtime=false;
            }
            menu_redraw_required = 1;
            break;
        case KEY_RIGHT:
            if ( binside_fcf_edit_hours && !binside_fcf_edit_minutes &&  !binside_fcf_edit_floodtime && manual_settings_fcf_hours < 23)
            {
                manual_settings_fcf_hours++;
                bFloodValuesChanged=true;
            } else if ( !binside_fcf_edit_hours && binside_fcf_edit_minutes && !binside_fcf_edit_floodtime && manual_settings_fcf_minutes < 59)
            {
                manual_settings_fcf_minutes++;
                bFloodValuesChanged=true;
            } else if ( !binside_fcf_edit_hours && !binside_fcf_edit_minutes && binside_fcf_edit_floodtime && manual_settings_fcf_floodtime < 59)
            {
                manual_settings_fcf_floodtime++;
                bFloodValuesChanged=true;
            }
            
            menu_redraw_required = 1;
            break;
        case KEY_LEFT:
            if ( binside_fcf_edit_hours && !binside_fcf_edit_minutes &&  !binside_fcf_edit_floodtime && manual_settings_fcf_hours > 0)
            {
                manual_settings_fcf_hours--;
                bFloodValuesChanged=true;
            } else if ( !binside_fcf_edit_hours && binside_fcf_edit_minutes &&  !binside_fcf_edit_floodtime && manual_settings_fcf_minutes > 0)
            {
                manual_settings_fcf_minutes--;
                bFloodValuesChanged=true;
            } else if ( !binside_fcf_edit_hours && !binside_fcf_edit_minutes && binside_fcf_edit_floodtime && manual_settings_fcf_floodtime > 0)
            {
                manual_settings_fcf_floodtime--;
                bFloodValuesChanged=true;
            }
            
            menu_redraw_required = 1;
            break;
    }
}


void drawLightTimingsPage(void)
{
    // graphic commands to redraw the complete screen should be placed here
    //u8g.setDefaultForegroundColor();
    //u8g.setDefaultBackgroundColor();
    
    char charBufHours[4];
    char charBufMinutes[3];
    char charBufLightTimeHours[4];
    char charBufLightTimeMinutes[3];
    String strBufHours=String(manual_settings_lt_hours, DEC);  //declaring string
    String strBufMinutes=String(manual_settings_lt_minutes, DEC);  //declaring string
    String strBufLightTimeHours=String(manual_settings_lt_lighttime_hours, DEC);  //declaring string
    String strBufLightTimeMinutes=String(manual_settings_lt_lighttime_minutes, DEC);  //declaring string
    strBufHours.toCharArray(charBufHours,4);
    strBufMinutes.toCharArray(charBufMinutes,3);
    strBufLightTimeHours.toCharArray(charBufLightTimeHours,4);
    strBufLightTimeMinutes.toCharArray(charBufLightTimeMinutes,3);
    
    
    u8g.setFont(u8g_font_tpssr);
    u8g.setDefaultForegroundColor();
    u8g.drawStr( 15, 10, "LIGHT TIME SETTINGS");
    
    
    
    
    u8g.drawStr( 10, 25, "Start at      :");
    
    u8g.drawStr( 10, 40, "Light for     :");
    
    
    
    if ( binside_lt_edit_hours )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(58, 14, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 60, 25, charBufHours);
    
    if (  binside_lt_edit_minutes )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(78, 14, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 80, 25, charBufMinutes);
    
    if (  binside_lt_edit_lighttime_hours )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(58, 29, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 60, 40, charBufLightTimeHours);
    
    if (  binside_lt_edit_lighttime_minutes )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(78, 29, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 80, 40, charBufLightTimeMinutes);
    
    
    //u8g.setDefaultForegroundColor();
    
}



void updateLightTimingsMenu(void) {
    if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
        return;
    }
    last_key_code = uiKeyCode;
    
    switch ( uiKeyCode ) {
        case KEY_SELECT:
            if ( !binside_lt_edit_hours && !binside_lt_edit_minutes && !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes)
            {
                binside_lt_edit_hours = true;
                binside_lt_edit_minutes = false;
                binside_lt_edit_lighttime_hours=false;
                binside_lt_edit_lighttime_minutes=false;
            } else if ( binside_lt_edit_hours && !binside_lt_edit_minutes && !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes)
            {
                binside_lt_edit_hours = false;
                binside_lt_edit_minutes = true;
                binside_lt_edit_lighttime_hours=false;
                binside_lt_edit_lighttime_minutes=false;
            } else if ( !binside_lt_edit_hours && binside_lt_edit_minutes && !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes)
            {
                binside_lt_edit_hours = false;
                binside_lt_edit_minutes = false;
                binside_lt_edit_lighttime_hours=true;
                binside_lt_edit_lighttime_minutes=false;
            } else if ( !binside_lt_edit_hours && !binside_lt_edit_minutes && binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes)
            {
                binside_lt_edit_hours = false;
                binside_lt_edit_minutes = false;
                binside_lt_edit_lighttime_hours=false;
                binside_lt_edit_lighttime_minutes=true;
            } else if ( !binside_lt_edit_hours && !binside_lt_edit_minutes && !binside_lt_edit_lighttime_hours && binside_lt_edit_lighttime_minutes)
            {
                binside_lt_edit_hours = false;
                binside_lt_edit_minutes = false;
                binside_lt_edit_lighttime_hours=false;
                binside_lt_edit_lighttime_minutes=false;
            }
            menu_redraw_required = 1;
            break;
        case KEY_RIGHT:
            if ( binside_lt_edit_hours && !binside_lt_edit_minutes &&  !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes && manual_settings_lt_hours < 23)
            {
                manual_settings_lt_hours++;
                bLightValuesChanged=true;
            } else if ( !binside_lt_edit_hours && binside_lt_edit_minutes &&  !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes && manual_settings_lt_minutes < 59)
            {
                manual_settings_lt_minutes++;
                bLightValuesChanged=true;
            } else if (  !binside_lt_edit_hours && !binside_lt_edit_minutes &&  binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes && manual_settings_lt_lighttime_hours < 23)
            {
                manual_settings_lt_lighttime_hours++;
                bLightValuesChanged=true;
            } else if ( !binside_lt_edit_hours && !binside_lt_edit_minutes &&  !binside_lt_edit_lighttime_hours && binside_lt_edit_lighttime_minutes && manual_settings_lt_lighttime_minutes < 59)
            {
                manual_settings_lt_lighttime_minutes++;
                bLightValuesChanged=true;
            }
            
            menu_redraw_required = 1;
            break;
        case KEY_LEFT:
            if ( binside_lt_edit_hours && !binside_lt_edit_minutes &&  !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes && manual_settings_lt_hours > 0)
            {
                manual_settings_lt_hours--;
                bLightValuesChanged=true;
            } else if ( !binside_lt_edit_hours && binside_lt_edit_minutes &&  !binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes && manual_settings_lt_minutes > 0)
            {
                manual_settings_lt_minutes--;
                bLightValuesChanged=true;
            } else if (  !binside_lt_edit_hours && !binside_lt_edit_minutes &&  binside_lt_edit_lighttime_hours && !binside_lt_edit_lighttime_minutes && manual_settings_lt_lighttime_hours > 0)
            {
                manual_settings_lt_lighttime_hours--;
                bLightValuesChanged=true;
            } else if ( !binside_lt_edit_hours && !binside_lt_edit_minutes &&  !binside_lt_edit_lighttime_hours && binside_lt_edit_lighttime_minutes && manual_settings_lt_lighttime_minutes > 0)
            {
                manual_settings_lt_lighttime_minutes--;
                bLightValuesChanged=true;
            }
            
            menu_redraw_required = 1;
            break;
    }
}


void drawDateSettingsPage(void)
{
    // graphic commands to redraw the complete screen should be placed here
    //u8g.setDefaultForegroundColor();
    //u8g.setDefaultBackgroundColor();
    
    char charBufDay[3];
    char charBufMonth[3];
    char charBufYear[5];
    char charBufHours[3];
    char charBufMinutes[3];
    char charBufSeconds[3];
    
    String strBufDay=String(settings_date_day, DEC);  //declaring string
    String strBufMonth=String(settings_date_month, DEC);  //declaring string
    String strBufYear=String(settings_date_year, DEC);  //declaring string
    String strBufHours=String(settings_date_hours, DEC);  //declaring string
    String strBufMinutes=String(settings_date_minutes, DEC);  //declaring string
    String strBufSeconds=String(settings_date_seconds, DEC);  //declaring string
    
    strBufDay.toCharArray(charBufDay,3);
    strBufMonth.toCharArray(charBufMonth,3);
    strBufYear.toCharArray(charBufYear,5);
    strBufHours.toCharArray(charBufHours,3);
    strBufMinutes.toCharArray(charBufMinutes,3);
    strBufSeconds.toCharArray(charBufSeconds,3);
    
    u8g.setFont(u8g_font_tpssr);
    u8g.setDefaultForegroundColor();
    u8g.drawStr( 2, 10, "DATE 23.12.2015 15:25");
    
    
    
    
    u8g.drawStr( 2, 25, "Day:         Hours:");
    
    u8g.drawStr( 2, 40, "Month:      Minutes:");
    
    u8g.drawStr( 2, 55, "Year:        Seconds:");
    
    
    
    if ( binside_date_edit_day )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(40, 14, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 42, 25, charBufDay);
    
    if (  binside_date_edit_month )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(40, 29, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 42, 40, charBufMonth);
    
    if (  binside_date_edit_year )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(27, 44, 30, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 29, 55, charBufYear);
    
    if (  binside_date_edit_hours )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(110, 14, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 112, 25, charBufHours);
    
    if (  binside_date_edit_minutes )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(110, 29, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 112, 40, charBufMinutes);
    
    if (  binside_date_edit_seconds )
    {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(110, 44, 15, 12);
        u8g.setDefaultBackgroundColor();
    } else u8g.setDefaultForegroundColor();
    u8g.drawStr( 112, 55, charBufSeconds);
    
    
    //u8g.setDefaultForegroundColor();
    
}



void updateDateSettingsMenu(void) {
    if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
        return;
    }
    last_key_code = uiKeyCode;
    
    switch ( uiKeyCode ) {
        case KEY_SELECT:
            if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds)
            {
                binside_date_edit_day=true;
                binside_date_edit_month=false;
                binside_date_edit_year=false;
                binside_date_edit_hours=false;
                binside_date_edit_minutes=false;
                binside_date_edit_seconds=false;
            } else if ( binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds)
            {
                binside_date_edit_day=false;
                binside_date_edit_month=true;
                binside_date_edit_year=false;
                binside_date_edit_hours=false;
                binside_date_edit_minutes=false;
                binside_date_edit_seconds=false;
            } else if ( !binside_date_edit_day && binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds)
            {
                binside_date_edit_day=false;
                binside_date_edit_month=false;
                binside_date_edit_year=true;
                binside_date_edit_hours=false;
                binside_date_edit_minutes=false;
                binside_date_edit_seconds=false;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds)
            {
                binside_date_edit_day=false;
                binside_date_edit_month=false;
                binside_date_edit_year=false;
                binside_date_edit_hours=true;
                binside_date_edit_minutes=false;
                binside_date_edit_seconds=false;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds)
            {
                binside_date_edit_day=false;
                binside_date_edit_month=false;
                binside_date_edit_year=false;
                binside_date_edit_hours=false;
                binside_date_edit_minutes=true;
                binside_date_edit_seconds=false;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && binside_date_edit_minutes && !binside_date_edit_seconds)
            {
                binside_date_edit_day=false;
                binside_date_edit_month=false;
                binside_date_edit_year=false;
                binside_date_edit_hours=false;
                binside_date_edit_minutes=false;
                binside_date_edit_seconds=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && binside_date_edit_seconds)
            {
                binside_date_edit_day=false;
                binside_date_edit_month=false;
                binside_date_edit_year=false;
                binside_date_edit_hours=false;
                binside_date_edit_minutes=false;
                binside_date_edit_seconds=false;
            }
            menu_redraw_required = 1;
            break;
        case KEY_RIGHT:
            if ( binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_day < 31)
            {
                settings_date_day++;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_month < 12)
            {
                settings_date_month++;
                bLightValuesChanged=true;
            } else if (  !binside_date_edit_day && !binside_date_edit_month && binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_year < 2100)
            {
                settings_date_year++;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_hours < 23)
            {
                settings_date_hours++;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_minutes < 59)
            {
                settings_date_minutes++;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && binside_date_edit_seconds && settings_date_seconds < 59)
            {
                settings_date_seconds++;
                bLightValuesChanged=true;
            }
            
            menu_redraw_required = 1;
            break;
        case KEY_LEFT:
            if ( binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_day > 0)
            {
                settings_date_day--;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_month > 1)
            {
                settings_date_month--;
                bLightValuesChanged=true;
            } else if (  !binside_date_edit_day && !binside_date_edit_month && binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_year > 2015)
            {
                settings_date_year--;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && binside_date_edit_hours && !binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_hours > 0)
            {
                settings_date_hours--;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && binside_date_edit_minutes && !binside_date_edit_seconds && settings_date_minutes > 0)
            {
                settings_date_minutes--;
                bLightValuesChanged=true;
            } else if ( !binside_date_edit_day && !binside_date_edit_month && !binside_date_edit_year && !binside_date_edit_hours && !binside_date_edit_minutes && binside_date_edit_seconds && settings_date_seconds > 0)
            {
                settings_date_seconds++;
                bLightValuesChanged=true;
            }
            
            menu_redraw_required = 1;
            break;
    }
}




/*****************************************************************************/

void setup() {

    
    Serial.begin(9600);
    
    loadConfig();
    
    // assign default color value
    if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
        u8g.setColorIndex(255);     // white
    }
    else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
        u8g.setColorIndex(3);         // max intensity
    }
    else if ( u8g.getMode() == U8G_MODE_BW ) {
        u8g.setColorIndex(1);         // pixel on
    }
    else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
        u8g.setHiColorByRGB(255,255,255);
    }
    
    MainDisplayTimer.SetTimer(0,0,30);
    MainDisplayTimer.StartTimer();
    
    menu_redraw_required = 1;     // force initial redraw
    
    //CurrentErrors[4].uiErorState=true;
    //CurrentErrors[4].uiErrorCode=2;
    //CurrentErrors[7].uiErorState=true;
    //CurrentErrors[7].uiErrorCode=5;

}

void loop() {
    
    
    get_key();
    //delay(20);
    
    
    MainDisplayTimer.Timer();
    
    if ( MainDisplayTimer.TimeCheck(0,0,0) )
    {
        if ( !bErrorPage )
        {
            if ( !bMainMenuPage2 )
            {
                bMainMenuPage2=true;
            
            } else
            {
                bMainMenuPage2=false;
            }
        }
        
        MainDisplayTimer.ResetTimer();
    }
    
    /***************************************************************************************************/
    /*                                      Flood & Light timers                                       */
    /***************************************************************************************************/
    
   

    
    
    if ( !bFloodValuesChanged )
    {
        if ( !bFloodCycleRun && !bFloodStayRun )
        {
            FloodCycleTimer.SetTimer(uiCurrentFloodCycleHours,uiCurrentFloodCycleMinutes,0);
            FloodCycleTimer.StartTimer();
            bFloodCycleRun = true;
        }
        
        if ( bFloodCycleRun )
        {
            FloodCycleTimer.Timer();
        }
        
        if ( FloodCycleTimer.TimeCheck(0,0,0)  && bFloodCycleRun )
        {
            FloodCycleTimer.StopTimer();
            bFloodCycleRun=false;
            FloodStayTimer.SetTimer(0,uiCurrentFloodTime,0);
            FloodStayTimer.StartTimer();
            bFloodStayRun=true;
            
            //switch pump relay on
            
            Serial.println("Flooding started");
            
        }
        
        
        if ( bFloodStayRun )
        {
            FloodStayTimer.Timer();
        }
        
        
        if ( FloodStayTimer.TimeCheck(0,0,0) && bFloodStayRun )
        {
            FloodStayTimer.StopTimer();
            bFloodCycleRun=true;
            FloodCycleTimer.SetTimer(uiCurrentFloodCycleHours,uiCurrentFloodCycleMinutes,0);
            FloodCycleTimer.StartTimer();
            bFloodStayRun=false;
            //switch pump relay off
            Serial.println("Flooding stopped");
        }
    } else
    {
        FloodCycleTimer.StopTimer();
        FloodStayTimer.StopTimer();
        
        //switch pump relay off
        
        bFloodCycleRun=false;
        bFloodStayRun=false;
        FloodCycleTimer.SetTimer(uiCurrentFloodCycleHours,uiCurrentFloodCycleMinutes,0);
        FloodCycleTimer.StartTimer();
        bFloodCycleRun = true;
        Serial.println("Floodtimer restarted");

    }
    /***************************************************************************************************/
    /*                                      End Flood & Light timers                                   */
    /***************************************************************************************************/
    
    

    
    if ( binsideMainMenu )
    {
        
        if (  menu_redraw_required != 0 ) {
            u8g.firstPage();
            do  {
                drawmMenu();
            } while( u8g.nextPage() );
            menu_redraw_required = 0;
            
        }
        
        updatemMenu();
        
    } else if ( !binsideMainMenu && !binsideModeMenu && !binsideManualMenu && !binsideFloodCycleFreqMenu && !binsideFloodCycleFreqMenu && !binsideLightSettingsMenu && !binsideDateSettingsMenu)
    {
        //if (  menu_redraw_required != 0 ) {
        u8g.firstPage();
        do {
            
            if ( !bMainMenuPage2 && !bErrorPage ) { drawMainPage(); } else if (!bErrorPage && bMainMenuPage2) {drawMainPage2();} else if (bErrorPage) {drawErrorPage();}
        } while( u8g.nextPage() );
        //menu_redraw_required = 0;
        //}
    } else if ( binsideModeMenu )
    {
        if (  menu_redraw_required != 0 ) {
            u8g.firstPage();
            do {
                drawmodeMenu();
            } while( u8g.nextPage() );
            menu_redraw_required = 0;
        }
        
        updatemodeMenu();
    } else if ( binsideManualMenu )
    {
        if (  menu_redraw_required != 0 ) {
            u8g.firstPage();
            do {
                drawmanualMenu();
            } while( u8g.nextPage() );
            menu_redraw_required = 0;
        }
        
        updatemanualMenu();
    } else if ( binsideFloodCycleFreqMenu )
    {
        if (  menu_redraw_required != 0 ) {
            u8g.firstPage();
            do {
                drawFloodCycleFrequencyPage();
            } while( u8g.nextPage() );
            menu_redraw_required = 0;
        }
        
        updateFloodCycleFrequencyMenu();
    } else if ( binsideLightSettingsMenu )
    {
        if (  menu_redraw_required != 0 ) {
            u8g.firstPage();
            do {
                drawLightTimingsPage();
            } while( u8g.nextPage() );
            menu_redraw_required = 0;
        }
        
        updateLightTimingsMenu();
    }else if ( binsideDateSettingsMenu )
    {
        if (  menu_redraw_required != 0 ) {
            u8g.firstPage();
            do {
                drawDateSettingsPage();
            } while( u8g.nextPage() );
            menu_redraw_required = 0;
        }
        
        updateDateSettingsMenu();
    }
    
    
    //if first run - start flood for x minutes
    //
    
    //delay(100);
    
}





