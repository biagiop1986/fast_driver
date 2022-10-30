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

		acc.submit(ctrl);
		std::cout << "submitted command " << ctrl.command << std::endl;

		acc.wait_for_completion(out);
		std::cout << "completed command " << out.command << " with output " << out.outputs[0] << std::endl;
	}

	std::cout << "Submit, submit, submit & wait, wait, wait" << std::endl;
	acc.submit(
		control(acc.get_submit_counter(), 3u, 3u, 0u, 0u),
		control(acc.get_submit_counter() + 1, 3u, 3u, 0u, 0u),
		control(acc.get_submit_counter() + 2, 3u, 3u, 0u, 0u)
	);
	std::cout << "submitted 3 commands" << std::endl;

	auto outputs = acc.wait_for_completion();
	std::cout << "completed 3 commands with outputs ";
	std::cout << outputs[0].outputs[0];
	for(size_t i = 1; i < outputs.size(); ++i)
		std::cout << ", " << outputs[i].outputs[0];
	std::cout << std::endl;

	return 0;
}
