#include <iostream>

// Really stupid function (just use +)
int add(int, int);

// A more useful function
int factorial(int);

int main()
{
    std::cout << "Hello C++ world from team HiveMind!" << std::endl;
    int dummyVar = 0;
    std::cout << "Running add() on dummyVar: " <<  add(dummyVar++, ++dummyVar) << std::endl;
    std::cout << "dummyVar is now: " << dummyVar << std::endl;
    std::cout << "5! = " << factorial(5) << std::endl;
    return 0;

    return 0;
}

int add(int a, int b)
{
    return a + b;
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
