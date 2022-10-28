#ifndef __USER_DRIVER_H
#define __USER_DRIVER_H

#include <cstdint>
#include <string>

#include "../common/common.h"

namespace user_driver
{

constexpr size_t SUBMIT_SIZE = 664;
constexpr size_t COMPLETE_SIZE = 280;

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
        to_complete_index_ = rhs.to_complete_index_;

        rhs.fd_ = -1;
        rhs.submit_ = nullptr;
        rhs.complete_ = nullptr;

        return *this;
    }

    virtual ~Accelerator();

    void submit_one_command(const control&);
    void wait_for_one_completion(output&);

    uint32_t get_submit_counter() const;

  private:
    int fd_;
    submission_ring_buffer* submit_;
    const volatile completion_ring_buffer* complete_;
    uint32_t submit_counter_;
    uint32_t to_complete_index_;
};

}

#endif /* __USER_DRIVER_H */
