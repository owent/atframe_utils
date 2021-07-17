/**
 * @file static_assert.h
 * @brief 导入静态断言（STD_STATIC_ASSERT）<br />
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2013-12-25
 *
 * @history
 *       2021-07-17 移除老版本适配
 */
#ifndef STD_STATIC_ASSERT_H
#define STD_STATIC_ASSERT_H

#pragma once

#define STD_STATIC_ASSERT(exp) static_assert(exp, #exp)
#define STD_STATIC_ASSERT_MSG(exp, msg) static_assert(exp, msg)

#endif /* STD_STATIC_ASSERT_H */
