#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include "user_driver.h"

namespace user_driver
{

uint32_t ring_buffer_distance(uint32_t lower, uint32_t upper)
{
    if(upper >= lower)
        return upper - lower;
    else
        return (upper + NUM_SLOTS) - lower;
}

Accelerator::Accelerator() : submit_counter_(0u), completion_counter_(0u)
{
    fd_ = open(FILE_PATH, O_RDWR);
    if(fd_ < 0)
        throw std::runtime_error("Error while opening device file");

    submit_ = (submission_ring_buffer*)mmap(NULL, SUBMIT_RING_BUFFER_SIZE, PROT_WRITE, MAP_SHARED, fd_, 0);
    complete_ = (completion_ring_buffer*)mmap(NULL, COMPLETE_RING_BUFFER_SIZE, PROT_READ, MAP_SHARED, fd_, getpagesize());

    if(!submit_ || !complete_)
        throw std::runtime_error("Error while mmapping ring buffers");
}

Accelerator::Accelerator(Accelerator&& rhs)
{
    *this = std::move(rhs);
}

Accelerator::~Accelerator()
{
    if(submit_)
        munmap(submit_, SUBMIT_RING_BUFFER_SIZE);
    if(complete_)
        munmap(const_cast<completion_ring_buffer*>(complete_), COMPLETE_RING_BUFFER_SIZE);
    if(fd_ > 0)
        close(fd_);
}

void Accelerator::submit(const std::vector<control>& vec)
{
    auto remaining_slots = NUM_SLOTS - (submit_counter_ - completion_counter_);
    if(vec.size() > remaining_slots)
    {
        std::stringstream ss;
        ss << "At most " << remaining_slots << " commands can be enqueued - ";
        ss << ((remaining_slots == NUM_SLOTS) ? " try to split your workload" :
            "wait for some completion before enqueuing new commands");
        throw std::runtime_error(ss.str());
    }

    for(const auto& ctrl : vec)
        memcpy(submit_->controls + (submit_counter_++ % NUM_SLOTS), &ctrl, sizeof(ctrl));

    submit_->index = submit_counter_ % NUM_SLOTS;
    ioctl(fd_, 0ul);
}

std::vector<output> Accelerator::wait_for_completion()
{
	std::vector<output> outvec;
	outvec.reserve(submit_counter_ - completion_counter_);

	for( ; completion_counter_ < submit_counter_; ++completion_counter_)
	{
		output out;
		uint32_t to_complete_index = completion_counter_ % NUM_SLOTS;
	    while(complete_->index == to_complete_index)
	    {
		    // wait...
	    }
	    memcpy(&out, (const void*)(complete_->outputs + to_complete_index), sizeof(output));
		outvec.push_back(out);
	}

    return outvec;
}

uint32_t Accelerator::get_submit_counter() const
{
    return submit_counter_;
}

uint32_t Accelerator::get_completion_counter() const
{
    return completion_counter_;
}


}
