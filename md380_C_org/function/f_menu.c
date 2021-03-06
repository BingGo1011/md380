//======================================================================
//
// 菜单处理
//
// Time-stamp: <2012-08-14 12:01:32  Shisheng.Zhi, 0354>
//
//======================================================================

#include "f_menu.h"
#include "f_main.h"
#include "f_runSrc.h"
#include "f_frqSrc.h"
#include "f_io.h"
#include "f_ui.h"
#include "f_eeprom.h"
#include "f_comm.h"
#include "f_posCtrl.h"
#include "f_invPara.h"
#include "f_fcDeal.h"
#include "f_p2p.h"

#if F_DEBUG_RAM

#define DEBUG_F_USER_MENU_MODE              0   // 用户菜单模式
#define DEBUG_F_CHECK_MENU_MODE             0   // 校验菜单模式
#define DEBUG_F_NO_SAME                     0   // NoSameDeal
#define DEBUG_F_DISP_DIDO_STATUS_SPECIAL    0   // DIDO直观显示
#define DEBUG_F_GROUP_HIDE                  0   // groupHide
#define DEBUG_F_MOTOR_FUNCCODE1             0
#define DEBUG_F_MFK                         0   // MFK处理
#define DEBUG_F_PASSWORD                    0   // 密码处理
#define DEBUG_F_ERROR_TUNE_USE_SHIFT        0   // 故障/调谐时可以使用shift
#define DEBUG_FRQ_POINT                     0
#define DEBUG_ACC_DEC_TIME_POINT            0

#elif 1

#define DEBUG_F_NO_SAME                     1
#define DEBUG_F_USER_MENU_MODE              1
#define DEBUG_F_CHECK_MENU_MODE             1
#define DEBUG_F_DISP_DIDO_STATUS_SPECIAL    1
#define DEBUG_F_GROUP_HIDE                  1
#define DEBUG_F_MOTOR_FUNCCODE1             1
#define DEBUG_F_MFK                         1
#define DEBUG_F_PASSWORD                    1
#define DEBUG_F_ERROR_TUNE_USE_SHIFT        1   // 故障/调谐时可以使用shift
#define DEBUG_FRQ_POINT                     1
#define DEBUG_ACC_DEC_TIME_POINT            1

#endif




// 2级菜单可以使用shift按键
// 1,2级菜单闪烁, 输入用户密码时闪烁
//#define NEWMENU_MENU3_USE_LEFT_SFIFT    0  // 3级菜单可以使用MFK按键作为左移
//#define NEWMENU_REMEMBER_GRADE          0  // 记忆group的grade


// 十进制的个位、十位、百位、千位、万位
const int16 decNumber[5] = {1,      10,     100,    1000, 10000};
// 十六进制的个位、十位、百位、千位
const int16 hexNumber[4] = {0x0001, 0x0010, 0x0100, 0x1000};
const int16 deltaK[3] = {0, 1, -1};

int16 userMenuModeFcIndex;



#if 0
struct RESTORE_COMPANY_PARA_EXCEPT_INDEX
{
    Uint16 start;
    Uint16 end;
};

#define RESTORE_COMPANY_PARA_EXCEPT_NUMBER  6
static const struct RESTORE_COMPANY_PARA_EXCEPT_INDEX exceptRestoreSeries[RESTORE_COMPANY_PARA_EXCEPT_NUMBER] =
{
{GetCodeIndex(funcCode.group.ff[0]), GetCodeIndex(funcCode.group.ff[FFNUM-1])}, // FF 厂家参数
{GetCodeIndex(funcCode.group.fp[0]), GetCodeIndex(funcCode.group.fp[FPNUM-1])}, // FP 功能码管理
{GetCodeIndex(funcCode.group.f1[0]), GetCodeIndex(funcCode.group.f1[F1NUM-2])}, // F1 电机参数
{GetCodeIndex(funcCode.group.a3[0]), GetCodeIndex(funcCode.group.a3[F1NUM-2])}, // A3 第2电机参数
{GetCodeIndex(funcCode.group.ae[0]), GetCodeIndex(funcCode.group.ae[AENUM-1])}, // AE AIAO出厂校正
{GetCodeIndex(funcCode.code.errorLatest1), LAST_ERROR_RECORD_INDEX},    // 第一次故障类型，..., 最后一个故障记录
};
#endif


Uint16 GetDispDigits(Uint16 index);

struct FC_MOTOR_DEBUG   // 性能调试使用功能码组的个数
{
    Uint16 fc;  // 功能码
    Uint16 u;   // 监视
};
struct FC_MOTOR_DEBUG motorDebugFc;


enum MENU0_DISP_STATUS menu0DispStatus;     // 0级菜单的显示状态
void UpdateMenu0DispStatus(void);

struct MENU_FUNC_CODE
{
    Uint16 group;
    Uint16 grade;
};
struct MENU_FUNC_CODE menuFc[MENU_MODE_MAX+1];      // 切换菜单模式时记忆group和grade
// 菜单模式
enum MENU_MODE menuMode;        // 菜单模式
enum MENU_MODE menuModeTmp;     // 菜单模式，临时值
enum MENU_MODE menuModeOld;

enum MENU_MODE_OPERATE menuModeStatus;
enum FAC_PASS_RANDOM_VIEW_OPERATE facPassViewStatus;

#if DEBUG_F_NO_SAME
// 频率源，功能码的值要互斥
#define FRQ_SRC_NO_SAME_NUMER   2
const Uint16 frqSrcFuncIndex[] =
{
    GetCodeIndex(funcCode.code.frqXSrc),
    GetCodeIndex(funcCode.code.frqYSrc),
};

// DI，
const Uint16 diFuncIndex[] =
{
    GetCodeIndex(funcCode.code.diFunc[0]),
    GetCodeIndex(funcCode.code.diFunc[1]),
    GetCodeIndex(funcCode.code.diFunc[2]),
    GetCodeIndex(funcCode.code.diFunc[3]),
    GetCodeIndex(funcCode.code.diFunc[4]),

    GetCodeIndex(funcCode.code.diFunc[5]),
    GetCodeIndex(funcCode.code.diFunc[6]),
    GetCodeIndex(funcCode.code.diFunc[7]),
    GetCodeIndex(funcCode.code.diFunc[8]),
    GetCodeIndex(funcCode.code.diFunc[9]),

    GetCodeIndex(funcCode.code.vdiFunc[0]),
    GetCodeIndex(funcCode.code.vdiFunc[1]),
    GetCodeIndex(funcCode.code.vdiFunc[2]),
    GetCodeIndex(funcCode.code.vdiFunc[3]),
    GetCodeIndex(funcCode.code.vdiFunc[4]),

    GetCodeIndex(funcCode.code.aiAsDiFunc[0]),
    GetCodeIndex(funcCode.code.aiAsDiFunc[1]),
    GetCodeIndex(funcCode.code.aiAsDiFunc[2]),
};
#endif



#define DECIMAL_DISPLAY_UPDATE_TIME     6       // 0级菜单显示(运行时显示，停机时显示)小数点后2位显示缓冲更新时间，_*12ms
#define UP_DOWN_DEAL_DONE_TIME          800     // UP/DOWN处理之后的处理时间，用于停止闪烁或者快速闪烁，_ms




#define ONE_PLACE           0   // 个位
#define TEN_PLACE           1   // 十位
#define HUNDRED_PLACE       2   // 百位
#define THOUSAND_PLACE      3   // 千位
#define TEN_THOUSAND_PLACE  4   // 万位


//===================================================================
enum MENU_LEVEL menuLevel;      // 当前菜单级别，即0,1,2,3级菜单
Uint16 menu3Number;             // 3级菜单显示的值
Uint16 menuPwdNumber;           // pwd菜单的值

struct CURRENT_FUNC_CODE
{
    Uint16 index;               // 当前功能码在funcCode.all[]数组的下标

    Uint16 group;               // 当前功能码的group
    Uint16 grade;               // 当前功能码的grade
};
struct CURRENT_FUNC_CODE curFc; // 当前功能码
Uint16 curFcDispDigits;         // 当前功能码的显示位数


LOCALF Uint16 ticker4LowerDisp;  // 运行时显示，最后2位不要更新过快

Uint16 superFactoryPass;
Uint16 groupHidePwdStatus;          // 功能码组隐藏
Uint16 groupHideChkOkFlag;          // 功能码组隐藏密码校验ok

#define FC_READ_ONLY_FLAG                                               \
    ((!funcCode.code.userPasswordReadOnly) ||                           \
    (curFc.index == GetCodeIndex(funcCode.code.userPassword)) ||        \
    (curFc.index == GetCodeIndex(funcCode.code.userPasswordReadOnly))   \
    )

// 是否可写。1-当前可写，0-当前不可写
#define IsWritable(attribute)                                       \
 ((FC_READ_ONLY_FLAG) &&                                            \
 ((ATTRIBUTE_READ_AND_WRITE == (attribute).bit.writable) ||         \
 ((ATTRIBUTE_READ_ONLY_WHEN_RUN == (attribute).bit.writable) &&     \
 (!runFlag.bit.run)))                                               \
)                                                                   \

enum FACTORY_PWD_STATUS
{
    FACTORY_PWD_LOCK,          // 厂家密码, lock状态
    FACTORY_PWD_UNLOCK         // 厂家密码, unlock状态
};
LOCALF enum FACTORY_PWD_STATUS factoryPwdStatus;    // 厂家密码，默认为0(lock)
// 初始值为lock状态。
// 仅当：进入FF-00，输入正确密码后enter，factoryPwdStatus ==> unlock状态
// 当：退回到2级菜单(FF-xx -> Fx)
//     进入不是FF组的3级菜单(FF-xx -> Fx-xx，enter)
//     factoryPwdStatus ==> lock状态

Uint16 accDecFrqPrcFlag;   // 键盘UP/DOWN修改频率标志
Uint16 bFrqDigitalDone4WaitDelay; // UP/DOWN完成之后一段时间的显示处理时间
LOCALF Uint16 accDecFrqTicker;
Uint16 frqDisp;
Uint16 frqAimDisp;
Uint16 frqPLCDisp;       // PLC可编程卡读取运行
Uint16 frqAimPLCDisp;    // PLC卡编程卡读取设定频率
Uint16 pidFuncRefDisp;
Uint16 pidFuncFdbDisp;
Uint16 outVoltageDisp;
Uint16 outCurrentDisp;      // 输出电流，实际值，
Uint16 itDisp;              // 输出转矩
Uint16 loadSpeedDisp;
Uint16 currentOcDisp;
Uint16 pulseInFrqDisp;
Uint16 frqRunDisp;
Uint16 pcOriginDisp;
Uint16 frqXDisp;
Uint16 frqYDisp;
Uint32 torqueCurrentAct;   // 转矩电流

// 包示的文件
// U组，停机状态显示等
#include "f_funcCode_disp.c"


// 按下RUN键
LOCALD void MenuOnRun(void);

// 按下STOP键
LOCALD void MenuOnStop(void);

// 按下MF.K键
LOCALD void MenuOnMfk(void);


// 按下PRG键
LOCALD void Menu0OnPrg(void);
void Menu1OnPrg(void);
LOCALD void Menu2OnPrg(void);
LOCALD void Menu3OnPrg(void);

// 按下UP键
LOCALD void Menu0OnUp(void);    // 0级菜单下按键UP的函数
LOCALD void Menu1OnUp(void);    // 1级菜单下按键UP的函数
LOCALD void Menu2OnUp(void);    // 2级菜单下按键UP的函数
LOCALD void Menu3OnUp(void);    // 3级菜单下按键UP的函数

// 按下DOWN键
LOCALD void Menu0OnDown(void);
LOCALD void Menu1OnDown(void);
LOCALD void Menu2OnDown(void);
LOCALD void Menu3OnDown(void);

// 按下ENTER键
LOCALD void Menu0OnEnter(void);
LOCALD void Menu1OnEnter(void);
LOCALD void Menu2OnEnter(void);
LOCALD void Menu3OnEnter(void);

// 按下SHIFT键
LOCALD void Menu0OnShift(void);
LOCALD void Menu1OnShift(void);
LOCALD void Menu2OnShift(void);
LOCALD void Menu3OnShift(void);

// 按下QUICK键
LOCALD void MenuOnQuick(void);


LOCALD void Menu0OnUpDown(void);
LOCALD void Menu1OnUpDown(Uint16 flag);
LOCALD void Menu2OnUpDown(Uint16 flag);
LOCALD void Menu3OnUpDown(Uint16 flag);


// 显示缓冲更新
LOCALD void UpdateMenu0DisplayBuffer(void);
LOCALD void UpdateMenu1DisplayBuffer(void);
LOCALD void UpdateMenu2DisplayBuffer(void);
LOCALD void UpdateMenu3DisplayBuffer(void);
LOCALD void UpdateDisplayBufferAttribute(const Uint16 data, const union FUNC_ATTRIBUTE attribute);
void UpdateDisplayBufferVisualIoStatus(Uint32 value);
void UpdateDisplayBufferVisualDiFunc(Uint16 valueH, Uint32 valueL);
LOCALD void UpdateErrorDisplayBuffer(void);
LOCALD void UpdateTuneDisplayBuffer(void);


LOCALD void MenuPwdOnPrg(void);
LOCALD void MenuPwdHintOnUp(Uint16 flag);
LOCALD void MenuPwdHint2Input(void);
LOCALD void MenuPwdHintOnDown(Uint16 flag);
LOCALD void MenuPwdHintOnShift(void);
LOCALD void MenuPwdHintOnQuick(void);
LOCALD void UpdateMenuPwdHintDisplayBuffer(void);

LOCALD void MenuPwdInputOnPrg(void);
LOCALD void MenuPwdInputOnUp(void);
LOCALD void MenuPwdInputOnEnter(void);
LOCALD void MenuPwdInputOnDown(void);
LOCALD void MenuPwdInputOnShift(void);
LOCALD void MenuPwdInputOnQuick(void);
LOCALD void UpdateMenuPwdInputDisplayBuffer(void);

void MenuPwdInputOnUpDown(Uint16 flag);

void MenuModeSwitch(void);


struct MENU_ATTRIBUTE menuAttri[MENU_LEVEL_NUM];
#if F_DEBUG_RAM
#pragma DATA_SECTION(menu, "const_zone");
#endif
const sysMenu menu[MENU_LEVEL_NUM] =
{
// 0级菜单
    {Menu0OnPrg,        Menu0OnUpDown,      Menu0OnEnter,
     MenuOnMfk,         Menu0OnUpDown,      Menu0OnShift,
     MenuOnRun,         MenuOnStop,         MenuOnQuick,
     UpdateMenu0DisplayBuffer},
// 1级菜单
    {Menu1OnPrg,        Menu1OnUp,          Menu1OnEnter,
     MenuOnMfk,         Menu1OnDown,        Menu1OnShift,
     MenuOnRun,         MenuOnStop,         MenuOnQuick,
     UpdateMenu1DisplayBuffer},
// 2级菜单
    {Menu2OnPrg,        Menu2OnUp,          Menu2OnEnter,
     MenuOnMfk,         Menu2OnDown,        Menu2OnShift,
     MenuOnRun,         MenuOnStop,         MenuOnQuick,
     UpdateMenu2DisplayBuffer},
// 3级菜单
    {Menu3OnPrg,        Menu3OnUp,          Menu3OnEnter,
     MenuOnMfk,         Menu3OnDown,        Menu3OnShift,
     MenuOnRun,         MenuOnStop,         MenuOnQuick,
     UpdateMenu3DisplayBuffer},
// PwdHint菜单
    {MenuPwdOnPrg,      MenuPwdHint2Input,  MenuPwdHint2Input,
     MenuOnMfk,         MenuPwdHint2Input,  MenuPwdHintOnShift,
     MenuOnRun,         MenuOnStop,         MenuPwdHintOnQuick,
     UpdateMenuPwdHintDisplayBuffer},
// PwdInput菜单
    {MenuPwdOnPrg,      MenuPwdInputOnUp,   MenuPwdInputOnEnter,
     MenuOnMfk,         MenuPwdInputOnDown, MenuPwdInputOnShift,
     MenuOnRun,         MenuOnStop,         MenuPwdInputOnQuick,
     UpdateMenuPwdInputDisplayBuffer},
};
//(void (*)(void))Menu0OnPrg

   
// 翻转处理函数
LOCALD Uint16 OverTurnDeal(Uint16 data, Uint16 upper, Uint16 lower, Uint16 flag);
// 0级菜单循环移位函数
LOCALD void cycleShiftDeal(Uint16 flag);



struct GROUP_DISPLAY
{
    Uint16 dispF;   // 功能码组F0, 显示F
    Uint16 disp0;   // 功能码组F0, 显示0
};
struct GROUP_DISPLAY groupDisplay;
void UpdateGroupDisplay(Uint16 group);

Uint16 GroupUpDown(const Uint16 funcCodeGrade[], Uint16 group, Uint16 flag);
void DealUserMenuModeGroupGrade(Uint16 flag);
void GetGroupGrade(Uint16 index);

//-------------------------------------------------
Uint16 checkMenuModeCmd;    // 搜索指令。1-开始搜索，0-无搜索指令/搜索完成
Uint16 checkMenuModePara;   // 搜索指令的参数
enum CHECK_MENU_MODE_DEAL checkMenuModeDealStatus;
Uint16 checkMenuModeSerachNone; // 搜索，没有找到与出厂值不同的功能码，标志。
// 1-没有找到与出厂值不同的功能码
// 0-找到了与出厂值不同的功能码
void DealCheckMenuModeGroupGrade(Uint16 flag);
//-------------------------------------------------

Uint16 LimitOverTurnDeal(const Uint16 limit[], Uint16 data, Uint16 upper, Uint16 low, Uint16 flag);
void MotorDebugFcDeal(void);
void fghldf(Uint16 dest[], const Uint16 src[], Uint16 length);
void Menu0AddMenuLevel(void);
Uint16 ValidateTuneCmd(Uint16 value, Uint16 motorIndex);




void GroupHideDeal(Uint16 funcCodeGrade[]);


//=====================================================================
//
// 所有菜单级别下，按下run键的处理
// run按键在 运行命令函数中处理，直接使用当前的按键。
//
//=====================================================================
LOCALF void MenuOnRun(void)
{
    ;
}


//=====================================================================
//
// 所有菜单级别下，按下stop键的处理
// run按键在 运行命令函数中处理，直接使用当前的按键。
//
//=====================================================================
LOCALF void MenuOnStop(void)
{
    ;
}



// quick处理
void MenuOnQuick(void)
{
    Uint16 digit[5];
    enum MENU_MODE menuModeNext[3];

    // 仅有零级索引则QUICK无效
    if (!funcCode.code.menuMode)
    {
        menuModeTmp = MENU_MODE_BASE;
        return;
    }
    
    if (MENU_MODE_ON_QUICK == menuModeStatus)   // 第1次按键，显示当前模式；之后才改变模式
    {
        GetNumberDigit1(digit, funcCode.code.menuMode);
        if (digit[0])
		{
            menuModeNext[0] = MENU_MODE_USER;
		}
        else
        {
            menuModeNext[0] = MENU_MODE_CHECK;
        }

        if (digit[1])
		{
            menuModeNext[1] = MENU_MODE_CHECK;
		}
        else
        {
            menuModeNext[1] = MENU_MODE_BASE;
        }

        menuModeNext[2] = MENU_MODE_BASE;

        menuModeTmp = menuModeNext[((Uint16)menuModeTmp) - 1];
        
    }

    menuModeStatus = MENU_MODE_ON_QUICK;
}



LOCALF void Menu1OnUp(void)
{
    Menu1OnUpDown(ON_UP_KEY);
}


LOCALF void Menu2OnUp(void)
{
    Menu2OnUpDown(ON_UP_KEY);
}


LOCALF void Menu3OnUp(void)
{
    Menu3OnUpDown(ON_UP_KEY);
}



LOCALF void Menu1OnDown(void)
{
    Menu1OnUpDown(ON_DOWN_KEY);
}


LOCALF void Menu2OnDown(void)
{
    Menu2OnUpDown(ON_DOWN_KEY);
}


LOCALF void Menu3OnDown(void)
{
    Menu3OnUpDown(ON_DOWN_KEY);
}


//=====================================================================
//
// 所有菜单级别下，按下MF.K键的处理
//
//=====================================================================
LOCALF void MenuOnMfk(void)
{
#if DEBUG_F_MFK
    if (tuneCmd)
    {
        return;
    }

    switch (funcCode.code.mfkKeyFunc)
    {
        case FUNCCODE_mfkKeyFunc_SWITCH: // 与操作面板命令通道切换
            // F0-00功能码设定值为操作面板，MF.K键的操作面板命令源通切换命令不起作用
            if (FUNCCODE_runSrc_PANEL != funcCode.code.runSrc)
            {
                keyFunc = KEY_SWITCH;
            }
            break;

        case FUNCCODE_mfkKeyFunc_REVERSE:   // 正反转切换
            if (FUNCCODE_runSrc_PANEL == runSrc)    // 当前命令源通道为操作面板
            {
                keyFunc = KEY_REV;
            }
            break;

        case FUNCCODE_mfkKeyFunc_FWD_JOG:
            keyFunc = KEY_FWD_JOG;
            break;

        case FUNCCODE_mfkKeyFunc_REV_JOG:
            keyFunc = KEY_REV_JOG;
            break;
            
        default:
            break;
    }
#endif
}


//=====================================================================
//
// menu0, 零级菜单
//
//=====================================================================
LOCALF void Menu0OnPrg(void)
{
    if (tuneCmd)
    {
        if (!runFlag.bit.tune)
        {
            menuLevel = MENU_LEVEL_2;    // 取消调谐，回到2级菜单
            tuneCmd = 0;
        }
        else
            Menu0AddMenuLevel();
//        return;
    }

#if DEBUG_F_PASSWORD
    else if ((funcCode.code.userPassword)    // 有用户密码
            && (menuMode != MENU_MODE_USER)  // 当前不为用户定制参数模式
//       || (funcCode.code.userPasswordReadOnly)
        )
    {
        menuLevel = MENU_LEVEL_PWD_HINT;    // 进入PWD_HINT菜单，提示进行密码输入
    }
    else                        // 没有密码
#endif
    {
        Menu0AddMenuLevel();
    }

    menuAttri[menuLevel].operateDigit = 0;
}


LOCALF void Menu0OnEnter(void)
{
    if (MENU_MODE_ON_QUICK == menuModeStatus)       // 按下QUICK后，再按键ENTER
    {
        MenuModeSwitch();
        return;
    }
}


void MenuModeSwitch(void)
{
    menuModeOld = menuMode;

    menuModeStatus = MENU_MODE_NONE;
        
    // 切换菜单模式时记忆group和grade
    menuFc[menuMode].group = curFc.group;       // 保存old
    menuFc[menuMode].grade = curFc.grade;
    menuMode = menuModeTmp;
    curFc.group = menuFc[menuModeTmp].group;    // 恢复new
    curFc.grade = menuFc[menuModeTmp].grade;

// 若恢复之后的group为厂家参数组，且
// menuMode被改变，厂家密码，lock状态
    if ((menuModeOld != menuMode)
        && (FC_GROUP_FACTORY == curFc.group)
        )
    {
        factoryPwdStatus = FACTORY_PWD_LOCK;
        curFc.grade = 0;                        // 进入lock状态，grade清零
    }

    MenuModeDeal();
	
// 改变menuMode时，3级菜单更改为2级菜单。
    if (menuModeOld == MENU_MODE_USER)
    {
        Menu0OnPrg();
    }
    else if ((menuLevel == MENU_LEVEL_3)
        && (menuModeOld != menuMode)
        )
    {
        menuLevel = MENU_LEVEL_2;
    }
// -C-模式没有1级菜单
// 0/1级菜单下改变为-C-模式，进入2级菜单
    else if (((menuLevel == MENU_LEVEL_0) || (menuLevel == MENU_LEVEL_1))
        && (MENU_MODE_CHECK == menuMode)
        )
    {
        Menu0OnPrg();
    }
// 用户定制菜单无1级菜单
// 0/1级菜单下改变为用户定制模式，进入2级菜单
    else if (((menuLevel == MENU_LEVEL_0) || (menuLevel == MENU_LEVEL_1))
        && (MENU_MODE_USER == menuMode)
        )
    {
        Menu0OnPrg();
    }
// 0级菜单下更改，进入1级菜单
    else if (menuLevel == MENU_LEVEL_0)
    {
        Menu0OnPrg();
    }

// 改变menuMode时，当前操作bit为0
    if (menuModeOld != menuMode)
    {
        menuAttri[menuLevel].operateDigit = 0;
    }

    
}


LOCALF void Menu0OnUpDown(void)
{
    if (!tuneCmd)
    {
        accDecFrqPrcFlag = ACC_DEC_FRQ_WAIT;
        accDecFrqTicker = 0;
        frqKeyUpDownDelta = upDownDelta;
    }
}


LOCALF void Menu0OnShift(void)
{
    ticker4LowerDisp = 0;       // 0级菜单下按键shift，显示最后2位数字

#if DEBUG_F_ERROR_TUNE_USE_SHIFT
// 0级菜单下，有故障时按键shift仍然有效
    if (MENU0_DISP_STATUS_RUN_STOP != menu0DispStatus)
    {
        menu0DispStatus = MENU0_DISP_STATUS_RUN_STOP;   // 进入运行/停机显示状态
    }
    else
#elif 1
    if ((!tuneCmd)
        && (!errorCode)         // 0级菜单下，有故障时按键shift无效
        )
#endif
    {
        cycleShiftDeal(1);      // 0级菜单显示的循环移位处理
    }
}


//=====================================================================
//
// menu1, 一级菜单
//
//=====================================================================
void Menu1OnPrg(void)
{
    menuLevel = MENU_LEVEL_0;
    groupHideChkOkFlag = 0;

    ticker4LowerDisp = 0;   // 菜单级别重新置为0时，ticker清零
}


LOCALF void Menu1OnEnter()
{
    if (MENU_MODE_ON_QUICK == menuModeStatus)       // 按下QUICK后，再按键ENTER
    {
        MenuModeSwitch();
        return;
    }
    
    menuLevel = MENU_LEVEL_2;
    menuAttri[menuLevel].operateDigit = 0;

    if (FC_GROUP_FACTORY == curFc.group)    // 进入FF组，grade重置，在输入正确密码后再恢复
        curFc.grade = 0;
}


LOCALF void Menu1OnUpDown(Uint16 flag)
{
    curFc.group = GroupUpDown(funcCodeGradeCurMenuMode, curFc.group, flag);

// 不记忆grade，在修改group时，grade就清零
    curFc.grade = 0;
}


LOCALF void Menu1OnShift(void)
{
    ;
}


//=====================================================================
//
// menu2, 二级菜单
//
//=====================================================================
LOCALF void Menu2OnPrg(void)
{
    menuLevel = MENU_LEVEL_1;
    
    if ((MENU_MODE_CHECK == menuMode)
        || (MENU_MODE_USER == menuMode)
        )
    {
        //menuLevel = MENU_LEVEL_0;
        Menu1OnPrg();
    }

//  退回到2级菜单(FF-xx -> Fx)
//  factoryPwdStatus ==> lock状态
    factoryPwdStatus = FACTORY_PWD_LOCK;
}


LOCALF void Menu2OnEnter(void)
{
    if (MENU_MODE_ON_QUICK == menuModeStatus)       // 按下QUICK后，再按键ENTER
    {
        MenuModeSwitch();
        return;
    }

    if ((MENU_MODE_CHECK == menuMode)
        && (checkMenuModeSerachNone)
        )
    {
        return;
    }
    
//     进入不是FF组的3级菜单(FF-xx -> Fx-00，enter)
//     factoryPwdStatus ==> lock状态
    if (curFc.group != FC_GROUP_FACTORY)
    {
        factoryPwdStatus = FACTORY_PWD_LOCK;
    }

    menuAttri[MENU_LEVEL_2].operateDigit = 0;
    curFc.index = GetGradeIndex(curFc.group, curFc.grade);

    {
        menuLevel = MENU_LEVEL_3;

        menuAttri[MENU_LEVEL_3].operateDigit = 0;
        menu3Number = funcCode.all[curFc.index];

        // 确定功能码显示位数
        curFcDispDigits = GetDispDigits(curFc.index);
    }
}


LOCALF void Menu2OnUpDown(Uint16 flag)
{
    if (MENU_MODE_USER == menuMode)
    {
        DealUserMenuModeGroupGrade(flag);
    }
    else if (MENU_MODE_CHECK == menuMode)
    {
        checkMenuModeCmd = 1;
        checkMenuModePara = flag;
    }
    else if (menuAttri[MENU_LEVEL_2].operateDigit >= 3)   // 修改currentGroup
    {
        Menu1OnUpDown(flag);
    }
    else                        // 修改currentGrade
    {
        int16 delta = 1;
        Uint16 tmp;

        if ((FC_GROUP_FACTORY == curFc.group) && (FACTORY_PWD_LOCK == factoryPwdStatus)) // 厂家密码
        {
            Menu2OnEnter();     // FF时，按键UP/DOWN也可以进入密码输入状态
            return;
        }

#if 1
        tmp = OverTurnDeal(curFc.grade, funcCodeGradeCurMenuMode[curFc.group] - 1, 0, flag);
        if (curFc.grade == tmp)    // 没有翻转
        {
            if (1 == menuAttri[MENU_LEVEL_2].operateDigit)
                delta = 10;

            if (ON_DOWN_KEY == flag)
                delta = -delta;

            curFc.grade = LimitDeal(0, curFc.grade, funcCodeGradeCurMenuMode[curFc.group] - 1, 0, delta);
        }
        else
        {
            curFc.grade = tmp;
        }
#elif 1
        if (1 == menuAttri[MENU_LEVEL_2].operateDigit)
            delta = 10;

        if (ON_DOWN_KEY == flag)
            delta = -delta;

        curFc.grade += delta;
        if ((int16)curFc.grade >= (int16)(funcCodeGradeCurMenuMode[curFc.group])
        {
            curFc.grade = 0;
        }
        else if ((int16)curFc.grade < 0)
        {
            curFc.grade = funcCodeGradeCurMenuMode[curFc.group] - 1;
        }
#endif
    }
}


LOCALF void Menu2OnShift(void)
{
// 用户定制
// 非出厂值
// 菜单模式
// 没有shift
    if (MENU_MODE_BASE == menuMode)
    {
        if (menuAttri[MENU_LEVEL_2].operateDigit == 0)
            menuAttri[MENU_LEVEL_2].operateDigit = 3;
        else if (menuAttri[MENU_LEVEL_2].operateDigit == 1)
            menuAttri[MENU_LEVEL_2].operateDigit = 0;
        else if (menuAttri[MENU_LEVEL_2].operateDigit == 3)
            menuAttri[MENU_LEVEL_2].operateDigit = 1;
    }
}


//=====================================================================
//
// menu3, 三级菜单
//
//=====================================================================
LOCALF void Menu3OnPrg(void)
{
    menuLevel = MENU_LEVEL_2;
    menuAttri[MENU_LEVEL_2].operateDigit = 0;
}


LOCALF void Menu3OnEnter(void)
{
    Uint16 writable = funcCodeAttribute[curFc.index].attribute.bit.writable;

    if (MENU_MODE_ON_QUICK == menuModeStatus)       // 按下QUICK后，再按键ENTER
    {
        MenuModeSwitch();
        return;
    }

// 正在操作EEPROM，不响应"保存功能码"，但是响应其他按键。
    if (FUNCCODE_RW_MODE_NO_OPERATION != funcCodeRwMode)
    {
        if ((FACTORY_PWD_INDEX != curFc.index)        // FF-00
            && (ATTRIBUTE_READ_ONLY_ANYTIME != writable)    // (任何时候都)只读的功能码
            )
        {
            return;
        }
    }

// 对于运行中只读的功能码，在停机时修改了(但未按ENTER)，再运行，按ENTER，不响应。
// 对于任何时候都只读的功能码，不要这么处理。
    if ((runFlag.bit.run)      // 运行
        && (ATTRIBUTE_READ_ONLY_WHEN_RUN == writable) // 运行时只读
        && (funcCode.all[curFc.index] != menu3Number) // 值被改变
        )
    {
        return;
    }

    // U组功能码全部只读
    if (curFc.group >= FUNCCODE_GROUP_U0)
    {
        return;
    }

#if DEBUG_F_RESTORE_COMPANY_PARA_DEAL
    // FP-01参数初始化，FF-00厂家密码，调谐，不要保存
    if (GetCodeIndex(funcCode.code.paraInitMode) == curFc.index) // FP-01, 参数初始化
    {
        if (FUNCCODE_paraInitMode_CLEAR_RECORD == menu3Number) // 清除记录
        {
            ClearRecordDeal();
        }
        else
        {
            if (FUNCCODE_paraInitMode_RESTORE_COMPANY_PARA == menu3Number) // FP-01==1
            {
                funcCodeRwModeTmp = FUNCCODE_paraInitMode_RESTORE_COMPANY_PARA;
            }
            else if (FUNCCODE_paraInitMode_RESTORE_COMPANY_PARA_ALL == menu3Number) // FP-01==3
            {
                //funcCodeRwModeTmp = FUNCCODE_paraInitMode_RESTORE_COMPANY_PARA_ALL;
            }
            else if (FUNCCODE_paraInitMode_SAVE_USER_PARA == menu3Number) // FP-01==4
            {
                funcCodeRwModeTmp = FUNCCODE_paraInitMode_SAVE_USER_PARA;
            }
            else if (FUNCCODE_paraInitMode_RESTORE_USER_PARA == menu3Number) // FP-01==5
            {
                if ((funcCode.code.saveUserParaFlag1 == USER_PARA_SAVE_FLAG1)
                    && (funcCode.code.saveUserParaFlag2 == USER_PARA_SAVE_FLAG2))
                {
                    // 恢复已保存的用户参数
                    funcCodeRwModeTmp = FUNCCODE_paraInitMode_RESTORE_USER_PARA;
                }
                else
                {
                    // 之前未进行用户参数保存操作,该功能无效
                    return;
                }
            }
        }
    }
    else
#endif
        if (FACTORY_PWD_INDEX == curFc.index) // FF-00, 厂家密码
//    else if ((DISPLAY_F == curFc.group) && (FACTORY_PWD_LOCK == factoryPwdStatus)) // FF-00, 厂家密码
    {
        if (COMPANY_PASSWORD != menu3Number) // 厂家密码不正确
        {
            menu3Number = 0;
            menuAttri[MENU_LEVEL_3].operateDigit = 0;
            return;
        }
        else
        {
            factoryPwdStatus = FACTORY_PWD_UNLOCK;    // 厂家密码正确，解锁
        }
    }
    else if (GetCodeIndex(funcCode.code.tuneCmd) == curFc.index) // 电机1调谐
    {
        if (COMM_ERR_PARA == ValidateTuneCmd(menu3Number, MOTOR_SN_1))
            return;
    }
    else if (GetCodeIndex(funcCode.code.motorFcM2.tuneCmd) == curFc.index) // 电机2调谐
    {
        if (COMM_ERR_PARA == ValidateTuneCmd(menu3Number, MOTOR_SN_2))
            return;
    }
    else if (GetCodeIndex(funcCode.code.motorFcM3.tuneCmd) == curFc.index) // 电机3调谐
    {
        if (COMM_ERR_PARA == ValidateTuneCmd(menu3Number, MOTOR_SN_3))
            return;
    }
    else if (GetCodeIndex(funcCode.code.motorFcM4.tuneCmd) == curFc.index) // 电机4调谐
    {
        if (COMM_ERR_PARA == ValidateTuneCmd(menu3Number, MOTOR_SN_4))
            return;
    }
    else if (ATTRIBUTE_READ_ONLY_ANYTIME != writable) // 非 任何时候都只读 的功能码
    // 除上面几个功能码之外，修改的值都需要保存
    {
// 某些互斥的功能码。ModifyFunccodeEnter()中会对funcCode赋值。
// 须放在 功率改变处理 和 机型改变处理 后面。
        if (COMM_ERR_PARA == ModifyFunccodeEnter(curFc.index, menu3Number))
            return;

        //funcCode.all[curFc.index] = menu3Number; // RAM
        SaveOneFuncCode(curFc.index);  // 这时不用考虑与EEPROM中的值是否相同。
    }

    if (MENU_MODE_USER == menuMode)
    {
        DealUserMenuModeGroupGrade(ON_UP_KEY);
    }
    else if (MENU_MODE_CHECK == menuMode)
    {
        checkMenuModeCmd = 1;
        checkMenuModePara = ON_UP_KEY;
    }
    else if (++curFc.grade >= funcCodeGradeCurMenuMode[curFc.group])
    {
        curFc.grade = 0;
    }

    // 调谐功能码，进入调谐状态，menuLevel改为0级。
    if ((tuneCmd) &&
        ((curFc.index == GetCodeIndex(funcCode.code.tuneCmd)) 
        || (curFc.index == GetCodeIndex(funcCode.code.motorFcM2.tuneCmd)) 
        || (curFc.index == GetCodeIndex(funcCode.code.motorFcM3.tuneCmd)) 
        || (curFc.index == GetCodeIndex(funcCode.code.motorFcM4.tuneCmd)) )
        )
    {
        menuLevel = MENU_LEVEL_0;
    }
    else
    {
        menuLevel = MENU_LEVEL_2;
        menuAttri[MENU_LEVEL_2].operateDigit = 0;
    }
}



// 确认调谐命令是否有效
Uint16 ValidateTuneCmd(Uint16 value, Uint16 motorsN)
{
    if (motorsN != motorSn)       // 是否为当前选择电机
    {
        return COMM_ERR_PARA;
    }
    
    if (MOTOR_TYPE_PMSM == motorFc.motorPara.elem.motorType)    // pmsm
    {
        if ((FUNCCODE_tuneCmd_PMSM_11 != value) &&
            (FUNCCODE_tuneCmd_PMSM_12 != value) &&
            (FUNCCODE_tuneCmd_PMSM_13 != value)
            )
        {
            return COMM_ERR_PARA;
        }
    }
    else        // 异步机
    {
        if ((FUNCCODE_tuneCmd_ACI_STATIC != value) &&
            (FUNCCODE_tuneCmd_ACI_WHOLE != value)
            )
        {
            return COMM_ERR_PARA;
        }
    }

    if ((errorCode == ERROR_NONE) && (FUNCCODE_runSrc_PANEL == runSrc))       // 仅面板命令通道，才能调谐
    {
        tuneCmd = value;
        return COMM_ERR_NONE;
    }
    else
    {
        return COMM_ERR_PARA;
    }
}



LOCALF void Menu3OnUpDown(Uint16 flag)
{
    int16 delta;
    Uint16 flag1 = 1;

    if (curFc.group >= FUNCCODE_GROUP_U0) // U组，显示
    {   // U组，3级菜单下，UP/DOWN自动到next grade，同时需要更新currentIndex
        Menu2OnUpDown(flag);
        curFc.index = GetGradeIndex(curFc.group, curFc.grade);

        flag1 = 0;
    }
#if DEBUG_F_USER_MENU_MODE
    else if (USER_MENU_GROUP == curFc.group)     // FE组，用户定制功能码
    {
        if (2 == menuAttri[MENU_LEVEL_3].operateDigit)
        {
            Uint16 group;
            Uint16 funcCodeGrade[FUNCCODE_GROUP_NUM];   // 堆栈够用
            
            memcpy(funcCodeGrade, funcCodeGradeAll, (FUNCCODE_GROUP_NUM));          
            funcCodeGrade[FC_GROUP_FACTORY] = 0;   // 用户定制菜单不可设置FF组
            funcCodeGrade[USER_MENU_GROUP] = 0;  // 用户定制菜单不可设置CC组

#if DEBUG_F_MOTOR_FUNCCODE
            MotorDebugFcDeal();
            funcCodeGrade[FUNCCODE_GROUP_CF] = motorDebugFc.fc;
            funcCodeGrade[FUNCCODE_GROUP_UF] = motorDebugFc.u;
#endif

            group = GroupUpDown(funcCodeGrade, menu3Number / 100, flag);
            menu3Number = group * 100; // 改变组时，grade清零

            flag1 = 0;
        }
    }
#endif

    if (flag1)  // 真的需要UP/DOWN
    {
        // 十进制约束
        if (ATTRIBUTE_MULTI_LIMIT_HEX != funcCodeAttribute[curFc.index].attribute.bit.multiLimit)
        {
            delta = decNumber[menuAttri[MENU_LEVEL_3].operateDigit];
        }
        else    // 多个功能码，十六进制
        {
            delta = hexNumber[menuAttri[MENU_LEVEL_3].operateDigit];
        }

        if (ON_DOWN_KEY == flag)
            delta = -delta;

        ModifyFunccodeUpDown(curFc.index, &menu3Number, delta);
    }
}


LOCALF void Menu3OnShift(void)
{
    Uint16 max = curFcDispDigits;
    
#if DEBUG_F_USER_MENU_MODE
    if (USER_MENU_GROUP == curFc.group)     // FE组，用户定制功能码
    {
        max = 3;
    }
#endif

    if (!menuAttri[MENU_LEVEL_3].operateDigit)
        menuAttri[MENU_LEVEL_3].operateDigit = max - 1;
    else
        menuAttri[MENU_LEVEL_3].operateDigit--;
}


//=====================================================================
//
// 功能码限幅处理
//
// 输入:
// signal--- 需要限幅处理的功能码的signal，
// data  --- 修改之前的数据
// upper --- 功能码上限
// lower --- 功能码下限
// delta --- delta>0, UP; delta<0, DOWN; delta==0, 检测当前值是否在限值范围内
//
// 返回：修改之后的值
//
// 注意：
// 在有符号时，要考虑以下情况：
//   upper = 30000, data = +25000, delta = +10000   ==> data = 30000
// 在无符号时，要考虑以下情况：
//   lower = 5,     data = 4,       delta = -0      ==> data = 5
//   lower = 5,     data = 6,       delta = -10     ==> data = 5
//   upper = 65535, data = 60000,   delta = +10000  ==> data = 65535
//
//=====================================================================
Uint16 LimitDeal(Uint16 signal, Uint16 data, Uint16 upper, Uint16 lower, int16 delta)
{
    int32 data1, upper1, lower1;

    if (signal) // 有符号
    {
        data1 = (int32)(int16)data;
        upper1 = (int32)(int16)upper;
        lower1 = (int32)(int16)lower;
    }
    else        // 无符号
    {
        data1 = (int32)data;
        upper1 = (int32)upper;
        lower1 = (int32)lower;
    }

// 相当于对原值也进行比较了
    data1 += delta;
    if (data1 > upper1)         // 上限处理
    {
        data1 = upper1;
    }
    if (data1 < lower1)         // 下限处理
    {
        data1 = lower1;
    }

    return (Uint16)data1;
}


//=====================================================================
//
// 某些功能码的设定不能相同。目前280有DI端子，320还有主辅频率源
//
// 输入：
// index        -- 需要进行处理功能码的index
// funcIndex[]  -- 这些设定不能相同功能码的index数组
// number       -- funcIndex[]的长度
// data         -- 当前功能码，想设定的值
// upper        -- 功能码上限
// lower        -- 功能码下限
// delta        -- 功能码值当前可以增加的delta。正数--UP, 负数--DOWN，0--仅判断是否与其他功能码设定相同
//
// 返回:
//      delta不为0时，返回与其他功能码设定不同的数据。
//      delta为0时，相同则返回0，否则返回输入值。
//
//=====================================================================
Uint16 NoSameDeal(Uint16 index, const Uint16 funcIndex[], int16 number, int16 data, int16 upper, int16 lower, int16 delta)
{
#if DEBUG_F_NO_SAME
    Uint16 i;

#if 1
    // 翻转
    if ((data == upper) && (delta > 0))
    {
        data = lower;       // 0(是无功能，)可以重复。
    }
    else if ((data == lower) && (delta < 0))
    {
        data = upper;       // upper不可以重复，还要进行处理。
    }
    else
#endif
    {
        data = LimitDeal(0, data, upper, lower, delta); // 目前都是无符号
    }

    if (data == lower)      // 0是无功能，可以重复。返回。
        return data;

    for (;;)
    {
        for (i = 0; i < number; i++)    // 对所有不能重复的index
        {
            if ((funcIndex[i] != index) && (data == funcCode.all[funcIndex[i]])) // 若与其他功能码设定相同
            {
                if (delta > 0)
                {
                    data += 1;              // 遇到重复，自动加1(也许可以改为delta)跳过。
                    if (data > upper)       // 若加1之后超过最大值，变为最小值。
                    {
                        data = lower;       // 0是无功能，可以重复
                    }
                }
                else if (delta < 0)
                {
                    data -= 1;              // 遇到重复，自动减1(也许可以改为delta)跳过
                    if (data < lower)       // 若减1之后小于最小值，变为最大值。
                    {
                        data = upper;       // 最大值继续比较
                    }
                }
                else
                {
                    data = lower;
                }

                break;
            }
        }

        if ((i >= number) ||    // 一直没有重复
            (data == lower)     // 遇到重复，自动加1跳过。若加1之后超过最大值，改为最小值。
            )
            break;
    }

    return data;
#endif
}


//=====================================================================
//
// 0级菜单显示的循环移位函数。
// 输入:
//      flag -- 1 按键shift之后， 调用本函数
//              0 改变功能码之后，调用本函数
//
//
//=====================================================================
LOCALF void cycleShiftDeal(Uint16 flag)
{
    Uint16 *bit = &funcCode.code.dispParaStopBit;
    Uint32 para = funcCode.code.ledDispParaStop;
    Uint16 max = STOP_DISPLAY_NUM;

    if (runFlag.bit.run)
    {
        bit = &funcCode.code.dispParaRunBit;
        para = funcCode.code.ledDispParaRun1 + ((Uint32)funcCode.code.ledDispParaRun2 << 16);
        max = RUN_DISPLAY_NUM;
    }

    if (!para)  // 防止设置为0
    {
        para = 1;
    }

    if ((!flag) && (para & (0x1UL << *bit)))    // 改变功能码，且当前显示bit仍然要显示
        return;

    do
    {
        if (++(*bit) >= max)
        {
            if (errorCode)  // 有故障
            {
                menu0DispStatus = MENU0_DISP_STATUS_ERROR;  // 进入故障/告警显示状态
            }

            if (tuneCmd)
            {
                menu0DispStatus = MENU0_DISP_STATUS_TUNE;   // 进入调谐显示
            }

            *bit = 0;
        }
    }
    while (!(para & (0x1UL << *bit)));
}


//=====================================================================
//
// 翻转处理函数
//
// 输入:
//      data  -- 可能需要翻转的数据
//      upper -- 上限
//      lower -- 下限
//      flag  -- UP/DOWN标志
//
// 返回：
//      翻转之后的值。
//
//=====================================================================
LOCALF Uint16 OverTurnDeal(Uint16 data, Uint16 upper, Uint16 lower, Uint16 flag)
{
    if (ON_UP_KEY == flag)
    {
        if (upper == data)
        {
            data = lower;
        }
    }
    else
    {
        if (lower == data)
        {
            data = upper;
        }
    }

    return data;
}

//=====================================================================
//
// 0级菜单下的显示缓冲更新函数, 12ms调用1次
// 运行/停机时LED显示，正在修改设定频率时，不显示数据最前面的0
// 显示的闪烁，暂时不使用滤波
//
//=====================================================================
LOCALF void UpdateMenu0DisplayBuffer(void)
{
    Uint16 menu0Number;             // 0级菜单显示的值
    union FUNC_ATTRIBUTE attributeMenu0;

    UpdateMenu0DispStatus();

    if (MENU0_DISP_STATUS_RUN_STOP == menu0DispStatus)
    {
        if (bFrqDigital || bFrqDigitalDone4WaitDelay)   // 正在修改设定频率, 或者修改之后还在一定时间内
        {
            if (!bFrqDigital)
            {
                if (++accDecFrqTicker >= UP_DOWN_DEAL_DONE_TIME / 12)     // _*12ms. 在0级菜单按键UP/DOWN改变频率完成后，暂停一会
                {
                    accDecFrqTicker = 0;
                    bFrqDigitalDone4WaitDelay = 0;
                }
            }

            menu0Number = frqAimDisp;
            attributeMenu0 = funcCodeAttribute[MAX_FRQ_INDEX].attribute;    // 显示属性与MAX_FRQ一致
            attributeMenu0.bit.point = funcCode.code.frqPoint;
//                menuAttri[MENU_LEVEL_0].winkFlag = 0;        // 在0级菜ハ掳聪耈P/DOWN时, 之后_时间也不闪烁
        }
        else
        {
            Uint16 bitDisp;

            if (runFlag.bit.run) // 运行时LED显示
            {
                bitDisp = funcCode.code.dispParaRunBit;
            }
            else                      // 停机时LED显示
            {
                if (funcCode.code.dispParaStopBit >= STOP_DISPLAY_NUM)
                {
                    funcCode.code.dispParaStopBit = 0;
                }
                bitDisp = stopDispIndex[funcCode.code.dispParaStopBit];

                menuAttri[MENU_LEVEL_0].winkFlag = 0xf8; // 0级菜单下停机时全闪烁
            }

            attributeMenu0 = dispAttributeU0[bitDisp];

            // 运行频率、设定频率显示，小数点
            if ((DISP_FRQ_RUN == bitDisp) 
                || (DISP_FRQ_AIM == bitDisp)
                || (DISP_FRQ_RUN_FDB == bitDisp)
                )
            {
                attributeMenu0.bit.point = funcCode.code.frqPoint;
            }
            // 小数点
            else if (DISP_OUT_CURRENT == bitDisp)  // 输出电流
            {
                if (invPara.type > invPara.pointLimit)
                    attributeMenu0.bit.point--;
            }
            else if (DISP_LOAD_SPEED == bitDisp) // 负载速度显示
            {
                attributeMenu0.bit.point = funcCode.code.speedDispPointPos; // 速度显示小数点位
            }
            
            menu0Number = funcCode.group.u0[bitDisp];          
            
        }

        UpdateDisplayBufferAttribute(menu0Number, attributeMenu0);
    }
    else if (MENU0_DISP_STATUS_ERROR == menu0DispStatus)    // 故障/告警显示
    {
        UpdateErrorDisplayBuffer();
    }
    else if (MENU0_DISP_STATUS_TUNE == menu0DispStatus)     // 调谐显示
    {
        UpdateTuneDisplayBuffer();
    }
}


// 更新0级菜单显示状态
void UpdateMenu0DispStatus(void)
{
    static Uint16 errorCodeOld4Menu0;
    static Uint16 tuneCmdOld4Menu0;

    if ((!errorCodeOld4Menu0)
        && errorCode
        )       // 从无故障到有故障
    {
        menu0DispStatus = MENU0_DISP_STATUS_ERROR;  // 进入故障/告警显示状态
    }
    errorCodeOld4Menu0 = errorCode;

    if ((!tuneCmdOld4Menu0)
        && tuneCmd
        )
    {
        menu0DispStatus = MENU0_DISP_STATUS_TUNE;   // 进入调谐显示
    }
    tuneCmdOld4Menu0 = tuneCmd;

    if ((!errorCode)
        && (!tuneCmd)
        )
    {
        menu0DispStatus = MENU0_DISP_STATUS_RUN_STOP;
    }
}


//=====================================================================
//
// 1级菜单下的显示缓冲更新函数
//
//=====================================================================
LOCALF void UpdateMenu1DisplayBuffer(void) // 1级菜单显示：@@@FX
{
    UpdateGroupDisplay(curFc.group);

// 数码管显示
    displayBuffer[0] = DISPLAY_CODE[DISPLAY_NULL];
    displayBuffer[1] = DISPLAY_CODE[DISPLAY_NULL];
    displayBuffer[2] = DISPLAY_CODE[DISPLAY_NULL];
    displayBuffer[3] = DISPLAY_CODE[groupDisplay.dispF];
    displayBuffer[4] = DISPLAY_CODE[groupDisplay.disp0];

//    menuAttri[MENU_LEVEL_1].winkFlag = 0x08;    // 闪烁最后一位，与MD320不同
}


//=====================================================================
//
// 2级菜单下的显示缓冲更新函数
//
//=====================================================================
LOCALF void UpdateMenu2DisplayBuffer(void) // 显示 FX-XX
{
    Uint16 digit[5];
    Uint16 flag = 0;

    if ((MENU_MODE_CHECK == menuMode)
        && (checkMenuModeSerachNone)
        )
    {
        flag = 1;
    }
    else if ((MENU_MODE_USER == menuMode)
        && (0 == funcCode.code.userCustom[userMenuModeFcIndex])
        )
    {
        flag = 2;
    }

    if (!flag)
    {
        UpdateGroupDisplay(curFc.group);

        GetNumberDigit(digit, curFc.grade, DECIMAL);

        // 数码管显示
        displayBuffer[0] = DISPLAY_CODE[groupDisplay.dispF];
        displayBuffer[1] = DISPLAY_CODE[groupDisplay.disp0];
        displayBuffer[2] = DISPLAY_CODE[DISPLAY_LINE];
        displayBuffer[3] = DISPLAY_CODE[digit[1]];
        displayBuffer[4] = DISPLAY_CODE[digit[0]];

        if (MENU_MODE_CHECK == menuMode)        // 非出厂值模式
        {
            displayBuffer[0] = DISPLAY_CODE[DISPLAY_c];// & DISPLAY_CODE[DISPLAY_DOT];
            displayBuffer[1] = DISPLAY_CODE[groupDisplay.dispF];
            displayBuffer[2] = DISPLAY_CODE[groupDisplay.disp0] & DISPLAY_CODE[DISPLAY_DOT];
        }
        else if (MENU_MODE_USER == menuMode)    // 用户定制模式
        {
            displayBuffer[0] = DISPLAY_CODE[DISPLAY_u];// & DISPLAY_CODE[DISPLAY_DOT];
            displayBuffer[1] = DISPLAY_CODE[groupDisplay.dispF];
            displayBuffer[2] = DISPLAY_CODE[groupDisplay.disp0] & DISPLAY_CODE[DISPLAY_DOT];
        }

        menuAttri[MENU_LEVEL_2].winkFlag = 0x01 << (3 + menuAttri[MENU_LEVEL_2].operateDigit);
    }
    else
    {
        // 数码管显示
        if (1 == flag)
        {
            displayBuffer[0] = DISPLAY_CODE[DISPLAY_c] & DISPLAY_CODE[DISPLAY_DOT];
        }
        else
        {
            displayBuffer[0] = DISPLAY_CODE[DISPLAY_u] & DISPLAY_CODE[DISPLAY_DOT];
        }
        
        displayBuffer[1] = DISPLAY_CODE[DISPLAY_N];
        displayBuffer[2] = DISPLAY_CODE[DISPLAY_U];
        displayBuffer[3] = DISPLAY_CODE[DISPLAY_L];
        displayBuffer[4] = DISPLAY_CODE[DISPLAY_L];
    }
}


//=====================================================================
//
// 3级菜单下的显示缓冲更新函数
// 更新displayBuffer[]
//
//=====================================================================
LOCALF void UpdateMenu3DisplayBuffer(void)
{
    union FUNC_ATTRIBUTE attribute;
    Uint16 tmp = menu3Number;

#if (DEBUG_F_DISP_DIDO_STATUS_SPECIAL)
// DIDO状态直观显示
    if ((curFc.index == GetCodeIndex(funcCode.group.u0[DISP_DI_STATUS_SPECIAL1])) || 
        (curFc.index == GetCodeIndex(funcCode.group.u0[DISP_DO_STATUS_SPECIAL1]))
        )
    {
        Uint32 value;

        // DI输入状态直观显示，DI1-DI10, VDI1-VDI5, AI1asDI-AI3asDI
        if (GetCodeIndex(funcCode.group.u0[DISP_DI_STATUS_SPECIAL1]) == curFc.index)
        {
            value = diStatus.a.all & 0x000FFFFF;
        }
        // DO输入状态直观显示，FMR,RELAY1,RELAY2,DO1,DO2,VDO1-VDO5
        else if (GetCodeIndex(funcCode.group.u0[DISP_DO_STATUS_SPECIAL1]) == curFc.index)
        {
            value = doStatus.a.all & 0x000FFFFF;
        }
        
        UpdateDisplayBufferVisualIoStatus(value);
        
        return;
    }
// DI功能状态直观显示
    else if ((curFc.index == GetCodeIndex(funcCode.group.u0[DISP_DI_FUNC_SPECIAL1])) ||
             (curFc.index == GetCodeIndex(funcCode.group.u0[DISP_DI_FUNC_SPECIAL2]))
             )
    {
        Uint16 high;        // 高8位
        Uint32 low;         // 低32位

        // DI功能状态直观显示1，diFunc1-diFunc40
        if (GetCodeIndex(funcCode.group.u0[DISP_DI_FUNC_SPECIAL1]) == curFc.index)
        {
            high = (diFunc.f2.all >> 1) & 0x00FF;
            low = ((diFunc.f2.all & 0x0001) << 31) + (diFunc.f1.all >> 1);
        }
        // DI功能状态直观显示2，diFunc41-diFunc80
        else if (GetCodeIndex(funcCode.group.u0[DISP_DI_FUNC_SPECIAL2]) == curFc.index)
        {
            high = 0;
            low = diFunc.f2.all >> 9;
        }
        
        UpdateDisplayBufferVisualDiFunc(high, low);
        
        return;
    }
#endif

    if (FUNCCODE_GROUP_U0 == curFc.group)               // U0，显示
    {
        attribute = dispAttributeU0[curFc.grade];      
        //funcCode.all[curFc.index] = *pDispValueU0[curFc.grade]; // UpdateU0Data()中更新
    }
    
#if DEBUG_F_POSITION_CTRL
    else if (FUNCCODE_GROUP_U0 + 1 == curFc.group)          // U1，显示
    {
        attribute = dispAttributeU1[curFc.grade];
    }
#endif
#if DEBUG_F_MOTOR_FUNCCODE
    else if (FUNCCODE_GROUP_UF == curFc.group)          // UF，显示
    {
        attribute.all = UF_VIEW_ATTRIBUTE;
    }
#endif
#if DEBUG_F_PLC_CTRL
    else if (FUNCCODE_GROUP_U0 + 3 == curFc.group)          // U3，显示
    {
        attribute.all = 0x1040;
    }
#endif
    else    // 功能码组，包括U1组
    {
        attribute = funcCodeAttribute[curFc.index].attribute;
    }

    // 非面板命令通道，调谐功能码不闪烁。
    if (((TUNE_CMD_INDEX_1 == curFc.index) 
        || (TUNE_CMD_INDEX_2 == curFc.index)
        || (TUNE_CMD_INDEX_3 == curFc.index)
        || (TUNE_CMD_INDEX_4 == curFc.index)
        )
// 380目前，不可通讯调谐
        && (FUNCCODE_runSrc_PANEL != runSrc)
        )
    {
        attribute.bit.writable = ATTRIBUTE_READ_ONLY_ANYTIME;
    }

    if (IsWritable(attribute))             // 当前可修改
    {
        menuAttri[MENU_LEVEL_3].winkFlag = 0x01 << (3 + menuAttri[MENU_LEVEL_3].operateDigit);
    }
    else                                    // 当前不可修改
    {
        tmp = funcCode.all[curFc.index];    // 随时更新显示为功能码的值
    }

    
    // 最近一次故障频率
    if (GetCodeIndex(funcCode.code.errorScene3.elem.errorFrq) == curFc.index)
    {
        attribute.bit.point = funcCode.code.errorFrqUnit & 0x000F;
    }
    // 第二次故障频率
    else if (GetCodeIndex(funcCode.code.errorScene2.elem.errorFrq) == curFc.index)
    {
        attribute.bit.point = (funcCode.code.errorFrqUnit >> 4) & 0x000F;
    }
    // 第一次故障频率
    else if (GetCodeIndex(funcCode.code.errorScene1.elem.errorFrq) == curFc.index)
    {
        attribute.bit.point = (funcCode.code.errorFrqUnit >> 8 ) & 0x000F;
    }
    // 负载速度显示
    else if(GetCodeIndex(funcCode.group.u0[DISP_LOAD_SPEED]) == curFc.index)
    {
        attribute.bit.point = funcCode.code.speedDispPointPos;
    }  
#if DEBUG_ACC_DEC_TIME_POINT
    // 加减速时间的小数点
    else if ((GetCodeIndex(funcCode.code.accTime1) == curFc.index) ||
        (GetCodeIndex(funcCode.code.accTime2) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.accTime3) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.accTime4) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.jogAccTime) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.decTime1) == curFc.index) ||
        (GetCodeIndex(funcCode.code.decTime2) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.decTime3) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.decTime4) == curFc.index)  ||
        (GetCodeIndex(funcCode.code.jogDecTime) == curFc.index) 
        )
    {
        attribute.bit.point = funcCode.code.accDecTimeUnit;
    }
#endif    
// 频率指令单位
#if DEBUG_FRQ_POINT
    else if (((attribute.bit.point == 2)  // 单位为0.01Hz或是F4-12 端子UP/DN速率
        && (attribute.bit.unit == 1)) 
        || (GetCodeIndex(funcCode.code.diUpDownSlope)  == curFc.index)
        )
    {
        attribute.bit.point -= (2 - funcCode.code.frqPoint);
    }

#endif    
// 额定电流，空载电流，故障电流，U0-04显示电流
    else if ((GetCodeIndex(funcCode.code.motorParaM1.elem.ratingCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorParaM1.elem.zeroLoadCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.ratingCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.zeroLoadCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.ratingCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.zeroLoadCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.ratingCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.zeroLoadCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.errorScene3.elem.errorCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.code.errorScene2.elem.errorCurrent) == curFc.index) ||
        (GetCodeIndex(funcCode.group.u0[DISP_OUT_CURRENT]) == curFc.index)         
        )
    {
        if (invPara.type > invPara.pointLimit)
            attribute.bit.point--;
    }	    
// 电机参数
    else if ((GetCodeIndex(funcCode.code.motorParaM1.elem.statorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorParaM1.elem.rotorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorParaM1.elem.leakInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorParaM1.elem.mutualInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorParaM1.elem.pmsmRs) == curFc.index) ||   // 同步机定子电阻
        (GetCodeIndex(funcCode.code.motorParaM1.elem.pmsmLd) == curFc.index) ||   // 同步机d轴电感
        (GetCodeIndex(funcCode.code.motorParaM1.elem.pmsmLq) == curFc.index) ||   // 同步机q轴电感
        
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.statorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.rotorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.leakInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.mutualInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.pmsmRs) == curFc.index) ||   // 同步机定子电阻
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.pmsmLd) == curFc.index) ||   // 同步机d轴电感
        (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.pmsmLq) == curFc.index) ||   // 同步机q轴电感
       
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.statorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.rotorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.leakInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.mutualInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.pmsmRs) == curFc.index) ||   // 同步机定子电阻
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.pmsmLd) == curFc.index) ||   // 同步机d轴电感
        (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.pmsmLq) == curFc.index) ||   // 同步机q轴电感
       
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.statorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.rotorResistance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.leakInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.mutualInductance) == curFc.index) ||
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.pmsmRs) == curFc.index) ||   // 同步机定子电阻
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.pmsmLd) == curFc.index) ||   // 同步机d轴电感
        (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.pmsmLq) == curFc.index)      // 同步机q轴电感
        )
    {
        if (invPara.type > invPara.pointLimit)
        {
            attribute.bit.point++;
        }
    }
    
    UpdateDisplayBufferAttribute(tmp, attribute);
}


//=====================================================================
//
// 根据属性，更新显示数据缓冲的：负号、位数、小数点和单位
//
//=====================================================================
LOCALF void UpdateDisplayBufferAttribute(const Uint16 data, union FUNC_ATTRIBUTE attribute)
{
    static Uint16 digit[5];
    int16 i;
    Uint16 digits;              // data的位数
    Uint16 bMinus = 0;          // 是否显示符号标志，1-要显示负号
    Uint16 dataTmp;
    Uint16 a,b;
    Uint16 mode;

// 1. 显示位数和显示负号
    // 有符号，且为负数，需要显示负号-
    if ((attribute.bit.signal) // 符号，unsignal->0; signal->1.
        && ((int16)(data) < 0))
    {
        dataTmp = -(int16)(data);
        bMinus = 1;

         // 为有符号数据且值为负且显示值超过4位
        if ((attribute.bit.point) 
            && ((int16)data < (-9999))
            )
        {
            attribute.bit.point--;
            dataTmp = dataTmp/10;
        }
    }
    else
    {
        dataTmp = data;
    }

// 获取每一位的显示值
    a = digit[1];   // 保存
    b = digit[0];

    mode = DECIMAL;
    if (ATTRIBUTE_MULTI_LIMIT_HEX == attribute.bit.multiLimit)  // 十六进制约束
    {
        mode = HEX;
    }
    digits = GetNumberDigit(digit, dataTmp, mode);

    if (ticker4LowerDisp)  // 非0级菜单，或者0级菜单下最后2位到了更新时间，或者0级菜单  按键shift
    {
        digit[1] = a;
        digit[0] = b;
    }

// 用户定制功能码组的显示
#if DEBUG_F_USER_MENU_MODE
    // 显示 uFx.yz
    if ((USER_MENU_GROUP == curFc.group) &&      // 用户定制功能码组
        (MENU_LEVEL_3 == menuLevel))
    {
        Uint16 group;

        digit[4] = DISPLAY_u;

        group = menu3Number / 100;
        UpdateGroupDisplay(group);
        digit[3] = groupDisplay.dispF;
        digit[2] = groupDisplay.disp0;

        attribute.bit.point = 2;
    }
#endif

    if ((MENU_LEVEL_0 == menuLevel) // 0级菜单，不显示数字前面的零
        || ((MENU_LEVEL_3 == menuLevel) && (curFc.group >= FUNCCODE_GROUP_U0)) // U组，显示
            )
    {
        if (mode == HEX)
        {
            if (attribute.bit.displayBits < DISPLAY_8LED_NUM)
            {
                // 十六进制显示加前缀 H.
                digits = attribute.bit.displayBits + 1;
                digit[attribute.bit.displayBits] = DISPLAY_H;    // 显示H
                attribute.bit.point = attribute.bit.displayBits; // 显示H.
            }
        }

        if (++ticker4LowerDisp >= DECIMAL_DISPLAY_UPDATE_TIME)  // 小数点后2位更新时间，_*12ms
        {
            ticker4LowerDisp = 0;
        }
    }
    else //if (MENU_LEVEL_3 == menuLevel)    // 0，3级菜单才会调用本函数
    {
        ticker4LowerDisp = 0;

        // 十六进制数据
        if (mode == HEX)
        {
            // 有空余数码管
            if (curFcDispDigits < DISPLAY_8LED_NUM)
            {
                // 3级菜单的十六进制数据显示加前缀 H.
                digits = curFcDispDigits + 1;          // 增加一位显示位
                digit[curFcDispDigits] = DISPLAY_H;    // 显示数据为H
                attribute.bit.point = curFcDispDigits; // 显示数据为H.
            }
            // 数码管显示已无空余，不显示H.
            else
            {
                digits = curFcDispDigits;       // 3级菜单
            }
        }
        // 十进制数据
        else
        {
            digits = curFcDispDigits;          // 3级菜单
        }
    }
    
    if (attribute.bit.point >= digits)  // 至少显示到小数点位置
    {
        digits = attribute.bit.point + 1;
    }

    for (i = DISPLAY_8LED_NUM - 1; i >= 0; i--) // 显示位数
    {
        if (i < digits)
        {
            displayBuffer[(DISPLAY_8LED_NUM - 1) - i] = DISPLAY_CODE[digit[i]];
        }
        else
        {
            displayBuffer[(DISPLAY_8LED_NUM - 1) - i] = DISPLAY_CODE[DISPLAY_NULL];
        }
    }
    
    if (bMinus)                 // 显示负号-
    {
        int16 tmp = DISPLAY_8LED_NUM - 1 - digits;
        if (tmp < 0)
            tmp = 0;

        if (!digit[4])  // 最高位为0
        {
            displayBuffer[tmp] = DISPLAY_CODE[DISPLAY_LINE];
        }
        else
        {
            ///displayBuffer[(DISPLAY_8LED_NUM - 1)] &= DISPLAY_CODE[DISPLAY_DOT];// 最后1位LED显示小数点
            //displayBuffer[tmp] = DISPLAY_CODE[DISPLAY_LINE_1];
            //displayBuffer[tmp] &= 0xdf;
            //displayBuffer[tmp] ^= 0x40;
            //displayBuffer[tmp] ^= ~0xBf;
            //displayBuffer[tmp] ^= 0x02;
            attribute.bit.point = 0;
        }
    }


// 2. 显示小数点
    if (attribute.bit.point)
    {
        displayBuffer[(DISPLAY_8LED_NUM - 1) - attribute.bit.point] &= DISPLAY_CODE[DISPLAY_DOT];
    }

// 3. 显示单位
    if (attribute.bit.unit & (0x01U << ATTRIBUTE_UNIT_HZ_BIT))   // 显示单位,Hz
    {
        displayBuffer[5] &= LED_CODE[LED_HZ];   // 亮
    }

    if (attribute.bit.unit & (0x01U << ATTRIBUTE_UNIT_A_BIT))    // 显示单位,A
    {
        displayBuffer[5] &= LED_CODE[LED_A];
    }

    if (attribute.bit.unit & (0x01U << ATTRIBUTE_UNIT_V_BIT))    // 显示单位,V
    {
        displayBuffer[5] &= LED_CODE[LED_V];
    }
}


//=====================================================================
//
// 故障码显示
//
//=====================================================================
void UpdateErrorDisplayBuffer(void)
{
    Uint16 digit[5];

    GetNumberDigit1(digit, errorCode);

        // 数码管显示
    displayBuffer[0] = DISPLAY_CODE[DISPLAY_E];
    displayBuffer[1] = DISPLAY_CODE[DISPLAY_r];
    displayBuffer[2] = DISPLAY_CODE[DISPLAY_r];
    displayBuffer[3] = DISPLAY_CODE[digit[1]];
    displayBuffer[4] = DISPLAY_CODE[digit[0]];

    if ((runFlag.bit.run)    // 告警
        || (ERROR_LEVEL_RUN == errorAttribute.bit.level)
        )
    {
        displayBuffer[0] = DISPLAY_CODE[DISPLAY_NULL];
        displayBuffer[1] = DISPLAY_CODE[DISPLAY_NULL];
        displayBuffer[2] = DISPLAY_CODE[DISPLAY_A];
    }
}


//=====================================================================
//
// 调谐显示
//
//=====================================================================
LOCALF void UpdateTuneDisplayBuffer(void)
{
    // 数码管显示
    displayBuffer[0] = DISPLAY_CODE[DISPLAY_NULL];
    displayBuffer[1] = DISPLAY_CODE[DISPLAY_T];
    displayBuffer[2] = DISPLAY_CODE[DISPLAY_U];
    displayBuffer[3] = DISPLAY_CODE[DISPLAY_N];
    displayBuffer[4] = DISPLAY_CODE[DISPLAY_E];
}


//=====================================================================
//
// 输入：
// *data  要修改的数据的当前值
//
// 输出：
// *data  修改之后的数据
//
// 参数：
// index  修改功能码的index
// delta  增量。正数，UP；负数，DOWN；0，通讯调用。
//
// 返回：
// COMM_ERR_NONE
// COMM_ERR_PARA       无效参数
// COMM_ERR_READ_ONLY  参数更改无效
//
//=====================================================================
Uint16 ModifyFunccodeUpDown(Uint16 index, Uint16 *data, int16 delta)
{
    union FUNC_ATTRIBUTE attribute = funcCodeAttribute[index].attribute;
    Uint16 upper;
    Uint16 lower;
    Uint16 tmp = *data;
    Uint16 flag = COMM_ERR_NONE;
    int16 i;
    int16 flag1 = 0;
    Uint16 multiLimit;

// 参数读写特性，0x-参数只读，10-运行中只读，11-可以读写
    if (IsWritable(attribute))
    {
#if DEBUG_F_USER_MENU_MODE
        if ((USER_MENU_GROUP == curFc.group)     // FE组，用户定制功能码
		|| ((index >= GetCodeIndex(funcCode.code.userCustom[0])) 
			&& (index <= GetCodeIndex(funcCode.code.userCustom[FENUM-1])))
		)
        {
            Uint16 group = *data / 100;
			
			if ((group >= FUNCCODE_GROUP_NUM) || (group == USER_MENU_GROUP))
			{
				return COMM_ERR_PARA;
			}
			
            upper = group * 100 + funcCodeGradeAll[group] - 1;
            lower = group * 100 + 0;

			if (upper < lower)
			{
				return COMM_ERR_PARA;
			}
        }
        else
#endif
        {
            upper = funcCodeAttribute[index].upper;
            if (attribute.bit.upperLimit)
                upper = funcCode.all[upper];

            // S曲线的起始段和结束段之和最大为1000.
            // sCurveStartPhaseTime + sCurveEndPhaseTime <= 100.0%
            if (index == GetCodeIndex(funcCode.code.sCurveStartPhaseTime))
            {
                upper = 1000 - funcCode.code.sCurveEndPhaseTime;
            }
            else if (index == GetCodeIndex(funcCode.code.sCurveEndPhaseTime))
            {
                upper = 1000 - funcCode.code.sCurveStartPhaseTime;
            }

            lower = funcCodeAttribute[index].lower;
            if (attribute.bit.lowerLimit)
                lower = funcCode.all[lower];

            // 该功能码为多个功能码的组合
            multiLimit = funcCodeAttribute[index].attribute.bit.multiLimit;

            if ((ATTRIBUTE_MULTI_LIMIT_SINGLE != multiLimit) && (!delta))  // 组合功能码且通讯调用
            {
                Uint16 dataDigit[5],upperDigit[5], lowerDigit[5];
                Uint16 bit;
               // const int16 *p = decNumber;
                Uint16 mode = DECIMAL;

                if (ATTRIBUTE_MULTI_LIMIT_HEX == multiLimit)
                {
                    //p = hexNumber;
                    mode = HEX;
                }

				GetNumberDigit(dataDigit, *data, mode);
				GetNumberDigit(upperDigit, upper, mode);
				GetNumberDigit(lowerDigit, lower, mode);

                for (bit = 0; bit < 5; bit++)
                {
                    if ((dataDigit[bit] > upperDigit[bit]) 
                    || (dataDigit[bit] < lowerDigit[bit]))
					{
                        return COMM_ERR_PARA;
					}
                }

                return COMM_ERR_NONE;
            }
                
            
            if ((ATTRIBUTE_MULTI_LIMIT_SINGLE != multiLimit)   // 组合功能码且非通讯调用
                && (delta)
                )
            {
                Uint16 digit[5];
                Uint16 tmp;
                Uint16 bit = menuAttri[MENU_LEVEL_3].operateDigit;
                const int16 *p = decNumber;
                Uint16 mode = DECIMAL;

                if (ATTRIBUTE_MULTI_LIMIT_HEX == multiLimit)
                {
                    p = hexNumber;
                    mode = HEX;
                }

                GetNumberDigit(digit, *data, mode);
                tmp = *data - digit[bit] * (*(p+bit));

                GetNumberDigit(digit, upper, mode);
                upper = tmp + digit[bit] * (*(p+bit));

                GetNumberDigit(digit, lower, mode);
                lower = tmp + digit[bit] * (*(p+bit));
            }

#if DEBUG_F_INV_TYPE_RELATE
            // 机型不能超过范围
            if (GetCodeIndex(funcCode.code.inverterType) == index)  // FF-01
            {
                if ((100 != delta) && (-100 != delta))   // 没有在修改百位
                {
                    Uint16 i;
                    i = *data / 100;

                    upper = invTypeLimitTable[i].upper + (i * 100);
                    lower = invTypeLimitTable[i].lower + (i * 100);
                }
            }
            else 
#endif
            // 最大频率，不同频率指令小数点的范围应该不同。
            if (GetCodeIndex(funcCode.code.maxFrq) == index)
            {
                lower = 50 * decNumber[funcCode.code.frqPoint];
            }
        }

// 上下限处理
#if DEBUG_F_NO_SAME
//--------------------------------------------------------
// NoSameDeal()
        // 主频率源X和辅频率源Y的设定值不能一样
        for (i = sizeof(frqSrcFuncIndex) - 1; i >= 0; i--)
        {
            if (frqSrcFuncIndex[i] == index)
            {
                *data = NoSameDeal(index, frqSrcFuncIndex, sizeof(frqSrcFuncIndex), *data, upper, lower, delta);
                flag1 = 1;
            }

            if (flag1)
                break;
        }

        // DI(DI, VDI, AiAsDi)端子的功能，不能重复
        for (i = sizeof(diFuncIndex) - 1; i >= 0; i--)
        {
            if (diFuncIndex[i] == index)
            {
                *data = NoSameDeal(index, diFuncIndex, sizeof(diFuncIndex), *data, upper, lower, delta);
                flag1 = 1;
            }

            if (flag1)  // 有相等的index，跳出
                break;
        }
//--------------------------------------------------------
#endif
        if (!flag1)
        {
            *data = LimitDeal(attribute.bit.signal, *data, upper, lower, delta);
        }

        if (*data != tmp)       // 通讯调用时，发现数据被更改，即参数错误。
            flag = COMM_ERR_PARA;
    }
    else
    {
        flag = COMM_ERR_READ_ONLY;
    }

    return flag;
}




//=====================================================================
//
// 参数：
// index  要修改功能码的index
// *data  数据
//
// 返回：
// COMM_ERR_NONE
// COMM_ERR_PARA       无效参数
//
//=====================================================================
Uint16 ModifyFunccodeEnter(Uint16 index, Uint16 dataNew)
{
    Uint16 dataOld;
    Uint16 ret = COMM_ERR_NONE;

    dataOld = funcCode.all[index];
    funcCode.all[index] = dataNew;        // 保存到RAM


    if (dataOld != dataNew)
    {
        // 进入了某些功能码，还要修改一些变量
        // F0-07, 频率源选择
        // F0-08, 预置频率
        if ((GetCodeIndex(funcCode.code.presetFrq) == index) ||
            (GetCodeIndex(funcCode.code.frqCalcSrc) == index))
        {
            // 数字设定频率源，修改了预置频率，要相应修改设定频率。进入预置频率后enter就修改
            ResetUpDownFrq();
        }
#if DEBUG_F_MOTOR_POWER_RELATE
        else if (GetCodeIndex(funcCode.code.motorParaM1.elem.ratingPower) == index)         // 修改电机额定功率，立即修改相关功能码
        {
            MotorPowerRelatedParaDeal(dataNew, MOTOR_SN_1);
        }
        else if (GetCodeIndex(funcCode.code.motorFcM2.motorPara.elem.ratingPower) == index) // 修改电机额定功率，立即修改相关功能码
        {
            MotorPowerRelatedParaDeal(dataNew, MOTOR_SN_2);
        }
        else if (GetCodeIndex(funcCode.code.motorFcM3.motorPara.elem.ratingPower) == index) // 修改电机额定功率，立即修改相关功能码
        {
            MotorPowerRelatedParaDeal(dataNew, MOTOR_SN_3);
        }
        else if (GetCodeIndex(funcCode.code.motorFcM4.motorPara.elem.ratingPower) == index) // 修改电机额定功率，立即修改相关功能码
        {
            MotorPowerRelatedParaDeal(dataNew, MOTOR_SN_4);
        }   
#endif
        else if (GetCodeIndex(funcCode.code.menuMode) == index)  // 修改菜单操作模式
        {
            Uint16 digit[5];
            GetNumberDigit(digit, menu3Number, DECIMAL);
        		menuModeTmp = menuMode;
        		// 当前菜单操作模式选择无效后
        		if (((menuModeTmp == MENU_MODE_USER) && (!digit[0]))
                  || ((menuModeTmp == MENU_MODE_CHECK) && (!digit[1])))
        		{
        			// 更改当前菜单操作模式为已有选择模式
        			menuModeTmp = MENU_MODE_BASE;
        			MenuModeSwitch();
        		}
        }
        // 更新功能参数组显示隐藏属性
        else if ((GetCodeIndex(funcCode.code.aiaoCalibrateDisp) == curFc.index)
            || (GetCodeIndex(funcCode.code.funcParaView) == curFc.index))
        {
            MenuModeDeal();
        }
#if DEBUG_F_INV_TYPE_RELATE
        else if (GetCodeIndex(funcCode.code.inverterType) == index) // 修改变频器机型 FF-01，立即修改相关功能码
        {
            // 机型不能超过范围
            ret = ValidateInvType();
            if (!ret)  // 机型有效
            {
                InverterTypeRelatedParaDeal();
            }
        }
#endif
#if 0
        else if (GetCodeIndex(funcCode.code.commOverTime) == index) // FA-04, 通讯超时时间
        {
            if ((dataNew) && (!dataOld))  // 0->非0
                commTicker = 0;  // 修改了通讯延时时间，重新开始计时。
        }
#endif
    }

    // 仅DI5可以定义为DI_FUNC_APTP_ZERO
    if ((GetCodeIndex(funcCode.code.diFunc[0]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[1]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[2]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[3]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[5]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[6]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[7]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[8]) == index)
        || (GetCodeIndex(funcCode.code.diFunc[9]) == index)
        )
    {
        if (dataNew == DI_FUNC_APTP_ZERO)
        {
            ret = COMM_ERR_PARA;
        }
    }
#if DEBUG_F_GROUP_HIDE
    else if (GetCodeIndex(funcCode.code.funcParaView) == curFc.index)
    {
        MenuModeDeal();
    }
#endif
#if DEBUG_F_MOTOR_FUNCCODE1
    // 性能调试功能码
    else if (GetCodeIndex(funcCode.code.motorDebugFc) == curFc.index)
    {
        MenuModeDeal();
    }
#endif
    // AIAO校正功能码隐藏
    else if (GetCodeIndex(funcCode.code.aiaoCalibrateDisp) == curFc.index)
    {
        MenuModeDeal();
    }
    else if ((GetCodeIndex(funcCode.code.ledDispParaRun1) == index) ||
        (GetCodeIndex(funcCode.code.ledDispParaRun2) == index) ||
        (GetCodeIndex(funcCode.code.ledDispParaStop) == index)
        )
    {
        cycleShiftDeal(0);      // 0级菜单显示的循环移位处理
    }
    // 频率指令小数点
    else if (GetCodeIndex(funcCode.code.frqPoint) == index)
    {
        if (funcCode.code.maxFrq < 50 * decNumber[funcCode.code.frqPoint])
        {
            funcCode.code.maxFrq = 50 * decNumber[funcCode.code.frqPoint];

            // 某些功能码是其他功能码上下限的处理
            LimitOtherCodeDeal(MAX_FRQ_INDEX);   // 最大频率
        }
    }
#if (DEBUG_F_POSITION_CTRL)
    // aptp零点输入端子，使用中断
    // 认为DI5为HDI
    else if (GetCodeIndex(funcCode.code.diFunc[4]) == index)
    {
        // 改变成aptp零点输入
        if ((dataOld != DI_FUNC_APTP_ZERO) && (dataNew == DI_FUNC_APTP_ZERO))
        {
            InitSetEcap4WithInt();
            InitSetAptpZero();
        }
        else if ((dataOld == DI_FUNC_APTP_ZERO) && (dataNew != DI_FUNC_APTP_ZERO))
        {
            ;
        }
    }
    // FVC的PG卡选择, 1-QEP1,0-QEP2(扩展)
    // 这里仅进行位置控制PG的初始化
    else if (GetCodeIndex(funcCode.code.pgParaM1.elem.fvcPgSrc) == index)
    {
        if ((dataOld != FUNCCODE_fvcPgSrc_QEP1) && (dataNew == FUNCCODE_fvcPgSrc_QEP1))
        {
            //InitEQep2Gpio();
            aptpAbsZeroOk = 0;
            pEQepRegsFvc = &EQep1Regs;   // FVC的PG卡

            EALLOW;
            pEQepRegsFvc->QEPCTL.bit.IEL = 01;      // FVC QEPx Index event latch
            EDIS;

            InitSetPcEQep();
        }
        else if ((dataOld == FUNCCODE_fvcPgSrc_QEP1) && (dataNew != FUNCCODE_fvcPgSrc_QEP1))
        {
            //InitEQep1Gpio();
            aptpAbsZeroOk = 0;
            pEQepRegsPc = &EQep1Regs;    // 位置控制的PG卡

            EALLOW;
            pEQepRegsFvc->QEPCTL.bit.IEL = 01;      // FVC QEPx Index event latch
            EDIS;

            InitSetPcEQep();
        }
    }
    // 位置指令脉冲逻辑
    else if (GetCodeIndex(funcCode.code.pcPulseLogic) == index)
    {
        // 将QEP_PC-B反向
        pEQepRegsPc->QDECCTL.bit.QBP = dataNew;
    }
    // 位置指令脉冲方式
    else if (GetCodeIndex(funcCode.code.pcPulseType) == index)
    {
        if (FUNCCODE_pcPulseType_PULSE_AND_DIR == funcCode.code.pcPulseType)    // 脉冲+方向
        {
            pEQepRegsPc->QDECCTL.bit.QSRC = 01; // Direction-count mode (QCLK = xCLK, QDIR = xDIR)
        }
        else if (FUNCCODE_pcPulseType_QUADRATURE == funcCode.code.pcPulseType)  // 2路正交脉冲
        {
            pEQepRegsPc->QDECCTL.bit.QSRC = 00; // quadrature count mode
        }
    }
    // 位置指令相序
    else if (GetCodeIndex(funcCode.code.pcPulseSwap) == index)
    {
        pEQepRegsPc->QDECCTL.bit.SWAP = dataNew;
    }
    // 速度反馈AB相序
#if 0    // AB相序交给性能处理
    else if (GetCodeIndex(funcCode.code.fvcPgLogic) == index)
    {
        // 将QEP_速度控制-AB交换
        pEQepRegsFvc->QDECCTL.bit.SWAP = dataNew;
    }
#endif
    // 改变零点, aptpAbsZeroOk清零
    else if (GetCodeIndex(funcCode.code.pcZeroSelect) == index)
    {
        if (dataOld != dataNew)
        {
            aptpAbsZeroOk = 0;
        }
    }
#endif
    else
    {
        LimitOtherCodeDeal(index);      // 某些功能码是其他功能码上下限的处理
    }

    if (COMM_ERR_PARA == ret)           // 更改无效，则恢复之前的值
    {
        funcCode.all[index] = dataOld; // 恢复
    }

    return ret;
}


//=====================================================================
//
// 显示数据处理，目前2ms调用1次
//
//=====================================================================
#define OUT_TORQUE_FRQ_DISP_FILTER_TIME 75  // 输出频率滤波时间系数
LOCALF LowPassFilter torQueFrqDispLpf = LPF_DEFALUTS;
void DispDataDeal(void)
{
// 运行频率显示。
// 显示功能传递给性能的瞬时值，而不是性能的反馈频率(或者性能实际发送的频率)
    frqDisp = ABS_INT32(frqDroop);

#if DEBUG_F_PLC_CTRL
    frqPLCDisp = (int16)(frqDroop*10000/maxFrq);
#endif


    if (RUN_MODE_TORQUE_CTRL == runMode)    // 转矩控制
    {
        torQueFrqDispLpf.t = OUT_TORQUE_FRQ_DISP_FILTER_TIME;
        torQueFrqDispLpf.in = ABS_INT32(frqRun);
        torQueFrqDispLpf.calc(&torQueFrqDispLpf);
        frqDisp = torQueFrqDispLpf.out;

#if DEBUG_F_PLC_CTRL
        frqPLCDisp = (int16)(torQueFrqDispLpf.out*10000/maxFrq);
#endif
    }

#if DEBUG_F_PLC_CTRL
     if (funcCode.code.runDir == FUNCCODE_runDir_REVERSE)
     {
        frqPLCDisp = -((int16)frqPLCDisp);
     }
#endif    

// 设定频率显示。跳跃频率之前，点动之后的值
// 放在UpdateFrqAim()中。

    pidFuncRefDisp = ((Uint32)funcCode.code.pidDisp * ABS_INT32(pidFunc.ref) + (1 << 14)) >> 15;
    pidFuncFdbDisp = ((Uint32)funcCode.code.pidDisp * ABS_INT32(pidFunc.fdb) + (1 << 14)) >> 15;

    // 负载速度
    // 停机时显示设定频率*系数
    // 运行时显示运行频率*系数
    if (runFlag.bit.run)
    {
        loadSpeedDisp = (ABS_INT32(frq) * funcCode.code.speedDispCoeff) / 10000;
    }
    else
    {
        loadSpeedDisp = (ABS_INT32(frqAim) * funcCode.code.speedDispCoeff) / 10000;
    }
    
    pulseInFrqDisp = (pulseInFrq + 5) / 10;

    torqueCurrentAct = (Uint32)torqueCurrent * currentPu >> 12;
     
// 输出功率计算，现在由驱动直接计算
#if 0//DEBUG_F_OUT_POWER
    //+============ 小数点
    outPower = (Uint32)torqueCurrentAct * outVoltageDisp * 135 / (100UL * 1000);   // 单位为0.1KW，效率默认为1.35/1.732
    itDisp = (Uint32)torqueCurrentAct * 1000 / motorFc.motorPara.elem.ratingCurrent;       // 输出转矩
#endif

    funcCode.code.inverterGpTypeDisp = funcCode.code.inverterGpType; // F0-00, GP类型显示

//  速度反馈PG卡AB相序，性能做
//    pEQepRegsFvc->QDECCTL.bit.SWAP = funcCode.code.fvcPgLogic;
}


#if 0
//=====================================================================
//
// 恢复出厂参数，恢复至RAM
//
//=====================================================================
void RestoreCompanyParaRamDeal(Uint16 i)
{
    Uint16 flag = 0;    // 0, 恢复

    if (indexRestoreExceptSeries < RESTORE_COMPANY_PARA_EXCEPT_NUMBER)
    {
        if ((exceptRestoreSeries[indexRestoreExceptSeries].start <= i) &&
            (i <= exceptRestoreSeries[indexRestoreExceptSeries].end))
        {
            flag = 1;
        }
        if (i >= exceptRestoreSeries[indexRestoreExceptSeries].end)
        {
            indexRestoreExceptSeries++;
        }
    }

    if (indexRestoreExceptSingle < CLEAR_RECORD_NUM)   // 这些功能码不恢复
    {
        if (clearRecord[indexRestoreExceptSingle] == i)
        {
            flag = 1;
            indexRestoreExceptSingle++;
        }
    }

    if (!flag)
    {
        funcCode.all[i] = funcCodeInit.all[i]; // 恢复成出厂参数
    }
}
#endif


// 0级菜单下增加菜单级别
void Menu0AddMenuLevel(void)
{
    menuLevel = MENU_LEVEL_1;

    if ((MENU_MODE_CHECK == menuMode)
        || (MENU_MODE_USER == menuMode)
        )
    {
        Menu1OnEnter();
    }
}


//=====================================================================
//
// 确定功能码的显示位数
//
// 实际显示位数，应该为:
// 1. 无符号
//    上限数值的位数
// 2. 有符号
//    上限和下限数值的绝对值的大值的位数
//
//=====================================================================
Uint16 GetDispDigits(Uint16 index)
{
    Uint16 upper;
    Uint16 lower;
    Uint16 value;
    Uint16 digits;
    Uint16 mode;
    Uint16 digit[5];
    const FUNCCODE_ATTRIBUTE *p = &funcCodeAttribute[index];

// 获得实际的上限

// 获取上限数值
    while (p->attribute.bit.upperLimit)
    {
        p = &funcCodeAttribute[p->upper];
    }
    upper = p->upper;
    
    if (!p->attribute.bit.signal)   // 无符号
    {
        value = upper;
    }
    else                            // 有符号
    {
        // 获取下限数值
        while (p->attribute.bit.lowerLimit)
        {
            p = &funcCodeAttribute[p->lower];
        }
        lower = p->lower;

        // 取绝对值
        upper = ABS_INT16((int16)upper);
        lower = ABS_INT16((int16)lower);

        // 取绝对值的大值
        value = (upper > lower) ? upper : lower;
    }

// 获取上限的位数
    mode = DECIMAL;

    if (ATTRIBUTE_MULTI_LIMIT_HEX == p->attribute.bit.multiLimit)   // 十六进制约束
    {
        mode = HEX;
    }

    digits = GetNumberDigit(digit, value, mode);

    return digits;
}


//=====================================================================
//
// 根据currentGroup，更新显示的组，即groupDisplay
// F0,…,FE,FF,FP,A0,…,AF,B0,…,BF,C0,…,CF,
//
//=====================================================================
void UpdateGroupDisplay(Uint16 group)
{
    Uint16 currentGroupDispF;
    Uint16 currentGroupDisp0;

// F0,…,FE,FF,FP,A0,…,AF,B0,…,BF,C0,…,CF,
    currentGroupDispF = DISPLAY_F;  //stamp:MD380_DISPLAY
    //currentGroupDispF = DISPLAY_E;    //
    if (group <= FUNCCODE_GROUP_FP)         // F0-FP
    {
        currentGroupDisp0 = group;

    }
    else if (group <= FUNCCODE_GROUP_CF)    // A0,…,AF,B0,…,BF,C0,…,CF,
    {
        Uint16 tmp1;
        Uint16 tmp2;

        tmp1 = (group - FUNCCODE_GROUP_FP - 1) / 16;
        tmp2 = (group - FUNCCODE_GROUP_FP - 1) % 16;

      currentGroupDispF = DISPLAY_A + tmp1; //MD380_DISPLAY
        //currentGroupDispF = DISPLAY_F + tmp1;//
        currentGroupDisp0 = tmp2;
    }
    else    // U组
    {
      currentGroupDispF = DISPLAY_U; //MD380_DISPLAY
        //currentGroupDispF = DISPLAY_P;//stamp:
        currentGroupDisp0 = group - FUNCCODE_GROUP_U0;
    }

    groupDisplay.dispF = currentGroupDispF;  
    groupDisplay.disp0 = currentGroupDisp0;  //MD380_DISPLAY

	//if((groupDisplay.dispF == DISPLAY_E)&&(groupDisplay.disp0 == 15)) // 注释掉恢复 md380 
	//{
       // groupDisplay.dispF = DISPLAY_H;
       // groupDisplay.disp0 = 17;  //display HH
	//}
}


// group的UP/DOWN
Uint16 GroupUpDown(const Uint16 funcCodeGrade[], Uint16 group, Uint16 flag)
{
    group = LimitOverTurnDeal(funcCodeGrade, group, FUNCCODE_GROUP_NUM, FC_START_GROUP, flag);

    return group;
}


// 用户定制功能码菜单模式的处理
void DealUserMenuModeGroupGrade(Uint16 flag)
{
#if DEBUG_F_USER_MENU_MODE
    userMenuModeFcIndex = LimitOverTurnDeal(funcCode.code.userCustom, userMenuModeFcIndex, FENUM, 0, flag);

// 个位、十位表示用户定制的grade
// 百位、千位表示用户定制的group
    curFc.group = funcCode.code.userCustom[userMenuModeFcIndex] / 100;
    curFc.grade = funcCode.code.userCustom[userMenuModeFcIndex] % 100;
#endif
}


// data的范围是 [low, upper-1]
Uint16 LimitOverTurnDeal(const Uint16 limit[], Uint16 data, Uint16 upper, Uint16 low, Uint16 flag)
{
    Uint16 loopNumber = 0;
    
    do
    {
        data += deltaK[flag];

        if ((int16)data >= (int16)upper)
        {
            data = low;
        }
        else if ((int16)data < (int16)low)
        {
            data = upper - 1;
        }

        loopNumber++;
    }
    while ((!limit[data]) 
        && (loopNumber < upper - low)
        );

    return data;
}


// 校验菜单
#define CHECK_MENU_MODE_NUMBER_ONCE     70      // 1拍内的循环次数
void DealCheckMenuModeGroupGrade(Uint16 flag)
{
#if DEBUG_F_CHECK_MENU_MODE
    int16 delta = 1;
    static Uint16 group;
    static Uint16 grade;
    Uint16 gradeTmp;
    Uint16 loopNumber = 0;
    Uint16 checkMenuModeFcIndex;

    if (CHECK_MENU_MODE_DEAL_CMD == checkMenuModeDealStatus)   // 新的搜索指令
    {
        group = curFc.group;
        grade = curFc.grade;
    }
    checkMenuModeDealStatus = CHECK_MENU_MODE_DEAL_CMD;

    if (ON_DOWN_KEY == flag)    // 向下搜索
    {
        delta = -delta;
    }

    do
    {
        gradeTmp = grade;
        if (funcCodeGradeCurMenuMode[group] > 1)    // 该group有效
        {
            grade = LimitDeal(0, grade, funcCodeGradeCurMenuMode[group] - 1, 0, delta); // funcCodeGradeAll
        }

        if (grade == gradeTmp)                      // grade到达限值，要改变group
        {
            group = LimitOverTurnDeal(funcCodeGradeCurMenuMode, group, FUNCCODE_GROUP_U0 - 1, FUNCCODE_GROUP_F0, flag);

            if (ON_UP_KEY == flag)
            {
                grade = 0;
            }
            else
            {
                grade = funcCodeGradeCurMenuMode[group] - 1;    // 该group的最后一个grade
            }
        }

        checkMenuModeFcIndex = GetGradeIndex(group, grade);

        // 1拍内不能占用太多时间
        // 否则，下一拍再进行搜索
        if (++loopNumber >= CHECK_MENU_MODE_NUMBER_ONCE)        
        {
            checkMenuModeDealStatus = CHECK_MENU_MODE_DEAL_SERACHING;
        }

        if ((curFc.grade == grade)      // if 之前没有else
            && (curFc.group == group)
            )   // 又回到之前的功能码，说明没有其它与出厂值不同的功能码
        {
            checkMenuModeDealStatus = CHECK_MENU_MODE_DEAL_END_NONE;   // 遍历一遍，没有找到新的与出厂值不同的功能码
            checkMenuModeSerachNone = 1;
        }
        
        // 考虑出厂值与机型相关的非电机参数功能码
        // if 之前没有else
        if ((funcCode.all[checkMenuModeFcIndex] != GetFuncCodeInit(checkMenuModeFcIndex, 0)) &&
            (funcCodeAttribute[checkMenuModeFcIndex].attribute.bit.writable != 2))
        {
            checkMenuModeDealStatus = CHECK_MENU_MODE_DEAL_END_ONCE;  // 找到新的与出厂值不同的功能码
            checkMenuModeSerachNone = 0;
        }
    }
    while (CHECK_MENU_MODE_DEAL_CMD == checkMenuModeDealStatus);

#if 0
    if (CHECK_MENU_MODE_DEAL_END_NONE == checkMenuModeDealStatus)  // 没有功能码与出厂值不同，显示F0-00
    {
        group = 0;
        grade = 0;
    }
#endif

    if (CHECK_MENU_MODE_DEAL_SERACHING != checkMenuModeDealStatus)  // 已经完成搜索
    {
        curFc.group = group;
        curFc.grade = grade;

        checkMenuModeCmd = 0;       // 完成搜索
        checkMenuModeDealStatus = CHECK_MENU_MODE_DEAL_CMD;         // 准备下一次搜索
    }
#endif
}



// 菜单模式处理
// 更新funcCodeGradeCurrentMenuMode
void MenuModeDeal(void)
{
    Uint16 digit[5];

    UpdataFuncCodeGrade(funcCodeGradeCurMenuMode);
    GetNumberDigit1(digit, funcCode.code.menuMode);

    switch (menuMode)
    {
        case MENU_MODE_BASE:       // 基本菜单，主要为目前320功能码(出厂)
            break;       

#if DEBUG_F_USER_MENU_MODE
        case MENU_MODE_USER:        // 用户定制菜单
            if (MENU_MODE_USER != menuModeOld)
            {
                DealUserMenuModeGroupGrade(ON_UP_KEY);
            }
            
            curFc.group = funcCode.code.userCustom[userMenuModeFcIndex] / 100;
            curFc.grade = funcCode.code.userCustom[userMenuModeFcIndex] % 100;
            break;
#endif

#if DEBUG_F_CHECK_MENU_MODE
        case MENU_MODE_CHECK:        // 校对菜单，仅显示与出厂值不同的功能码
            if (MENU_MODE_CHECK != menuModeOld)
            {
                checkMenuModeCmd = 1;
                checkMenuModePara = ON_UP_KEY;
            }
            break;
#endif
        default:
            break;
    }

}


void UpdataFuncCodeGrade(Uint16 funcCodeGrade[])
{
#if 1
    int16 i;
    Uint16 digit[5];

    memcpy(funcCodeGrade, funcCodeGradeAll, FUNCCODE_GROUP_NUM);
    
    GetNumberDigit(digit, funcCode.code.funcParaView, DECIMAL);  // 得到每位的值，0 or 1

    // U组显示属性
    if(digit[0] == 0)
    {
        for (i = 0; i < 16; i++)
        {
            funcCodeGrade[FUNCCODE_GROUP_U0 + i] = 0;
        }
    }
    
    // A组显示属性
    if(digit[1] == 0)
    {
        for (i = 0; i < 16; i++)
        {
            funcCodeGrade[FUNCCODE_GROUP_A0 + i] = 0;
        }
    }
    
    // B组显示属性
    if(digit[2] == 0)
    {
        for (i = 0; i < 16; i++)
        {
            funcCodeGrade[FUNCCODE_GROUP_B0 + i] = 0;
        }
    }

    // C组显示属性
    if(digit[3] == 0)
    {
        for (i = 0; i < 16; i++)
        {
            funcCodeGrade[FUNCCODE_GROUP_C0 + i] = 0;
        }
    }

    // 性能调试显示属性
#if DEBUG_F_MOTOR_FUNCCODE
    MotorDebugFcDeal();
    funcCodeGrade[FUNCCODE_GROUP_CF] = motorDebugFc.fc; // CF组个数
    funcCodeGrade[FUNCCODE_GROUP_UF] = motorDebugFc.u;  // UF组个数
#endif
    
    // AIAO校正功能码显示属性
    if (funcCode.code.aiaoCalibrateDisp)
    {
        funcCodeGrade[FUNCCODE_GROUP_AE] = AENUM;
    }
#endif

}


//====================================================================
// 按位(个位、十位、百位、千位、万位)薷牡氖乒δ苈耄晃�
// 每位仅能为0, 1
// 例如，功能码A6-06(虚拟VDI端子功能码设定有效状态)，
// 功能码显示为 11101，转换为 29。
//====================================================================
Uint16 FcDigit2Bin(Uint16 value)
{
    Uint16 tmp = 0;
    Uint16 digit[5];
    int16 i;

    GetNumberDigit(digit, value, DECIMAL);  // 得到每位的值，0 or 1

    for (i = 5-1; i >= 0; i--)
    {
        if (digit[i])       //if (1 == digit[i])    // 如果位为1.
        {
            tmp += 1 << i;
        }
    }

    return tmp;
}

//====================================================================
//
// 性能调试的功能码组CF、UF组的个数处理
//
//====================================================================
void MotorDebugFcDeal(void)
{
#if DEBUG_F_MOTOR_FUNCCODE
#if DEBUG_F_MOTOR_FUNCCODE1
    Uint16 fc;
    Uint16 u;

    fc = funcCode.code.motorDebugFc % 100;
    u = funcCode.code.motorDebugFc / 100;
    if (fc > CFNUM)
    {
        fc = CFNUM;
    }
    if (u > UFNUM)
    {
        u = UFNUM;
    }

    motorDebugFc.fc = fc;
    motorDebugFc.u = u;
#endif
#endif
}


//====================================================================
//
// 上电时的菜单初始化
//
//====================================================================
void MenuInit(void)
{
    menuAttri[MENU_LEVEL_0].winkFlag = 0x00F8;
	// 默认会选择第一级索引
    menuModeTmp = MENU_MODE_BASE;
    menuMode = menuModeTmp;
    MenuModeDeal();
    menuLevel = MENU_LEVEL_0;
}


#if DEBUG_F_DISP_DIDO_STATUS_SPECIAL
// 更新DIDO状态的直观显示
void UpdateDisplayBufferVisualIoStatus(Uint32 value)
{
    int16 i;
    Uint16 bit[4];

    for (i = 4; i >= 0; i--)
    {
        bit[0] = (value & (0x01 << 0)) >> 0;    // 获得0/1
        bit[1] = (value & (0x01 << 1)) >> 1;
        bit[2] = (value & (0x01 << 2)) >> 2;
        bit[3] = (value & (0x01 << 3)) >> 3;

        displayBuffer[i] = (bit[0] << 1) |      // 1表示显示位置，在数码管的位置
                           (bit[1] << 2) |      // 2表示显示位置，在数码管的位置
                           (bit[2] << 5) |
                           (bit[3] << 4) |
                           (~DISPLAY_CODE[DISPLAY_LINE]);
        displayBuffer[i] = ~displayBuffer[i];

        value >>= 4;
    }

    //displayBuffer[5] = LED_CODE[LED_NULL];
}


// DI功能的显示
// valueH, 高 8位
// valueL, 低32位
void UpdateDisplayBufferVisualDiFunc(Uint16 valueH, Uint32 valueL)
{
    //displayBuffer[5] = LED_CODE[LED_NULL];
    
    displayBuffer[4] = ~(Uint16)((valueL >> 0)  & 0xff);
    displayBuffer[3] = ~(Uint16)((valueL >> 8)  & 0xff);
    displayBuffer[2] = ~(Uint16)((valueL >> 16) & 0xff);
    displayBuffer[1] = ~(Uint16)((valueL >> 24) & 0xff);
    displayBuffer[0] = ~(Uint16)((valueH >> 0)  & 0xff);
}
#endif



#if DEBUG_F_PASSWORD      // 密码。包括用户密码，只读用户密码，功能码组隐藏密码
void MenuPwdOnPrg(void)
{
    // 如果正在进行密码检测，按键PRG退回到0级菜单。
    menuLevel = MENU_LEVEL_0;       // 重新复位

#if DEBUG_F_GROUP_HIDE
    if (groupHidePwdStatus)         // 重新复位，回到2级菜单下
    {
        groupHidePwdStatus = 0;
        menuLevel = MENU_LEVEL_2;
    }
#endif
}

// 按键ENTER, UP, DOWN，都进入MENU_LEVEL_PWD_INPUT
LOCALD void MenuPwdHint2Input(void)
{
    menuLevel = MENU_LEVEL_PWD_INPUT;
    menuAttri[MENU_LEVEL_PWD_INPUT].operateDigit = 0;
    menuPwdNumber = 0;                    // 密码初始为0
}

LOCALD void MenuPwdInputOnEnter(void)
{
    {
        if ((menuPwdNumber == funcCode.code.userPassword) ||    // 用户密码
            (menuPwdNumber == SUPER_USER_PASSWORD))             // 超级密码
        {
            Menu0AddMenuLevel();
        }
        else
        {
            menuLevel = MENU_LEVEL_PWD_HINT;
        }
    }
}

// 密码提示下(-----)按SHIFT键显示密码明文
LOCALD void MenuPwdHintOnShift(void)
{
#if DEBUG_RANDOM_FACPASS      
     facPassViewStatus= FAC_PASS_VIEW;

    // 生成超级密码
    if (!factoryPwd)
    {
        Uint32 time;
        Uint16 pwd[4];
        time = GetTime();
        factoryPwd = (time >> 16) + (time & 0xFFFF);
        pwd[0] = factoryPwd>>8;
        pwd[1] = SUPER_USER_PASSWORD_SOURCE1&0xFF;
        pwd[2] = SUPER_USER_PASSWORD_SOURCE1>>8;
        pwd[3] = factoryPwd&0xFF;
        superFactoryPass = CrcValueByteCalc(pwd, 4);
    }
#else
    MenuPwdHint2Input();
#endif
}

LOCALD void MenuPwdInputOnUp(void)
{
    MenuPwdInputOnUpDown(ON_UP_KEY);
}



LOCALD void MenuPwdInputOnDown(void)
{
    MenuPwdInputOnUpDown(ON_DOWN_KEY);
}



void MenuPwdInputOnUpDown(Uint16 flag)
{
    int16 delta;

    delta = decNumber[menuAttri[MENU_LEVEL_PWD_INPUT].operateDigit];

    if (ON_DOWN_KEY == flag)
        delta = -delta;

    menuPwdNumber = LimitDeal(0, menuPwdNumber, 65535, 0, delta);
}



LOCALD void MenuPwdInputOnShift(void)
{
    if (menuAttri[MENU_LEVEL_PWD_INPUT].operateDigit == 0)
        menuAttri[MENU_LEVEL_PWD_INPUT].operateDigit = 4;
    else
        menuAttri[MENU_LEVEL_PWD_INPUT].operateDigit--;
}



LOCALD void UpdateMenuPwdHintDisplayBuffer(void)
{  
#if DEBUG_RANDOM_FACPASS    
    if (facPassViewStatus == FAC_PASS_VIEW)
    {
        // 显示密码明文
        Uint16 digit[5];
        GetNumberDigit(digit, factoryPwd, 0);
        displayBuffer[0] = DISPLAY_CODE[digit[4]];
        displayBuffer[1] = DISPLAY_CODE[digit[3]];
        displayBuffer[2] = DISPLAY_CODE[digit[2]];
        displayBuffer[3] = DISPLAY_CODE[digit[1]];
        displayBuffer[4] = DISPLAY_CODE[digit[0]];
        menuAttri[MENU_LEVEL_PWD_HINT].winkFlag = 0;
    }
    else
#endif        
    {
        // 显示-----
        displayBuffer[0] = DISPLAY_CODE[DISPLAY_LINE];
        displayBuffer[1] = DISPLAY_CODE[DISPLAY_LINE];
        displayBuffer[2] = DISPLAY_CODE[DISPLAY_LINE];
        displayBuffer[3] = DISPLAY_CODE[DISPLAY_LINE];
        displayBuffer[4] = DISPLAY_CODE[DISPLAY_LINE];
        menuAttri[MENU_LEVEL_PWD_HINT].winkFlag = 0x08; // 320闪烁最后一位
    }
    
}



LOCALD void UpdateMenuPwdInputDisplayBuffer(void)
{   // 用户密码输入
    Uint16 digit[5];

    GetNumberDigit(digit, menuPwdNumber, DECIMAL);

// 数码管显示
    displayBuffer[0] = DISPLAY_CODE[digit[4]];
    displayBuffer[1] = DISPLAY_CODE[digit[3]];
    displayBuffer[2] = DISPLAY_CODE[digit[2]];
    displayBuffer[3] = DISPLAY_CODE[digit[1]];
    displayBuffer[4] = DISPLAY_CODE[digit[0]];

    menuAttri[MENU_LEVEL_PWD_INPUT].winkFlag = 0x01U << (3 + menuAttri[MENU_LEVEL_PWD_INPUT].operateDigit);
}

#elif 1

LOCALD void MenuPwdOnPrg(void){}
LOCALD void MenuPwdHint2Input(void){}
LOCALD void MenuPwdInputOnEnter(void){}
LOCALD void MenuPwdInputOnUp(void){}
LOCALD void MenuPwdInputOnDown(void){}
LOCALD void MenuPwdInputOnShift(void){}
LOCALD void UpdateMenuPwdHintDisplayBuffer(void){}
LOCALD void UpdateMenuPwdInputDisplayBuffer(void){}

#endif

LOCALD void MenuPwdHintOnQuick(void)
{
}
LOCALD void MenuPwdInputOnQuick(void)
{
}




