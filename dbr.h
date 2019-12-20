#ifndef DBR_H
#define DBR_H

typedef struct {
    unsigned table_count;   /* FAT表个数 */
    unsigned section_size;  /* 每扇区字节数 */
    unsigned table_section_count; /* FAT表扇区数 */
    unsigned reserved_section_count; /* 保留扇区数 */
    unsigned cluster_size; /* 每簇扇区数 */
    unsigned root_cluster; /* 根目录簇序号 */
    long cluster_count; /* 簇个数 */
} DBR;

#endif // DBR_H
