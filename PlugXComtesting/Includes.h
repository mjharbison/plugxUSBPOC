#pragma once
//typedef unsigned long NTSTATUS;

#define STATUS_SUCCESS                 ((NTSTATUS)0x00000000UL)
#define STATUS_BUFFER_ALL_ZEROS        ((NTSTATUS)0x00000117UL)
//#define STATUS_INVALID_PARAMETER       ((NTSTATUS)0xC000000DUL)
#define STATUS_UNSUPPORTED_COMPRESSION ((NTSTATUS)0xC000025FUL)
#define STATUS_NOT_SUPPORTED_ON_SBS    ((NTSTATUS)0xC0000300UL)
#define STATUS_BUFFER_TOO_SMALL        ((NTSTATUS)0xC0000023UL)
#define STATUS_BAD_COMPRESSION_BUFFER  ((NTSTATUS)0xC0000242UL)

HMODULE ntdll = GetModuleHandle(L"ntdll.dll");

typedef NTSTATUS(__stdcall* _RtlCompressBuffer)(
	USHORT CompressionFormatAndEngine,
	PUCHAR UncompressedBuffer,
	ULONG UncompressedBufferSize,
	PUCHAR CompressedBuffer,
	ULONG CompressedBufferSize,
	ULONG UncompressedChunkSize,
	PULONG FinalCompressedSize,
	PVOID WorkSpace
	);

typedef NTSTATUS(__stdcall* _RtlDecompressBuffer)(
	USHORT CompressionFormat,
	PUCHAR UncompressedBuffer,
	ULONG UncompressedBufferSize,
	PUCHAR CompressedBuffer,
	ULONG CompressedBufferSize,
	PULONG FinalUncompressedSize
	);

typedef NTSTATUS(__stdcall* _RtlGetCompressionWorkSpaceSize)(
	USHORT CompressionFormatAndEngine,
	PULONG CompressBufferWorkSpaceSize,
	PULONG CompressFragmentWorkSpaceSize
	);


//typedef const UNICODE_STRING* PCUNICODE_STRING;
void Status_Error_Code(NTSTATUS ntstatus)
{
	switch (ntstatus) {
	case STATUS_BUFFER_ALL_ZEROS:
		printf("Error: STATUS_BUFFER_ALL_ZEROS\n");
		break;
	case STATUS_INVALID_PARAMETER:
		printf("Error: STATUS_INVALID_PARAMETER\n");
		break;
	case STATUS_UNSUPPORTED_COMPRESSION:
		printf("Error: STATUS_UNSUPPORTED_COMPRESSION\n");
		break;
	case STATUS_NOT_SUPPORTED_ON_SBS:
		printf("Error: STATUS_NOT_SUPPORTED_ON_SBS\n");
		break;
	case STATUS_BUFFER_TOO_SMALL:
		printf("Error: STATUS_BUFFER_TOO_SMALL\n");
		break;
	case STATUS_BAD_COMPRESSION_BUFFER:
		printf("Error: STATUS_BAD_COMPRESSION_BUFFER\n");
		break;
	default:
		printf("Error is Unknown!\n");
	}
}