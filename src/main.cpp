#include <iostream>

extern "C" {
	double foo(double);
}

int main()
{
	std::cout << "foo(3)" << foo(3) << std::endl;
	return 0;
}
