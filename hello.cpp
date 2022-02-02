#include <iostream>

// A more useful function
int factorial(int);

int main()
{
    std::cout << "Hello C++ world from team HiveMind!" << std::endl;
    std::cout << "5! = " << factorial(5) << std::endl;
    return 0;
}

int factorial(int a)
{
    int returnVal = 1;
    while(a > 1)
    {
        returnVal *= a--;
    }
    return returnVal;
}