#pragma once//#pragma once用于防止头文件被多次包含，从而避免重复定义和编译错误。

/*
noncopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
它的设计目的是通过继承机制，让派生类自动获得不可复制的特性。
*/

class noncopyable {
public:
    noncopyable(const noncopyable&) = delete; //禁止编译器形成默认构造函数
    noncopyable& operator=(const noncopyable&) = delete; //禁止该类的拷贝构造符
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};