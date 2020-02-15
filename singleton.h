//
// Created by jxq on 20-2-15.
//

#ifndef MYSYLAR_SINGLETON_H
#define MYSYLAR_SINGLETON_H

#include <bits/shared_ptr.h>

namespace sylar{

    /**
     * @brief 单例模式封装类
     * @details T 类型
     *          X 为了创造多个实例对应的Tag
     *          N 同一个Tag创造多个实例索引
     */
    template<class T, class X = void, int N = 0>
    class Singleton {
    public:
        // 返回单例裸指针
        static T* GetInstance() {
            static T v;
            return &v;
        }
    };

    template<class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        static std::shared_ptr<T> GetInstance() {
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };
};

#endif //MYSYLAR_SINGLETON_H
