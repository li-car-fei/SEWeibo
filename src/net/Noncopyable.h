#ifndef __MY_NONCOPYABLE_H_
#define __MY_NONCOPYABLE_H_

/*
不可拷贝基类：继承此基类的派生类将不能使用拷贝构造函数与拷贝赋值运算符
*/

namespace wd
{
    class Noncopyable
    {
    protected:
        Noncopyable() {}
        ~Noncopyable() {}
        // 弃用拷贝构造
        Noncopyable(const Noncopyable &lhs) = delete;
        // 弃用拷贝赋值
        const Noncopyable& operator=(const Noncopyable &lhs) = delete;
    };
} // namespace wd


#endif 