/********************************************************************************
* @File name  : GPS模块
* @Description: 串口6-对应GPS
* @Author     : ZHLE
*  Version Date        Modification Description
	13、GPS(4G模块)： 串口6， 波特率：9600，引脚分配为： 
	      BDS_TX：    PC6
        BDS_RX：    PC7	
********************************************************************************/

#include "./DRIVER/inc/ATGM336H.h"
#include "appconfig.h"

/* 参数 */
#define GPS_BUFF_LEN  2048

uint8_t g_gps_buffer[GPS_BUFF_LEN];
atgm336h_data_t sg_atgm336h_param_t; // 定位信息

/*
*********************************************************************************************************
*	函 数 名: gps_get_data
*	功能说明: 获取数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void gps_get_data(uint8_t *buff, uint16_t len)
{
	if(sg_atgm336h_param_t.status == 0)
	{
		memcpy(g_gps_buffer,buff,len);
		sg_atgm336h_param_t.status = 1;
	}
}
/*
*********************************************************************************************************
*	函 数 名: atgm336h_decode_nmea_xxgga
*	功能说明: 解析$XXGGA类型的NMEA消息
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t atgm336h_decode_nmea_xxgga(void)
{
	uint8_t ret = 0;
	if(sg_atgm336h_param_t.status == 1)
	{		
		sg_atgm336h_param_t.is_valid = 1;
		
		// Find GNGGA sentence (GNSS combined positioning data including GPS and BDS)
		char *gga_sentence = strstr((char*)g_gps_buffer, "$GNGGA");
		if (gga_sentence == NULL) {
			// If GNGGA not found, try to find GPGGA
			gga_sentence = strstr((char*)g_gps_buffer, "$GPGGA");
			if (gga_sentence == NULL) {
				// If still not found, try to find BDGA (BDS-only GGA)
				gga_sentence = strstr((char*)g_gps_buffer, "$BDGGA");
				if (gga_sentence == NULL) {
					ret = 1;
				}
			}
		}
		// The rest of the parsing logic remains the same for all GGA variants
		// Split each field of GGA sentence
		char *token = strtok(gga_sentence, ",");
		uint8_t field_index = 0;
		char latitude_str[20] = {0};
		char longitude_str[20] = {0};
		char altitude_str[20] = {0};
		char hdop_str[20] = {0};
		char satellites_str[5] = {0};
		char fix_quality_str[5] = {0};
		
		while (token != NULL && field_index <= 10) {
			switch (field_index) {
			case 1: // Time (HHMMSS format)
				break;
			case 2: // Latitude (ddmm.mmmm format)
				strncpy(latitude_str, token, sizeof(latitude_str) - 1);
				break;
			case 3: // Latitude direction (N/S)
				sg_atgm336h_param_t.lat_dir = token[0];
				break;
			case 4: // Longitude (dddmm.mmmm format)
				strncpy(longitude_str, token, sizeof(longitude_str) - 1);
				break;
			case 5: // Longitude direction (E/W)
				sg_atgm336h_param_t.lon_dir = token[0];
				break;
			case 6: // Fix quality indicator
				strncpy(fix_quality_str, token, sizeof(fix_quality_str) - 1);
				break;
			case 7: // Number of satellites in use
				strncpy(satellites_str, token, sizeof(satellites_str) - 1);
				break;
			case 8: // Horizontal dilution of precision
				strncpy(hdop_str, token, sizeof(hdop_str) - 1);
				break;
			case 9: // Altitude
				strncpy(altitude_str, token, sizeof(altitude_str) - 1);
				break;
			case 10:  
				break;
			}
			token = strtok(NULL, ",");
			field_index++;
		}
		
		// Convert fix quality
		if (strlen(fix_quality_str) > 0) {
			sg_atgm336h_param_t.fix_quality = atoi(fix_quality_str);
		}
		
		// Convert number of satellites
		if (strlen(satellites_str) > 0) {
			sg_atgm336h_param_t.num_satellites = atoi(satellites_str);
		}
		
		// Convert horizontal dilution of precision
		if (strlen(hdop_str) > 0) {
			sg_atgm336h_param_t.hdop = atof(hdop_str);
		}
		
		// Convert altitude
		if (strlen(altitude_str) > 0) {
			sg_atgm336h_param_t.altitude = atof(altitude_str);
		}
		
		// Convert latitude (from ddmm.mmmm format to decimal format)
		if (strlen(latitude_str) > 0) {
			char deg_str[4] = {0};
			char min_str[10] = {0};
			int deg = 0;
			double min = 0.0;
			
			// For latitude, first two digits are degrees, the rest are minutes
			strncpy(deg_str, latitude_str, 2);
			deg = atoi(deg_str);
			strcpy(min_str, latitude_str + 2);
			min = atof(min_str);
			
			// Convert to decimal format
			sg_atgm336h_param_t.latitude = deg + min / 60.0;
			
			// Adjust sign according to direction
			if (sg_atgm336h_param_t.lat_dir == 'S') {
				sg_atgm336h_param_t.latitude = -sg_atgm336h_param_t.latitude;
			}
		}
		
		// Convert longitude (from dddmm.mmmm format to decimal format)
		if (strlen(longitude_str) > 0) {
			char deg_str[5] = {0};
			char min_str[10] = {0};
			int deg = 0;
			double min = 0.0;
			
			// For longitude, first three digits are degrees, the rest are minutes
			strncpy(deg_str, longitude_str, 3);
			deg = atoi(deg_str);
			strcpy(min_str, longitude_str + 3);
			min = atof(min_str);
			
			// Convert to decimal format
			sg_atgm336h_param_t.longitude = deg + min / 60.0;
			
			// Adjust sign according to direction
			if (sg_atgm336h_param_t.lon_dir == 'W') {
				sg_atgm336h_param_t.longitude = -sg_atgm336h_param_t.longitude;
			}
		}
		
		// Only consider data valid when fix quality is greater than 0
		if (sg_atgm336h_param_t.fix_quality > 0) {
			sg_atgm336h_param_t.is_valid = 0;
		}
		ret = 0;
	}
	else
	{
		ret = 2;
	}
	sg_atgm336h_param_t.status = 0;
	memset(g_gps_buffer,0,GPS_BUFF_LEN);
	return ret;
}

/*
*********************************************************************************************************
*	函 数 名: atgm336h_decode_nmea_xxgga
*	功能说明: 解析$XXGGA类型的NMEA消息
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
atgm336h_data_t* atgm336h_get_gnss_data(void)
{
	return &sg_atgm336h_param_t;
}

/*
*********************************************************************************************************
*	函 数 名: ATGM338H_test
*	功能说明: 测试
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void ATGM338H_test(void)
{
	while(1)
	{
		atgm336h_decode_nmea_xxgga();
		
		if (sg_atgm336h_param_t.is_valid == 0) {

			printf("\n=== GNSS Data Parsing Result ===\n");
			printf("Fix Status: Valid\n");
			printf("Fix Quality: %d (0=Invalid, 1=GPS Fix, 2=DGPS Fix, 3=PPS Fix, 4=RTK, 5=Float RTK)\n", sg_atgm336h_param_t.fix_quality);
			printf("Number of Satellites: %d\n", sg_atgm336h_param_t.num_satellites);
			printf("Horizontal Dilution of Precision: %.2f\n", sg_atgm336h_param_t.hdop);
			printf("Latitude: %.10f %c\n", fabs(sg_atgm336h_param_t.latitude), sg_atgm336h_param_t.lat_dir);
			printf("Longitude: %.10f %c\n", fabs(sg_atgm336h_param_t.longitude), sg_atgm336h_param_t.lon_dir);
			printf("Decimal Latitude: %.10f\n", sg_atgm336h_param_t.latitude);
			printf("Decimal Longitude: %.10f\n", sg_atgm336h_param_t.longitude);
			printf("Altitude: %.10f\n", sg_atgm336h_param_t.altitude);
			printf("==============================\n\n");

		} else {

			printf("\n=== GNSS Data Parsing Result ===\n");
			printf("Fix Status: Invalid\n");
			printf("No valid positioning data found. Please ensure the device is connected to satellites and working properly\n");
			printf("==============================\n\n");

		}
		delay_ms(1000);
	}
}










