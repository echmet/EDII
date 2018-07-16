#ifndef ECHMET_EDII_IPC_NETWORK_H
#define ECHMET_EDII_IPC_NETWORK_H

#ifdef ECHMET_COMPILER_MSVC
  #define EDII_PACKED_STRUCT_BEGIN __pragma(pack(push, 1)) struct
  #define EDII_PACKED_STRUCT_END __pragma(pack(pop))
#elif defined(ECHMET_COMPILER_GCC_LIKE) || defined(ECHMET_COMPILER_MINGW) || defined(ECHMET_COMPILER_MSYS)
  #define EDII_PACKED_STRUCT_BEGIN struct  __attribute__((packed))
  #define EDII_PACKED_STRUCT_END
#endif

#include "edii_ipc_common.h"

#include <stdint.h>

#define EDII_IPCS_SOCKET_NAME "echmet_edii_socket"

static const uint16_t EDII_IPCS_PACKET_MAGIC = 0x091E;

enum EDII_IPCSockRequestType {
  EDII_REQUEST_SUPPORTED_FORMATS = 0x1,
  EDII_REQUEST_LOAD_DATA = 0x2,
  EDII_REQUEST_LOAD_DATA_DESCRIPTOR = 0x3,
  EDII_REQUEST_ABI_VERSION = 0x4
};

enum EDII_IPCSockResult {
  EDII_IPCS_SUCCESS = 0x1,
  EDII_IPCS_FAILURE = 0x2
};

enum EDII_IPCSockResponseType {
  EDII_RESPONSE_SUPPORTED_FORMAT_HEADER = 0x1,
  EDII_RESPONSE_LOAD_DATA_HEADER = 0x2,
  EDII_RESPONSE_SUPPORTED_FORMAT_DESCRIPTOR = 0x3,
  EDII_RESPONSE_LOAD_DATA_DESCRIPTOR = 0x4,
  EDII_RESPONSE_LOAD_OPTION_DESCRIPTOR = 0x5,
  EDII_RESPONSE_ABI_VERSION = 0x6
};

enum EDII_IPCSocketLoadDataMode {
  EDII_IPCS_LOAD_INTERACTIVE = 0x1,
  EDII_IPCS_LOAD_HINT = 0x2,
  EDII_IPCS_LOAD_FILE = 0x3
};

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockRequestHeader {
  uint16_t magic;
  uint8_t requestType;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockResponseHeader {
  uint16_t magic;
  uint8_t responseType;
  uint8_t status;

  int32_t items;
  uint32_t errorLength;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockLoadDataRequestDescriptor {
  uint16_t magic;
  uint8_t requestType;
  uint8_t mode;

  int32_t loadOption;
  uint32_t tagLength;
  uint32_t filePathLength;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockSupportedFormatResponseDescriptor {
  uint16_t magic;
  uint8_t responseType;
  uint8_t status;

  uint32_t longDescriptionLength;
  uint32_t shortDescriptionLength;
  uint32_t tagLength;
  uint32_t loadOptionsLength;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockLoadOptionDescriptor {
  uint16_t magic;
  uint8_t responseType;
  uint8_t status;

  uint32_t optionLength;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockLoadDataResponseDescriptor {
  uint16_t magic;
  uint8_t responseType;
  uint8_t status;

  uint32_t nameLength;
  uint32_t dataIdLength;
  uint32_t pathLength;
  uint32_t xDescriptionLength;
  uint32_t yDescriptionLength;
  uint32_t xUnitLength;
  uint32_t yUnitLength;
  uint32_t datapointsLength;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockDatapoint {
  double x;
  double y;
};
EDII_PACKED_STRUCT_END

EDII_PACKED_STRUCT_BEGIN EDII_IPCSockResponseABIVersion {
	uint16_t magic;
	uint8_t responseType;
	uint8_t status;

	int32_t major;
	int32_t minor;
};
EDII_PACKED_STRUCT_END

#endif // ECHMET_EDII_IPC_NETWORK_H
