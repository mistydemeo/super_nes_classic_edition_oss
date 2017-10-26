/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <asm/arch/cpu.h>
#include <sys_config.h>
#include <smc.h>
#include <pmu.h>

DECLARE_GLOBAL_DATA_PTR;

static script_sub_key_t *sw_cfg_get_subkey(const char *script_buf, const char *main_key, const char *sub_key)
{
    script_head_t *hd = (script_head_t *)script_buf;
    script_main_key_t *mk = (script_main_key_t *)(hd + 1);
    script_sub_key_t *sk = NULL;
    int i, j;

    for (i = 0; i < hd->main_key_count; i++) {

        if (strcmp(main_key, mk->main_name)) {
            mk++;
            continue;
        }

        for (j = 0; j < mk->lenth; j++) {
            sk = (script_sub_key_t *)(script_buf + (mk->offset<<2) + j * sizeof(script_sub_key_t));
            if (!strcmp(sub_key, sk->sub_name)) return sk;
        }
    }
    return NULL;
}

int sw_cfg_get_int(const char *script_buf, const char *main_key, const char *sub_key)
{
    script_sub_key_t *sk = NULL;
    char *pdata;
    int value;

    sk = sw_cfg_get_subkey(script_buf, main_key, sub_key);
    if (sk == NULL) {
        return -1;
    }

    if (((sk->pattern >> 16) & 0xffff) == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD) {
        pdata = (char *)(script_buf + (sk->offset<<2));
        value = *((int *)pdata);
        return value;
    }

    return -1;
}

char *sw_cfg_get_str(const char *script_buf, const char *main_key, const char *sub_key, char *buf)
{
    script_sub_key_t *sk = NULL;
    char *pdata;

    sk = sw_cfg_get_subkey(script_buf, main_key, sub_key);
    if (sk == NULL) {
        return NULL;
    }

    if (((sk->pattern >> 16) & 0xffff) == SCIRPT_PARSER_VALUE_TYPE_STRING) {
        pdata = (char *)(script_buf + (sk->offset<<2));
        memcpy(buf, pdata, ((sk->pattern >> 0) & 0xffff));
        return (char *)buf;
    }

    return NULL;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
//static int gpio_probe_axpgpio_value(int port_num)
//{
//	char pmu_name[16];
//	if((port_num == 0) || (port_num == 1))
//	{
//		if(axp_probe_supply_pmu_name(pmu_name))
//		{
//			return -1;
//		}
//
//		if(port_num == 0)
//		{
//			return axp_probe_supply_status_byname(pmu_name, "power0");
//		}
//		else if(port_num == 1)
//		{
//			return axp_probe_supply_status_byname(pmu_name, "power1");
//		}
//	}
//
//	return -1;
//}

/**########################################################################################
 *
 *                        Script Operations
 *
-#########################################################################################*/
//static  char  *script_mod_buf = (char *)(4); //pointer to first key
//static  int    script_main_key_count = -1;

static  int   _test_str_length(char *str)
{
    int length = 0;

    while(str[length++])
    {
        if(length > 32)
        {
            length = 32;
            break;
        }
    }

    return length;
}

int script_parser_init(char *script_buf)
{
    script_head_t   *script_head;

	if(script_buf)
    {
        gd->script_mod_buf = script_buf;
        script_head = (script_head_t *)script_buf;

        gd->script_main_key_count = script_head->main_key_count;

        return SCRIPT_PARSER_OK;
    }
    else
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }
}
#ifdef CONFIG_SMALL_MEMSIZE
void save_config(void)
{
    script_head_t *script_head;
    uint script_length = 0;

    script_head = (script_head_t *)gd->script_mod_buf;
    script_length = script_head->length;
    if(script_length)
    {
        printf("save config for small mem_size \n");
        gd->script_mod_buf = (char *)malloc(script_length * sizeof(char));
        if(!gd->script_mod_buf)
        {
            printf("do not have enough memory to save config \n");
            return ;
        }
        memcpy((void *)gd->script_mod_buf,(void *)script_head ,script_length);
    }
    return ;
}


void reload_config(void)
{
    script_head_t *script_head;
    uint script_length = 0;

    script_head = (script_head_t *)gd->script_mod_buf;
    script_length = script_head->length;
    if(script_length)
    {
        printf("reload config to 0x43000000 \n");
        memcpy((void*)SYS_CONFIG_MEMBASE,(void*)script_head ,script_length);
    }
    return ;
}
#endif
unsigned script_get_length(void)
{
	script_head_t *orign_head = (script_head_t *)gd->script_mod_buf;

	return orign_head->length;
}

int script_parser_exit(void)
{
    gd->script_mod_buf = NULL;
    gd->script_main_key_count = 0;

    return SCRIPT_PARSER_OK;
}

uint script_parser_subkey( script_main_key_t* main_name,char *subkey_name , uint *pattern)
{
    script_main_key_t *main_key = NULL;
    script_sub_key_t  *sub_key  = NULL;
    int i = 0;

    if((!gd->script_mod_buf)||(gd->script_main_key_count<=0))
    {
        return 0;
    }
    if((main_name == NULL)||(subkey_name == NULL))
    {
        printf("main_name is invalid \n");
        return 0;
    }

    main_key = main_name;
    for(i = 0;i<main_key->lenth;i++)
    {
        sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (i * sizeof(script_sub_key_t)));
        if(strcmp(sub_key->sub_name,subkey_name))
            continue;
        *pattern = (sub_key->pattern>>16)&0xffff;
        return (uint)sub_key;
    }
    return 0;
}

uint script_parser_fetch_subkey_start(char *main_name)
{
	char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    int    i;
    /* check params */
    if((!gd->script_mod_buf) || (gd->script_main_key_count <= 0))
    {
        return 0;
    }

    if(main_name == NULL)
    {
		return 0;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

		return (uint)main_key;
	}

	return 0;
}


int script_parser_fetch_subkey_next(uint hd, char *sub_name, int value[], int *index)
{
    script_main_key_t  *main_key;
    script_sub_key_t   *sub_key = NULL;
    int    j;
    int    pattern;

	if(!hd)
	{
		return -1;
	}

	main_key = (script_main_key_t *)hd;
    /* now find sub key */
    for(j = *index; j < main_key->lenth; j++)
    {
    	sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

        pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
		if(pattern == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD)
		{
			value[0] = *(int *)(gd->script_mod_buf + (sub_key->offset<<2));
			strcpy(sub_name, sub_key->sub_name);

			*index = j + 1;

			return SCRIPT_PARSER_OK;
		}
		else if(pattern == SCIRPT_PARSER_VALUE_TYPE_STRING)
		{
			strcpy((void *)value, gd->script_mod_buf + (sub_key->offset<<2));
			strcpy(sub_name, sub_key->sub_name);

			*index = j + 1;

			return SCRIPT_PARSER_OK;
		}
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_fetch(char *main_name, char *sub_name, int value[], int count)
{
    char   main_bkname[32], sub_bkname[32];
    char   *main_char, *sub_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, word_count;
    /* check params */
    if((!gd->script_mod_buf) || (gd->script_main_key_count <= 0))
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if((main_name == NULL) || (sub_name == NULL))
    {
		return SCRIPT_PARSER_KEYNAME_NULL;
    }

    if(value == NULL)
    {
		return SCRIPT_PARSER_DATA_VALUE_NULL;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }
    sub_char = sub_name;
    if(_test_str_length(sub_name) > 31)
    {
        memset(sub_bkname, 0, 32);
        strncpy(sub_bkname, sub_name, 31);
        sub_char = sub_bkname;
    }
    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        /* now find sub key */
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            if(strcmp(sub_key->sub_name, sub_char))
            {
                continue;
            }
            pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
            word_count = (sub_key->pattern>> 0) & 0xffff; /*get count of word */

            switch(pattern)
            {
                case SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD:
                    value[0] = *(int *)(gd->script_mod_buf + (sub_key->offset<<2));
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_STRING:
                    if(count < word_count)
                    {
                        word_count = count;
                    }
                    memcpy((char *)value, gd->script_mod_buf + (sub_key->offset<<2), word_count << 2);
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD:
                    break;
                case SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD:
                {
                    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)value;
                    /* buffer space enough? */
                    if(sizeof(script_gpio_set_t) > (count<<2))
                    {
                        return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
                    }
                    strcpy( user_gpio_cfg->gpio_name, sub_char);
                    memcpy(&user_gpio_cfg->port, gd->script_mod_buf + (sub_key->offset<<2),  sizeof(script_gpio_set_t) - 32);
                    break;
                }
            }

            return SCRIPT_PARSER_OK;
        }
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_fetch_ex(char *main_name, char *sub_name, int value[], script_parser_value_type_t *type, int count)
{
    char   main_bkname[32], sub_bkname[32];
    char   *main_char, *sub_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, word_count;
    /* check params */
    if((!gd->script_mod_buf) || (gd->script_main_key_count <= 0))
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if((main_name == NULL) || (sub_name == NULL))
    {
		return SCRIPT_PARSER_KEYNAME_NULL;
    }

    if(value == NULL)
    {
		return SCRIPT_PARSER_DATA_VALUE_NULL;
    }

    /* truncate string if size >31 bytes */
    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }
    sub_char = sub_name;
    if(_test_str_length(sub_name) > 31)
    {
        memset(sub_bkname, 0, 32);
        strncpy(sub_bkname, sub_name, 31);
        sub_char = sub_bkname;
    }
    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        /* now find sub key */
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
            if(strcmp(sub_key->sub_name, sub_char))
            {
                continue;
            }
            pattern    = (sub_key->pattern>>16) & 0xffff; /* get datatype */
            word_count = (sub_key->pattern>> 0) & 0xffff; /*get count of word */

            switch(pattern)
            {
                case SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD:
                    value[0] = *(int *)(gd->script_mod_buf + (sub_key->offset<<2));
                    *type = SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD;
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_STRING:
                    if(count < word_count)
                    {
                        word_count = count;
                    }
                    memcpy((char *)value, gd->script_mod_buf + (sub_key->offset<<2), word_count << 2);
                    *type = SCIRPT_PARSER_VALUE_TYPE_STRING;
                    break;

                case SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD:
					*type = SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD;
                    break;
                case SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD:
                {
                    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)value;
                    /* buffer space enough? */
                    if(sizeof(script_gpio_set_t) > (count<<2))
                    {
                        return SCRIPT_PARSER_BUFFER_NOT_ENOUGH;
                    }
                    strcpy( user_gpio_cfg->gpio_name, sub_char);
                    memcpy(&user_gpio_cfg->port, gd->script_mod_buf + (sub_key->offset<<2),  sizeof(script_gpio_set_t) - 32);
                    *type = SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD;
                    break;
                }
            }

            return SCRIPT_PARSER_OK;
        }
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_patch_all(char *main_name, void *str, uint data_count)
{
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int    i, j;
	int    data_max;
	int    pattern;
	uint   *data = (uint *)str;

	//潰脤褐掛buffer岆瘁湔婓
	if(!gd->script_mod_buf)
	{
		return SCRIPT_PARSER_EMPTY_BUFFER;
	}
	//潰脤翋瑩靡備睿赽瑩靡備岆瘁峈諾
	if(main_name == NULL)
	{
		return SCRIPT_PARSER_KEYNAME_NULL;
	}
	//潰脤杅擂buffer岆瘁峈諾
	if(str == NULL)
	{
		return SCRIPT_PARSER_DATA_VALUE_NULL;
	}
	//悵湔翋瑩靡備睿赽瑩靡備ㄛ彆閉徹31趼誹寀諍31趼誹
	for(i=0;i<gd->script_main_key_count;i++)
	{
		main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
		if(strcmp(main_key->main_name, main_name))    //彆翋瑩祥饜ㄛ扆梑狟珨跺翋瑩
		{
			continue;
		}
		if(data_count > main_key->lenth)
		{
			data_max = main_key->lenth;
		}
		else
		{
			data_max = data_count;
		}
		//翋瑩饜ㄛ扆梑赽瑩靡備饜
		for(j=0;j<data_max;j++)
		{
			sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

			pattern    = (sub_key->pattern>>16) & 0xffff;             //鳳杅擂腔濬倰
			//堤杅擂
			if(pattern == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD)                      //等word杅擂濬倰
			{
				*(int *)(gd->script_mod_buf + (sub_key->offset<<2)) = *(int *)data;
				data ++;
    		}
		}

		return 0;
	}

	return -1;
}

int script_parser_patch(char *main_name, char *sub_name, void *str, int str_size)
{
	char   main_bkname[32], sub_bkname[32];
	char   *main_char, *sub_char;
	script_main_key_t  *main_key = NULL;
	script_sub_key_t   *sub_key = NULL;
	int    i, j;
	int    pattern, word_count;

	//潰脤褐掛buffer岆瘁湔婓
	if(!gd->script_mod_buf)
	{
		return SCRIPT_PARSER_EMPTY_BUFFER;
	}
	//潰脤翋瑩靡備睿赽瑩靡備岆瘁峈諾
	if((main_name == NULL) || (sub_name == NULL))
	{
		return SCRIPT_PARSER_KEYNAME_NULL;
	}
	//潰脤杅擂buffer岆瘁峈諾
	if(str == NULL)
	{
		return SCRIPT_PARSER_DATA_VALUE_NULL;
	}
	//悵湔翋瑩靡備睿赽瑩靡備ㄛ彆閉徹31趼誹寀諍31趼誹
	main_char = main_name;
	if(_test_str_length(main_name) > 31)
	{
	    memset(main_bkname, 0, 32);
		strncpy(main_bkname, main_name, 31);
		main_char = main_bkname;
	}
    sub_char = sub_name;
	if(_test_str_length(sub_name) > 31)
	{
		memset(sub_bkname, 0, 32);
		strncpy(sub_bkname, sub_name, 31);
		sub_char = sub_bkname;
	}
	for(i=0;i<gd->script_main_key_count;i++)
	{
		main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
		if(strcmp(main_key->main_name, main_char))    //彆翋瑩祥饜ㄛ扆梑狟珨跺翋瑩
		{
			continue;
		}
		//翋瑩饜ㄛ扆梑赽瑩靡備饜
		for(j=0;j<main_key->lenth;j++)
		{
			sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
			if(strcmp(sub_key->sub_name, sub_char))    //彆翋瑩祥饜ㄛ扆梑狟珨跺翋瑩
			{
				continue;
			}
			pattern    = (sub_key->pattern>>16) & 0xffff;             //鳳杅擂腔濬倰
			word_count = (sub_key->pattern>> 0) & 0xffff;             //鳳垀梩蚚腔word跺杅
			//堤杅擂
			if(pattern == SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD)                      //等word杅擂濬倰
			{
				*(int *)(gd->script_mod_buf + (sub_key->offset<<2)) = *(int *)str;

    			return SCRIPT_PARSER_OK;
    		}
    		else if(pattern == SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD)
    		{
    			script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)str;

    			memset(gd->script_mod_buf + (sub_key->offset<<2), 0, 24);
    			memcpy(gd->script_mod_buf + (sub_key->offset<<2), &user_gpio_cfg->port, 24);

    			return SCRIPT_PARSER_OK;
    		}
    		else if(pattern == SCIRPT_PARSER_VALUE_TYPE_STRING)
    		{
    			if(str_size > word_count)
				{
					str_size = word_count;
				}
				memset(gd->script_mod_buf + (sub_key->offset<<2), 0, word_count << 2);
				memcpy(gd->script_mod_buf + (sub_key->offset<<2), str, str_size << 2);

				return SCRIPT_PARSER_OK;
    		}
		}
	}

	return SCRIPT_PARSER_KEY_NOT_FIND;
}

int script_parser_subkey_count(char *main_name)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    int    i;

    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        return main_key->lenth;
    }

    return -1;
}

int script_parser_mainkey_count(void)
{
    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    return  gd->script_main_key_count;
}

int script_parser_mainkey_get_gpio_count(char *main_name)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    int    i, j;
    int    pattern, gpio_count = 0;

    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));

            pattern    = (sub_key->pattern>>16) & 0xffff;

            if(SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD == pattern)
            {
                gpio_count ++;
            }
        }
    }

    return gpio_count;
}

int script_parser_mainkey_get_gpio_cfg(char *main_name, void *gpio_cfg, int gpio_count)
{
    char   main_bkname[32];
    char   *main_char;
    script_main_key_t  *main_key = NULL;
    script_sub_key_t   *sub_key = NULL;
    script_gpio_set_t  *user_gpio_cfg = (script_gpio_set_t *)gpio_cfg;
    int    i, j;
    int    pattern, user_index;

    if(!gd->script_mod_buf)
    {
        return SCRIPT_PARSER_EMPTY_BUFFER;
    }

    if(main_name == NULL)
    {
        return SCRIPT_PARSER_KEYNAME_NULL;
    }

    memset(user_gpio_cfg, 0, sizeof(script_gpio_set_t) * gpio_count);

    main_char = main_name;
    if(_test_str_length(main_name) > 31)
    {
        memset(main_bkname, 0, 32);
        strncpy(main_bkname, main_name, 31);
        main_char = main_bkname;
    }

    for(i=0;i<gd->script_main_key_count;i++)
    {
        main_key = (script_main_key_t *)(gd->script_mod_buf + (sizeof(script_head_t)) + i * sizeof(script_main_key_t));
        if(strcmp(main_key->main_name, main_char))
        {
            continue;
        }

       /* printf("mainkey name = %s\n", main_key->main_name);*/
        user_index = 0;
        for(j=0;j<main_key->lenth;j++)
        {
            sub_key = (script_sub_key_t *)(gd->script_mod_buf + (main_key->offset<<2) + (j * sizeof(script_sub_key_t)));
          /*  printf("subkey name = %s\n", sub_key->sub_name);*/
            pattern    = (sub_key->pattern>>16) & 0xffff;
           /* printf("subkey pattern = %d\n", pattern);*/

            if(SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD == pattern)
            {
                strcpy( user_gpio_cfg[user_index].gpio_name, sub_key->sub_name);
                memcpy(&user_gpio_cfg[user_index].port, gd->script_mod_buf + (sub_key->offset<<2), sizeof(script_gpio_set_t) - 32);
                user_index++;
                if(user_index >= gpio_count)
                {
                    break;
                }
            }
        }
        return SCRIPT_PARSER_OK;
    }

    return SCRIPT_PARSER_KEY_NOT_FIND;
}

/**#############################################################################################################
 *
 *                           GPIO(PIN) Operations
 *
-##############################################################################################################*/
#define PIO_REG_CFG(n, i)               ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x00))
#define PIO_REG_DLEVEL(n, i)            ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x14))
#define PIO_REG_PULL(n, i)              ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x1C))
#define PIO_REG_DATA(n)                   ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + 0x10))

#define PIO_REG_CFG_VALUE(n, i)          (readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x00))
#define PIO_REG_DLEVEL_VALUE(n, i)       (readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x14))
#define PIO_REG_PULL_VALUE(n, i)         (readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x1C))
#define PIO_REG_DATA_VALUE(n)            (readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + 0x10))
#define PIO_REG_BASE(n)                    ((volatile unsigned int *)(SUNXI_PIO_BASE +((n)-1)*24))

#ifdef SUNXI_R_PIO_BASE

#define R_PIO_REG_CFG(n, i)               ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x00))
#define R_PIO_REG_DLEVEL(n, i)            ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x14))
#define R_PIO_REG_PULL(n, i)              ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x1C))
#define R_PIO_REG_DATA(n)                 ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + 0x10))

#define R_PIO_REG_CFG_VALUE(n, i)          (smc_readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x00))
#define R_PIO_REG_DLEVEL_VALUE(n, i)       (smc_readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x14))
#define R_PIO_REG_PULL_VALUE(n, i)         (smc_readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x1C))
#define R_PIO_REG_DATA_VALUE(n)            (smc_readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + 0x10))
#define R_PIO_REG_BASE(n)                    ((volatile unsigned int *)(SUNXI_R_PIO_BASE +((n)-12)*24))

#endif
typedef struct
{
    int mul_sel;
    int pull;
    int drv_level;
    int data;
} gpio_status_set_t;

typedef struct
{
    char    gpio_name[32];
    int     port;
    int     port_num;
    gpio_status_set_t user_gpio_status;
    gpio_status_set_t hardware_gpio_status;
} system_gpio_set_t;

/*
****************************************************************************************************
*
*             CSP_PIN_init
*
*  Description:
*       init
*
*  Parameters:
*  Return value:
*        EGPIO_SUCCESS/EGPIO_FAIL
****************************************************************************************************
*/

int sw_gpio_init(void)
{
    return script_parser_init((char *)SYS_CONFIG_MEMBASE);
}

/*
****************************************************************************************************
*
*             CSP_PIN_exit
*
*  Description:
*       exit
*
*  Parameters:
*
*  Return value:
*        EGPIO_SUCCESS/EGPIO_FAIL
****************************************************************************************************
*/
__s32 gpio_exit(void)
{
    return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
static int gpio_set_axpgpio_value(int pmu_type, int port_num, int level)
{
	int ret =  0;

	if(port_num == 0)
	{
		ret = axp_set_supply_status(pmu_type, PMU_SUPPLY_GPIO0, 0, level);
	}
	else if (port_num == 1)
	{
		ret = axp_set_supply_status(pmu_type, PMU_SUPPLY_GPIO1, 0, level);
	}

	if(ret)
	{
		printf("set axp gpio failed\n");
		return -1;
	}

	return 0;
}
#endif

#define GPIO_REG_READ(reg)              (smc_readl((unsigned int)(reg)))
#define GPIO_REG_WRITE(reg, value)      (smc_writel(value, (unsigned int)(reg)))

#define PIOC_REG_o_CFG0                 0x00
#define PIOC_REG_o_CFG1                 0x04
#define PIOC_REG_o_CFG2                 0x08
#define PIOC_REG_o_CFG3                 0x0C
#define PIOC_REG_o_DATA                 0x10
#define PIOC_REG_o_DRV0                 0x14
#define PIOC_REG_o_DRV1                 0x18
#define PIOC_REG_o_PUL0                 0x1C
#define PIOC_REG_o_PUL1                 0x20


/*
************************************************************************************************************
*
*                                             gpio_request_early
*
*    滲杅靡備ㄩ
*
*    統杅蹈桶ㄩ
*
*
*
*    殿隙硉  ㄩ
*
*    佽隴    ㄩ
*
*
************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
int gpio_request_early(void  *user_gpio_list, __u32 group_count_max, __s32 set_gpio)
{
	user_gpio_set_t    *tmp_user_gpio_data, *gpio_list;
	__u32				first_port;                      //悵湔淩淏衄虴腔GPIO腔跺杅
	__u32               tmp_group_func_data = 0;
	__u32               tmp_group_pull_data = 0;
	__u32               tmp_group_dlevel_data = 0;
	__u32               tmp_group_data_data = 0;
	__u32               data_change = 0;
//	__u32			   *tmp_group_port_addr;
	volatile __u32     *tmp_group_func_addr = NULL,   *tmp_group_pull_addr = NULL;
	volatile __u32     *tmp_group_dlevel_addr = NULL, *tmp_group_data_addr = NULL;
	__u32  				port = 0, port_num = 0, pre_port_num = 0, port_num_func = 0, port_num_pull = 0;
	__u32  				pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff;
    __u32  				pre_port_num_pull = 0x7fffffff;
	__s32               i, tmp_val;
	int cpus_flag = 0;

	gpio_list = (user_gpio_set_t *)user_gpio_list;
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;                         //黍堤傷諳杅硉
	    port_num = tmp_user_gpio_data->port_num;                     //黍堤傷諳笢腔議珨跺GPIO
	    pre_port_num = port_num;
	    if(!port)
	    {
			continue;
	    }

		port_num_func = (port_num >> 3);
		port_num_pull = (port_num >> 4);
	    if(port == 0xffff)
		{
			tmp_group_data_data = tmp_user_gpio_data->data & 1;
			data_change = 1;

			pre_port          = port;
	        pre_port_num_func = port_num_func;
	        pre_port_num_pull = port_num_pull;
		}
		else
		{
			if(port >= 12)
				cpus_flag = 1;
			else
				cpus_flag = 0;

			if(!cpus_flag)
			{
				tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
				tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
				tmp_group_data_addr    = PIO_REG_DATA(port);                 //載陔data敵湔
			}
			else
			{
				tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
				tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
				tmp_group_data_addr    = R_PIO_REG_DATA(port);                 //載陔data敵湔

			}
	        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
	        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
	        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
	        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);

	        pre_port          = port;
	        pre_port_num_func = port_num_func;
	        pre_port_num_pull = port_num_pull;
	        //載陔髡夔敵湔
	        tmp_val = (port_num - (port_num_func << 3)) << 2;
	        tmp_group_func_data &= ~(0x07 << tmp_val);
	        if(set_gpio)
	        {
				tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
	        }
	        //跦擂pull腔硉樵隅岆瘁載陔pull敵湔
	        tmp_val = (port_num - (port_num_pull << 4)) << 1;
	        if(tmp_user_gpio_data->pull >= 0)
	        {
				tmp_group_pull_data &= ~(                           0x03  << tmp_val);
				tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
	        }
	        //跦擂driver level腔硉樵隅岆瘁載陔driver level敵湔
	        if(tmp_user_gpio_data->drv_level >= 0)
	        {
				tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
				tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
	        }
	        //跦擂蚚誧怀ㄛ眕摯髡夔煦饜樵隅岆瘁載陔data敵湔
	        if(tmp_user_gpio_data->mul_sel == 1)
	        {
	            if(tmp_user_gpio_data->data >= 0)
	            {
					tmp_val = tmp_user_gpio_data->data & 1;
	                tmp_group_data_data &= ~(1 << port_num);
	                tmp_group_data_data |= tmp_val << port_num;
	                data_change = 1;
	            }
	        }
		}

        break;
	}
	//潰脤岆瘁衄杅擂湔婓
	if(first_port >= group_count_max)
	{
	    return -1;
	}

	//悵湔蚚誧杅擂
	for(i = first_port + 1; i < group_count_max; i++)
	{
		tmp_user_gpio_data = gpio_list + i;                 //gpio_set甡棒硌砃蚚誧腔藩跺GPIO杅郪傖埜
	    port     = tmp_user_gpio_data->port;                //黍堤傷諳杅硉
	    port_num = tmp_user_gpio_data->port_num;            //黍堤傷諳笢腔議珨跺GPIO
	    if(!port)
	    {
			break;
	    }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

		if(port >= 12)
			cpus_flag = 1;
		else
			cpus_flag = 0;

        if((port_num_pull != pre_port_num_pull) || (port != pre_port) || (pre_port == 0xffff))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
        {
            if(pre_port == 0xffff)
            {
				if(data_change)
				{
					data_change = 0;
					gpio_set_axpgpio_value(0, pre_port_num, tmp_group_data_data);
				}
            }
            else
            {
				GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);     //隙迡髡夔敵湔
				GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);     //隙迡pull敵湔
				GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //隙迡driver level敵湔
				if(data_change)
				{
					data_change = 0;
					GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //隙迡data敵湔
				}
            }
			if(port == 0xffff)
			{
				tmp_group_data_data = tmp_user_gpio_data->data & 1;
				data_change = 1;
			}
			else
			{
				if(!cpus_flag)
				{
					tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
					tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
					tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
					tmp_group_data_addr    = PIO_REG_DATA(port);                 //載陔data敵湔
				}
				else
				{
					tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
					tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
					tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
					tmp_group_data_addr    = R_PIO_REG_DATA(port);                 //載陔data敵湔

				}
	            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
	            tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
	            tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
	            tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
			}
        }
        else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
        {
            if(pre_port == 0xffff)
            {
				gpio_set_axpgpio_value(0, pre_port_num, tmp_group_data_data);
            }
            else
            {
				GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //寀硐隙迡髡夔敵湔
            }

			if(port == 0xffff)
			{
				tmp_group_data_data = tmp_user_gpio_data->data & 1;
			}
			else
			{
				if(!cpus_flag)
					tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				else
					tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);
	            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
			}
        }
		//悵湔絞茞璃敵湔杅擂
        pre_port_num_pull = port_num_pull;                      //扢离絞GPIO傖峈珨跺GPIO
        pre_port_num_func = port_num_func;
        pre_port          = port;
		pre_port_num	  = port_num;
        if(port == 0xffff)
        {
        	if(tmp_user_gpio_data->data >= 0)
            {
            	tmp_group_data_data = tmp_user_gpio_data->data & 1;
                data_change = 1;
            }
        }
        else
        {
        	//載陔髡夔敵湔
	        tmp_val = (port_num - (port_num_func << 3)) << 2;
	        if(tmp_user_gpio_data->mul_sel >= 0)
	        {
				tmp_group_func_data &= ~(                              0x07  << tmp_val);
				if(set_gpio)
				{
					tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
				}
			}
	        //跦擂pull腔硉樵隅岆瘁載陔pull敵湔
	        tmp_val = (port_num - (port_num_pull << 4)) << 1;
	        if(tmp_user_gpio_data->pull >= 0)
	        {
				tmp_group_pull_data &= ~(                           0x03  << tmp_val);
				tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
	        }
	        //跦擂driver level腔硉樵隅岆瘁載陔driver level敵湔
	        if(tmp_user_gpio_data->drv_level >= 0)
	        {
				tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
				tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
	        }
	        //跦擂蚚誧怀ㄛ眕摯髡夔煦饜樵隅岆瘁載陔data敵湔
	        if(tmp_user_gpio_data->mul_sel == 1)
	        {
				if(tmp_user_gpio_data->data >= 0)
	            {
					tmp_val = tmp_user_gpio_data->data & 1;
	                tmp_group_data_data &= ~(1 << port_num);
	                tmp_group_data_data |= tmp_val << port_num;
	                data_change = 1;
	            }
	        }
        }
    }
    //for悜遠賦旰ㄛ彆湔婓遜羶衄隙迡腔敵湔ㄛ涴爵迡隙善茞璃絞笢
	if(port == 0xffff)
	{
		if(data_change)
		{
			gpio_set_axpgpio_value(0, port_num, tmp_group_data_data);
		}
	}
	else
	{
		if(tmp_group_func_addr)                         //硐猁載陔徹敵湔華硊ㄛ憩褫眕勤茞璃董硉
		{                                               //饒繫參垀衄腔硉窒隙迡善茞璃敵湔
			GPIO_REG_WRITE(tmp_group_func_addr,   tmp_group_func_data);   //隙迡髡夔敵湔
			GPIO_REG_WRITE(tmp_group_pull_addr,   tmp_group_pull_data);   //隙迡pull敵湔
			GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //隙迡driver level敵湔
			if(data_change)
			{
				GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //隙迡data敵湔
			}
		}
	}

    return 0;
}
#else
 int gpio_request_early(void  *user_gpio_list, __u32 group_count_max, __s32 set_gpio)
{
	user_gpio_set_t    *tmp_user_gpio_data, *gpio_list;
	__u32				first_port;                      //悵湔淩淏衄虴腔GPIO腔跺杅
	__u32               tmp_group_func_data;
	__u32               tmp_group_pull_data;
	__u32               tmp_group_dlevel_data;
	__u32               tmp_group_data_data;
	__u32               data_change = 0;
//	__u32			   *tmp_group_port_addr;
	volatile __u32     *tmp_group_func_addr,   *tmp_group_pull_addr;
	volatile __u32     *tmp_group_dlevel_addr, *tmp_group_data_addr;
	__u32  				port, port_num, port_num_func, port_num_pull;
	__u32  				pre_port, pre_port_num_func;
	__u32  				pre_port_num_pull;
	__s32               i, tmp_val;


   	gpio_list = (user_gpio_set_t *)user_gpio_list;

    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;                         //黍堤傷諳杅硉
	    port_num = tmp_user_gpio_data->port_num;                     //黍堤傷諳笢腔議珨跺GPIO
	    if(!port)
	    {
	    	continue;
	    }
	    port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
        tmp_group_data_addr    = PIO_REG_DATA(port);                 //載陔data敵湔

        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);

        pre_port          = port;
        pre_port_num_func = port_num_func;
        pre_port_num_pull = port_num_pull;
        //載陔髡夔敵湔
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        tmp_group_func_data &= ~(0x07 << tmp_val);
        if(set_gpio)
        {
        	tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
        }
        //跦擂pull腔硉樵隅岆瘁載陔pull敵湔
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if(tmp_user_gpio_data->pull >= 0)
        {
        	tmp_group_pull_data &= ~(                           0x03  << tmp_val);
        	tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //跦擂driver level腔硉樵隅岆瘁載陔driver level敵湔
        if(tmp_user_gpio_data->drv_level >= 0)
        {
        	tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
        	tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
        }
        //跦擂蚚誧怀ㄛ眕摯髡夔煦饜樵隅岆瘁載陔data敵湔
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
            	tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }

        break;
	}
	//潰脤岆瘁衄杅擂湔婓
	if(first_port >= group_count_max)
	{
	    return -1;
	}
	//悵湔蚚誧杅擂
	for(i = first_port + 1; i < group_count_max; i++)
	{
		tmp_user_gpio_data = gpio_list + i;                 //gpio_set甡棒硌砃蚚誧腔藩跺GPIO杅郪傖埜
	    port     = tmp_user_gpio_data->port;                //黍堤傷諳杅硉
	    port_num = tmp_user_gpio_data->port_num;            //黍堤傷諳笢腔議珨跺GPIO
	    if(!port)
	    {
	    	break;
	    }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);     //隙迡髡夔敵湔
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);     //隙迡pull敵湔
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //隙迡driver level敵湔
            if(data_change)
            {
                data_change = 0;
                GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //隙迡data敵湔
            }

        	tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
        	tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
        	tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
        	tmp_group_data_addr    = PIO_REG_DATA(port);                 //載陔data敵湔

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
            tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
            tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
            tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
        }
        else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //寀硐隙迡髡夔敵湔
            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        }
		//悵湔絞茞璃敵湔杅擂
        pre_port_num_pull = port_num_pull;                      //扢离絞GPIO傖峈珨跺GPIO
        pre_port_num_func = port_num_func;
        pre_port          = port;

        //載陔髡夔敵湔
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        if(tmp_user_gpio_data->mul_sel >= 0)
        {
        	tmp_group_func_data &= ~(                              0x07  << tmp_val);
        	if(set_gpio)
        	{
        		tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
        	}
        }
        //跦擂pull腔硉樵隅岆瘁載陔pull敵湔
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if(tmp_user_gpio_data->pull >= 0)
        {
        	tmp_group_pull_data &= ~(                           0x03  << tmp_val);
        	tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //跦擂driver level腔硉樵隅岆瘁載陔driver level敵湔
        if(tmp_user_gpio_data->drv_level >= 0)
        {
        	tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
        	tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
        }
        //跦擂蚚誧怀ㄛ眕摯髡夔煦饜樵隅岆瘁載陔data敵湔
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
            	tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }
    }
    //for悜遠賦旰ㄛ彆湔婓遜羶衄隙迡腔敵湔ㄛ涴爵迡隙善茞璃絞笢
    if(tmp_group_func_addr)                         //硐猁載陔徹敵湔華硊ㄛ憩褫眕勤茞璃董硉
    {                                               //饒繫參垀衄腔硉窒隙迡善茞璃敵湔
        GPIO_REG_WRITE(tmp_group_func_addr,   tmp_group_func_data);   //隙迡髡夔敵湔
        GPIO_REG_WRITE(tmp_group_pull_addr,   tmp_group_pull_data);   //隙迡pull敵湔
        GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //隙迡driver level敵湔
        if(data_change)
        {
            GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //隙迡data敵湔
        }
    }

    return 0;
}

#endif



/*
************************************************************************************************************
*
*                                             CSP_GPIO_Request
*
*    滲杅靡備ㄩ
*
*    統杅蹈桶ㄩgpio_list      湔溫垀衄蚚善腔GPIO杅擂腔杅郪ㄛGPIO蔚眻諉妏蚚涴跺杅郪
*
*               group_count_max  杅郪腔傖埜跺杅ㄛGPIO扢隅腔奀緊ㄛ蔚紱釬腔GPIO郔湮祥閉徹涴跺硉
*
*    殿隙硉  ㄩ
*
*    佽隴    ㄩ婃奀羶衄酕喳芼潰脤
*
*
************************************************************************************************************
*/

#ifdef SUNXI_R_PIO_BASE   //盓厥CPUS GPIO雄

u32 gpio_request(user_gpio_set_t *gpio_list, __u32 group_count_max)
{
    char               *user_gpio_buf;                                        //偌桽char濬倰扠
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;                      //user_gpio_set蔚岆扠囀湔腔曆梟
    user_gpio_set_t  *tmp_user_gpio_data;
    __u32                real_gpio_count = 0, first_port;                      //悵湔淩淏衄虴腔GPIO腔跺杅
    __u32               tmp_group_func_data = 0;
    __u32               tmp_group_pull_data = 0;
    __u32               tmp_group_dlevel_data = 0;
    __u32               tmp_group_data_data = 0;
    __u32               func_change = 0, pull_change = 0;
    __u32               dlevel_change = 0, data_change = 0;
    volatile __u32  *tmp_group_func_addr = NULL, *tmp_group_pull_addr = NULL;
    volatile __u32  *tmp_group_dlevel_addr = NULL, *tmp_group_data_addr = NULL;
    __u32  port, port_num, pre_port_num, port_num_func, port_num_pull;
    __u32  pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff;
    __u32  pre_port_num_pull = 0x7fffffff;
    __s32  i, tmp_val;
    int cpus_flag = 0;
    if((!gpio_list) || (!group_count_max))
    {
        return (u32)0;
    }
    for(i = 0; i < group_count_max; i++)
    {
        tmp_user_gpio_data = gpio_list + i;                 //gpio_set甡棒硌砃藩跺GPIO杅郪傖埜
        if(!tmp_user_gpio_data->port)
        {
            continue;
        }
        real_gpio_count ++;
    }

    //SYSCONFIG_DEBUG("to malloc space for pin \n");
    user_gpio_buf = (char *)malloc(16 + sizeof(system_gpio_set_t) * real_gpio_count);   //扠囀湔ㄛ嗣扠16跺趼誹ㄛ蚚衾湔溫GPIO跺杅脹陓洘
    if(!user_gpio_buf)
    {
        return (u32)0;
    }
    memset(user_gpio_buf, 0, 16 + sizeof(system_gpio_set_t) * real_gpio_count);         //忑珂窒錨
    *(int *)user_gpio_buf = real_gpio_count;                                           //悵湔衄虴腔GPIO跺杅
    user_gpio_set = (system_gpio_set_t *)(user_gpio_buf + 16);                         //硌砃菴珨跺賦凳极
    //袧掘菴珨跺GPIO杅擂
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;                         //黍堤傷諳杅硉
        port_num = tmp_user_gpio_data->port_num;                     //黍堤傷諳笢腔議珨跺GPIO
        pre_port_num = port_num;
        if(!port)
        {
            continue;
        }
		port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if(port == 0xffff)
        {
        	tmp_group_data_data = tmp_user_gpio_data->data & 1;
        }
        else
        {
        	if(port >= 12)
            	cpus_flag = 1;
			else
				cpus_flag = 0;

	        if(!cpus_flag)
	        {
	            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
	            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
	            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
	            tmp_group_data_addr    = PIO_REG_DATA(port);                 //載陔data敵湔
	        }
	        else
	        {
	            tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
	            tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
	            tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
	            tmp_group_data_addr    = R_PIO_REG_DATA(port);                 //載陔data敵湔
	        }
	        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
	        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
	        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
	        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
        }
        break;
    }
    if(first_port >= group_count_max)
    {
        return 0;
    }
    //悵湔蚚誧杅擂
    for(i = first_port; i < group_count_max; i++)
    {
        tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        tmp_user_gpio_data = gpio_list + i;                 //gpio_set甡棒硌砃蚚誧腔藩跺GPIO杅郪傖埜
        port     = tmp_user_gpio_data->port;                //黍堤傷諳杅硉
        port_num = tmp_user_gpio_data->port_num;            //黍堤傷諳笢腔議珨跺GPIO
        if(!port)
        {
            continue;
        }

        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

		if(port >= 12)
        {
            cpus_flag = 1;
        }
		else
		{
			cpus_flag = 0;
		}

        //羲宎悵湔蚚誧杅擂
        strcpy(tmp_sys_gpio_data->gpio_name, tmp_user_gpio_data->gpio_name);
        tmp_sys_gpio_data->port                       = port;
        tmp_sys_gpio_data->port_num                   = port_num;
        tmp_sys_gpio_data->user_gpio_status.mul_sel   = tmp_user_gpio_data->mul_sel;
        tmp_sys_gpio_data->user_gpio_status.pull      = tmp_user_gpio_data->pull;
        tmp_sys_gpio_data->user_gpio_status.drv_level = tmp_user_gpio_data->drv_level;
        tmp_sys_gpio_data->user_gpio_status.data      = tmp_user_gpio_data->data;

        if((port_num_pull != pre_port_num_pull) || (port != pre_port) || (pre_port == 0xffff))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
        {
            if(pre_port == 0xffff)
            {
            	if(data_change)
            	{
            		data_change = 0;
            		gpio_set_axpgpio_value(0, pre_port_num, tmp_group_data_data);
            	}
            }
            else
            {
            	if(func_change)
	            {
	                GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //隙迡髡夔敵湔
	                func_change = 0;
	            }
	            if(pull_change)
	            {
	                pull_change = 0;
	                GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);    //隙迡pull敵湔
	            }
	            if(dlevel_change)
	            {
	                dlevel_change = 0;
	                GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);  //隙迡driver level敵湔
	            }
	            if(data_change)
	            {
	                data_change = 0;
	                GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data);    //隙迡
	            }
	        }

	        if(port == 0xffff)
	        {
	        	tmp_group_data_data = tmp_user_gpio_data->data;
	        	data_change = 1;
	        }
	        else
	        {
	        	if(!cpus_flag)
	            {
	                tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
	                tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
	                tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
	                tmp_group_data_addr    = PIO_REG_DATA(port);                  //載陔data敵湔
	            }
	            else
	            {

	                tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
	                tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔??
	                tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
	                tmp_group_data_addr    = R_PIO_REG_DATA(port);                  //載陔data敵湔
	            }
	            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
		        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
		        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
		        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
	        }
        }
        else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
        {
            if(pre_port == 0xffff)
            {
            	gpio_set_axpgpio_value(0, pre_port_num, tmp_group_data_data);
            }
            else
            {
				GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //寀硐隙迡髡夔敵湔
            }

            if(port == 0xffff)
            {
            	tmp_group_data_data = tmp_user_gpio_data->data;
	        	data_change = 1;
            }
            else
            {
            	if(!cpus_flag)
	            {
	                tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
	            }
	            else
	            {
	                tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
	            }
	            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
            }
        }
        //悵湔絞茞璃敵湔杅擂
        pre_port_num_pull = port_num_pull;                      //扢离絞GPIO傖峈珨跺GPIO
        pre_port_num_func = port_num_func;
        pre_port          = port;
		pre_port_num	  = port_num;

        if(port == 0xffff)
        {
        	if(tmp_user_gpio_data->data >= 0)
            {
                tmp_group_data_data = tmp_user_gpio_data->data;
	        	data_change = 1;
            }
        }
        else
        {
        	//載陔髡夔敵湔
	        if(tmp_user_gpio_data->mul_sel >= 0)
	        {
	            tmp_val = (port_num - (port_num_func<<3)) << 2;
	            tmp_sys_gpio_data->hardware_gpio_status.mul_sel = (tmp_group_func_data >> tmp_val) & 0x07;
	            tmp_group_func_data &= ~(                              0x07  << tmp_val);
	            tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
	            func_change = 1;
	        }
	        //跦擂pull腔硉樵隅岆瘁載陔pull敵湔

	        tmp_val = (port_num - (port_num_pull<<4)) << 1;

	        if(tmp_user_gpio_data->pull >= 0)
	        {
	            tmp_sys_gpio_data->hardware_gpio_status.pull = (tmp_group_pull_data >> tmp_val) & 0x03;
	            if(tmp_user_gpio_data->pull >= 0)
	            {
	                tmp_group_pull_data &= ~(                           0x03  << tmp_val);
	                tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
	                pull_change = 1;
	            }
	        }
	        //跦擂driver level腔硉樵隅岆瘁載陔driver level敵湔
	        if(tmp_user_gpio_data->drv_level >= 0)
	        {
	            tmp_sys_gpio_data->hardware_gpio_status.drv_level = (tmp_group_dlevel_data >> tmp_val) & 0x03;
	            if(tmp_user_gpio_data->drv_level >= 0)
	            {
	                tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
	                tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
	                dlevel_change = 1;
	            }
	        }
	        //跦擂蚚誧怀ㄛ眕摯髡夔煦饜樵隅岆瘁載陔data敵湔
	        if(tmp_user_gpio_data->mul_sel == 1)
	        {
	            if(tmp_user_gpio_data->data >= 0)
	            {
	                tmp_val = tmp_user_gpio_data->data;
	                tmp_val &= 1;
	                tmp_group_data_data &= ~(1 << port_num);
	                tmp_group_data_data |= tmp_val << port_num;
	                data_change = 1;
	            }
	        }
        }
    }

    //for悜遠賦旰ㄛ彆湔婓遜羶衄隙迡腔敵湔ㄛ涴爵迡隙善茞璃絞笢
    if(port == 0xffff)
	{
		if(data_change)
		{
			gpio_set_axpgpio_value(0, port_num, tmp_group_data_data);
		}
	}
    else
    {
		if(tmp_group_func_addr)                         //硐猁載陔徹敵湔華硊ㄛ憩褫眕勤茞璃董硉
	    {                                               //饒繫參垀衄腔硉窒隙迡善茞璃敵湔
	        GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);       //隙迡髡夔敵湔
	        if(pull_change)
	        {
	            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);    //隙迡pull敵湔
	        }
	        if(dlevel_change)
	        {
	            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);  //隙迡driver level敵湔
	        }
	        if(data_change)
	        {
	            GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data);    //隙迡data敵湔
	        }
	    }
    }
    return (u32)user_gpio_buf;
}

#else   //盓厥羶衄CPUS怢腔gpio雄

u32 gpio_request(user_gpio_set_t *gpio_list, __u32 group_count_max)
{
    char               *user_gpio_buf;                                        //偌桽char濬倰扠
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;                      //user_gpio_set蔚岆扠囀湔腔曆梟
    user_gpio_set_t  *tmp_user_gpio_data;
    __u32                real_gpio_count = 0, first_port;                      //悵湔淩淏衄虴腔GPIO腔跺杅
    __u32               tmp_group_func_data = 0;
    __u32               tmp_group_pull_data = 0;
    __u32               tmp_group_dlevel_data = 0;
    __u32               tmp_group_data_data = 0;
    __u32               func_change = 0, pull_change = 0;
    __u32               dlevel_change = 0, data_change = 0;
    volatile __u32  *tmp_group_func_addr = NULL, *tmp_group_pull_addr = NULL;
    volatile __u32  *tmp_group_dlevel_addr = NULL, *tmp_group_data_addr = NULL;
    __u32  port, port_num, port_num_func, port_num_pull;
    __u32  pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff;
    __u32  pre_port_num_pull = 0x7fffffff;
    __s32  i, tmp_val;
    if((!gpio_list) || (!group_count_max))
    {
        return (u32)0;
    }
    for(i = 0; i < group_count_max; i++)
    {
        tmp_user_gpio_data = gpio_list + i;                 //gpio_set甡棒硌砃藩跺GPIO杅郪傖埜
        if(!tmp_user_gpio_data->port)
        {
            continue;
        }
        real_gpio_count ++;
    }

    //SYSCONFIG_DEBUG("to malloc space for pin \n");
    user_gpio_buf = (char *)malloc(16 + sizeof(system_gpio_set_t) * real_gpio_count);   //扠囀湔ㄛ嗣扠16跺趼誹ㄛ蚚衾湔溫GPIO跺杅脹陓洘
    if(!user_gpio_buf)
    {
        return (u32)0;
    }
    memset(user_gpio_buf, 0, 16 + sizeof(system_gpio_set_t) * real_gpio_count);         //忑珂窒錨
    *(int *)user_gpio_buf = real_gpio_count;                                           //悵湔衄虴腔GPIO跺杅
    user_gpio_set = (system_gpio_set_t *)(user_gpio_buf + 16);                         //硌砃菴珨跺賦凳极
    //袧掘菴珨跺GPIO杅擂
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port     = tmp_user_gpio_data->port;                         //黍堤傷諳杅硉
        port_num = tmp_user_gpio_data->port_num;                     //黍堤傷諳笢腔議珨跺GPIO
        if(!port)
        {
            continue;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
        tmp_group_data_addr    = PIO_REG_DATA(port);                 //載陔data敵湔

        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);
        break;
    }
    if(first_port >= group_count_max)
    {
        return 0;
    }
    //悵湔蚚誧杅擂
    for(i = first_port; i < group_count_max; i++)
    {
        tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        tmp_user_gpio_data = gpio_list + i;                 //gpio_set甡棒硌砃蚚誧腔藩跺GPIO杅郪傖埜
        port     = tmp_user_gpio_data->port;                //黍堤傷諳杅硉
        port_num = tmp_user_gpio_data->port_num;            //黍堤傷諳笢腔議珨跺GPIO
        if(!port)
        {
            continue;
        }
        //羲宎悵湔蚚誧杅擂
        strcpy(tmp_sys_gpio_data->gpio_name, tmp_user_gpio_data->gpio_name);
        tmp_sys_gpio_data->port                       = port;
        tmp_sys_gpio_data->port_num                   = port_num;
        tmp_sys_gpio_data->user_gpio_status.mul_sel   = tmp_user_gpio_data->mul_sel;
        tmp_sys_gpio_data->user_gpio_status.pull      = tmp_user_gpio_data->pull;
        tmp_sys_gpio_data->user_gpio_status.drv_level = tmp_user_gpio_data->drv_level;
        tmp_sys_gpio_data->user_gpio_status.data      = tmp_user_gpio_data->data;

        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
        {
            if(func_change)
            {
                GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //隙迡髡夔敵湔
                func_change = 0;
            }
            if(pull_change)
            {
                pull_change = 0;
                GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);    //隙迡pull敵湔
            }
            if(dlevel_change)
            {
                dlevel_change = 0;
                GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);  //隙迡driver level敵湔
            }
            if(data_change)
            {
                data_change = 0;
                GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data);    //隙迡
            }

            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
            tmp_group_data_addr    = PIO_REG_DATA(port);                  //載陔data敵湔

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
	        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
	        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
	        tmp_group_data_data    = GPIO_REG_READ(tmp_group_data_addr);

        }
        else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //寀硐隙迡髡夔敵湔

           tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        }
        //悵湔絞茞璃敵湔杅擂
        pre_port_num_pull = port_num_pull;                      //扢离絞GPIO傖峈珨跺GPIO
        pre_port_num_func = port_num_func;
        pre_port          = port;

        //載陔髡夔敵湔
        if(tmp_user_gpio_data->mul_sel >= 0)
        {
            tmp_val = (port_num - (port_num_func<<3)) << 2;
            tmp_sys_gpio_data->hardware_gpio_status.mul_sel = (tmp_group_func_data >> tmp_val) & 0x07;
            tmp_group_func_data &= ~(                              0x07  << tmp_val);
            tmp_group_func_data |=  (tmp_user_gpio_data->mul_sel & 0x07) << tmp_val;
            func_change = 1;
        }
        //跦擂pull腔硉樵隅岆瘁載陔pull敵湔

        tmp_val = (port_num - (port_num_pull<<4)) << 1;

        if(tmp_user_gpio_data->pull >= 0)
        {
            tmp_sys_gpio_data->hardware_gpio_status.pull = (tmp_group_pull_data >> tmp_val) & 0x03;
            if(tmp_user_gpio_data->pull >= 0)
            {
                tmp_group_pull_data &= ~(                           0x03  << tmp_val);
                tmp_group_pull_data |=  (tmp_user_gpio_data->pull & 0x03) << tmp_val;
                pull_change = 1;
            }
        }
        //跦擂driver level腔硉樵隅岆瘁載陔driver level敵湔
        if(tmp_user_gpio_data->drv_level >= 0)
        {
            tmp_sys_gpio_data->hardware_gpio_status.drv_level = (tmp_group_dlevel_data >> tmp_val) & 0x03;
            if(tmp_user_gpio_data->drv_level >= 0)
            {
                tmp_group_dlevel_data &= ~(                                0x03  << tmp_val);
                tmp_group_dlevel_data |=  (tmp_user_gpio_data->drv_level & 0x03) << tmp_val;
                dlevel_change = 1;
            }
        }
        //跦擂蚚誧怀ㄛ眕摯髡夔煦饜樵隅岆瘁載陔data敵湔
        if(tmp_user_gpio_data->mul_sel == 1)
        {
            if(tmp_user_gpio_data->data >= 0)
            {
                tmp_val = tmp_user_gpio_data->data;
                tmp_val &= 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }
    }

    //for悜遠賦旰ㄛ彆湔婓遜羶衄隙迡腔敵湔ㄛ涴爵迡隙善茞璃絞笢
    if(tmp_group_func_addr)                         //硐猁載陔徹敵湔華硊ㄛ憩褫眕勤茞璃董硉
    {                                               //饒繫參垀衄腔硉窒隙迡善茞璃敵湔
        GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);       //隙迡髡夔敵湔
        if(pull_change)
        {
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);    //隙迡pull敵湔
        }
        if(dlevel_change)
        {
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);  //隙迡driver level敵湔
        }
        if(data_change)
        {
            GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data);    //隙迡data敵湔
        }
    }
    return (u32)user_gpio_buf;
}
#endif
/*
************************************************************************************************************
*
*                                             gpio_request_ex
*
*    滲杅靡備ㄩ
*
*    統杅佽隴: main_name   換輛腔翋瑩靡備ㄛ饜耀輸(雄靡備)
*
*               sub_name    換輛腔赽瑩靡備ㄛ彆岆諾ㄛ桶尨窒ㄛ瘁寀扆梑善饜腔等黃GPIO
*
*    殿隙硉  ㄩ0 :    err
*              other: success
*
*    佽隴    ㄩ婃奀羶衄酕喳芼潰脤
*
*
************************************************************************************************************
*/
u32 gpio_request_ex(char *main_name, const char *sub_name)  //扢掘扠GPIO滲杅孺桯諉諳
{
    user_gpio_set_t    *gpio_list=NULL;
    user_gpio_set_t     one_gpio;
       __u32               gpio_handle;
    __s32               gpio_count;

    if(!sub_name){
            gpio_count = script_parser_mainkey_get_gpio_count(main_name);
            if(gpio_count <= 0)
            {
                /*printf("err: gpio count < =0 ,gpio_count is: %d \n", gpio_count);*/
                return 0;
            }
            gpio_list = (user_gpio_set_t *)malloc(sizeof(system_gpio_set_t) * gpio_count); //扠珨還奀囀湔ㄛ蚚衾悵湔蚚誧杅擂
            if(!gpio_list){
         /*   printf("malloc gpio_list error \n");*/
                return 0;
            }
        if(!script_parser_mainkey_get_gpio_cfg(main_name,gpio_list,gpio_count)){
            gpio_handle = gpio_request(gpio_list, gpio_count);
            free(gpio_list);

        }else{
            return 0;
        }
        }else{
            if(script_parser_fetch((char *)main_name, (char *)sub_name, (int *)&one_gpio, (sizeof(user_gpio_set_t) >> 2)) < 0){
           /* printf("script parser fetch err. \n");*/
            return 0;
            }

            gpio_handle = gpio_request(&one_gpio, 1);
        }

        return gpio_handle;
}
/*
************************************************************************************************************
*
*                                             gpio_request_simple
*
*    滲杅靡備ㄩ
*
*    統杅佽隴: main_name   換輛腔翋瑩靡備ㄛ饜耀輸(雄靡備)
*
*               sub_name    換輛腔赽瑩靡備ㄛ彆岆諾ㄛ桶尨窒ㄛ瘁寀扆梑善饜腔等黃GPIO
*
*    殿隙硉  ㄩ0 :    err
*              other: success
*
*    佽隴    ㄩ婃奀羶衄酕喳芼潰脤
*
*
************************************************************************************************************
*/
int gpio_request_simple(char *main_name, const char *sub_name)  //扢掘扠GPIO滲杅孺桯諉諳
{
    user_gpio_set_t     gpio_list[16];
    __s32               gpio_count;
    int ret = -1;

    if(!sub_name)
    {
        gpio_count = script_parser_mainkey_get_gpio_count(main_name);
        if(gpio_count <= 0)
        {
            printf("err: gpio count < =0 ,gpio_count is: %d \n", gpio_count);
            return -1;
        }
        memset(gpio_list, 0, 16 * sizeof(user_gpio_set_t));
        if(!script_parser_mainkey_get_gpio_cfg(main_name, gpio_list, gpio_count))
        {
            ret = gpio_request_early(gpio_list, gpio_count, 1);
        }
    }
    else
    {
        if(script_parser_fetch((char *)main_name, (char *)sub_name, (int *)gpio_list, (sizeof(user_gpio_set_t) >> 2)) < 0)
        {
       		printf("script parser fetch err. \n");
        	return 0;
        }

        ret = gpio_request_early(gpio_list, 1, 1);
    }

    return ret;
}

/*
****************************************************************************************************
*
*             CSP_PIN_DEV_release
*
*  Description:
*       庋溫議軀憮扢掘腔pin
*
*  Parameters:
*         p_handler    :    handler
*       if_release_to_default_status : 岆瘁庋溫善埻宎袨怓(敵湔埻衄袨怓)
*
*  Return value:
*        EGPIO_SUCCESS/EGPIO_FAIL
****************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32 gpio_release(u32 p_handler, __s32 if_release_to_default_status)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max, first_port;                    //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    __u32               tmp_group_func_data = 0;
    __u32               tmp_group_pull_data = 0;
    __u32               tmp_group_dlevel_data = 0;
    volatile __u32     *tmp_group_func_addr = NULL,   *tmp_group_pull_addr = NULL;
    volatile __u32     *tmp_group_dlevel_addr = NULL;
    __u32               port, port_num, port_num_pull, port_num_func;
    __u32               pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff, pre_port_num_pull = 0x7fffffff;
    __u32               i, tmp_val;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(!group_count_max)
    {
        return EGPIO_FAIL;
    }
    if(if_release_to_default_status == 2)
    {
        //SYSCONFIG_DEBUG("gpio module :  release p_handler = %x\n",p_handler);
        free((char *)p_handler);

        return EGPIO_SUCCESS;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //黍蚚誧杅擂
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_sys_gpio_data  = user_gpio_set + first_port;
        port     = tmp_sys_gpio_data->port;                 //黍堤傷諳杅硉
        port_num = tmp_sys_gpio_data->port_num;             //黍堤傷諳笢腔議珨跺GPIO
        if(!port)
		{
            continue;
		}
		if(port>=12)
			cpus_flag = 1;
		else
			cpus_flag = 0;
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);
		if(!cpus_flag)
		{
			tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
			tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
			tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
		}
		else
		{
			tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
			tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
			tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔
		}
        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        break;
    }
    if(first_port >= group_count_max)
    {
        return 0;
    }
    for(i = first_port; i < group_count_max; i++)
    {
        tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        port     = tmp_sys_gpio_data->port;                 //黍堤傷諳杅硉
        port_num = tmp_sys_gpio_data->port_num;             //黍堤傷諳笢腔議珨跺GPIO
		if(port >= 12)
			cpus_flag = 1;
		else
			cpus_flag = 0;
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //隙迡髡夔敵湔
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);    //隙迡pull敵湔
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);  //隙迡driver level敵湔

			if(!cpus_flag)
			{
				tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
				tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
			}
			else
			{
				tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
				tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
			}

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
            tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
            tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        }
        else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);                 //寀硐隙迡髡夔敵湔
			if(!cpus_flag)
				tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
			else
				tmp_group_func_addr    = R_PIO_REG_CFG(port , port_num_func);
            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        }

        pre_port_num_pull = port_num_pull;
        pre_port_num_func = port_num_func;
        pre_port          = port;
        //載陔髡夔敵湔
        tmp_group_func_data &= ~(0x07 << ((port_num - (port_num_func<<3)) << 2));
        //載陔pull袨怓敵湔
        tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
        tmp_group_pull_data &= ~(0x03  << tmp_val);
        tmp_group_pull_data |= (tmp_sys_gpio_data->hardware_gpio_status.pull & 0x03) << tmp_val;
        //載陔driver袨怓敵湔
        tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
        tmp_group_dlevel_data &= ~(0x03  << tmp_val);
        tmp_group_dlevel_data |= (tmp_sys_gpio_data->hardware_gpio_status.drv_level & 0x03) << tmp_val;
    }
    if(tmp_group_func_addr)                              //硐猁載陔徹敵湔華硊ㄛ憩褫眕勤茞璃董硉
    {                                                    //饒繫參垀衄腔硉窒隙迡善茞璃敵湔
        GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //隙迡髡夔敵湔
    }
    if(tmp_group_pull_addr)
    {
        GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);
    }
    if(tmp_group_dlevel_addr)
    {
        GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);
    }

    free((char *)p_handler);

    return EGPIO_SUCCESS;
}
#else
__s32 gpio_release(u32 p_handler, __s32 if_release_to_default_status)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max, first_port;                    //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    __u32               tmp_group_func_data = 0;
    __u32               tmp_group_pull_data = 0;
    __u32               tmp_group_dlevel_data = 0;
    volatile __u32     *tmp_group_func_addr = NULL,   *tmp_group_pull_addr = NULL;
    volatile __u32     *tmp_group_dlevel_addr = NULL;
    __u32               port, port_num, port_num_pull, port_num_func;
    __u32               pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff, pre_port_num_pull = 0x7fffffff;
    __u32               i, tmp_val;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(!group_count_max)
    {
        return EGPIO_FAIL;
    }
    if(if_release_to_default_status == 2)
    {
        //SYSCONFIG_DEBUG("gpio module :  release p_handler = %x\n",p_handler);
        free((char *)p_handler);

        return EGPIO_SUCCESS;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //黍蚚誧杅擂
    for(first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_sys_gpio_data  = user_gpio_set + first_port;
        port     = tmp_sys_gpio_data->port;                 //黍堤傷諳杅硉
        port_num = tmp_sys_gpio_data->port_num;             //黍堤傷諳笢腔議珨跺GPIO
        if(!port)
        {
            continue;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
        tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);  //載陔pull敵湔
        tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull);//載陔level敵湔

        tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        break;
    }
    if(first_port >= group_count_max)
    {
        return 0;
    }
    for(i = first_port; i < group_count_max; i++)
    {
        tmp_sys_gpio_data  = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        port     = tmp_sys_gpio_data->port;                 //黍堤傷諳杅硉
        port_num = tmp_sys_gpio_data->port_num;             //黍堤傷諳笢腔議珨跺GPIO

        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //隙迡髡夔敵湔
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);    //隙迡pull敵湔
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);  //隙迡driver level敵湔

            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔

            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
            tmp_group_pull_data    = GPIO_REG_READ(tmp_group_pull_addr);
            tmp_group_dlevel_data  = GPIO_REG_READ(tmp_group_dlevel_addr);
        }
        else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);                 //寀硐隙迡髡夔敵湔
            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
            tmp_group_func_data    = GPIO_REG_READ(tmp_group_func_addr);
        }

        pre_port_num_pull = port_num_pull;
        pre_port_num_func = port_num_func;
        pre_port          = port;
        //載陔髡夔敵湔
        tmp_group_func_data &= ~(0x07 << ((port_num - (port_num_func<<3)) << 2));
        //載陔pull袨怓敵湔
        tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
        tmp_group_pull_data &= ~(0x03  << tmp_val);
        tmp_group_pull_data |= (tmp_sys_gpio_data->hardware_gpio_status.pull & 0x03) << tmp_val;
        //載陔driver袨怓敵湔
        tmp_val              =  (port_num - (port_num_pull<<4)) << 1;
        tmp_group_dlevel_data &= ~(0x03  << tmp_val);
        tmp_group_dlevel_data |= (tmp_sys_gpio_data->hardware_gpio_status.drv_level & 0x03) << tmp_val;
    }
    if(tmp_group_func_addr)                              //硐猁載陔徹敵湔華硊ㄛ憩褫眕勤茞璃董硉
    {                                                    //饒繫參垀衄腔硉窒隙迡善茞璃敵湔
        GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);    //隙迡髡夔敵湔
    }
    if(tmp_group_pull_addr)
    {
        GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);
    }
    if(tmp_group_dlevel_addr)
    {
        GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data);
    }

    free((char *)p_handler);

    return EGPIO_SUCCESS;
}
#endif
/*
**********************************************************************************************************************
*                                               CSP_PIN_Get_All_Gpio_Status
*
* Description:
*                鳳蚚誧扠徹腔垀衄GPIO腔袨怓
* Arguments  :
*        p_handler    :    handler
*        gpio_status    :    悵湔蚚誧杅擂腔杅郪
*        gpio_count_max    :    杅郪郔湮跺杅ㄛ旌轎杅郪埣賜
*       if_get_user_set_flag   :   黍梓祩ㄛ桶尨黍蚚誧扢隅杅擂麼氪岆妗暱杅擂
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_get_all_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, __u32 gpio_count_max, __u32 if_get_from_hardware)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max, first_port;                    //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    user_gpio_set_t  *script_gpio;
    __u32               port_num_func, port_num_pull;
    volatile __u32     *tmp_group_func_addr = NULL, *tmp_group_pull_addr;
    volatile __u32     *tmp_group_data_addr, *tmp_group_dlevel_addr;
    __u32               port, port_num;
    __u32               pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff, pre_port_num_pull = 0x7fffffff;
    __u32               i;
	int cpus_flag = 0;
    if((!p_handler) || (!gpio_status))
    {
        return EGPIO_FAIL;
    }
    if(gpio_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(group_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    if(group_count_max > gpio_count_max)
    {
        group_count_max = gpio_count_max;
    }
    //黍蚚誧杅擂
    //桶尨黍蚚誧跤隅腔杅擂
    if(!if_get_from_hardware)
    {
        for(i = 0; i < group_count_max; i++)
        {
            tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data?裒藙糒賮鬍PIO諾潔
            script_gpio       = gpio_status + i;               //script_gpio硌砃蚚誧換輛腔諾潔

            script_gpio->port      = tmp_sys_gpio_data->port;                       //黍堤port杅擂
            script_gpio->port_num  = tmp_sys_gpio_data->port_num;                   //黍堤port_num杅擂
            script_gpio->pull      = tmp_sys_gpio_data->user_gpio_status.pull;      //黍堤pull杅擂
            script_gpio->mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;   //黍堤髡夔杅擂
            script_gpio->drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level; //黍堤雄夔薯杅擂
            script_gpio->data      = tmp_sys_gpio_data->user_gpio_status.data;      //黍堤data杅擂
            strcpy(script_gpio->gpio_name, tmp_sys_gpio_data->gpio_name);
        }
    }
    else
    {
        for(first_port = 0; first_port < group_count_max; first_port++)
        {
            tmp_sys_gpio_data  = user_gpio_set + first_port;
            port     = tmp_sys_gpio_data->port;               //黍堤傷諳杅硉
            port_num = tmp_sys_gpio_data->port_num;           //黍堤傷諳笢腔議珨跺GPIO

            if(!port)
            {
                continue;
            }
			if(port >= 12)
				cpus_flag = 1;
			else
				cpus_flag = 0;
            port_num_func = (port_num >> 3);
            port_num_pull = (port_num >> 4);
			if(!cpus_flag)
			{
				tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
				tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
				tmp_group_data_addr    = PIO_REG_DATA(port);                  //載陔data敵湔
            }
			else
			{
				tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
				tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
				tmp_group_data_addr    = R_PIO_REG_DATA(port);                  //載陔data敵湔
			}
			break;
        }
        if(first_port >= group_count_max)
        {
            return 0;
        }
        for(i = first_port; i < group_count_max; i++)
        {
            tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
            script_gpio       = gpio_status + i;               //script_gpio硌砃蚚誧換輛腔諾潔

            port     = tmp_sys_gpio_data->port;                //黍堤傷諳杅硉
            port_num = tmp_sys_gpio_data->port_num;            //黍堤傷諳笢腔議珨跺GPIO

            script_gpio->port = port;                          //黍堤port杅擂
            script_gpio->port_num  = port_num;                 //黍堤port_num杅擂
            strcpy(script_gpio->gpio_name, tmp_sys_gpio_data->gpio_name);

            port_num_func = (port_num >> 3);
            port_num_pull = (port_num >> 4);

            if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
            {
				if(!cpus_flag)
				{
					tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
					tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
					tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
					tmp_group_data_addr    = PIO_REG_DATA(port);                  //載陔data敵湔
				}
				else
				{
					tmp_group_func_addr    = R_PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
					tmp_group_pull_addr    = R_PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
					tmp_group_dlevel_addr  = R_PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
					tmp_group_data_addr    = R_PIO_REG_DATA(port);                  //載陔data敵湔
				}
            }
            else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
            {
				if(!cpus_flag)
					tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
				else
					tmp_group_func_addr = R_PIO_REG_CFG(port,port_num_func);
            }

            pre_port_num_pull = port_num_pull;
            pre_port_num_func = port_num_func;
            pre_port          = port;
            //跤蚚誧諷璃董硉
            script_gpio->pull      = (GPIO_REG_READ(tmp_group_pull_addr)   >> ((port_num - (port_num_pull<<4))<<1)) & 0x03;    //黍堤pull杅擂
            script_gpio->drv_level = (GPIO_REG_READ(tmp_group_dlevel_addr) >> ((port_num - (port_num_pull<<4))<<1)) & 0x03;    //黍堤髡夔杅擂
            script_gpio->mul_sel   = (GPIO_REG_READ(tmp_group_func_addr)   >> ((port_num - (port_num_func<<3))<<2)) & 0x07;    //黍堤髡夔杅擂
            if(script_gpio->mul_sel <= 1)
            {
                script_gpio->data  = (GPIO_REG_READ(tmp_group_data_addr)   >>   port_num) & 0x01;                              //黍堤data杅擂
            }
            else
            {
                script_gpio->data = -1;
            }
        }
    }

    return EGPIO_SUCCESS;
}
#else
__s32  gpio_get_all_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, __u32 gpio_count_max, __u32 if_get_from_hardware)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max, first_port;                    //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    user_gpio_set_t  *script_gpio;
    __u32               port_num_func, port_num_pull;
    volatile __u32     *tmp_group_func_addr = NULL, *tmp_group_pull_addr;
    volatile __u32     *tmp_group_data_addr, *tmp_group_dlevel_addr;
    __u32               port, port_num;
    __u32               pre_port = 0x7fffffff, pre_port_num_func = 0x7fffffff, pre_port_num_pull = 0x7fffffff;
    __u32               i;

    if((!p_handler) || (!gpio_status))
    {
        return EGPIO_FAIL;
    }
    if(gpio_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(group_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    if(group_count_max > gpio_count_max)
    {
        group_count_max = gpio_count_max;
    }
    //黍蚚誧杅擂
    //桶尨黍蚚誧跤隅腔杅擂
    if(!if_get_from_hardware)
    {
        for(i = 0; i < group_count_max; i++)
        {
            tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
            script_gpio       = gpio_status + i;               //script_gpio硌砃蚚誧換輛腔諾潔

            script_gpio->port      = tmp_sys_gpio_data->port;                       //黍堤port杅擂
            script_gpio->port_num  = tmp_sys_gpio_data->port_num;                   //黍堤port_num杅擂
            script_gpio->pull      = tmp_sys_gpio_data->user_gpio_status.pull;      //黍堤pull杅擂
            script_gpio->mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;   //黍堤髡夔杅擂
            script_gpio->drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level; //黍堤雄夔薯杅擂
            script_gpio->data      = tmp_sys_gpio_data->user_gpio_status.data;      //黍堤data杅擂
            strcpy(script_gpio->gpio_name, tmp_sys_gpio_data->gpio_name);
        }
    }
    else
    {
        for(first_port = 0; first_port < group_count_max; first_port++)
        {
            tmp_sys_gpio_data  = user_gpio_set + first_port;
            port     = tmp_sys_gpio_data->port;               //黍堤傷諳杅硉
            port_num = tmp_sys_gpio_data->port_num;           //黍堤傷諳笢腔議珨跺GPIO

            if(!port)
            {
                continue;
            }
            port_num_func = (port_num >> 3);
            port_num_pull = (port_num >> 4);
            tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
            tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
            tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
            tmp_group_data_addr    = PIO_REG_DATA(port);                  //載陔data敵湔
            break;
        }
        if(first_port >= group_count_max)
        {
            return 0;
        }
        for(i = first_port; i < group_count_max; i++)
        {
            tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
            script_gpio       = gpio_status + i;               //script_gpio硌砃蚚誧換輛腔諾潔

            port     = tmp_sys_gpio_data->port;                //黍堤傷諳杅硉
            port_num = tmp_sys_gpio_data->port_num;            //黍堤傷諳笢腔議珨跺GPIO

            script_gpio->port = port;                          //黍堤port杅擂
            script_gpio->port_num  = port_num;                 //黍堤port_num杅擂
            strcpy(script_gpio->gpio_name, tmp_sys_gpio_data->gpio_name);

            port_num_func = (port_num >> 3);
            port_num_pull = (port_num >> 4);

            if((port_num_pull != pre_port_num_pull) || (port != pre_port))    //彆楷珋絞竘褐腔傷諳祥珨祡ㄛ麼氪垀婓腔pull敵湔祥珨祡
            {
                tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
                tmp_group_pull_addr    = PIO_REG_PULL(port, port_num_pull);   //載陔pull敵湔
                tmp_group_dlevel_addr  = PIO_REG_DLEVEL(port, port_num_pull); //載陔level敵湔
                tmp_group_data_addr    = PIO_REG_DATA(port);                  //載陔data敵湔
            }
            else if(pre_port_num_func != port_num_func)                       //彆楷珋絞竘褐腔髡夔敵湔祥珨祡
            {
                tmp_group_func_addr    = PIO_REG_CFG(port, port_num_func);   //載陔髡夔敵湔華硊
            }

            pre_port_num_pull = port_num_pull;
            pre_port_num_func = port_num_func;
            pre_port          = port;
            //跤蚚誧諷璃董硉
            script_gpio->pull      = (GPIO_REG_READ(tmp_group_pull_addr)   >> ((port_num - (port_num_pull<<4))<<1)) & 0x03;    //黍堤pull杅擂
            script_gpio->drv_level = (GPIO_REG_READ(tmp_group_dlevel_addr) >> ((port_num - (port_num_pull<<4))<<1)) & 0x03;    //黍堤髡夔杅擂
            script_gpio->mul_sel   = (GPIO_REG_READ(tmp_group_func_addr)   >> ((port_num - (port_num_func<<3))<<2)) & 0x07;    //黍堤髡夔杅擂
            if(script_gpio->mul_sel <= 1)
            {
                script_gpio->data  = (GPIO_REG_READ(tmp_group_data_addr)   >>   port_num) & 0x01;                              //黍堤data杅擂
            }
            else
            {
                script_gpio->data = -1;
            }
        }
    }

    return EGPIO_SUCCESS;
}

#endif
/*
**********************************************************************************************************************
*                                               CSP_GPIO_Get_One_PIN_Status
*
* Description:
*                鳳蚚誧扠徹腔垀衄GPIO腔袨怓
* Arguments  :
*        p_handler    :    handler
*        gpio_status    :    悵湔蚚誧杅擂腔杅郪
*        gpio_name    :    猁紱釬腔GPIO腔靡備
*       if_get_user_set_flag   :   黍梓祩ㄛ桶尨黍蚚誧扢隅杅擂麼氪岆妗暱杅擂
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_get_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, __u32 if_get_from_hardware)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    __u32               port_num_func, port_num_pull;
    __u32               port, port_num;
    __u32               i, tmp_val1, tmp_val2;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if((!p_handler) || (!gpio_status))
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(group_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    else if((group_count_max > 1) && (!gpio_name))
    {
        return EGPIO_FAIL;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //黍蚚誧杅擂
    //桶尨黍蚚誧跤隅腔杅擂
    for(i = 0; i < group_count_max; i++)
    {
        tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
        {
            continue;
        }
        strcpy(gpio_status->gpio_name, tmp_sys_gpio_data->gpio_name);
        port                   = tmp_sys_gpio_data->port;
        port_num               = tmp_sys_gpio_data->port_num;
        gpio_status->port      = port;                                              //黍堤port杅擂
        gpio_status->port_num  = port_num;                                          //黍堤port_num杅擂
		if(port >= 12)
			cpus_flag = 1;
		else
			cpus_flag = 0;
        if(!if_get_from_hardware)                                                    //絞猁黍堤蚚誧扢數腔杅擂
        {
            gpio_status->mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;   //植蚚誧換輛杅擂笢黍堤髡夔杅擂
            gpio_status->pull      = tmp_sys_gpio_data->user_gpio_status.pull;      //植蚚誧換輛杅擂笢黍堤pull杅擂
            gpio_status->drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level; //植蚚誧換輛杅擂笢黍堤雄夔薯杅擂
            gpio_status->data      = tmp_sys_gpio_data->user_gpio_status.data;      //植蚚誧換輛杅擂笢黍堤data杅擂
        }
        else                                                                        //絞黍堤敵湔妗暱腔統杅
        {
			port_num_func = (port_num >> 3);
			port_num_pull = (port_num >> 4);

			tmp_val1 = ((port_num - (port_num_func << 3)) << 2);
			tmp_val2 = ((port_num - (port_num_pull << 4)) << 1);
			if(!cpus_flag)
			{
				gpio_status->mul_sel   = (PIO_REG_CFG_VALUE(port, port_num_func)>>tmp_val1) & 0x07;       //植茞璃笢黍堤髡夔敵湔
				gpio_status->pull      = (PIO_REG_PULL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;      //植茞璃笢黍堤pull敵湔
				gpio_status->drv_level = (PIO_REG_DLEVEL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;    //植茞璃笢黍堤level敵湔
			}
			else
			{
				gpio_status->mul_sel   = (R_PIO_REG_CFG_VALUE(port, port_num_func)>>tmp_val1) & 0x07;       //植茞璃笢黍堤髡夔敵湔
				gpio_status->pull      = (R_PIO_REG_PULL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;      //植茞璃笢黍堤pull敵湔
				gpio_status->drv_level = (R_PIO_REG_DLEVEL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;    //植茞璃笢黍堤level敵湔
			}
			if(gpio_status->mul_sel <= 1)
			{
				if(!cpus_flag)
					gpio_status->data = (PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;                     //植茞璃笢黍堤data敵湔
				else
					gpio_status->data = (R_PIO_REG_DATA_VALUE(port)>> port_num) & 0x01;
			}
			else
			{
				gpio_status->data = -1;
			}
        }

        break;
    }

    return EGPIO_SUCCESS;
}
#else
__s32  gpio_get_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, __u32 if_get_from_hardware)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    __u32               port_num_func, port_num_pull;
    __u32               port, port_num;
    __u32               i, tmp_val1, tmp_val2;

    //潰脤換輛腔曆梟腔衄虴俶
    if((!p_handler) || (!gpio_status))
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(group_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    else if((group_count_max > 1) && (!gpio_name))
    {
        return EGPIO_FAIL;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //黍蚚誧杅擂
    //桶尨黍蚚誧跤隅腔杅擂
    for(i = 0; i < group_count_max; i++)
    {
        tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
        {
            continue;
        }
        strcpy(gpio_status->gpio_name, tmp_sys_gpio_data->gpio_name);
        port                   = tmp_sys_gpio_data->port;
        port_num               = tmp_sys_gpio_data->port_num;
        gpio_status->port      = port;                                              //黍堤port杅擂
        gpio_status->port_num  = port_num;                                          //黍堤port_num杅擂

        if(!if_get_from_hardware)                                                    //絞猁黍堤蚚誧扢數腔杅擂
        {
            gpio_status->mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;   //植蚚誧換輛杅擂笢黍堤髡夔杅擂
            gpio_status->pull      = tmp_sys_gpio_data->user_gpio_status.pull;      //植蚚誧換輛杅擂笢黍堤pull杅擂
            gpio_status->drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level; //植蚚誧換輛杅擂笢黍堤雄夔薯杅擂
            gpio_status->data      = tmp_sys_gpio_data->user_gpio_status.data;      //植蚚誧換輛杅擂笢黍堤data杅擂
        }
        else                                                                        //絞黍堤敵湔妗暱腔統杅
        {
			port_num_func = (port_num >> 3);
			port_num_pull = (port_num >> 4);

			tmp_val1 = ((port_num - (port_num_func << 3)) << 2);
			tmp_val2 = ((port_num - (port_num_pull << 4)) << 1);
			gpio_status->mul_sel   = (PIO_REG_CFG_VALUE(port, port_num_func)>>tmp_val1) & 0x07;       //植茞璃笢黍堤髡夔敵湔
			gpio_status->pull      = (PIO_REG_PULL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;      //植茞璃笢黍堤pull敵湔
			gpio_status->drv_level = (PIO_REG_DLEVEL_VALUE(port, port_num_pull)>>tmp_val2) & 0x03;    //植茞璃笢黍堤level敵湔
			if(gpio_status->mul_sel <= 1)
			{
				gpio_status->data = (PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;                     //植茞璃笢黍堤data敵湔
			}
			else
			{
				gpio_status->data = -1;
			}
        }

        break;
    }

    return EGPIO_SUCCESS;
}

#endif
/*
**********************************************************************************************************************
*                                               CSP_PIN_Set_One_Gpio_Status
*
* Description:
*                鳳蚚誧扠徹腔GPIO腔議珨跺腔袨怓
* Arguments  :
*        p_handler    :    handler
*        gpio_status    :    悵湔蚚誧杅擂腔杅郪
*        gpio_name    :    猁紱釬腔GPIO腔靡備
*       if_get_user_set_flag   :   黍梓祩ㄛ桶尨黍蚚誧扢隅杅擂麼氪岆妗暱杅擂
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_set_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, __u32 if_set_to_current_input_status)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    user_gpio_set_t     script_gpio;
    volatile __u32     *tmp_addr;
    __u32               port_num_func, port_num_pull;
    __u32               port, port_num;
    __u32               i, reg_val, tmp_val;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if((!p_handler) || (!gpio_name))
    {
        return EGPIO_FAIL;
    }
    if((if_set_to_current_input_status) && (!gpio_status))
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(group_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //黍蚚誧杅擂
    //桶尨黍蚚誧跤隅腔杅擂
    for(i = 0; i < group_count_max; i++)
    {
        tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
        {
            continue;
        }

        port          = tmp_sys_gpio_data->port;                           //黍堤port杅擂
        port_num      = tmp_sys_gpio_data->port_num;                       //黍堤port_num杅擂
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);
		if(port >= 12)
			cpus_flag = 1;
		else
			cpus_flag = 0;
        if(if_set_to_current_input_status)                                 //跦擂絞蚚誧扢隅党淏
        {
            //党蜊FUCN敵湔
            script_gpio.mul_sel   = gpio_status->mul_sel;
            script_gpio.pull      = gpio_status->pull;
            script_gpio.drv_level = gpio_status->drv_level;
            script_gpio.data      = gpio_status->data;
        }
        else
        {
            script_gpio.mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;
            script_gpio.pull      = tmp_sys_gpio_data->user_gpio_status.pull;
            script_gpio.drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level;
            script_gpio.data      = tmp_sys_gpio_data->user_gpio_status.data;
        }

        if(script_gpio.mul_sel >= 0)
        {
			if(!cpus_flag)
				tmp_addr = PIO_REG_CFG(port, port_num_func);
			else
				tmp_addr = R_PIO_REG_CFG(port, port_num_func);
            reg_val = GPIO_REG_READ(tmp_addr);                                                       //党蜊FUNC敵湔
            tmp_val = (port_num - (port_num_func<<3))<<2;
            reg_val &= ~(0x07 << tmp_val);
            reg_val |=  (script_gpio.mul_sel) << tmp_val;
            GPIO_REG_WRITE(tmp_addr, reg_val);
        }
        //党蜊PULL敵湔
        if(script_gpio.pull >= 0)
        {
			if(!cpus_flag)
				tmp_addr = PIO_REG_PULL(port, port_num_pull);
			else
				tmp_addr = R_PIO_REG_PULL(port, port_num_pull);
            reg_val = GPIO_REG_READ(tmp_addr);                                                     //党蜊FUNC敵湔
            tmp_val = (port_num - (port_num_pull<<4))<<1;
            reg_val &= ~(0x03 << tmp_val);
            reg_val |=  (script_gpio.pull) << tmp_val;
            GPIO_REG_WRITE(tmp_addr, reg_val);
        }
        //党蜊DLEVEL敵湔
        if(script_gpio.drv_level >= 0)
        {
			if(!cpus_flag)
				tmp_addr = PIO_REG_DLEVEL(port, port_num_pull);
			else
				tmp_addr = R_PIO_REG_DLEVEL(port, port_num_pull);
            reg_val = GPIO_REG_READ(tmp_addr);                                                         //党蜊FUNC敵湔
            tmp_val = (port_num - (port_num_pull<<4))<<1;
            reg_val &= ~(0x03 << tmp_val);
            reg_val |=  (script_gpio.drv_level) << tmp_val;
            GPIO_REG_WRITE(tmp_addr, reg_val);
        }
        //党蜊data敵湔
        if(script_gpio.mul_sel == 1)
        {
            if(script_gpio.data >= 0)
            {
				if(!cpus_flag)
					tmp_addr = PIO_REG_DATA(port);
				else
					tmp_addr = R_PIO_REG_DATA(port);
                reg_val = GPIO_REG_READ(tmp_addr);;                                                      //党蜊DATA敵湔
                reg_val &= ~(0x01 << port_num);
                reg_val |=  (script_gpio.data & 0x01) << port_num;
                GPIO_REG_WRITE(tmp_addr, reg_val);
            }
        }

        break;
    }

    return EGPIO_SUCCESS;
}
#else
__s32  gpio_set_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, __u32 if_set_to_current_input_status)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set, *tmp_sys_gpio_data;
    user_gpio_set_t     script_gpio;
    volatile __u32     *tmp_addr;
    __u32               port_num_func, port_num_pull;
    __u32               port, port_num;
    __u32               i, reg_val, tmp_val;

    //潰脤換輛腔曆梟腔衄虴俶
    if((!p_handler) || (!gpio_name))
    {
        return EGPIO_FAIL;
    }
    if((if_set_to_current_input_status) && (!gpio_status))
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    if(group_count_max <= 0)
    {
        return EGPIO_FAIL;
    }
    user_gpio_set = (system_gpio_set_t *)(tmp_buf + 16);
    //黍蚚誧杅擂
    //桶尨黍蚚誧跤隅腔杅擂
    for(i = 0; i < group_count_max; i++)
    {
        tmp_sys_gpio_data = user_gpio_set + i;             //tmp_sys_gpio_data硌砃扠腔GPIO諾潔
        if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
        {
            continue;
        }

        port          = tmp_sys_gpio_data->port;                           //黍堤port杅擂
        port_num      = tmp_sys_gpio_data->port_num;                       //黍堤port_num杅擂
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if(if_set_to_current_input_status)                                 //跦擂絞蚚誧扢隅党淏
        {
            //党蜊FUCN敵湔
            script_gpio.mul_sel   = gpio_status->mul_sel;
            script_gpio.pull      = gpio_status->pull;
            script_gpio.drv_level = gpio_status->drv_level;
            script_gpio.data      = gpio_status->data;
        }
        else
        {
            script_gpio.mul_sel   = tmp_sys_gpio_data->user_gpio_status.mul_sel;
            script_gpio.pull      = tmp_sys_gpio_data->user_gpio_status.pull;
            script_gpio.drv_level = tmp_sys_gpio_data->user_gpio_status.drv_level;
            script_gpio.data      = tmp_sys_gpio_data->user_gpio_status.data;
        }

        if(script_gpio.mul_sel >= 0)
        {
            tmp_addr = PIO_REG_CFG(port, port_num_func);
            reg_val = GPIO_REG_READ(tmp_addr);                                                       //党蜊FUNC敵湔
            tmp_val = (port_num - (port_num_func<<3))<<2;
            reg_val &= ~(0x07 << tmp_val);
            reg_val |=  (script_gpio.mul_sel) << tmp_val;
            GPIO_REG_WRITE(tmp_addr, reg_val);
        }
        //党蜊PULL敵湔
        if(script_gpio.pull >= 0)
        {
            tmp_addr = PIO_REG_PULL(port, port_num_pull);
            reg_val = GPIO_REG_READ(tmp_addr);                                                    //党蜊FUNC敵湔
            tmp_val = (port_num - (port_num_pull<<4))<<1;
            reg_val &= ~(0x03 << tmp_val);
            reg_val |=  (script_gpio.pull) << tmp_val;
            GPIO_REG_WRITE(tmp_addr, reg_val);
        }
        //党蜊DLEVEL敵湔
        if(script_gpio.drv_level >= 0)
        {
            tmp_addr = PIO_REG_DLEVEL(port, port_num_pull);
            reg_val = GPIO_REG_READ(tmp_addr);                                                   //党蜊FUNC敵湔
            tmp_val = (port_num - (port_num_pull<<4))<<1;
            reg_val &= ~(0x03 << tmp_val);
            reg_val |=  (script_gpio.drv_level) << tmp_val;
            GPIO_REG_WRITE(tmp_addr, reg_val);
        }
        //党蜊data敵湔
        if(script_gpio.mul_sel == 1)
        {
            if(script_gpio.data >= 0)
            {
                tmp_addr = PIO_REG_DATA(port);
                reg_val = GPIO_REG_READ(tmp_addr);                                                      //党蜊DATA敵湔
                reg_val &= ~(0x01 << port_num);
                reg_val |=  (script_gpio.data & 0x01) << port_num;
                GPIO_REG_WRITE(tmp_addr, reg_val);
            }
        }

        break;
    }

    return EGPIO_SUCCESS;
}
#endif
/*
**********************************************************************************************************************
*                                               CSP_GPIO_Set_One_PIN_IO_Status
*
* Description:
*                党蜊蚚誧扠徹腔GPIO笢腔議珨跺IO諳腔ㄛ怀怀堤袨怓
* Arguments  :
*        p_handler    :    handler
*        if_set_to_output_status    :    扢离傖怀堤袨怓遜岆怀袨怓
*        gpio_name    :    猁紱釬腔GPIO腔靡備
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_set_one_pin_io_status(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32      *tmp_group_func_addr = NULL;
    __u32               port, port_num, port_num_func;
    __u32                i, reg_val;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(if_set_to_output_status > 1)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);
    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;
	if(port >= 12)
		cpus_flag = 1;
	if(!cpus_flag)
		tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);
	else
		tmp_group_func_addr = R_PIO_REG_CFG(port, port_num_func);

    reg_val = GPIO_REG_READ(tmp_group_func_addr);
    reg_val &= ~(0x07 << (((port_num - (port_num_func<<3))<<2)));
    reg_val |=   if_set_to_output_status << (((port_num - (port_num_func<<3))<<2));
	GPIO_REG_WRITE(tmp_group_func_addr, reg_val);

    return EGPIO_SUCCESS;
}
#else
__s32  gpio_set_one_pin_io_status(u32 p_handler, __u32 if_set_to_output_status, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32      *tmp_group_func_addr = NULL;
    __u32               port, port_num, port_num_func;
    __u32                i, reg_val;

    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(if_set_to_output_status > 1)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);
    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;

    tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);
    reg_val = GPIO_REG_READ(tmp_group_func_addr);
    reg_val &= ~(0x07 << (((port_num - (port_num_func<<3))<<2)));
    reg_val |=   if_set_to_output_status << (((port_num - (port_num_func<<3))<<2));
    GPIO_REG_WRITE(tmp_group_func_addr, reg_val);

    return EGPIO_SUCCESS;
}


#endif
/*
**********************************************************************************************************************
*                                               CSP_GPIO_Set_One_PIN_Pull
*
* Description:
*                党蜊蚚誧扠徹腔GPIO笢腔議珨跺IO諳腔ㄛPULL袨怓
* Arguments  :
*        p_handler    :    handler
*        if_set_to_output_status    :    垀扢离腔pull袨怓
*        gpio_name    :    猁紱釬腔GPIO腔靡備
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_set_one_pin_pull(u32 p_handler, __u32 set_pull_status, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32      *tmp_group_pull_addr = NULL;
    __u32               port, port_num, port_num_pull;
    __u32                i, reg_val;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(set_pull_status >= 4)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);
    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_pull = port_num >> 4;

	if(port >= 12)
		cpus_flag = 1;
	if(!cpus_flag)
		tmp_group_pull_addr = PIO_REG_DLEVEL(port, port_num_pull);
	else
		tmp_group_pull_addr = R_PIO_REG_DLEVEL(port,port_num_pull);

    reg_val = GPIO_REG_READ(tmp_group_pull_addr);
    reg_val &= ~(0x03 << (((port_num - (port_num_pull<<4))<<1)));
    reg_val |=  (set_pull_status << (((port_num - (port_num_pull<<4))<<1)));
    GPIO_REG_WRITE(tmp_group_pull_addr, reg_val);

    return EGPIO_SUCCESS;
}
#else
__s32  gpio_set_one_pin_pull(u32 p_handler, __u32 set_pull_status, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32      *tmp_group_pull_addr = NULL;
    __u32               port, port_num, port_num_pull;
    __u32                i, reg_val;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(set_pull_status >= 4)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);
    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_pull = port_num >> 4;

    tmp_group_pull_addr = PIO_REG_PULL(port, port_num_pull);
    reg_val = GPIO_REG_READ(tmp_group_pull_addr);
    reg_val &= ~(0x03 << (((port_num - (port_num_pull<<4))<<1)));
    reg_val |=  (set_pull_status << (((port_num - (port_num_pull<<4))<<1)));
    GPIO_REG_WRITE(tmp_group_pull_addr, reg_val);

    return EGPIO_SUCCESS;
}
#endif
/*
**********************************************************************************************************************
*                                               CSP_GPIO_Set_One_PIN_driver_level
*
* Description:
*                党蜊蚚誧扠徹腔GPIO笢腔議珨跺IO諳腔ㄛ雄夔薯
* Arguments  :
*        p_handler    :    handler
*        if_set_to_output_status    :    垀扢离腔雄夔薯脹撰
*        gpio_name    :    猁紱釬腔GPIO腔靡備
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_set_one_pin_driver_level(u32 p_handler, __u32 set_driver_level, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32      *tmp_group_dlevel_addr = NULL;
    __u32               port, port_num, port_num_dlevel;
    __u32                i, reg_val;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(set_driver_level >= 4)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_dlevel = port_num >> 4;

	if(port >= 12)
		cpus_flag = 1;
	if(!cpus_flag)
		tmp_group_dlevel_addr = PIO_REG_DLEVEL(port, port_num_dlevel);
	else
		tmp_group_dlevel_addr = R_PIO_REG_DLEVEL(port,port_num_dlevel);

    reg_val = GPIO_REG_READ(tmp_group_dlevel_addr);
    reg_val &= ~(0x03 << (((port_num - (port_num_dlevel<<4))<<1)));
    reg_val |=  (set_driver_level << (((port_num - (port_num_dlevel<<4))<<1)));
    GPIO_REG_WRITE(tmp_group_dlevel_addr, reg_val);

    return EGPIO_SUCCESS;
}
#else
__s32  gpio_set_one_pin_driver_level(u32 p_handler, __u32 set_driver_level, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32      *tmp_group_dlevel_addr = NULL;
    __u32               port, port_num, port_num_dlevel;
    __u32                i, reg_val;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(set_driver_level >= 4)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_dlevel = port_num >> 4;

    tmp_group_dlevel_addr = PIO_REG_DLEVEL(port, port_num_dlevel);
    reg_val = GPIO_REG_READ(tmp_group_dlevel_addr);
    reg_val &= ~(0x03 << (((port_num - (port_num_dlevel<<4))<<1)));
    reg_val |=  (set_driver_level << (((port_num - (port_num_dlevel<<4))<<1)));
    GPIO_REG_WRITE(tmp_group_dlevel_addr, reg_val);

    return EGPIO_SUCCESS;
}
#endif
/*
**********************************************************************************************************************
*                                               CSP_GPIO_Read_One_PIN_Value
*
* Description:
*                黍蚚誧扠徹腔GPIO笢腔議珨跺IO諳腔傷諳腔萇
* Arguments  :
*        p_handler    :    handler
*        gpio_name    :    猁紱釬腔GPIO腔靡備
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_read_one_pin_value(u32 p_handler, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    __u32               port, port_num, port_num_func, func_val;
    __u32                i, reg_val;
    int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;

    if(port >= 12)
        cpus_flag = 1;
    if(!cpus_flag)
    {
        reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);
    }
    else
    {
         reg_val  = R_PIO_REG_CFG_VALUE(port, port_num_func);
    }

    func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
    if(func_val == 0)
    {
        if(!cpus_flag)
        {
            reg_val = (PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;
        }
        else
        {
            reg_val = (R_PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;
        }

        return reg_val;
    }

    return EGPIO_FAIL;
}
#else
__s32  gpio_read_one_pin_value(u32 p_handler, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    __u32               port, port_num, port_num_func, func_val;
    __u32                i, reg_val;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;

    reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);

    func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
    if(func_val == 0)
    {
        reg_val = (PIO_REG_DATA_VALUE(port) >> port_num) & 0x01;
        return reg_val;
    }

    return EGPIO_FAIL;
}
#endif
/*
**********************************************************************************************************************
*                                               CSP_GPIO_Write_One_PIN_Value
*
* Description:
*                党蜊蚚誧扠徹腔GPIO笢腔議珨跺IO諳腔傷諳腔萇
* Arguments  :
*        p_handler    :    handler
*       value_to_gpio:  猁扢离腔萇腔萇揤
*        gpio_name    :    猁紱釬腔GPIO腔靡備
* Returns    :
*
* Notes      :
*
**********************************************************************************************************************
*/
#ifdef SUNXI_R_PIO_BASE
__s32  gpio_write_one_pin_value(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32     *tmp_group_data_addr = NULL;
    __u32               port, port_num, port_num_func, func_val;
    __u32                i, reg_val;
	int cpus_flag = 0;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(value_to_gpio >= 2)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;
	if(port == 0xffff)
	{
		gpio_set_axpgpio_value(0, port_num, value_to_gpio);
		return EGPIO_SUCCESS;
	}
	else
	{
		if(port >= 12 )
		cpus_flag = 1;
		if(!cpus_flag)
			reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);
		else
			reg_val = R_PIO_REG_CFG_VALUE(port,port_num_func);
	    func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
	    if(func_val == 1)
	    {
			if(!cpus_flag)
				tmp_group_data_addr = PIO_REG_DATA(port);
			else
				tmp_group_data_addr = R_PIO_REG_DATA(port);

			reg_val = GPIO_REG_READ(tmp_group_data_addr);
	        reg_val &= ~(1 << port_num);
	        reg_val |=  (value_to_gpio << port_num);
	        GPIO_REG_WRITE(tmp_group_data_addr, reg_val);

	        return EGPIO_SUCCESS;
	    }
	}

    return EGPIO_FAIL;
}
#else
__s32  gpio_write_one_pin_value(u32 p_handler, __u32 value_to_gpio, const char *gpio_name)
{
    char               *tmp_buf;                                        //蛌遙傖char濬倰
    __u32               group_count_max;                                //郔湮GPIO跺杅
    system_gpio_set_t  *user_gpio_set = NULL, *tmp_sys_gpio_data;
    volatile __u32     *tmp_group_data_addr = NULL;
    __u32               port, port_num, port_num_func, func_val;
    __u32                i, reg_val;
    //潰脤換輛腔曆梟腔衄虴俶
    if(!p_handler)
    {
        return EGPIO_FAIL;
    }
    if(value_to_gpio >= 2)
    {
        return EGPIO_FAIL;
    }
    tmp_buf = (char *)p_handler;
    group_count_max = *(int *)tmp_buf;
    tmp_sys_gpio_data = (system_gpio_set_t *)(tmp_buf + 16);

    if(group_count_max == 0)
    {
        return EGPIO_FAIL;
    }
    else if(group_count_max == 1)
    {
        user_gpio_set = tmp_sys_gpio_data;
    }
    else if(gpio_name)
    {
        for(i=0; i<group_count_max; i++)
        {
            if(strcmp(gpio_name, tmp_sys_gpio_data->gpio_name))
            {
                tmp_sys_gpio_data ++;
                continue;
            }
            user_gpio_set = tmp_sys_gpio_data;
            break;
        }
    }
    if(!user_gpio_set)
    {
        return EGPIO_FAIL;
    }

    port     = user_gpio_set->port;
    port_num = user_gpio_set->port_num;
    port_num_func = port_num >> 3;

    reg_val  = PIO_REG_CFG_VALUE(port, port_num_func);
    func_val = (reg_val >> ((port_num - (port_num_func<<3))<<2)) & 0x07;
    if(func_val == 1)
    {
        tmp_group_data_addr = PIO_REG_DATA(port);
        reg_val = GPIO_REG_READ(tmp_group_data_addr);
        reg_val &= ~(1 << port_num);
        reg_val |=  (value_to_gpio << port_num);
        GPIO_REG_WRITE(tmp_group_data_addr, reg_val);

        return EGPIO_SUCCESS;
    }

    return EGPIO_FAIL;
}
#endif
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void upper(char *str)
{
	int i=0;
	char c;

	do
	{
		c=str[i];
		if(c=='\0')
		{
			return;
		}
		if((c>='a') && (c<='z'))
		{
			str[i]-=('a'-'A');
		}
		i++;
	}
	while(1);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void lower(char *str)
{
	int i=0;
	char c;

	do
	{
		c=str[i];
		if(c=='\0')
		{
			return;
		}
		if((c>='A') && (c<='Z'))
		{
			str[i]+=('a'-'A');
		}
		i++;
	}
	while(1);
}
