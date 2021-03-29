//
// Created by raigo on 27.03.21.
//

#ifndef PROTOCOL_TESTBED_FILESENDHEADER_H
#define PROTOCOL_TESTBED_FILESENDHEADER_H

#define ISE_PROTOCOL_OFFSET 0
#define ISE_PROTOCOL_SIZE 2
#define ISE_PROTOCOL_VERSION_OFFSET ISE_PROTOCOL_OFFSET + ISE_PROTOCOL_SIZE
#define ISE_PROTOCOL_VERSION_SIZE 2
#define ISE_CHECKSUM_OFFSET ISE_PROTOCOL_VERSION_OFFSET + ISE_PROTOCOL_VERSION_SIZE
#define ISE_CHECKSUM_SIZE 32
#define ISE_FILE_SIZE_OFFSET ISE_CHECKSUM_OFFSET + ISE_CHECKSUM_SIZE
#define ISE_FILE_SIZE 8
#define ISE_FILE_NAME_OFFSET ISE_FILE_SIZE_OFFSET + ISE_FILE_SIZE
#define ISE_FILE_NAME_SIZE 1024

#define ISE_HEADER_SIZE ISE_PROTOCOL_SIZE + ISE_PROTOCOL_VERSION_SIZE + ISE_CHECKSUM_SIZE + ISE_FILE_SIZE + ISE_FILE_NAME_SIZE
#define ISE_DATA_OFFSET ISE_PROTOCOL_OFFSET + ISE_HEADER_SIZE

#endif //PROTOCOL_TESTBED_FILESENDHEADER_H
