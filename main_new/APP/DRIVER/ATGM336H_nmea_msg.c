
#include "ATGM336H_nmea_msg.h"
#include "ATGM336H.h"
#include "appconfig.h"

/* ATGM336H模块NMEA消息句首和句末 */
#define ATGM336H_NMEA_MSG_SS          (0x24)           /* Start of sentence: $ */
#define ATGM336H_NMEA_MSG_ES          (0x0D0A)         /* Ending of sentence 回车和换行*/
																									 
/* ATGM336H模块NMEA消息地址段 */                
#define ATGM336H_NMEA_ADDRESS_GNGGA   "GNGGA"          /* $GNGGA */
#define ATGM336H_NMEA_ADDRESS_GNGLL   "GNGLL"          /* $GNGLL */
#define ATGM336H_NMEA_ADDRESS_GNGSA   "GNGSA"          /* $GNGSA */
#define ATGM336H_NMEA_ADDRESS_GPGSA   "GPGSA"          /* $GPGSA */
#define ATGM336H_NMEA_ADDRESS_BDGSA   "BDGSA"          /* $BDGSA */
#define ATGM336H_NMEA_ADDRESS_GPGSV   "GPGSV"          /* $GPGSV */
#define ATGM336H_NMEA_ADDRESS_BDGSV   "BDGSV"          /* $BDGSV */
#define ATGM336H_NMEA_ADDRESS_GNRMC   "GNRMC"          /* $GNRMC */
#define ATGM336H_NMEA_ADDRESS_GNVTG   "GNVTG"          /* $GNVTG */
#define ATGM336H_NMEA_ADDRESS_GNZDA   "GNZDA"          /* $GNZDA */

/* ATGM336H模块NMEA消息地址段长度 */
#define ATGM336H_NMEA_ADDRESS_LEN     5

/**
 * @brief       获取NMEA消息中指定索引的数据参数
 * @param       nmea           : NMEA消息
 *              parameter_index: 参数的索引
 *              parameter      : 返回的参数（指向NMEA消息中的数据）
 *              parameter_len  : 参数的长度
 * @retval      ATGM336H_EOK   : 成功获取NMEA消息中指定索引的数据参数
 *              ATGM336H_ERROR : NMEA消息中找不到指定索引的数据参数
 *              ATGM336H_EINVAL: 函数参数错误
 */
static uint8_t atgm336h_decode_nmea_parameter(uint8_t *nmea, uint8_t parameter_index, uint8_t **parameter, uint16_t *parameter_len)
{
    uint8_t *nmea_point;
    uint16_t parameter_counter = 0;
    
    if (nmea == NULL)
    {
        return ATGM336H_EINVAL;
    }
    
    if (*parameter == NULL)
    {
        return ATGM336H_EINVAL;
    }
    
    for (nmea_point=(nmea+1);(*nmea_point!=0x0D)&&(*(nmea_point+1)!=0x0A); nmea_point++)
    {
        if (parameter_index == 0)
        {
            *parameter = nmea_point - parameter_counter;
            
            if (parameter_len == NULL)
            {
                break;
            }
            
            if ((*nmea_point != ',') && (*nmea_point != '*'))
            {
                parameter_counter++;
                continue;
            }
            else
            {
                *parameter_len = parameter_counter;
                break;
            }
        }
        
        if ((*nmea_point == ',') || (*nmea_point == '*'))
        {
            parameter_index--;
        }
    }
    
    if (parameter_index != 0)
    {
        return ATGM336H_ERROR;
    }
    
    return ATGM336H_EOK;
}

/**
 * @brief       调整放大后数字的小数位数
 * @param       num      : 待调整数字
 *              float_len: num原本的小数位数
 *              fix_len  : num调整后的小数位数
 * @retval      无
 */
static void atgm336h_fit_num(int32_t *num, uint8_t float_len, uint8_t fix_len)
{
    if (num == NULL)
    {
        return;
    }
    
    while (float_len > fix_len)
    {
        *num /= 10;
        float_len--;
    }
    
    while (float_len < fix_len)
    {
        *num *= 10;
        float_len++;
    }
}

/**
 * @brief       将NMEA消息中的字符串参数（以','、'*'或回车（0x0D）结尾）转为数字
 * @param       str      : NMEA消息中的字符串参数
 *              num      : 转换后的数字
 *              float_len: 转换后的数字实际小数点的位数
 * @retval      ATGM336H_EOK   : 转换成功
 *              ATGM336H_EINVAL: 函数参数错误
 */
static uint8_t atgm336h_nmea_str2num(uint8_t *str, int32_t *num, uint8_t *float_len)
{
    uint8_t *str_point = str;
    int8_t negative_coefficient;
    uint8_t str_index_num;
    int32_t _num = 0;
    uint8_t _float_len = 0;
    
    if ((str == NULL) || (num == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    if ((*str_point != '-') && ((*str_point < '0') || (*str_point > '9')))
    {
        return ATGM336H_EINVAL;
    }
    
    if (*str_point == '-')
    {
        str_point++;
        negative_coefficient = -1;
    }
    else
    {
        negative_coefficient = 1;
    }
    
    while ((*str_point == '.') || ((*str_point >= '0') && (*str_point <= '9')))
    {
        if (_float_len != 0)
        {
            _float_len++;
        }
        
        if (*str_point == '.')
        {
            _float_len++;
            str_point++;
            continue;
        }
        
        str_index_num = *str_point - '0';
        _num = _num * 10 + str_index_num;
        str_point++;
    }
    
    *num = _num * negative_coefficient;
    if (float_len != NULL)
    {
        *float_len = (_float_len != 0) ? (_float_len - 1) : _float_len;
    }
    
    return ATGM336H_EOK;
}

/**
 * @brief       从数据缓冲中获取指定类型和索引的NMEA消息
 * @param       buf      : 数据缓冲
 *              nmea     : 指定获取的NMEA消息类型
 *              msg_index: 指定获取的NMEA消息类型在数据缓冲中的指定索引
 *                              0: 获取数据缓冲中第一个指定NMEA消息类型的消息
 *                              1: 获取数据缓冲中第一个指定NMEA消息类型的消息
 *                              2: 获取数据缓冲中第二个指定NMEA消息类型的消息
 *                              3: 获取数据缓冲中第三个指定NMEA消息类型的消息
 *                              以此类推......
 *              msg      : 获取到的NMEA消息（指向数据缓冲中的数据）
 * @retval      ATGM336H_EOK   : 成功从数据缓冲中获取到指定类型和索引的NMEA消息
 *              ATGM336H_ERROR : 数据缓冲中找不到指定类型或索引的NMEA消息
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_get_nmea_msg_from_buf(uint8_t *buf, atgm336h_nmea_msg_t nmea, uint8_t msg_index, uint8_t **msg)
{
    char address[ATGM336H_NMEA_ADDRESS_LEN + 1];
    uint8_t *buf_point;
    uint8_t address_index;
    
    if ((buf == NULL) || (msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    if (msg_index == 0)
    {
        msg_index = 1;
    }
    
    switch (nmea)
    {
        case ATGM336H_NMEA_MSG_GNGGA:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GNGGA);
            break;
        }
        case ATGM336H_NMEA_MSG_GNGLL:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GNGLL);
            break;
        }
        case ATGM336H_NMEA_MSG_GNGSA:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GNGSA);
            break;
        }
        case ATGM336H_NMEA_MSG_GPGSA:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GPGSA);
            break;
        }
        case ATGM336H_NMEA_MSG_BDGSA:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_BDGSA);
            break;
        }
        case ATGM336H_NMEA_MSG_GPGSV:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GPGSV);
            break;
        }
        case ATGM336H_NMEA_MSG_BDGSV:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_BDGSV);
            break;
        }
        case ATGM336H_NMEA_MSG_GNRMC:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GNRMC);
            break;
        }
        case ATGM336H_NMEA_MSG_GNVTG:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GNVTG);
            break;
        }
        case ATGM336H_NMEA_MSG_GNZDA:
        {
            sprintf(address, ATGM336H_NMEA_ADDRESS_GNZDA);
            break;
        }
        default:
        {
            return ATGM336H_EINVAL;
        }
    }
    
    /* 遍历数据缓冲 */
    for (buf_point=buf; *buf_point!='\0'; buf_point++)
    {
        /* 找到句首（'$'：0x24） */
        if (*buf_point == ATGM336H_NMEA_MSG_SS)
        {
            /* 判断是否为指定类型的NMEA消息 */
            for (address_index=0; address_index<ATGM336H_NMEA_ADDRESS_LEN; address_index++)
            {
                /* 若不是指定类型的NMEA消息，则退出去寻找下一个句首 */
                if (*(buf_point+1+address_index) != address[address_index])
                {
                    break;
                }
            }
            
            if (address_index == ATGM336H_NMEA_ADDRESS_LEN)
            {
                if (--msg_index == 0)
                {
                    *msg = buf_point;
                    return ATGM336H_EOK;
                }
            }
        }
    }
    
    return ATGM336H_ERROR;
}

/**
 * @brief       解析$XXGGA类型的NMEA消息
 * @param       xxgga_msg : 待解析的$XXGGA类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXGGA类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXGGA类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxgga(uint8_t *xxgga_msg, atgm336h_nmea_gga_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    int32_t _num;
    uint8_t float_len;
    
    if ((xxgga_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxgga_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'G') || (parameter[3] != 'G') || (parameter[4] != 'A')))
    {
        return ATGM336H_EINVAL;
    }
    
    /* UTC Time */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 1, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 3))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_time.hour = (_num / 10000000) % 100;
    decode_msg->utc_time.minute = (_num / 100000) % 100;
    decode_msg->utc_time.second = (_num / 1000) % 100;
    decode_msg->utc_time.millisecond = (_num / 1) % 1000;
    
    /* Latitude */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 2, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 5);
    decode_msg->latitude.degree = (_num / 10000000) * 100000;
    decode_msg->latitude.degree += ((_num % 10000000) / 60);
    
    /* N/S Indicator */
    ret = atgm336h_decode_nmea_parameter(xxgga_msg, 3, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'N') && (parameter[0] != 'S')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->latitude.indicator = (parameter[0] == 'N') ? ATGM336H_LATITUDE_NORTH : ATGM336H_LATITUDE_SOUTH;
    
    /* Longitude */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 4, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 5);
    decode_msg->longitude.degree = (_num / 10000000) * 100000;
    decode_msg->longitude.degree += ((_num % 10000000) / 60);
    
    /* E/W Indicator */
    ret = atgm336h_decode_nmea_parameter(xxgga_msg, 5, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'E') && (parameter[0] != 'W')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->longitude.indicator = (parameter[0] == 'E') ? ATGM336H_LONGITUDE_EAST : ATGM336H_LONGITUDE_WEST;
    
    /* GPS quality indicator */
    ret = atgm336h_decode_nmea_parameter(xxgga_msg, 6, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    switch (_num)
    {
        case 0:
        {
            decode_msg->gps_quality = ATGM336H_GPS_UNAVAILABLE;
            break;
        }
        case 1:
        {
            decode_msg->gps_quality = ATGM336H_GPS_VALID_SPS;
            break;
        }
        case 2:
        {
            decode_msg->gps_quality = ATGM336H_GPS_VALID_DIFFERENTIAL;
            break;
        }
        default:
        {
            return ATGM336H_ERROR;
        }
    }
    
    /* Satellites Used */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 7, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->satellite_num = _num;
    
    /* HDOP */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 8, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->hdop = _num;
    
    /* Altitude */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 9, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->altitude = _num;
    
    /* Geoidal Separation */
    
    /* DGPS Station ID */
    ret  = atgm336h_decode_nmea_parameter(xxgga_msg, 14, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->dgps_id = _num;
    
    return ATGM336H_EOK;
}

/**
 * @brief       解析$XXGLL类型的NMEA消息
 * @param       xxgll_msg : 待解析的$XXGLL类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXGLL类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXGLL类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxgll(uint8_t *xxgll_msg, atgm336h_nmea_gll_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    int32_t _num;
    uint8_t float_len;
    
    if ((xxgll_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxgll_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'G') || (parameter[3] != 'L') || (parameter[4] != 'L')))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Latitude */
    ret  = atgm336h_decode_nmea_parameter(xxgll_msg, 1, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 5);
    decode_msg->latitude.degree = (_num / 10000000) * 100000;
    decode_msg->latitude.degree += ((_num % 10000000) / 60);
    
    /* N/S Indicator */
    ret = atgm336h_decode_nmea_parameter(xxgll_msg, 2, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'N') && (parameter[0] != 'S')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->latitude.indicator = (parameter[0] == 'N') ? ATGM336H_LATITUDE_NORTH : ATGM336H_LATITUDE_SOUTH;
    
    /* Longitude */
    ret  = atgm336h_decode_nmea_parameter(xxgll_msg, 3, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 5);
    decode_msg->longitude.degree = (_num / 10000000) * 100000;
    decode_msg->longitude.degree += ((_num % 10000000) / 60);
    
    /* E/W Indicator */
    ret = atgm336h_decode_nmea_parameter(xxgll_msg, 4, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'E') && (parameter[0] != 'W')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->longitude.indicator = (parameter[0] == 'E') ? ATGM336H_LONGITUDE_EAST : ATGM336H_LONGITUDE_WEST;
    
    /* UTC Time */
    ret  = atgm336h_decode_nmea_parameter(xxgll_msg, 5, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 3))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_time.hour = (_num / 10000000) % 100;
    decode_msg->utc_time.minute = (_num / 100000) % 100;
    decode_msg->utc_time.second = (_num / 1000) % 100;
    decode_msg->utc_time.millisecond = (_num / 1) % 1000;
    
    /* Status */
    ret = atgm336h_decode_nmea_parameter(xxgll_msg, 6, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'A') && (parameter[0] != 'V')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->status = (parameter[0] == 'A') ? ATGM336H_XXGLL_DATA_VALID : ATGM336H_XXGLL_DATA_NO_VALID;
    
    return ATGM336H_EOK;
}

/**
 * @brief       解析$XXGSA类型的NMEA消息
 * @param       xxgsa_msg : 待解析的$XXGSA类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXGSA类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXGSA类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxgsa(uint8_t *xxgsa_msg, atgm336h_nmea_gsa_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    uint16_t parameter_len = 0;
    int32_t _num;
    uint8_t float_len;
    uint8_t satellite_index;
    
    if ((xxgsa_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxgsa_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'G') || (parameter[3] != 'S') || (parameter[4] != 'A')))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Mode */
    ret = atgm336h_decode_nmea_parameter(xxgsa_msg, 1, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'M') && (parameter[0] != 'A')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->mode = (parameter[0] == 'M') ? ATGM336H_GPS_OPERATING_MANUAL : ATGM336H_GPS_OPERATING_AUTOMATIC;
    
    /* Fix type */
    ret  = atgm336h_decode_nmea_parameter(xxgsa_msg, 2, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    switch (_num)
    {
        case 1:
        {
            decode_msg->type = ATGM336H_FIX_NOT_AVAILABLE;
            break;
        }
        case 2:
        {
            decode_msg->type = ATGM336H_FIX_2D;
            break;
        }
        case 3:
        {
            decode_msg->type = ATGM336H_FIX_3D;
            break;
        }
        default:
        {
            return ATGM336H_ERROR;
        }
    }
    
    /* Sstellite ID */
    for (satellite_index=0; satellite_index<12; satellite_index++)
    {
        ret = atgm336h_decode_nmea_parameter(xxgsa_msg, 3 + satellite_index, &parameter, &parameter_len);
        if (ret != ATGM336H_EOK)
        {
            return ATGM336H_ERROR;
        }
        if (parameter_len == 0)
        {
            decode_msg->satellite_id[satellite_index] = 0;
            continue;
        }
        
        ret = atgm336h_nmea_str2num(parameter, &_num, &float_len);
        if ((ret != ATGM336H_EOK) || (float_len != 0))
        {
            return ATGM336H_ERROR;
        }
        decode_msg->satellite_id[satellite_index] = _num;
    }
    
    /* PDOP */
    ret  = atgm336h_decode_nmea_parameter(xxgsa_msg, 15, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->pdop = _num;
    
    /* HDOP */
    ret  = atgm336h_decode_nmea_parameter(xxgsa_msg, 16, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->hdop = _num;
    
    /* VDOP */
    ret  = atgm336h_decode_nmea_parameter(xxgsa_msg, 17, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->vdop = _num;
    
    return ATGM336H_EOK;
}

/**
 * @brief       解析$XXGSV类型的NMEA消息
 * @param       xxgsv_msg : 待解析的$XXGSV类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXGSV类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXGSV类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxgsv(uint8_t *xxgsv_msg, atgm336h_nmea_gsv_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    uint16_t parameter_len;
    int32_t _num;
    uint8_t float_len;
    uint8_t msg_num;
    uint8_t msg_index;
    uint8_t *_xxgsv_msg;
    atgm336h_nmea_msg_t nmea_type;
    uint8_t satellite_index;
    
    if ((xxgsv_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxgsv_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'G') || (parameter[3] != 'S') || (parameter[4] != 'V')))
    {
        return ATGM336H_EINVAL;
    }
    if ((parameter[0] == 'G') && (parameter[1] == 'P'))
    {
        nmea_type = ATGM336H_NMEA_MSG_GPGSV;
    }
    else if ((parameter[0] == 'B') && (parameter[1] == 'D'))
    {
        nmea_type = ATGM336H_NMEA_MSG_BDGSV;
    }
    else
    {
        return ATGM336H_ERROR;
    }
    
    /* Number of message */
    ret  = atgm336h_decode_nmea_parameter(xxgsv_msg, 1, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    msg_num = _num;
    
    /* Satellite in view */
    ret  = atgm336h_decode_nmea_parameter(xxgsv_msg, 3, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0) || (_num < (msg_num - 1) * 4) || (_num > msg_num * 4))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->satellite_view = _num;
    
    if (decode_msg->satellite_view > 12)
    {
        decode_msg->satellite_view = 12;
        msg_num = (12 / 4) + (((12 % 4) == 0) ? 0 : 1);
    }
    
    for (msg_index=0; msg_index<msg_num; msg_index++)
    {
        ret = atgm336h_get_nmea_msg_from_buf(xxgsv_msg, nmea_type, msg_index + 1, &_xxgsv_msg);
        if (ret != ATGM336H_EOK)
        {
            return ATGM336H_ERROR;
        }
        
        /* Number of message */
        ret  = atgm336h_decode_nmea_parameter(_xxgsv_msg, 1, &parameter, NULL);
        ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
        if ((ret != ATGM336H_EOK) || (float_len != 0) || (_num < msg_num))
        {
            return ATGM336H_ERROR;
        }
        
        /* Sequence number */
        ret  = atgm336h_decode_nmea_parameter(_xxgsv_msg, 2, &parameter, NULL);
        ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
        if ((ret != ATGM336H_EOK) || (float_len != 0) || (_num != msg_index + 1))
        {
            return ATGM336H_ERROR;
        }
        
        /* Satellite in view */
        ret  = atgm336h_decode_nmea_parameter(_xxgsv_msg, 3, &parameter, NULL);
        ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
        if ((ret != ATGM336H_EOK) || (float_len != 0) || (_num < decode_msg->satellite_view))
        {
            return ATGM336H_ERROR;
        }
        
        
        for (satellite_index=0; satellite_index<4; satellite_index++)
        {
            /* Satellite ID */
            ret = atgm336h_decode_nmea_parameter(_xxgsv_msg, 4 + 4 * satellite_index + 0, &parameter, NULL);
            ret = atgm336h_nmea_str2num(parameter, &_num, &float_len);
            if ((ret != ATGM336H_EOK) || (float_len != 0))
            {
                return ATGM336H_ERROR;
            }
            decode_msg->satellite_info[msg_index * 4 + satellite_index].satellite_id = _num;
            
            /* Elevation */
            ret  = atgm336h_decode_nmea_parameter(_xxgsv_msg, 4 + 4 * satellite_index + 1, &parameter, NULL);
            ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
            if ((ret != ATGM336H_EOK) || (float_len != 0))
            {
                return ATGM336H_ERROR;
            }
            decode_msg->satellite_info[msg_index * 4 + satellite_index].elevation = _num;
            
            /* Azimuth */
            ret  = atgm336h_decode_nmea_parameter(_xxgsv_msg, 4 + 4 * satellite_index + 2, &parameter, NULL);
            ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
            if ((ret != ATGM336H_EOK) || (float_len != 0))
            {
                return ATGM336H_ERROR;
            }
            decode_msg->satellite_info[msg_index * 4 + satellite_index].azimuth = _num;
            
            /* SNR */
            ret = atgm336h_decode_nmea_parameter(_xxgsv_msg, 4 + 4 * satellite_index + 3, &parameter, &parameter_len);
            if (ret != ATGM336H_EOK)
            {
                return ATGM336H_ERROR;
            }
            if (parameter_len == 0)
            {
                decode_msg->satellite_info[msg_index * 4 + satellite_index].snr = 0;
                if ((msg_index == msg_num - 1) && (satellite_index == (4 - (decode_msg->satellite_view % 4)) - 1))
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            ret = atgm336h_nmea_str2num(parameter, &_num, &float_len);
            if ((ret != ATGM336H_EOK) || (float_len != 0))
            {
                return ATGM336H_ERROR;
            }
            decode_msg->satellite_info[msg_index * 4 + satellite_index].snr = _num;
            
            if ((msg_index == msg_num - 1) && (satellite_index == (4 - (decode_msg->satellite_view % 4)) - 1))
            {
                break;
            }
        }
    }
    
    return ATGM336H_EOK;
}

/**
 * @brief       解析$XXRMC类型的NMEA消息
 * @param       xxrmc_msg : 待解析的$XXRMC类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXRMC类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXRMC类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxrmc(uint8_t *xxrmc_msg, atgm336h_nmea_rmc_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    int32_t _num;
    uint8_t float_len;
    
    if ((xxrmc_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxrmc_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'R') || (parameter[3] != 'M') || (parameter[4] != 'C')))
    {
        return ATGM336H_EINVAL;
    }
    
    /* UTC Time */
    ret  = atgm336h_decode_nmea_parameter(xxrmc_msg, 1, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 3))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_time.hour = (_num / 10000000) % 100;
    decode_msg->utc_time.minute = (_num / 100000) % 100;
    decode_msg->utc_time.second = (_num / 1000) % 100;
    decode_msg->utc_time.millisecond = (_num / 1) % 1000;
    
    /* Status */
    ret = atgm336h_decode_nmea_parameter(xxrmc_msg, 2, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'V') && (parameter[0] != 'A')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->status = (parameter[0] == 'V') ? ATGM336H_NAVIGATION_WARNING : ATGM336H_NAVIGATION_VALID;
    
    /* Latitude */
    ret  = atgm336h_decode_nmea_parameter(xxrmc_msg, 3, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 5);
    decode_msg->latitude.degree = (_num / 10000000) * 100000;
    decode_msg->latitude.degree += ((_num % 10000000) / 60);
    
    /* N/S Indicator */
    ret = atgm336h_decode_nmea_parameter(xxrmc_msg, 4, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'N') && (parameter[0] != 'S')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->latitude.indicator = (parameter[0] == 'N') ? ATGM336H_LATITUDE_NORTH : ATGM336H_LATITUDE_SOUTH;
    
    /* Longitude */
    ret  = atgm336h_decode_nmea_parameter(xxrmc_msg, 5, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 5);
    decode_msg->longitude.degree = (_num / 10000000) * 100000;
    decode_msg->longitude.degree += ((_num % 10000000) / 60);
    
    /* E/W Indicator */
    ret = atgm336h_decode_nmea_parameter(xxrmc_msg, 6, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'E') && (parameter[0] != 'W')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->longitude.indicator = (parameter[0] == 'E') ? ATGM336H_LONGITUDE_EAST : ATGM336H_LONGITUDE_WEST;
    
    /* Speed over ground */
    ret  = atgm336h_decode_nmea_parameter(xxrmc_msg, 7, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->speed_ground = _num;
    
    /* Course over ground */
    ret  = atgm336h_decode_nmea_parameter(xxrmc_msg, 8, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    atgm336h_fit_num(&_num, float_len, 1);
    decode_msg->course_ground = _num;
    
    /* UTC Date */
    ret  = atgm336h_decode_nmea_parameter(xxrmc_msg, 9, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_date.year = (_num / 1) % 100 + 2000;
    decode_msg->utc_date.month = (_num / 100) % 100;
    decode_msg->utc_date.day = (_num / 10000) % 100;
    
    /* Mode indicator */
    ret = atgm336h_decode_nmea_parameter(xxrmc_msg, 12, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'N') && (parameter[0] != 'A') && (parameter[0] != 'D') && (parameter[0] != 'E')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->position_system_mode = (parameter[0] == 'N') ? ATGM336H_POS_SYS_DATA_NOT_VALID : ((parameter[0] == 'A') ? ATGM336H_POS_SYS_AUTONOMOUS : ((parameter[0] == 'D') ? ATGM336H_POS_SYS_DIFFERENTIAL : ATGM336H_POS_SYS_ESTIMATED));
    
    return ATGM336H_EOK;
}

/**
 * @brief       解析$XXVTG类型的NMEA消息
 * @param       xxvtg_msg : 待解析的$XXVTG类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXVTG类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXVTG类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxvtg(uint8_t *xxvtg_msg, atgm336h_nmea_vtg_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    uint16_t parameter_len;
    int32_t _num;
    uint8_t float_len;
    
    if ((xxvtg_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxvtg_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'V') || (parameter[3] != 'T') || (parameter[4] != 'G')))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Course */
    ret  = atgm336h_decode_nmea_parameter(xxvtg_msg, 1, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 1))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->course_true = _num;
    
    /* Course */
    ret = atgm336h_decode_nmea_parameter(xxvtg_msg, 3, &parameter, &parameter_len);
    if (ret != ATGM336H_EOK)
    {
        return ATGM336H_ERROR;
    }
    if (parameter_len == 0)
    {
        decode_msg->course_magnetic = 0;
    }
    else
    {
        ret = atgm336h_nmea_str2num(parameter, &_num, &float_len);
        if (ret != ATGM336H_EOK)
        {
            return ATGM336H_ERROR;
        }
        decode_msg->course_magnetic = _num;
    }
    
    /* Speed */
    ret  = atgm336h_decode_nmea_parameter(xxvtg_msg, 5, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 1))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->speed_knots = _num;
    
    /* Speed */
    ret  = atgm336h_decode_nmea_parameter(xxvtg_msg, 7, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 1))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->speed_kph = _num;
    
    /* Mode */
    ret = atgm336h_decode_nmea_parameter(xxvtg_msg, 9, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[0] != 'N') && (parameter[0] != 'A') && (parameter[0] != 'D') && (parameter[0] != 'E')))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->position_system_mode = (parameter[0] == 'N') ? ATGM336H_POS_SYS_DATA_NOT_VALID : ((parameter[0] == 'A') ? ATGM336H_POS_SYS_AUTONOMOUS : ((parameter[0] == 'D') ? ATGM336H_POS_SYS_DIFFERENTIAL : ATGM336H_POS_SYS_ESTIMATED));
    
    return ATGM336H_EOK;
}

/**
 * @brief       解析$XXZDA类型的NMEA消息
 * @param       xxzda_msg : 待解析的$XXZDA类型NMEA消息
 *              decode_msg: 解析后的数据
 * @retval      ATGM336H_EOK   : 解析$XXZDA类型的NMEA消息成功
 *              ATGM336H_ERROR : 解析$XXZDA类型的NMEA消息失败
 *              ATGM336H_EINVAL: 函数参数错误
 */
uint8_t atgm336h_decode_nmea_xxzda(uint8_t *xxzda_msg, atgm336h_nmea_zda_msg_t *decode_msg)
{
    uint8_t ret;
    uint8_t *parameter;
    int32_t _num;
    uint8_t float_len;
    
    if ((xxzda_msg == NULL) || (decode_msg == NULL))
    {
        return ATGM336H_EINVAL;
    }
    
    /* Address */
    ret = atgm336h_decode_nmea_parameter(xxzda_msg, 0, &parameter, NULL);
    if ((ret != ATGM336H_EOK) || ((parameter[2] != 'Z') || (parameter[3] != 'D') || (parameter[4] != 'A')))
    {
        return ATGM336H_EINVAL;
    }
    
    /* UTC Time */
    ret  = atgm336h_decode_nmea_parameter(xxzda_msg, 1, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 3))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_time.hour = (_num / 10000000) % 100;
    decode_msg->utc_time.minute = (_num / 100000) % 100;
    decode_msg->utc_time.second = (_num / 1000) % 100;
    decode_msg->utc_time.millisecond = (_num / 1) % 1000;
    
    /* UTC day */
    ret  = atgm336h_decode_nmea_parameter(xxzda_msg, 2, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_date.day = _num;
    
    /* UTC month */
    ret  = atgm336h_decode_nmea_parameter(xxzda_msg, 3, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_date.month = _num;
    
    /* UTC year */
    ret  = atgm336h_decode_nmea_parameter(xxzda_msg, 4, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->utc_date.year = _num;
    
    /* Local zone hours */
    ret  = atgm336h_decode_nmea_parameter(xxzda_msg, 5, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->local_zone_hour = _num;
    
    /* Local zone mintues */
    ret  = atgm336h_decode_nmea_parameter(xxzda_msg, 6, &parameter, NULL);
    ret += atgm336h_nmea_str2num(parameter, &_num, &float_len);
    if ((ret != ATGM336H_EOK) || (float_len != 0))
    {
        return ATGM336H_ERROR;
    }
    decode_msg->local_zone_minute = _num;
    
    return ATGM336H_EOK;
}
