#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/ioctl.h>
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

Accelerator::Accelerator() : submit_counter_(0u), to_complete_index_(0u)
{
	fd_ = open(FILE_PATH, O_RDWR);
	if(fd_ < 0)
		throw std::runtime_error("Error while opening device file");

	submit_ = (submission_ring_buffer*)mmap(NULL, SUBMIT_SIZE, PROT_WRITE, MAP_SHARED, fd_, 0);
	complete_ = (completion_ring_buffer*)mmap(NULL, COMPLETE_SIZE, PROT_READ, MAP_SHARED, fd_, getpagesize());

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
		munmap(submit_, SUBMIT_SIZE);
	if(complete_)
		munmap(const_cast<completion_ring_buffer*>(complete_), COMPLETE_SIZE);
	if(fd_ > 0)
		close(fd_);
}

void Accelerator::submit_one_command(const control& ctrl)
{
	memcpy(submit_->controls + submit_->index, &ctrl, sizeof(ctrl));
	submit_->index = (submit_->index + 1) % NUM_SLOTS;
	ioctl(fd_, 0ul);
	++submit_counter_;
}

void Accelerator::wait_for_one_completion(output& output)
{
	while(complete_->index == to_complete_index_)
	{
		// wait...
	}

	memcpy(&output, (const void*)(complete_->outputs + to_complete_index_), sizeof(output));
	to_complete_index_ = (to_complete_index_ + 1) % NUM_SLOTS;
}

uint32_t Accelerator::get_submit_counter() const
{
	return submit_counter_;
}


}
