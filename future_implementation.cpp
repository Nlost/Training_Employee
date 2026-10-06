#include <future>
#include <iostream>
#include <unistd.h>

using namespace std::chrono_literals;

int Func(int a, int b)
{
    sleep(5);
    return a + b;
}

int main()
{
    std::future<int> Value = std::async(std::launch::async, Func, 2, 3);
    while(true)
    {        
        try
        {
            if(Value.wait_for(100ms) == std::future_status::ready)
            {
            std::cout << "Ceva \n";
            std::cout << Value.get(); 
            std::cout << "Altceva\n";
            Value = std::async(std::launch::async, Func, 2, 3);
            }
        }
        catch(std::exception &e)
        {
            std::cout<< e.what()<<"\n";
        }
        }
    return 0;
}