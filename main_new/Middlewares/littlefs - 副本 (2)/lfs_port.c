#include "lfs.h"
#include "lfs_port.h"
#include "bsp.h"

/**
 * lfs与底层flash读数据接口
 * @param  c
 * @param  block  块编号
 * @param  off    块内偏移地址
 * @param  buffer 用于存储读取到的数据
 * @param  size   要读取的字节数
 * @return
 */
int block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	sf_ReadBuffer((uint8_t *)buffer, (block*(c->block_size))+off, size);
	return 0;
}

 
/**
 * lfs与底层flash写数据接口
 * @param  c
 * @param  block  块编号
 * @param  off    块内偏移地址
 * @param  buffer 待写入的数据
 * @param  size   待写入数据的大小
 * @return
 */
int block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	sf_WriteBuffer((uint8_t *)buffer, (block*(c->block_size))+off, size);
	return 0;
}
 
/**
 * lfs与底层flash擦除接口
 * @param  c
 * @param  block 块编号
 * @return
 */
int block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
	sf_EraseSector(block * c->block_size);
	return 0;
}
 
int block_device_sync(const struct lfs_config *c)
{
	return 0;
}
 
// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = 
{
    // block device operations
    .read  = block_device_read,
    .prog  = block_device_prog,
    .erase = block_device_erase,
    .sync  = block_device_sync,

    // block device configuration
    .read_size = 256,
    .prog_size = 256,
    .block_size  = 4096,
    .block_count = 2048, /* 25Q16 共计 16Mbit，即2MB, 4K一个扇区，共计 2*1024*1024/4096 = 512 个扇区 */
						            /* 25Q128 共计 128Mbit，即16MB, 4K一个扇区，共计 16*1024*1024/4096 = 4096 个扇区 */
    .lookahead 	 = 128,

};

// variables used by the filesystem
lfs_t 		g_lfs_t;

// entry point
int lfs_init_function(void) 
{
	lfs_file_t  lfs_file;
	/* 挂载设备 */
	int err = lfs_mount(&g_lfs_t, &cfg);

	if (err) 
	{
		err = lfs_format(&g_lfs_t, &cfg);
		err = lfs_mount(&g_lfs_t, &cfg);
		if (err) 
		{
			/* 格式化flash */
			sf_EraseChip();

			err = lfs_format(&g_lfs_t, &cfg);
			err = lfs_mount(&g_lfs_t, &cfg);
		}
  }
	
	// read current count
	uint32_t boot_count = 0;
	lfs_file_open(&g_lfs_t, &lfs_file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
	lfs_file_read(&g_lfs_t, &lfs_file, &boot_count, sizeof(boot_count));

	// update boot count
	boot_count += 1;

	lfs_file_rewind(&g_lfs_t, &lfs_file);
	lfs_file_write(&g_lfs_t, &lfs_file, &boot_count, sizeof(boot_count));
	
  printf("boot_count: %d\n", boot_count);
	// remember the storage is not updated until the file is closed successfully
	lfs_file_close(&g_lfs_t, &lfs_file);
	return 0;
}



int lfs_test(void)
{
	int err = 0;
//	lfs_file_t lfs_fp;
//	// read current count
//    uint32_t boot_count = 0;
//	uint32_t boot_count1 = 0;
//	
//    err = lfs_file_open(&lfs, &lfs_file, "boot_count.txt", LFS_O_RDWR | LFS_O_CREAT);
//	err = lfs_file_open(&lfs, &lfs_fp, "boot_test.txt", LFS_O_RDWR | LFS_O_CREAT);
//    err = lfs_file_read(&lfs, &lfs_file, &boot_count, sizeof(boot_count));
//	err = lfs_file_read(&lfs, &lfs_fp, &boot_count1, sizeof(boot_count1));
//	
//    // update boot count
//    boot_count += 1;
//	boot_count1 = boot_count%2 + boot_count;
//	
//    err = lfs_file_rewind(&lfs, &lfs_file);
//    err = lfs_file_write(&lfs, &lfs_file, &boot_count, sizeof(boot_count));
//	err = lfs_file_rewind(&lfs, &lfs_fp);
//	err = lfs_file_write(&lfs, &lfs_fp, &boot_count1, sizeof(boot_count1));
//	
//    // remember the storage is not updated until the file is closed successfully
//    err = lfs_file_close(&lfs, &lfs_file);
//	err = lfs_file_close(&lfs, &lfs_fp);
//	
//    // release any resources we were using
//    err = lfs_unmount(&lfs);
	
	return err;

}














