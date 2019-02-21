/**
 * @brief 有限状态机
 *
 * @version 1.0
 * @author OWenT
 * @date 2015-01-21
 *
 */
#pragma once

#include <map>
#include <list>
#include <functional>
#include "std/smart_ptr.h"

namespace util {
    namespace ds {

        /**
         * 有限状态机
         * @brief 必须有0状态
         */
        template <typename T, typename... TParams>
        class finite_state_machine {
        public:
            typedef T key_type;
            typedef std::function<void(key_type, key_type, TParams...)> value_type;
            typedef std::list<value_type> listener_list_type;
            typedef std::map<key_type, listener_list_type> listener_set_type;

            typedef struct {
                std::list<key_type> from;
                key_type to;
                value_type fn;
            } init_ele_type;

        public:
            finite_state_machine() : state_(static_cast<key_type>(0)) {}
            finite_state_machine(key_type init_state) : state_(init_state) {}

            key_type get_state() const { return state_; }

            bool set_state(key_type t, TParams... params) { return switch_to(t, params...); }

            bool test(key_type t) const { return test_switch_to(t); }

            bool empty() const { return !leave_from_listener_ && !enter_to_listener_ && !pairs_listener_; }

            listener_list_type *get_leave_listeners(key_type k) {
                if (!leave_from_listener_) {
                    return nullptr;
                }

                typename listener_set_type::iterator iter = leave_from_listener_->find(k);
                return (leave_from_listener_->end() == iter) ? nullptr : &iter->second;
            }

            listener_list_type *get_enter_listener_(key_type k) {
                if (!enter_to_listener_) {
                    return nullptr;
                }

                typename listener_set_type::iterator iter = enter_to_listener_->find(k);
                return (enter_to_listener_->end() == iter) ? nullptr : &iter->second;
            }

            listener_list_type *get_switch_listener_(key_type from, key_type to) {
                if (!pairs_listener_) {
                    return nullptr;
                }

                typename std::map<value_type, listener_set_type>::iterator iter_from = pairs_listener_->find(from);

                if (pairs_listener_->end() == iter_from) {
                    return nullptr;
                }

                typename listener_set_type::iterator iter_to = iter_from->second.find(to);

                if (iter_from->second.end() == iter_from) {
                    return nullptr;
                }

                return &iter_to->second;
            }

            void add_enter_listener(key_type k, value_type fn) {
                if (!enter_to_listener_) {
                    enter_to_listener_ = std::shared_ptr<listener_set_type>(new listener_set_type());
                }

                listener_list_type &ls = (*enter_to_listener_)[k];
                if (fn) {
                    ls.push_back(fn);
                }
            }

            void add_leave_listener(key_type k, value_type fn) {
                if (!leave_from_listener_) {
                    leave_from_listener_ = std::shared_ptr<listener_set_type>(new listener_set_type());
                }

                listener_list_type &ls = (*leave_from_listener_)[k];
                if (fn) {
                    ls.push_back(fn);
                }
            }

            void add_listener(key_type from, key_type to, value_type fn) {
                if (!pairs_listener_) {
                    pairs_listener_ = std::shared_ptr<std::map<key_type, listener_set_type> >(
                        new std::map<key_type, listener_set_type>());
                }

                listener_list_type &ls = (*pairs_listener_)[from][to];
                if (fn) {
                    ls.push_back(fn);
                }
            }

            template <size_t SIZE>
            void register_listener(init_ele_type init_ls[SIZE]) {
                for (size_t i = 0; i < SIZE; ++i) {
                    for (key_type &from : init_ls[i].from) {
                        add_listener(from, init_ls[i].to, init_ls[i].fn);
                    }
                }
            }

            template <typename TContainer>
            void register_listener(TContainer &init_ls) {
                for (init_ele_type &ele : init_ls) {
                    for (key_type &from : ele.from) {
                        add_listener(from, ele.to, ele.fn);
                    }
                }
            }

            void register_listener(finite_state_machine &other) {
                pairs_listener_ = other.pairs_listener_;
                leave_from_listener_ = other.leave_from_listener_;
                enter_to_listener_ = other.enter_to_listener_;
            }

        private:
            bool test_switch_to(key_type t) const {
                if (!pairs_listener_) {
                    return false;
                }

                auto iter_from = pairs_listener_->find(state_);

                if (pairs_listener_->end() == iter_from) {
                    return false;
                }

                auto iter_to = iter_from->second.find(t);

                if (iter_from->second.end() == iter_to) {
                    return false;
                }

                return true;
            }

            bool switch_to(key_type t, TParams... params) {
                if (!pairs_listener_) {
                    return false;
                }

                auto iter_from = pairs_listener_->find(state_);

                if (pairs_listener_->end() == iter_from) {
                    return false;
                }

                auto iter_to = iter_from->second.find(t);

                if (iter_from->second.end() == iter_to) {
                    return false;
                }

                typename listener_set_type::iterator iter_single;
                // 先触发离场状态回调
                if (leave_from_listener_) {
                    iter_single = leave_from_listener_->find(state_);
                    if (leave_from_listener_->end() != iter_single) {
                        for (value_type &fn : iter_single->second) {
                            fn(state_, t, params...);
                        }
                    }
                }

                // 再触发进场状态回调
                if (enter_to_listener_) {
                    iter_single = enter_to_listener_->find(t);
                    if (enter_to_listener_->end() != iter_single) {
                        for (value_type &fn : iter_single->second) {
                            fn(state_, t, params...);
                        }
                    }
                }

                // 最后触发切换状态回调
                for (value_type &fn : iter_to->second) {
                    fn(state_, t, params...);
                }

                state_ = t;
                return true;
            }

        private:
            key_type state_;
            std::shared_ptr<std::map<key_type, listener_set_type> > pairs_listener_;
            std::shared_ptr<listener_set_type> leave_from_listener_;
            std::shared_ptr<listener_set_type> enter_to_listener_;
        };
    }
}
