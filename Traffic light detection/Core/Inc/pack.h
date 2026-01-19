#ifndef  _PACK_H_
#define  _PACK_H_


uint8_t crc8_calculate(const uint8_t *data, uint32_t len);
uint32_t pack_data(uint8_t cmd, const data_collection_t *data, uint8_t *output_buffer);


#endif