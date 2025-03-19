#pragma once

#include <iostream>
#include <string>

//时间类
class Timestamp {
public:
    Timestamp(); // 默认构造
    explicit Timestamp(int64_t microSecondsSinceEpoch); // 非默认构造 explicit防止隐式转换
    static Timestamp now(); // 类中的静态成员函数 不依赖任何实例对象
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};