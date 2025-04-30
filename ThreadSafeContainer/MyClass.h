#ifndef __MYCLASS_H
#define __MYCLASS_H

#include <iostream>

class MyClass {
public:
    MyClass():_data(0){}
    MyClass(int data):_data(data){}
    MyClass(const MyClass& mc) {
        _data = mc._data;
    }
    MyClass& operator=(const MyClass& other) {
        _data = other._data;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const MyClass& myclass) {
        os << "MyClass Data is " << myclass._data;
        return os;
    }
private:
    int _data;
};

#endif