#ifndef __COMMON_H
#define __COMMON_H

/* Driver parameters */
#define PARAM_MAX 4
#define OUTPUT_MAX 1
#define NUM_SLOTS 16

#ifdef __cplusplus
namespace user_driver
{
#endif

struct control
{
	uint32_t command;
	uint64_t params[PARAM_MAX];
};

struct output
{
	uint32_t command;
	uint64_t outputs[OUTPUT_MAX];
};

#ifdef __cplusplus
} // namespace user_driver
#endif

#define DEVICE_NAME "sample_acc"
#define FILE_PATH "/dev/" DEVICE_NAME

#endif /* __COMMON_H */
