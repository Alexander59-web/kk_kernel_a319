#ifndef BUILD_LK
#include <linux/string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER ff или fff

#define LCM_ID_OTM8019A_CS					0x8019

#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
static LCM_UTIL_FUNCS lcm_util = {0};

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
//#define read_reg											lcm_util.dsi_read_reg()
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/

	{0x00, 1 , {0x00}},
	{0xff, 3, {0x80,0x19,0x01}},
	{0x00, 1, {0x80}},
	{0xff, 2, {0x80,0x19}},
	{0x00, 1, {0x8a}},
	{0xc4, 1, {0x40}},
	{0x00, 1, {0xa6}},
	{0xb3, 2, {0x20,0x01}},
	{0x00, 1, {0x90}},
	{0xc0, 6, {0x00,0x15,0x00,0x00,0x00,0x03}},
	{0x00, 1, {0xb4}},
	{0xc0, 1, {0x00}},
	{0x00, 1, {0x81}},
	{0xc1, 1, {0x33}},
	{0x00, 1, {0x81}},
	{0xc4, 1, {0x81}},
	{0x00, 1, {0x87}},
	{0xc4, 1, {0x00}},
	{0x00, 1, {0x89}},
	{0xc4, 1, {0x00}},
	{0x00, 1, {0x82}},
	{0xc5, 1, {0xb0}},
	{0x00, 1, {0x90}},
	{0xc5, 7, {0x6e,0xdb,0x06,0x91,0x44,0x44,0x23}},
	{0x00, 1, {0xb1}},
	{0xc5, 1, {0xa8}},
	{0x00, 1, {0x00}},
	{0xd8, 2, {0x6f,0x6f}},
	{0x00, 1, {0x00}},
	{0xd9, 1, {0x64}},
	{0x00, 1, {0x80}},
	{0xce, 12, {0x86,0x01,0x00,0x85,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xa0}},
	{0xce, 14, {0x18,0x05,0x83,0x39,0x00,0x00,0x00,0x18,0x04,0x83,0x3a,0x00,0x00,0x00}},
	{0x00, 1, {0xb0}},
	{0xce, 14, {0x18,0x03,0x83,0x3b,0x86,0x00,0x00,0x18,0x02,0x83,0x3c,0x88,0x00,0x00}},
	{0x00, 1, {0xc0}},
	{0xcf, 10, {0x01,0x01,0x20,0x20,0x00,0x00,0x01,0x02,0x00,0x00}},
	{0x00, 1, {0xd0}},
	{0xcf, 1, {0x00}},
	{0x00, 1, {0xc0}},
	{0xcb, 15, {0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xd0}},
	{0xcb, 1, {0x00}},
	{0x00, 1, {0xd5}},
	{0xcb, 10, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xe0}},
	{0xcb, 6, {0x01,0x01,0x01,0x01,0x01,0x00}},
	{0x00, 1, {0x80}},
	{0xcc, 10, {0x00,0x26,0x09,0x0b,0x01,0x25,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0x90}},
	{0xcc, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0x9a}},
	{0xcc, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xa0}},
	{0xcc, 11, {0x00,0x00,0x00,0x00,0x00,0x25,0x02,0x0c,0x0a,0x26,0x00}},
	{0x00, 1, {0xb0}},
	{0xcc, 10, {0x00,0x25,0x0c,0x0a,0x02,0x26,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xc0}},
	{0xcc, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xca}},
	{0xcc, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0x00, 1, {0xd0}},
	{0xcc, 11, {0x00,0x00,0x00,0x00,0x00,0x26,0x01,0x09,0x0b,0x25,0x00}},
	{0x00, 1, {0x00}},
	{0xe1, 20, {0x00,0x0e,0x1e,0x36,0x4e,0x65,0x6e,0x9c,0x89,0x9d,0x6c,0x5d,0x76,0x64,0x6a,0x64,0x5b,0x53,0x47,0x00}},
	{0x00, 1, {0x00}},
	{0xe2, 20, {0x00,0x0e,0x1e,0x36,0x4e,0x65,0x6e,0x9c,0x89,0x9d,0x6b,0x5c,0x76,0x64,0x6a,0x64,0x5b,0x53,0x46,0x00}},
	{0x00, 1, {0x98}},
	{0xc0, 1, {0x00}},
	{0x00, 1, {0xa9}},
	{0xc0, 1, {0x0a}},
	{0x00, 1, {0xb0}},
	{0xc1, 3, {0x20,0x00,0x00}},
	{0x00, 1, {0xe1}},
	{0xc0, 2, {0x40,0x30}},
	{0x00, 1, {0x80}},
	{0xc4, 1, {0x30}},
	{0x00, 1, {0x80}},
	{0xc1, 2, {0x03,0x33}},
	{0x00, 1, {0xa0}},
	{0xc1, 1, {0xe8}},
	{0x00, 1, {0x90}},
	{0xb6, 1, {0xb4}},
	{0x00, 1, {0x00}},
	{0xff, 3, {0xff,0xff,0xff}},
	{0x00, 1, {0x00}},
	{0x3a, 1, {0x77}},
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    
    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    
    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				//MDELAY(5);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util) //сделанно
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params) // сделанно костыльно
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI; //2

		params->width  = FRAME_WIDTH; //480
		params->height = FRAME_HEIGHT; //800

		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY; //1
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING; //0
		
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; //1
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE; //2
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB; //0
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST; //0
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB; //0
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888; //2
		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888; //2
		params->dsi.word_count=480*3; //1440

		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 32;
		params->dsi.vertical_frontporch					= 36;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 4;
		params->dsi.horizontal_backporch				= 55;
		params->dsi.horizontal_frontporch				= 55;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		params->dsi.horizontal_blanking_pixel			= 120;



		// Bit rate calculation
		//params->dsi.PLL_CLOCK=200;//210
		params->dsi.pll_div1=1;		// div1=0,1,2,3;div1_real=1,2,4,4 ----0: 546Mbps  1:273Mbps
		params->dsi.pll_div2=1;		// div2=0,1,2,3;div1_real=1,2,4,4	
		params->dsi.fbk_div =30;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
		params->dsi.fbk_sel = 1;
		//params->dsi.compatibility_for_nvk = 1;		// this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
}


static void lcm_init(void) //сделанно
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void) //сделанно
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void) //сделанно
{
	lcm_init();
}

static unsigned int lcm_compare_id(void) // сделанно
{
	int array[4];
	char buffer[5]; 
	char id_high=0;
	char id_low=0;
	int id=0;

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	array[0] = 0x00053700;
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);
	read_reg_v2(0xa1, buffer, 5);

	id_high = buffer[2];
	id_low = buffer[3];
	id = (id_high<<8) | id_low;

	#if defined(BUILD_LK)
		printf("OTM8018B CS uboot %s \n", __func__);
		printf("%s id = 0x%08x \n", __func__, id);
	#else
		printk("OTM8018B CS kernel %s \n", __func__);
		printk("%s id = 0x%08x \n", __func__, id);
	#endif

	return (LCM_ID_OTM8019A_CS == id)?1:0;
}

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK

    unsigned char buffer[1];
    unsigned int array[16];

    array[0] = 0x00013700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

      read_reg_v2(0x0A, buffer, 1);

    printk("lcm_esd_check  0x0A = %x\n",buffer[0]);


        if(buffer[0] != 0x9C)  
        {
        return TRUE;
        }

#if 0
	array[0] = 0x00013700;// read id return two byte,version and id
   	dsi_set_cmdq(array, 1, 1);
//	id = read_reg(0xF4);
	read_reg_v2(0x0D, buffer, 1);
#if defined(BUILD_UBOOT)
    printf("lcm_esd_check 0x0D =%x\n",buffer[0]);
#else
    printk("lcm_esd_check 0x0D =%x\n",buffer[0]);
#endif
   if(buffer[0] != 0x00)
   {
      return TRUE;
   }


   array[0] = 0x00013700;// read id return two byte,version and id
   dsi_set_cmdq(array, 1, 1);
//	id = read_reg(0xF4);
   read_reg_v2(0x0E, buffer, 1);
#if defined(BUILD_UBOOT)
    printf("lcm_esd_check  0x0E = %x\n",buffer[0]);
#else
    printk("lcm_esd_check  0x0E = %x\n",buffer[0]);
#endif
   if(buffer[0] != 0x00)
   {
      return TRUE;
   }

#endif
    
#endif
    return FALSE;
}


static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK
    lcm_init();
#endif

    return TRUE;
}


LCM_DRIVER otm8019a_wvga_dsi_vdo_lcm_drv = 
{
    .name			= "otm8019a_dsi_vdo_dijing",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,	
	//.esd_check    = lcm_esd_check,
    .esd_recover  = lcm_esd_recover,
#if (LCM_DSI_CMD_MODE)
	.set_backlight	= lcm_setbacklight,
    .update         = lcm_update,
#endif
};