/**
 * @file noncopyable.h
 * @brief 禁止复制基类,继承该类的子类不允许复制
 *
 *
 * @version 1.0
 * @author owent
 * @date 2012.02.21
 *
 * @history
 *
 *
 */

#ifndef _NONCOPYABLE_H_
#define _NONCOPYABLE_H_

namespace util {
    namespace design_pattern {

        class noncopyable {
        protected:
            noncopyable() {}

            ~noncopyable() {}

        private:
            noncopyable(const noncopyable &);

            const noncopyable &operator=(const noncopyable &);
        };
    }
}

#endif
