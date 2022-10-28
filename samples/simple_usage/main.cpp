#include <iostream>
#include <unistd.h>

#include "user_driver.h"

using namespace user_driver;

int main(const int argc, const char* argv[])
{
	const int LIMIT = 3;
	std::cout << "Starting process with pid " << getpid() << std::endl;

	Accelerator acc;
	std::cout << "Accelerator object created" << std::endl;

	std::cout << "Submit & wait, submit & wait, submit & wait" << std::endl;
	for(int i=0; i<LIMIT; ++i)
	{
		control ctrl;
		output out;

		ctrl.command = acc.get_submit_counter();
		ctrl.params[0] = ctrl.params[1] = 3u;

		acc.submit_one_command(ctrl);
		std::cout << "submitted command " << ctrl.command << std::endl;

		acc.wait_for_one_completion(out);
		std::cout << "completed command " << out.command << " with output " << out.outputs[0] << std::endl;
	}

	std::cout << "Submit, submit, submit & wait, wait, wait" << std::endl;
	for(int i=0; i<LIMIT; ++i)
	{
		control ctrl;
		ctrl.command = acc.get_submit_counter();
		ctrl.params[0] = ctrl.params[1] = 3u;

		acc.submit_one_command(ctrl);
		std::cout << "submitted command " << ctrl.command << std::endl;
	}
	for(int i=0; i<LIMIT; ++i)
	{
		output out;

		acc.wait_for_one_completion(out);
		std::cout << "completed command " << out.command << " with output " << out.outputs[0] << std::endl;
	}

	return 0;
}
