#ifndef __USER_DRIVER_H
#define __USER_DRIVER_H

#include <cstdint>
#include <cstring>
#include <sstream>
#include <sys/ioctl.h>
#include <type_traits>
#include <vector>

#include "../common/common.h"

namespace user_driver
{

constexpr size_t SUBMIT_RING_BUFFER_SIZE = 664;
constexpr size_t COMPLETE_RING_BUFFER_SIZE = 280;

template <typename... Ts>
struct and_struct;

template <>
struct and_struct<>
{
    static constexpr bool value = true;
};

template <typename T1, typename... Ts>
struct and_struct<T1, Ts...>
{
    static constexpr bool value = T1::value && and_struct<Ts...>::value;
};

struct submission_ring_buffer
{
    uint32_t __reserved;
    uint32_t index;
    control controls[NUM_SLOTS];
};

struct completion_ring_buffer
{
    uint32_t __reserved;
    uint32_t index;
    output outputs[NUM_SLOTS];
};

uint32_t ring_buffer_distance(uint32_t lower, uint32_t upper);

class Accelerator
{
  public:
    Accelerator();
    Accelerator(const Accelerator&) = delete;
    Accelerator(Accelerator&&);

    Accelerator operator=(const Accelerator&) = delete;
    Accelerator& operator=(Accelerator&& rhs)
    {
        fd_ = rhs.fd_;
        submit_ = rhs.submit_;
        complete_ = rhs.complete_;
        submit_counter_ = rhs.submit_counter_;
        completion_counter_ = rhs.completion_counter_;

        rhs.fd_ = -1;
        rhs.submit_ = nullptr;
        rhs.complete_ = nullptr;

        return *this;
    }

    virtual ~Accelerator();

    template <typename... Cs>
    void submit(const Cs&... ctrl)
    {
        static_assert(and_struct<std::is_same<Cs,control>...>::value,
            "Invoke submit with n control objects: submit(ctrl, ctrl, ctrl, ...) ");

        auto remaining_slots = NUM_SLOTS - (submit_counter_ - completion_counter_);
        if(sizeof...(Cs) > remaining_slots)
        {
            std::stringstream ss;
            ss << "At most " << remaining_slots << " commands can be enqueued - ";
            ss << ((remaining_slots == NUM_SLOTS) ? " try to split your workload" :
                "wait for some completion before enqueuing new commands");
            throw std::runtime_error(ss.str());
        }

        (
            [&]{
                memcpy(submit_->controls + (submit_counter_++ % NUM_SLOTS), &ctrl, sizeof(ctrl));
            }(), ...
        );
        submit_->index = (submit_counter_ % NUM_SLOTS);
	    ioctl(fd_, 0ul);
    }

    void submit(const std::vector<control>& vec);

    template <typename... Os>
    uint32_t wait_for_completion(Os&... outs)
    {
        static_assert(and_struct<std::is_same<Os,output>...>::value,
            "Invoke wait_for_completion with n output objects: wait_for_completion(out, out, out, ...) ");

        uint32_t old_completion_counter = completion_counter_;
        auto to_complete = std::min<size_t>(sizeof...(Os), submit_counter_ - completion_counter_);
        (
            [&]{
                if(!to_complete)
                    return;
                
                uint32_t to_complete_index = completion_counter_ % NUM_SLOTS;
	            while(complete_->index == to_complete_index)
	            {
		            // wait...
	            }

	            memcpy(&outs, (const void*)(complete_->outputs + to_complete_index), sizeof(output));
	            ++completion_counter_;

                --to_complete;
            }(), ...
        );

        return completion_counter_ - old_completion_counter;
    }

    std::vector<output> wait_for_completion();

    uint32_t get_submit_counter() const;
    uint32_t get_completion_counter() const;

  private:
    int fd_;
    submission_ring_buffer* submit_;
    const volatile completion_ring_buffer* complete_;
    uint32_t submit_counter_;
    uint32_t completion_counter_;
};

}

#endif /* __USER_DRIVER_H */
