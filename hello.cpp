#include <iostream>

// Really stupid function (just use +)
int add(int, int);

int main()
{
    std::cout << "Hello C++ world from team HiveMind!" << std::endl;

    int dummyVar = 0;
    std::cout << "Running add() on dummyVar: " <<  add(dummyVar++, ++dummyVar) << std::endl;
    std::cout << "dummyVar is now: " << dummyVar << std::endl;

    return 0;
}

int add(int a, int b)
{
    return a + b;
}