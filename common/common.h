#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
#include <string.h>
#endif

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
#ifdef __cplusplus
	control() : command(0u)
	{
		memset(params, 0, sizeof(uint64_t) * PARAM_MAX);
	}
	template <typename... Ts>
	control(uint32_t command_, Ts... param_) : command(command_)
	{
		static_assert(sizeof...(Ts) == PARAM_MAX, 
			"Init control with PARAM_MAX parameters of a type convertible to uint64_t");
		uint32_t i = 0u;
		([&]{
			params[i++] = param_;
		}(), ...);
	}
	control(const control&) = default;
#endif
	uint32_t command;
	uint64_t params[PARAM_MAX];
};

struct output
{
#ifdef __cplusplus
	output() : command(0u)
	{
		memset(outputs, 0, sizeof(uint64_t) * OUTPUT_MAX);
	}
	template <typename... Ts>
	output(uint32_t command_, Ts... output_) : command(command_)
	{
		static_assert(sizeof...(Ts) == OUTPUT_MAX, 
			"Init output with OUTPUT_MAX parameters of a type convertible to uint64_t");
		uint32_t i = 0u;
		([&]{
			outputs[i++] = output_;
		}(), ...);
	}
	output(const output&) = default;
#endif
	uint32_t command;
	uint64_t outputs[OUTPUT_MAX];
};

#ifdef __cplusplus
} // namespace user_driver
#endif

#define DEVICE_NAME "sample_acc"
#define FILE_PATH "/dev/" DEVICE_NAME

#endif /* __COMMON_H */
