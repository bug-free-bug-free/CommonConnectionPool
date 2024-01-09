#include <iostream>
#include <functional>
using namespace std;
void add(int  a, int b) {
    cout << a + b << endl;
}
void show(int a) {
    cout << a << endl;
}
template<typename F>
class myfunction{};

template<typename T, typename D>
class myfunction<T(D)>
{
public:
    using ptr_type = T(*)(D);
    myfunction(ptr_type ptr) :ptr(ptr)
    {}
    T operator()(D a) {
        return ptr(a);
    }
    ptr_type ptr;
};
int main() {

    myfunction<void(int)> fun(show);
    fun(1);
    return 0;
}