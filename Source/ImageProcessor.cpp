#include "ImageProcessor.h"

/* Shader Source
struct PushConstant
{
	float4x4 colorMatrix;
	float gamma;
};
[[vk::push_constant]] ConstantBuffer<PushConstant> Param : register(b0, space0);
[[vk::binding(0, 0)]] RWTexture2D<float4> mainTexture : register(u0, space0);

[numthreads(8, 8, 1)]
void main(uint2 pixelPos : SV_DispatchThreadID)
{
	float4 color = mainTexture[pixelPos];
	color = mul(Param.colorMatrix, color);
	color.rgb = pow(color.rgb, Param.gamma);
	mainTexture[pixelPos] = saturate(color);
}
*/

const static uint8_t ImageProcessor_spv[1268] =
{
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e,
	0x00, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00,
	0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64,
	0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00,
	0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x6d,
	0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0xa8,
	0x02, 0x00, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x74, 0x79, 0x70, 0x65, 0x2e, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61,
	0x6e, 0x74, 0x42, 0x75, 0x66, 0x66, 0x65, 0x72, 0x2e, 0x50, 0x75,
	0x73, 0x68, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x00,
	0x00, 0x00, 0x00, 0x06, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x4d, 0x61,
	0x74, 0x72, 0x69, 0x78, 0x00, 0x06, 0x00, 0x05, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x67, 0x61, 0x6d, 0x6d, 0x61,
	0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00,
	0x50, 0x61, 0x72, 0x61, 0x6d, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06,
	0x00, 0x06, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x32,
	0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
	0x54, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x00, 0x05, 0x00, 0x04,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00,
	0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0x07, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x47, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x21, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x10, 0x00,
	0x00, 0x00, 0x48, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x2b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x08, 0x00,
	0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x03, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x2b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0d, 0x00,
	0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x2c,
	0x00, 0x07, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
	0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
	0x00, 0x0c, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0b, 0x00,
	0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2c,
	0x00, 0x07, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00,
	0x00, 0x0f, 0x00, 0x00, 0x00, 0x18, 0x00, 0x04, 0x00, 0x11, 0x00,
	0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1e,
	0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
	0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00,
	0x00, 0x09, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x19, 0x00,
	0x09, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x16, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x13,
	0x00, 0x02, 0x00, 0x17, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04,
	0x00, 0x19, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
	0x1b, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
	0x00, 0x20, 0x00, 0x04, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x09, 0x00,
	0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x16, 0x00,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36,
	0x00, 0x05, 0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02,
	0x00, 0x1d, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x15, 0x00,
	0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x4f,
	0x00, 0x07, 0x00, 0x19, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x06, 0x00,
	0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x62,
	0x00, 0x06, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x41, 0x00, 0x05, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x22, 0x00,
	0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x3d,
	0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
	0x22, 0x00, 0x00, 0x00, 0x90, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00,
	0x00, 0x24, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x23, 0x00,
	0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x25,
	0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x41, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x26, 0x00,
	0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x3d,
	0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00,
	0x26, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x1b, 0x00, 0x00,
	0x00, 0x28, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x27, 0x00,
	0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x07, 0x00, 0x1b,
	0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x1a, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00,
	0x00, 0x4f, 0x00, 0x09, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x2a, 0x00,
	0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x08, 0x00, 0x0d, 0x00, 0x00,
	0x00, 0x2b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00,
	0x00, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x2c, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x63, 0x00, 0x05,
	0x00, 0x2c, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x2b, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38,
	0x00, 0x01, 0x00
};

const static uint8_t ImageProcessor_dxil[4228] =
{
	0x44, 0x58, 0x42, 0x43, 0xba, 0xea, 0xa1, 0x4b, 0x79, 0xce, 0x16,
	0x47, 0x4d, 0xa3, 0x12, 0x5d, 0xdf, 0x2a, 0x46, 0x58, 0x01, 0x00,
	0x00, 0x00, 0x84, 0x10, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3c,
	0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00,
	0x6c, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0xec, 0x07, 0x00,
	0x00, 0x08, 0x08, 0x00, 0x00, 0x53, 0x46, 0x49, 0x30, 0x08, 0x00,
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49,
	0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x53,
	0x56, 0x30, 0x80, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x18, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
	0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x53, 0x54, 0x41, 0x54, 0xf0, 0x06, 0x00, 0x00, 0x68,
	0x00, 0x05, 0x00, 0xbc, 0x01, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c,
	0x08, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0xd8, 0x06, 0x00,
	0x00, 0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00, 0xb3, 0x01,
	0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49,
	0x06, 0x10, 0x32, 0x39, 0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08,
	0x19, 0x1e, 0x04, 0x8b, 0x62, 0x80, 0x18, 0x45, 0x02, 0x42, 0x92,
	0x0b, 0x42, 0xc4, 0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a,
	0x32, 0x62, 0x88, 0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5,
	0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90, 0x11, 0x23, 0xc4,
	0x50, 0x41, 0x51, 0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x31,
	0x46, 0x06, 0x51, 0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1b,
	0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x40, 0x02, 0xa8, 0x0d,
	0x86, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x03, 0x20, 0x01, 0xd5, 0x06,
	0x62, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x01, 0x90, 0x00, 0x49, 0x18,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20,
	0x4c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00,
	0x50, 0x00, 0x00, 0x00, 0x32, 0x22, 0x88, 0x09, 0x20, 0x64, 0x85,
	0x04, 0x13, 0x23, 0xa4, 0x84, 0x04, 0x13, 0x23, 0xe3, 0x84, 0xa1,
	0x90, 0x14, 0x12, 0x4c, 0x8c, 0x8c, 0x0b, 0x84, 0xc4, 0x4c, 0x10,
	0x98, 0xc1, 0x08, 0x40, 0x09, 0x00, 0x0a, 0xe6, 0x08, 0xc0, 0xa0,
	0x0c, 0xc3, 0x30, 0x10, 0x31, 0x03, 0x70, 0xd3, 0x70, 0xf9, 0x13,
	0xf6, 0x10, 0x92, 0xbf, 0x12, 0xd2, 0x4a, 0x4c, 0x7e, 0x71, 0xdb,
	0xa8, 0x30, 0x0c, 0xc3, 0x18, 0xe6, 0x08, 0x10, 0x42, 0xee, 0x19,
	0x2e, 0x7f, 0xc2, 0x1e, 0x42, 0xf2, 0x43, 0xa0, 0x19, 0x16, 0x02,
	0x05, 0x49, 0x51, 0x8e, 0x41, 0x19, 0x86, 0x61, 0x18, 0x86, 0x81,
	0x96, 0xb2, 0x00, 0x83, 0x32, 0x0c, 0x83, 0x61, 0x18, 0x06, 0x42,
	0x4d, 0x19, 0x8c, 0xc1, 0xa0, 0xe7, 0xa8, 0xe1, 0xf2, 0x27, 0xec,
	0x21, 0x24, 0x9f, 0xdb, 0xa8, 0x62, 0x25, 0x26, 0xbf, 0xb8, 0x6d,
	0x44, 0x18, 0x86, 0x61, 0x14, 0x22, 0x1a, 0x94, 0x81, 0xa4, 0x52,
	0x18, 0x83, 0x61, 0x18, 0x44, 0xdd, 0x36, 0x5c, 0xfe, 0x84, 0x3d,
	0x84, 0xe4, 0xaf, 0x84, 0xe4, 0x50, 0x91, 0x40, 0xa4, 0x91, 0xf3,
	0x10, 0xd1, 0x84, 0x10, 0x12, 0x12, 0x86, 0xa1, 0x10, 0xca, 0xa0,
	0x58, 0x74, 0x1d, 0x34, 0x5c, 0xfe, 0x84, 0x3d, 0x84, 0xe4, 0xaf,
	0x84, 0xb4, 0x21, 0xcd, 0x80, 0x88, 0x61, 0x18, 0x90, 0x39, 0x82,
	0xa0, 0x14, 0xca, 0x90, 0x0d, 0x1a, 0x6d, 0x03, 0x01, 0xc3, 0x08,
	0x84, 0x31, 0x13, 0x19, 0x8c, 0x03, 0x3b, 0x84, 0xc3, 0x3c, 0xcc,
	0x83, 0x1b, 0xc8, 0xc2, 0x2d, 0xd0, 0x42, 0x39, 0xe0, 0x03, 0x3d,
	0xd4, 0x83, 0x3c, 0x94, 0x83, 0x1c, 0x90, 0x02, 0x1f, 0xd8, 0x43,
	0x39, 0x8c, 0x03, 0x3d, 0xbc, 0x83, 0x3c, 0xf0, 0x81, 0x39, 0xb0,
	0xc3, 0x3b, 0x84, 0x03, 0x3d, 0xb0, 0x01, 0x18, 0xd0, 0x81, 0x1f,
	0x80, 0x81, 0x1f, 0xa0, 0xc0, 0xa3, 0x2f, 0x09, 0xbc, 0xf3, 0x0e,
	0x47, 0x9a, 0x16, 0x00, 0x73, 0xa8, 0xc9, 0x97, 0xa6, 0x88, 0x12,
	0x26, 0x3f, 0xa5, 0xa4, 0x83, 0x73, 0x1a, 0x69, 0x02, 0x9a, 0x09,
	0x09, 0xa1, 0x71, 0xd0, 0xe1, 0x48, 0xd3, 0x02, 0x60, 0x0e, 0x35,
	0xf9, 0x29, 0x10, 0x01, 0x0c, 0x0a, 0x44, 0x1a, 0xe7, 0x08, 0x40,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x72, 0xc0, 0x87, 0x74,
	0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0, 0x87,
	0x0d, 0xaf, 0x50, 0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d,
	0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07,
	0x6d, 0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90,
	0x0e, 0x78, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71,
	0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07,
	0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40,
	0x07, 0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76,
	0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07,
	0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0,
	0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71,
	0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07,
	0x43, 0x9e, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x86, 0x3c, 0x04, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x16, 0x20, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf2, 0x34, 0x40, 0x00,
	0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xe4, 0x79,
	0x80, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60,
	0xc8, 0x23, 0x01, 0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xc0, 0x90, 0x87, 0x02, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x80, 0x21, 0xcf, 0x05, 0x04, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x9e, 0x0d, 0x08, 0x80,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb2, 0x40, 0x00,
	0x00, 0x10, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14, 0x19, 0x11,
	0x4c, 0x90, 0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x4a,
	0xa0, 0x20, 0x46, 0x00, 0x8a, 0xa1, 0x08, 0x4a, 0xa2, 0x10, 0x03,
	0x0a, 0xa1, 0x0c, 0xca, 0xa1, 0x00, 0x03, 0x4a, 0x84, 0xb4, 0x11,
	0x00, 0x2a, 0x0b, 0x10, 0x30, 0x10, 0x10, 0x88, 0xc4, 0x19, 0x00,
	0x1a, 0x67, 0x00, 0x88, 0x9c, 0x01, 0x20, 0x70, 0x06, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00,
	0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0xc4, 0x8e, 0x0c, 0x6f,
	0xec, 0xed, 0x4d, 0x0c, 0x24, 0xc6, 0x05, 0xc7, 0x45, 0xa6, 0x06,
	0x46, 0xc6, 0x05, 0x07, 0x04, 0x45, 0x8c, 0xe6, 0x26, 0x26, 0x06,
	0x67, 0x26, 0xa7, 0x2c, 0x65, 0x43, 0x10, 0x4c, 0x10, 0x86, 0x64,
	0x82, 0x30, 0x28, 0x1b, 0x84, 0x81, 0x98, 0x20, 0x0c, 0xcb, 0x06,
	0xc1, 0x30, 0x28, 0x8c, 0xcd, 0x4d, 0x10, 0x06, 0x66, 0xc3, 0x80,
	0x24, 0xc4, 0x04, 0x01, 0x0c, 0x34, 0x2e, 0x6d, 0x61, 0x69, 0x6e,
	0x54, 0x65, 0x78, 0x74, 0x75, 0x72, 0x65, 0x13, 0x84, 0xa1, 0x99,
	0x20, 0x68, 0xd5, 0x04, 0x61, 0x70, 0x36, 0x08, 0xc6, 0xb3, 0x61,
	0x31, 0x16, 0xc6, 0x30, 0x86, 0xc6, 0x71, 0x1c, 0x68, 0x43, 0x10,
	0x4d, 0x10, 0xc8, 0x20, 0x63, 0x01, 0x15, 0x26, 0x17, 0xd6, 0x36,
	0x41, 0x18, 0x9e, 0x0d, 0x88, 0x31, 0x51, 0x86, 0x31, 0x54, 0xc0,
	0x86, 0xc0, 0xda, 0x40, 0x00, 0xd2, 0x05, 0x4c, 0x10, 0xc4, 0xe0,
	0xe2, 0x32, 0xf6, 0xc6, 0xf6, 0x26, 0xd7, 0x14, 0x46, 0x27, 0x97,
	0x86, 0x37, 0x41, 0x18, 0xa0, 0x0d, 0xc3, 0xb6, 0x35, 0x13, 0x84,
	0x21, 0x9a, 0x20, 0x0c, 0xd2, 0x06, 0x24, 0xd1, 0x1a, 0xae, 0x33,
	0xbc, 0x87, 0xc5, 0x59, 0x58, 0x5b, 0x5b, 0xd8, 0x04, 0x61, 0x98,
	0x36, 0x18, 0x09, 0x18, 0x74, 0x61, 0xe0, 0x3d, 0x1b, 0x86, 0xea,
	0x13, 0x83, 0x09, 0xc2, 0x18, 0x60, 0x1b, 0x88, 0x84, 0xea, 0x8c,
	0x0d, 0x42, 0x55, 0x06, 0x1b, 0x0a, 0x23, 0x1b, 0x03, 0x32, 0x30,
	0x83, 0x09, 0x82, 0x00, 0x6c, 0x00, 0x36, 0x0c, 0x46, 0x1a, 0xa4,
	0xc1, 0x86, 0x40, 0x0d, 0x36, 0x0c, 0x03, 0x1a, 0xac, 0x01, 0x89,
	0xb6, 0xb0, 0x34, 0xb7, 0x09, 0x42, 0x19, 0x58, 0x1b, 0x06, 0x82,
	0x18, 0x36, 0x10, 0x86, 0x1b, 0x6c, 0x6f, 0xb0, 0xa1, 0x40, 0x83,
	0x36, 0x00, 0x30, 0x38, 0xa0, 0x61, 0xc6, 0xf6, 0x16, 0x46, 0x37,
	0x37, 0x41, 0x18, 0x28, 0x16, 0x69, 0x6e, 0x73, 0x74, 0x73, 0x44,
	0xe8, 0xca, 0xf0, 0xbe, 0xd8, 0xde, 0xc2, 0xc8, 0x98, 0xd0, 0x95,
	0xe1, 0x7d, 0xcd, 0xd1, 0xbd, 0xc9, 0x95, 0x6d, 0x40, 0xe4, 0x60,
	0x0e, 0xe8, 0x20, 0x0c, 0xea, 0x60, 0xb0, 0x83, 0xa1, 0x0a, 0x1b,
	0x9b, 0x5d, 0x9b, 0x4b, 0x1a, 0x59, 0x99, 0x1b, 0xdd, 0x94, 0x20,
	0xa8, 0x42, 0x86, 0xe7, 0x62, 0x57, 0x26, 0x37, 0x97, 0xf6, 0xe6,
	0x36, 0x25, 0x20, 0x9a, 0x90, 0xe1, 0xb9, 0xd8, 0x85, 0xb1, 0xd9,
	0x95, 0xc9, 0x4d, 0x09, 0x8c, 0x3a, 0x64, 0x78, 0x2e, 0x73, 0x68,
	0x61, 0x64, 0x65, 0x72, 0x4d, 0x6f, 0x64, 0x65, 0x6c, 0x53, 0x82,
	0xa4, 0x0c, 0x19, 0x9e, 0x8b, 0x5c, 0xd9, 0xdc, 0x5b, 0x9d, 0xdc,
	0x58, 0xd9, 0xdc, 0x94, 0xe0, 0xaa, 0x44, 0x86, 0xe7, 0x42, 0x97,
	0x07, 0x57, 0x16, 0xe4, 0xe6, 0xf6, 0x46, 0x17, 0x46, 0x97, 0xf6,
	0xe6, 0x36, 0x37, 0x45, 0x30, 0x83, 0x35, 0xa8, 0x43, 0x86, 0xe7,
	0x52, 0xe6, 0x46, 0x27, 0x97, 0x07, 0xf5, 0x96, 0xe6, 0x46, 0x37,
	0x37, 0x25, 0x80, 0x83, 0x2e, 0x64, 0x78, 0x2e, 0x63, 0x6f, 0x75,
	0x6e, 0x74, 0x65, 0x72, 0x73, 0x53, 0x02, 0x3b, 0x00, 0x00, 0x00,
	0x00, 0x79, 0x18, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x33, 0x08,
	0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43,
	0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73,
	0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80,
	0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c,
	0x66, 0x30, 0x05, 0x3d, 0x88, 0x43, 0x38, 0x84, 0x83, 0x1b, 0xcc,
	0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xcc, 0x78, 0x8c,
	0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70,
	0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20, 0x87, 0x19,
	0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e, 0x30,
	0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d,
	0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30,
	0x89, 0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c,
	0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60,
	0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37,
	0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07,
	0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87, 0x5f, 0x08,
	0x87, 0x71, 0x18, 0x87, 0x72, 0x98, 0x87, 0x79, 0x98, 0x81, 0x2c,
	0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30,
	0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c,
	0xe4, 0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c, 0xc4, 0x81,
	0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43, 0x39,
	0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43,
	0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc,
	0x82, 0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x0c, 0xc4, 0x21, 0x07,
	0x7c, 0x70, 0x03, 0x7a, 0x28, 0x87, 0x76, 0x80, 0x87, 0x19, 0xd1,
	0x43, 0x0e, 0xf8, 0xe0, 0x06, 0xe4, 0x20, 0x0e, 0xe7, 0xe0, 0x06,
	0xf6, 0x10, 0x0e, 0xf2, 0xc0, 0x0e, 0xe1, 0x90, 0x0f, 0xef, 0x50,
	0x0f, 0xf4, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00, 0x26, 0x00,
	0x00, 0x00, 0x76, 0x40, 0x0d, 0x97, 0xef, 0x3c, 0x3e, 0xd0, 0x34,
	0xce, 0x04, 0x4c, 0x44, 0x08, 0x34, 0xc3, 0x42, 0x58, 0xc1, 0x36,
	0x5c, 0xbe, 0xf3, 0xf8, 0x42, 0x40, 0x15, 0x05, 0x11, 0x95, 0x0e,
	0x30, 0x94, 0x84, 0x01, 0x08, 0x98, 0x5f, 0xdc, 0xb6, 0x21, 0x74,
	0xc3, 0xe5, 0x3b, 0x8f, 0x2f, 0x44, 0x04, 0x30, 0x11, 0x21, 0xd0,
	0x0c, 0x0b, 0xf1, 0x45, 0x0e, 0xb3, 0x21, 0xcd, 0x80, 0x34, 0x86,
	0x19, 0x48, 0xc3, 0xe5, 0x3b, 0x8f, 0x3f, 0x11, 0xd1, 0x84, 0x00,
	0x11, 0xe6, 0x17, 0xb7, 0x6d, 0x02, 0xd5, 0x70, 0xf9, 0xce, 0xe3,
	0x4f, 0xc4, 0x35, 0x51, 0x11, 0x51, 0x3a, 0xc0, 0xe0, 0x17, 0xb7,
	0x6d, 0x03, 0xd6, 0x70, 0xf9, 0xce, 0xe3, 0x4f, 0xc4, 0x35, 0x51,
	0x11, 0xc1, 0x4e, 0x4e, 0x44, 0xf8, 0xc5, 0x6d, 0x5b, 0x80, 0x34,
	0x5c, 0xbe, 0xf3, 0xf8, 0xd3, 0x11, 0x11, 0xc0, 0x20, 0x0e, 0x3e,
	0x72, 0xdb, 0x46, 0xf0, 0x0c, 0x97, 0xef, 0x3c, 0x3e, 0xd5, 0x00,
	0x11, 0xe6, 0x17, 0xb7, 0x6d, 0x00, 0x04, 0x03, 0x20, 0x0d, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xe1, 0xf4, 0xab, 0xb7, 0x6c, 0xc9,
	0xc8, 0x8b, 0xcb, 0xf4, 0xb4, 0x53, 0xbc, 0xd8, 0x59, 0x09, 0x44,
	0x58, 0x49, 0x4c, 0x74, 0x08, 0x00, 0x00, 0x68, 0x00, 0x05, 0x00,
	0x1d, 0x02, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x08, 0x01, 0x00,
	0x00, 0x10, 0x00, 0x00, 0x00, 0x5c, 0x08, 0x00, 0x00, 0x42, 0x43,
	0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00, 0x14, 0x02, 0x00, 0x00, 0x0b,
	0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32,
	0x39, 0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04,
	0x8b, 0x62, 0x80, 0x18, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42, 0xc4,
	0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x62, 0x88,
	0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32,
	0x42, 0xe4, 0x48, 0x0e, 0x90, 0x11, 0x23, 0xc4, 0x50, 0x41, 0x51,
	0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x31, 0x46, 0x06, 0x51,
	0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1b, 0x8c, 0xe0, 0xff,
	0xff, 0xff, 0xff, 0x07, 0x40, 0x02, 0xa8, 0x0d, 0x86, 0xf0, 0xff,
	0xff, 0xff, 0xff, 0x03, 0x20, 0x01, 0xd5, 0x06, 0x62, 0xf8, 0xff,
	0xff, 0xff, 0xff, 0x01, 0x90, 0x00, 0x49, 0x18, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20, 0x4c, 0x08, 0x06,
	0x00, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00, 0x50, 0x00, 0x00,
	0x00, 0x32, 0x22, 0x88, 0x09, 0x20, 0x64, 0x85, 0x04, 0x13, 0x23,
	0xa4, 0x84, 0x04, 0x13, 0x23, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12,
	0x4c, 0x8c, 0x8c, 0x0b, 0x84, 0xc4, 0x4c, 0x10, 0x98, 0xc1, 0x08,
	0x40, 0x09, 0x00, 0x0a, 0xe6, 0x08, 0xc0, 0xa0, 0x0c, 0xc3, 0x30,
	0x10, 0x31, 0x03, 0x70, 0xd3, 0x70, 0xf9, 0x13, 0xf6, 0x10, 0x92,
	0xbf, 0x12, 0xd2, 0x4a, 0x4c, 0x7e, 0x71, 0xdb, 0xa8, 0x30, 0x0c,
	0xc3, 0x18, 0xe6, 0x08, 0x10, 0x42, 0xee, 0x19, 0x2e, 0x7f, 0xc2,
	0x1e, 0x42, 0xf2, 0x43, 0xa0, 0x19, 0x16, 0x02, 0x05, 0x49, 0x51,
	0x8e, 0x41, 0x19, 0x86, 0x61, 0x18, 0x86, 0x81, 0x96, 0xb2, 0x00,
	0x83, 0x32, 0x0c, 0x83, 0x61, 0x18, 0x06, 0x42, 0x4d, 0x19, 0x8c,
	0xc1, 0xa0, 0xe7, 0xa8, 0xe1, 0xf2, 0x27, 0xec, 0x21, 0x24, 0x9f,
	0xdb, 0xa8, 0x62, 0x25, 0x26, 0xbf, 0xb8, 0x6d, 0x44, 0x18, 0x86,
	0x61, 0x14, 0x22, 0x1a, 0x94, 0x81, 0xa4, 0x52, 0x18, 0x83, 0x61,
	0x18, 0x44, 0xdd, 0x36, 0x5c, 0xfe, 0x84, 0x3d, 0x84, 0xe4, 0xaf,
	0x84, 0xe4, 0x50, 0x91, 0x40, 0xa4, 0x91, 0xf3, 0x10, 0xd1, 0x84,
	0x10, 0x12, 0x12, 0x86, 0xa1, 0x10, 0xca, 0xa0, 0x58, 0x74, 0x1d,
	0x34, 0x5c, 0xfe, 0x84, 0x3d, 0x84, 0xe4, 0xaf, 0x84, 0xb4, 0x21,
	0xcd, 0x80, 0x88, 0x61, 0x18, 0x90, 0x39, 0x82, 0xa0, 0x14, 0xca,
	0x90, 0x0d, 0x1a, 0x6d, 0x03, 0x01, 0xc3, 0x08, 0x84, 0x31, 0x13,
	0x19, 0x8c, 0x03, 0x3b, 0x84, 0xc3, 0x3c, 0xcc, 0x83, 0x1b, 0xc8,
	0xc2, 0x2d, 0xd0, 0x42, 0x39, 0xe0, 0x03, 0x3d, 0xd4, 0x83, 0x3c,
	0x94, 0x83, 0x1c, 0x90, 0x02, 0x1f, 0xd8, 0x43, 0x39, 0x8c, 0x03,
	0x3d, 0xbc, 0x83, 0x3c, 0xf0, 0x81, 0x39, 0xb0, 0xc3, 0x3b, 0x84,
	0x03, 0x3d, 0xb0, 0x01, 0x18, 0xd0, 0x81, 0x1f, 0x80, 0x81, 0x1f,
	0xa0, 0xc0, 0xa3, 0x2f, 0x09, 0xbc, 0xf3, 0x0e, 0x47, 0x9a, 0x16,
	0x00, 0x73, 0xa8, 0xc9, 0x97, 0xa6, 0x88, 0x12, 0x26, 0x3f, 0xa5,
	0xa4, 0x83, 0x73, 0x1a, 0x69, 0x02, 0x9a, 0x09, 0x09, 0xa1, 0x71,
	0xd0, 0xe1, 0x48, 0xd3, 0x02, 0x60, 0x0e, 0x35, 0xf9, 0x29, 0x10,
	0x01, 0x0c, 0x0a, 0x44, 0x1a, 0xe7, 0x08, 0x40, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87, 0x36,
	0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0, 0x87, 0x0d, 0xaf, 0x50,
	0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a,
	0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e,
	0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78, 0xa0,
	0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a,
	0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07,
	0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07, 0x7a, 0x60,
	0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07, 0x73,
	0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07,
	0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0, 0x07, 0x76, 0x40,
	0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a,
	0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86,
	0x3c, 0x04, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0c, 0x79, 0x16, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0xf2, 0x34, 0x40, 0x00, 0x0c, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xe4, 0x79, 0x80, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xc8, 0x23, 0x01,
	0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x90,
	0x87, 0x02, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x21, 0xcf, 0x05, 0x04, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x43, 0x9e, 0x0d, 0x08, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xb2, 0x40, 0x00, 0x00, 0x0c, 0x00,
	0x00, 0x00, 0x32, 0x1e, 0x98, 0x14, 0x19, 0x11, 0x4c, 0x90, 0x8c,
	0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x4a, 0xa0, 0x20, 0x8a,
	0x61, 0x04, 0xa0, 0x08, 0x4a, 0xa2, 0x10, 0x03, 0x0a, 0x81, 0xb4,
	0x11, 0x00, 0x2a, 0x0b, 0x10, 0x30, 0x10, 0x10, 0x88, 0xc8, 0x19,
	0x00, 0x02, 0x67, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x3d,
	0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0xc4,
	0x8e, 0x0c, 0x6f, 0xec, 0xed, 0x4d, 0x0c, 0x24, 0xc6, 0x05, 0xc7,
	0x45, 0xa6, 0x06, 0x46, 0xc6, 0x05, 0x07, 0x04, 0x45, 0x8c, 0xe6,
	0x26, 0x26, 0x06, 0x67, 0x26, 0xa7, 0x2c, 0x65, 0x43, 0x10, 0x4c,
	0x10, 0x86, 0x64, 0x82, 0x30, 0x28, 0x1b, 0x84, 0x81, 0xa0, 0x30,
	0x36, 0x37, 0x41, 0x18, 0x96, 0x0d, 0x83, 0x71, 0x10, 0x13, 0x84,
	0x81, 0x99, 0x20, 0x80, 0x01, 0x45, 0x60, 0x82, 0x30, 0x34, 0x13,
	0x04, 0x2d, 0x9a, 0x20, 0x0c, 0xce, 0x06, 0x21, 0x71, 0x36, 0x2c,
	0x89, 0xb2, 0x24, 0xc9, 0xc0, 0x34, 0x4d, 0xf3, 0x6c, 0x08, 0xa0,
	0x09, 0x02, 0x19, 0x4c, 0x13, 0x84, 0xe1, 0xd9, 0x80, 0x24, 0xd2,
	0x92, 0x24, 0xc3, 0x04, 0x6c, 0x08, 0xa8, 0x0d, 0x04, 0x10, 0x55,
	0xc0, 0x04, 0x41, 0x00, 0x48, 0xb4, 0x85, 0xa5, 0xb9, 0x4d, 0x10,
	0xca, 0x40, 0x9a, 0x20, 0x0c, 0xd0, 0x86, 0x81, 0x20, 0x86, 0x0d,
	0x44, 0x92, 0x69, 0xdb, 0x86, 0xe2, 0xc2, 0x00, 0x8b, 0xab, 0xc2,
	0xc6, 0x66, 0xd7, 0xe6, 0x92, 0x46, 0x56, 0xe6, 0x46, 0x37, 0x25,
	0x08, 0xaa, 0x90, 0xe1, 0xb9, 0xd8, 0x95, 0xc9, 0xcd, 0xa5, 0xbd,
	0xb9, 0x4d, 0x09, 0x88, 0x26, 0x64, 0x78, 0x2e, 0x76, 0x61, 0x6c,
	0x76, 0x65, 0x72, 0x53, 0x02, 0xa2, 0x0e, 0x19, 0x9e, 0xcb, 0x1c,
	0x5a, 0x18, 0x59, 0x99, 0x5c, 0xd3, 0x1b, 0x59, 0x19, 0xdb, 0x94,
	0xe0, 0x28, 0x43, 0x86, 0xe7, 0x22, 0x57, 0x36, 0xf7, 0x56, 0x27,
	0x37, 0x56, 0x36, 0x37, 0x25, 0xa8, 0xea, 0x90, 0xe1, 0xb9, 0x94,
	0xb9, 0xd1, 0xc9, 0xe5, 0x41, 0xbd, 0xa5, 0xb9, 0xd1, 0xcd, 0x4d,
	0x09, 0x38, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x4c, 0x00,
	0x00, 0x00, 0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14,
	0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07,
	0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed,
	0x10, 0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1,
	0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43, 0x38,
	0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03,
	0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79,
	0x48, 0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87,
	0x70, 0x20, 0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e, 0xe1,
	0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e,
	0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2,
	0x61, 0x1e, 0x66, 0x30, 0x89, 0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43,
	0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc,
	0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87,
	0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60,
	0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77,
	0x80, 0x87, 0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98, 0x87,
	0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5,
	0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1,
	0x1c, 0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca,
	0x21, 0x1c, 0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43,
	0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8,
	0xc3, 0x38, 0x94, 0x43, 0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f,
	0xbc, 0x83, 0x3c, 0xfc, 0x82, 0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3,
	0x0c, 0xc4, 0x21, 0x07, 0x7c, 0x70, 0x03, 0x7a, 0x28, 0x87, 0x76,
	0x80, 0x87, 0x19, 0xd1, 0x43, 0x0e, 0xf8, 0xe0, 0x06, 0xe4, 0x20,
	0x0e, 0xe7, 0xe0, 0x06, 0xf6, 0x10, 0x0e, 0xf2, 0xc0, 0x0e, 0xe1,
	0x90, 0x0f, 0xef, 0x50, 0x0f, 0xf4, 0x00, 0x00, 0x00, 0x71, 0x20,
	0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x76, 0x40, 0x0d, 0x97, 0xef,
	0x3c, 0x3e, 0xd0, 0x34, 0xce, 0x04, 0x4c, 0x44, 0x08, 0x34, 0xc3,
	0x42, 0x58, 0xc1, 0x36, 0x5c, 0xbe, 0xf3, 0xf8, 0x42, 0x40, 0x15,
	0x05, 0x11, 0x95, 0x0e, 0x30, 0x94, 0x84, 0x01, 0x08, 0x98, 0x5f,
	0xdc, 0xb6, 0x21, 0x74, 0xc3, 0xe5, 0x3b, 0x8f, 0x2f, 0x44, 0x04,
	0x30, 0x11, 0x21, 0xd0, 0x0c, 0x0b, 0xf1, 0x45, 0x0e, 0xb3, 0x21,
	0xcd, 0x80, 0x34, 0x86, 0x19, 0x48, 0xc3, 0xe5, 0x3b, 0x8f, 0x3f,
	0x11, 0xd1, 0x84, 0x00, 0x11, 0xe6, 0x17, 0xb7, 0x6d, 0x02, 0xd5,
	0x70, 0xf9, 0xce, 0xe3, 0x4f, 0xc4, 0x35, 0x51, 0x11, 0x51, 0x3a,
	0xc0, 0xe0, 0x17, 0xb7, 0x6d, 0x03, 0xd6, 0x70, 0xf9, 0xce, 0xe3,
	0x4f, 0xc4, 0x35, 0x51, 0x11, 0xc1, 0x4e, 0x4e, 0x44, 0xf8, 0xc5,
	0x6d, 0x5b, 0x80, 0x34, 0x5c, 0xbe, 0xf3, 0xf8, 0xd3, 0x11, 0x11,
	0xc0, 0x20, 0x0e, 0x3e, 0x72, 0xdb, 0x46, 0xf0, 0x0c, 0x97, 0xef,
	0x3c, 0x3e, 0xd5, 0x00, 0x11, 0xe6, 0x17, 0xb7, 0x6d, 0x00, 0x04,
	0x03, 0x20, 0x0d, 0x00, 0x61, 0x20, 0x00, 0x00, 0x97, 0x00, 0x00,
	0x00, 0x13, 0x04, 0x41, 0x2c, 0x10, 0x00, 0x00, 0x00, 0x0f, 0x00,
	0x00, 0x00, 0x34, 0x14, 0xd7, 0x0c, 0x40, 0xd9, 0x95, 0x43, 0xb9,
	0x94, 0x4a, 0xc9, 0x0e, 0x14, 0xec, 0x40, 0xe9, 0x06, 0x94, 0x46,
	0x11, 0x02, 0x95, 0x24, 0x44, 0x11, 0x06, 0x94, 0x41, 0x19, 0x06,
	0x10, 0x52, 0x02, 0x45, 0x50, 0x1e, 0x64, 0xcd, 0x21, 0x78, 0xcf,
	0x1c, 0xc2, 0x07, 0x06, 0x94, 0xcd, 0x41, 0x30, 0x0c, 0x43, 0x06,
	0x73, 0x10, 0x0c, 0xc3, 0x94, 0x01, 0x00, 0x23, 0x06, 0x08, 0x00,
	0x82, 0x60, 0xb0, 0x8d, 0x01, 0x24, 0x7c, 0xda, 0x88, 0x01, 0x02,
	0x80, 0x20, 0x18, 0x6c, 0x64, 0x10, 0x09, 0x60, 0xb0, 0x8d, 0x18,
	0x1c, 0x00, 0x08, 0x82, 0xc1, 0x65, 0x06, 0x51, 0x60, 0x8c, 0x18,
	0x18, 0x00, 0x08, 0x82, 0x01, 0xd1, 0x06, 0x91, 0x18, 0x8c, 0x18,
	0x18, 0x00, 0x08, 0x82, 0x01, 0xe1, 0x06, 0x92, 0x19, 0x8c, 0x18,
	0x1c, 0x00, 0x08, 0x82, 0xc1, 0x95, 0x06, 0x54, 0x81, 0x8c, 0x18,
	0x34, 0x00, 0x08, 0x82, 0xc1, 0xf2, 0x06, 0x50, 0xb0, 0x0d, 0xc2,
	0xb6, 0x6d, 0xdb, 0x68, 0x42, 0x00, 0x8c, 0x26, 0x08, 0xc1, 0x68,
	0xc2, 0x20, 0x8c, 0x26, 0x10, 0xc3, 0x88, 0xc1, 0x01, 0x80, 0x20,
	0x18, 0x48, 0x71, 0xf0, 0x25, 0x6a, 0x30, 0x9a, 0x10, 0x00, 0xa3,
	0x09, 0x42, 0x30, 0x9a, 0x30, 0x08, 0xa3, 0x09, 0xc4, 0x30, 0x62,
	0x70, 0x00, 0x20, 0x08, 0x06, 0x92, 0x1d, 0x90, 0x81, 0x23, 0x07,
	0xa3, 0x09, 0x01, 0x30, 0x9a, 0x20, 0x04, 0xa3, 0x09, 0x83, 0x30,
	0x9a, 0x40, 0x0c, 0x23, 0x06, 0x07, 0x00, 0x82, 0x60, 0x20, 0xed,
	0x41, 0x1a, 0x4c, 0x73, 0x30, 0x9a, 0x10, 0x00, 0xa3, 0x09, 0x42,
	0x30, 0x9a, 0x30, 0x08, 0xa3, 0x09, 0xc4, 0x30, 0x62, 0x70, 0x00,
	0x20, 0x08, 0x06, 0x12, 0x28, 0xb8, 0x01, 0x36, 0x06, 0xa3, 0x09,
	0x01, 0x30, 0x9a, 0x20, 0x04, 0xa3, 0x09, 0x83, 0x30, 0x9a, 0x40,
	0x0c, 0x36, 0x61, 0xf2, 0x19, 0x31, 0x40, 0x00, 0x10, 0x04, 0x03,
	0xaa, 0x14, 0xec, 0xe0, 0xc1, 0x82, 0x11, 0x03, 0x04, 0x00, 0x41,
	0x30, 0xa0, 0x4c, 0xe1, 0x0e, 0x16, 0x2c, 0x18, 0x31, 0x40, 0x00,
	0x10, 0x04, 0x03, 0xea, 0x14, 0xf0, 0xe0, 0xc0, 0x02, 0xb3, 0x38,
	0xf9, 0x8c, 0x18, 0x20, 0x00, 0x08, 0x82, 0x01, 0x95, 0x0a, 0x7a,
	0x20, 0x71, 0xc1, 0x88, 0x01, 0x02, 0x80, 0x20, 0x18, 0x50, 0xaa,
	0xb0, 0x07, 0x0e, 0x17, 0x8c, 0x18, 0x20, 0x00, 0x08, 0x82, 0x01,
	0xb5, 0x0a, 0x7c, 0xa0, 0x70, 0x81, 0x65, 0x60, 0x20, 0x9f, 0x11,
	0x03, 0x04, 0x00, 0x41, 0x30, 0xa0, 0x5a, 0xc1, 0x0f, 0x2a, 0x30,
	0x08, 0x46, 0x0c, 0x10, 0x00, 0x04, 0xc1, 0x80, 0x72, 0x85, 0x3f,
	0x88, 0xc0, 0x20, 0x18, 0x31, 0x40, 0x00, 0x10, 0x04, 0x03, 0xea,
	0x15, 0x40, 0xa1, 0x01, 0x83, 0xc0, 0x38, 0x32, 0x90, 0xcf, 0x88,
	0x01, 0x02, 0x80, 0x20, 0x18, 0x50, 0xb1, 0x20, 0x0a, 0x18, 0x19,
	0x04, 0x23, 0x06, 0x08, 0x00, 0x82, 0x60, 0x40, 0xc9, 0xc2, 0x28,
	0x50, 0x64, 0x10, 0x8c, 0x18, 0x20, 0x00, 0x08, 0x82, 0x01, 0x35,
	0x0b, 0xa4, 0x00, 0x91, 0x41, 0x30, 0x62, 0x70, 0x00, 0x20, 0x08,
	0x06, 0x52, 0x2d, 0x8c, 0x42, 0x1b, 0xa8, 0xc2, 0x68, 0x42, 0x00,
	0x8c, 0x18, 0x18, 0x00, 0x08, 0x82, 0xc1, 0x83, 0x0b, 0xa3, 0xf0,
	0x8c, 0x18, 0x18, 0x00, 0x08, 0x82, 0xc1, 0x93, 0x0b, 0xa4, 0xc0,
	0x8c, 0x18, 0x18, 0x00, 0x08, 0x82, 0xc1, 0xa3, 0x0b, 0xa5, 0x90,
	0xd8, 0x40, 0xc8, 0xc7, 0x86, 0x42, 0x3e, 0x36, 0x18, 0xf2, 0x19,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0xc7, 0x17, 0x50, 0x61, 0x18,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0xe7, 0x17, 0x52, 0x61, 0x18,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0x07, 0x1c, 0x54, 0x61, 0x18,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0x27, 0x1c, 0x5a, 0x61, 0x18,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0x47, 0x1c, 0x5c, 0x61, 0x18,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0x67, 0x1c, 0x5e, 0x61, 0x18,
	0x31, 0x30, 0x00, 0x10, 0x04, 0x83, 0x87, 0x1c, 0x60, 0xe1, 0x19,
	0x31, 0x70, 0x00, 0x10, 0x04, 0x83, 0xc6, 0x1c, 0x4c, 0x21, 0x0f,
	0xf6, 0x40, 0x0f, 0x66, 0x81, 0x18, 0x84, 0x60, 0x14, 0x10, 0x00,
	0x00, 0x00, 0x00, 0x00
};

namespace kdGfx
{
	static ImageProcessor gImageProcessor;

	void ImageProcessor::initSingleton(BackendType backend, const std::shared_ptr<Device>& device)
	{
		gImageProcessor._backend = backend;
		gImageProcessor._device = device;

		gImageProcessor._bindSetLayout = device->createBindSetLayout
		({
			{.binding = 0, .type = BindEntryType::StorageTexture }
		});

		Shader shader;
		switch (backend)
		{
		case BackendType::Vulkan:
			shader.code = ImageProcessor_spv;
			shader.codeSize = ArraySize(ImageProcessor_spv);
			break;
		case BackendType::DirectX12:
			shader.code = ImageProcessor_dxil;
			shader.codeSize = ArraySize(ImageProcessor_dxil);
			break;
		default:
			assert(false);
			break;
		}

		gImageProcessor._pipeline = device->createComputePipeline
		({
			.shader = shader,
			.pushConstantLayout = { sizeof(Param) },
			.bindSetLayouts = { gImageProcessor._bindSetLayout }
		});
	}

	void ImageProcessor::destroySingleton()
	{
		gImageProcessor._pipeline.reset();
		gImageProcessor._bindSetLayout.reset();
		gImageProcessor._device.reset();
	}

	ImageProcessor& ImageProcessor::singleton()
	{
		return gImageProcessor;
	}

	void ImageProcessor::process(const Param& param, const std::shared_ptr<Texture>& texture, uint32_t mipLevel)
	{
		auto bindSet = _device->createBindSet(_bindSetLayout);
		auto textureView = texture->createView({ .baseMipLevel = mipLevel });
		bindSet->bindTexture(0, textureView);

		auto queue = _device->getCommandQueue(CommandListType::General);
		auto commandList = _device->createCommandList(CommandListType::General);
		commandList->begin();
		commandList->resourceBarrier
		({
			.texture = texture,
			.oldState = TextureState::ShaderRead,
			.newState = TextureState::General,
			.subRange = { mipLevel, 1, 0, 1}
		});
		commandList->setPipeline(_pipeline);
		commandList->setPushConstant(&param);
		commandList->setBindSet(0, bindSet);
		constexpr uint32_t threadWidth = 8;
		constexpr uint32_t threadHeight = 8;
		uint32_t groupCountX = (texture->getWidth() + threadWidth - 1) / threadWidth;
		uint32_t groupCountY = (texture->getHeight() + threadHeight - 1) / threadHeight;
		commandList->dispatch(groupCountX, groupCountY, 1);
		commandList->resourceBarrier
		({
			.texture = texture,
			.oldState = TextureState::General,
			.newState = TextureState::ShaderRead,
			.subRange = { mipLevel, 1, 0, 1}
		});
		commandList->end();
		queue->submit({ commandList });
		queue->waitIdle();
	}
}
